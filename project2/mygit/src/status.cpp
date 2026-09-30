#include "status.hpp"
#include "repository.hpp"
#include "commit.hpp"
#include "object.hpp"
#include "tree.hpp"
#include "index.hpp"
#include "ignore.hpp"
#include "utils.hpp"

#include <iostream>
#include <fstream>
#include <sstream>
#include <set>
#include <windows.h>

namespace status_ops
{
    
namespace{

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

    // recursively extracts all fle paths and their blob hashes from a root tree object
    void flatten_tree(const std::string& tree_sha, const std::string& prefix, std::map<std::string, std::string>& out_files){

        if(tree_sha.empty()) return;

        auto result = object_db::read_object(tree_sha);

        if(result.first != ObjectType::TREE) return;

        Tree tree;
        tree.deserialize(result.second);

        for(const auto& entry : tree.entries){
            std::string full_name = prefix.empty() ? entry.name : prefix + "/" + entry.name;

            if(entry.mode == "40000"){
                flatten_tree(entry.sha_hex, full_name, out_files);
            }
            else{
                out_files[full_name] = entry.sha_hex;
            }
        }
    }

    // recursively scans the working tree for all non-ignored file paths
    void scan_working_dir(const std::string& dir_path, std::set<std::string>& out_paths, const IgnoreMatcher& matcher) {
        
        std::string search = dir_path + "/*";
        WIN32_FIND_DATAA fd;
        HANDLE h = FindFirstFileA(search.c_str(), &fd);

        if (h == INVALID_HANDLE_VALUE) return;

        do {
            std::string name = fd.cFileName;
            if (name == "." || name == "..") {
                continue;
            }

            std::string full = (dir_path == "." ? name : dir_path + "/" + name);
            bool is_dir = (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;

            std::string normalized = normalize_path(full);

            // Check ignore rules
            if (matcher.is_ignored(normalized, is_dir)) {
                continue;
            }

            if (is_dir) {
                scan_working_dir(full, out_paths, matcher);
            } else {
                out_paths.insert(normalized);
            }
        } while (FindNextFileA(h, &fd) != 0);

        FindClose(h);
    }

    // reads a file from disk and computes its standard git blob SHA
    std::string hash_file_on_disk(const std::string& path){

        std::ifstream file(path.c_str(), std::ios::binary);
        if(!file.is_open()) return "";

        std::stringstream buf;
        buf << file.rdbuf();
        std::string content  = buf.str();

        std::string header = "blob " + std::to_string(content.size());
        header.push_back('\0');

        return utils::sha1(header + content);

    }
}

void show_status(){

    std::string head_commit_sha = commit_helper::get_head_commit();
    std::map<std::string, std::string> head_files;

    if(!head_commit_sha.empty()){
        try{
            auto commit_obj = object_db::read_object(head_commit_sha);
            Commit c;
            c.deserialize(commit_obj.second);
            flatten_tree(c.tree_sha, "", head_files);
        }
        catch(...){
            // Unborn branch or empty repo
        }
        
    }

    Index idx = Index::read();

    std::map<std::string, IndexEntry> index_files;
    for (const auto& item : idx.entries) {
        index_files[normalize_path(item.first)] = item.second;
    }

    IgnoreMatcher matcher;
    matcher.load_rules();

    std::set<std::string> working_files;
    scan_working_dir(".", working_files, matcher);

    FileStatus st;

    // 1. compare HEAD vs Index (staged changes)
    for(const auto& item : index_files){
        const std::string& path = item.first;
        const std::string&staged_sha = item.second.sha;

        auto it = head_files.find(path);

        if(it == head_files.end()){
            st.staged_new.push_back(path);
        }
        else if(it->second != staged_sha){
            st.staged_modified.push_back(path);
        }
    }
    for(const auto& item : head_files){
        if(index_files.find(item.first) == index_files.end()){
            st.staged_deleted.push_back(item.first);
        }
    }

    // 2. compare Index vs Working Directory (unstaged changes)
    for(const auto& item : index_files){
        const std::string& path = item.first;
        const std::string&staged_sha = item.second.sha;

        if(working_files.find(path) == working_files.end()){
            st.unstaged_deleted.push_back(path);
        }
        else{
            std::string disk_sha = hash_file_on_disk(path);
            if(disk_sha != staged_sha && !disk_sha.empty()){
                st.unstaged_modified.push_back(path);
            }
        }    
    }

    // 3. Untracked files (Present on disk, but not in index and HEAD)
    for (const auto& path : working_files) {
        if ((index_files.find(path) == index_files.end()) && (head_files.find(path) == head_files.end())) {
            st.untracked.push_back(path);
        }
    }

    // 4. output results with ANSI coloring
    bool clean = true;

    if(!st.staged_new.empty() || !st.staged_modified.empty() || !st.staged_deleted.empty()){
            clean = false;

            std::cout << "Changes to be commited:\n";
            std::cout << "  (use \"mygit commit -m ...\" to commit)\n";

            for(const auto& f : st.staged_new) std::cout << "\033[32m\tnew file:    " << f << "\033[0m\n";
            for(const auto& f : st.staged_modified) std::cout << "\033[32m\tmodified:    " << f << "\033[0m\n";
            for(const auto& f : st.staged_deleted) std::cout << "\033[32m\tdeleted:    " << f << "\033[0m\n";
            std::cout << std::endl;
        }

    if(!st.unstaged_deleted.empty() || !st.unstaged_modified.empty()){
            clean = false;

            std::cout << "Changes not staged for commit:\n";
            std::cout << "  (use \"mygit add <file>...\" to update what will be committed)\n";
            
            for(const auto& f : st.unstaged_modified) std::cout << "\033[32m\tmodified:    " << f << "\033[0m\n";
            for(const auto& f : st.unstaged_deleted) std::cout << "\033[32m\tdeleted:    " << f << "\033[0m\n";
            std::cout << std::endl;
        }

    if(!st.untracked.empty()){
            clean = false;
            std::cout << "Untracked files:\n";
            std::cout << "  (use \"mygit add <file>...\" to include in what will be committed)\n";
            for(const auto& f : st.untracked) std::cout << "\033[31m\t" << f << "\033[0m\n";
            std::cout << std::endl;
        }

    if(clean){
        std::cout << "nothing to commit, working tree clean\n";
    }

}

} // namespace status_ops
