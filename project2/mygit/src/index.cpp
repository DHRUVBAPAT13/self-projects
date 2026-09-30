#include "index.hpp"
#include "repository.hpp"
#include "object.hpp"
#include "tree.hpp"
#include "ignore.hpp"

#include <iostream>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <windows.h>

namespace 
{
    
// normalize Windows backslashes to forward slashes
std::string normalize_path(std::string path){
    for(char& c : path){
        if(c == '\\') c = '/'; 
    }

    // strip leading "./" if present
    if(path.rfind("./", 0) == 0){
        path = path.substr(2);
    }
    return path;
}

// converts recursive staged paths into hierarchial Git Tree objects
std::string build_tree_recursive(const std::vector<IndexEntry>& file_entries, const std::string& current_prefix){

    std::vector<TreeEntry> tree_entries;
    std::map<std::string, std::vector<IndexEntry>> subdirs;

    for(const auto& item : file_entries){
        
        std::string rel = item.path.substr(current_prefix.size());
        size_t slash_pos = rel.find('/');

        if(slash_pos == std::string::npos){
            // normal file in current directory level
            tree_entries.push_back({item.mode, rel, item.sha});
        }
        else{
            std::string dir_name = rel.substr(0, slash_pos);
            subdirs[dir_name].push_back(item);
        }
    }

    for(const auto& pair : subdirs){

        std::string sub_prefix = current_prefix + pair.first + "/";
        std::string sub_tree_sha = build_tree_recursive(pair.second, sub_prefix);
        tree_entries.push_back({"40000", pair.first, sub_tree_sha});
    }

    Tree t(tree_entries);
    return t.write();
}

} // namespace 

Index Index::read(){
    Index idx;
    std::string index_path = repository::repo_path("index");
    std::ifstream in(index_path.c_str());
    if(!in.is_open()){
        return idx; // empty index file normal if nothing staged
    }

    std::string line;
    while (std::getline(in, line)){
        
        if(line.empty()) continue;

        std::istringstream ss(line);
        IndexEntry entry;
        if(ss >> entry.mode >> entry.sha >> entry.path){
            idx.entries[entry.path] = entry;
        }
    }

    return idx;
    
}

void Index::write() const{

    std::string index_path  = repository::repo_path("index");
    std::ofstream out(index_path.c_str());
    if(!out.is_open()){
        throw std::runtime_error("FATAL : unable to write index file\n");
    }

    for(const auto& pair : entries){
        out << pair.second.mode << " " << pair.second.sha << " " << pair.second.path << "\n";
    }
}

void Index::add_file(const std::string& rel_path) {
    std::string norm_path = normalize_path(rel_path);

    std::ifstream file(norm_path.c_str(), std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("fatal: pathspec '" + rel_path + "' did not match any files");
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string content = buffer.str();

    std::string blob_sha = object_db::write_object(ObjectType::BLOB, content);
    entries[norm_path] = {"100644", blob_sha, norm_path};
    
}

std::string Index::write_tree() const{

    std::vector<IndexEntry> file_list;
    file_list.reserve(entries.size());
    for(const auto& pair : entries){
        file_list.push_back(pair.second);
    }

    return build_tree_recursive(file_list, "");
}

void Index::add_path(const std::string& target_path){
    std::string norm = normalize_path(target_path);

    IgnoreMatcher matcher;
    matcher.load_rules();

    DWORD attr = GetFileAttributesA(norm.c_str());
    if (attr == INVALID_FILE_ATTRIBUTES) {
        throw std::runtime_error("fatal: path does not exist: " + target_path);
    }

    bool is_dir = (attr & FILE_ATTRIBUTE_DIRECTORY) != 0;
    if (matcher.is_ignored(norm, is_dir)) {
        return; // Skip silently if explicitly ignored
    }

    if (!is_dir) {
        add_file(norm);
        return;
    }

    std::string search_pattern = (norm == ".") ? "*" : norm + "/*";

    WIN32_FIND_DATAA fd;
    HANDLE hFind = FindFirstFileA(search_pattern.c_str(), &fd);
    if (hFind == INVALID_HANDLE_VALUE) return;

    do {
        std::string filename = fd.cFileName;
        if (filename == "." || filename == "..") {
            continue;
        }

        std::string subpath = (norm == "." ? filename : norm + "/" + filename);
        bool sub_is_dir = (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;

        if (matcher.is_ignored(subpath, sub_is_dir)) {
            continue;
        }

        if (sub_is_dir) {
            add_path(subpath);
        } else {
            add_file(subpath);
        }

    } while (FindNextFileA(hFind, &fd) != 0);

    FindClose(hFind);
}

bool Index::remove_entry(const std::string& rel_path, bool delete_working_file) {
    std::string norm = normalize_path(rel_path);

    auto it = entries.find(norm);
    if (it == entries.end()) {
        std::cerr << "FATAL : pathspec '" << rel_path << "' did not match any files\n";
        return false;
    }

    // Remove from in-memory staging table
    entries.erase(it);

    // Delete working directory copy if requested
    if (delete_working_file) {
        if (!DeleteFileA(norm.c_str())) {
            DWORD err = GetLastError();
            if (err != ERROR_FILE_NOT_FOUND) {
                std::cerr << "WARNING : unable to delete file from disk : " << norm << "\n";
            }
        }
    }

    return true;
}

bool Index::restore_file(const std::string& rel_path) {
    std::string norm = normalize_path(rel_path);

    auto it = entries.find(norm);
    if (it == entries.end()) {
        std::cerr << "FATAL: pathspec '" << rel_path << "' did not match any file in the index\n";
        return false;
    }

    // Read stored blob
    auto obj = object_db::read_object(it->second.sha);
    std::ofstream out(norm.c_str(), std::ios::binary);
    if (!out.is_open()) {
        std::cerr << "FATAL: unable to write to " << norm << "\n";
        return false;
    }

    out.write(obj.second.data(), obj.second.size());
    out.close();
    std::cout << "Restored " << norm << "\n";
    return true;
}