#include "branch.hpp"
#include "repository.hpp"
#include "commit.hpp"
#include "object.hpp"
#include "tree.hpp"

#include <iostream>
#include <fstream>
#include <windows.h>

namespace branch_ops{
    
namespace{
    
    bool make_directory(const std::string& path){

        if(CreateDirectoryA(path.c_str(), NULL)) return true;

        return (GetLastError() == ERROR_ALREADY_EXISTS);
    }

    std::string get_current_branch_name(){

        std::string head_path = repository::repo_path("HEAD");
        std::ifstream head_file(head_path.c_str());

        if(!head_file.is_open()) return "";

        std::string line;
        std::getline(head_file, line);
        size_t colon_pos = line.rfind(':');

        if(line.rfind("ref", 0) == 0 && colon_pos != std::string::npos){

            std::string ref = line.substr(colon_pos + 1);
            size_t first = ref.find_first_not_of(" \t");

            if(first != std::string::npos) ref = ref.substr(first);

            while (!ref.empty() && (ref.back() == '\r' || ref.back() == '\n' || ref.back() == ' ')){
                ref.pop_back();
            }

            std::string prefix = "refs/heads/";
            if(ref.rfind(prefix, 0) == 0){

                return ref.substr(prefix.size());
            }
        }

        return "";
    }

    //
    void restore_tree(const std::string& tree_sha, const std::string& target_dir){
        auto result = object_db::read_object(tree_sha);

        if(result.first != ObjectType::TREE){

            throw std::runtime_error("Object is not a TREE : " + tree_sha);
        }

        Tree tree;
        tree.deserialize(result.second);

        for(const auto& entry : tree.entries){
            std::string path = target_dir + "/" + entry.name;

            if(entry.mode == "40000"){
                make_directory(path);
                restore_tree(entry.sha_hex, path);
            }
            else{
                auto blob_data = object_db::read_object(entry.sha_hex);
                std::ofstream out(path.c_str(), std::ios::binary);

                if(out.is_open()){
                    out.write(blob_data.second.data(), blob_data.second.size());
                }
            }
        }
    }

} // namespace

void list_branches(){
    std::string heads_dir = repository::repo_path("refs/heads");
    std::string current_branch = get_current_branch_name();

    std::string search = heads_dir + "/*";
    WIN32_FIND_DATAA fd;
    HANDLE h = FindFirstFileA(search.c_str(), &fd);

    if(h == INVALID_HANDLE_VALUE) return;

    do{
        std::string name = fd.cFileName;

        if(name == "." || name == "..") continue;

        if(name == current_branch){
            std::cout << "* \033[32m" << name << "\033[0m\n";
        }
        else{
            std::cout << " " << name << "\n";
        }
    }while(FindNextFileA(h, &fd) != 0);
    
    FindClose(h);

}

bool create_branch(const std::string& branch_name) {
    std::string current_commit = commit_helper::get_head_commit();

    if (current_commit.empty()) {
        std::cerr << "fatal: not a valid object name: 'HEAD'\n";
        return false;
    }

    std::string branch_path = repository::repo_path("refs/heads/" + branch_name);
    DWORD attribs = GetFileAttributesA(branch_path.c_str());

    if (attribs != INVALID_FILE_ATTRIBUTES) {
        std::cerr << "fatal: a branch named '" << branch_name << "' already exists.\n";
        return false;
    }

    std::ofstream out(branch_path.c_str());

    if(!out.is_open()){
        std::cerr << "FATAL : Cannot create a branch reference file";
        return false;
    }
    out << current_commit << "\n";
    std::cout << "Created Branch " << branch_name << " at " << current_commit.substr(0, 7) << "\n";

    return true;

}

bool checkout(const std::string& target){

    std::string branch_path = repository::repo_path("refs/heads/" + target);
    DWORD attribs = GetFileAttributesA(branch_path.c_str());
    std::string commit_sha;
    bool is_branch = false;

    if(attribs != INVALID_FILE_ATTRIBUTES){
        // checking out an existing branch
        std::ifstream in(branch_path.c_str());
        std::getline(in, commit_sha);

        while(!commit_sha.empty() && (commit_sha.back() == '\r' || commit_sha.back() == '\n' || commit_sha.back() == ' ')){

            commit_sha.pop_back();
        }
        is_branch = true;
    }
    else{
        // checking out a raw commit SHA (detached HEAD)
        commit_sha = target;

    }

    // validate commit and obtain root tree
    auto obj = object_db::read_object(commit_sha);

    if(obj.first != ObjectType::COMMIT){
        std::cerr << "FATAL : Reference is not a commit : " << target << "\n";
        
        return false;
    }

    Commit c;
    c.deserialize(obj.second);

    // unpack tree to root directory
    std::string root = repository::find_root(".");
    restore_tree(c.tree_sha, root);

    // update HEAD
    std::string head_path = repository::repo_path("HEAD");
    std::ofstream head_file(head_path.c_str());

    if(is_branch){

        head_file << "ref: refs/heads/" + target << "\n";
        std::cout << "Switched to branch " << target << "\n";
    }
    else{
        head_file << commit_sha << "\n";
        std::cout << "Note : Switching to detached HEAD at " << commit_sha.substr(0, 7) << "\n";
    }

    return true;
    
}

} // namespace branch_ops