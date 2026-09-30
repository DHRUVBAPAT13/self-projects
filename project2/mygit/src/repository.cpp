#include "repository.hpp"
#include "miniz.h"
#include <iostream>
#include <fstream>
#include <direct.h>
#include <windows.h>

namespace repository{

namespace {
    // cross platform / windows directory creation
    bool make_dir(const std::string& path){
        int res = _mkdir(path.c_str());
        return (res == 0 || errno == EEXIST);
    }

    bool dir_exists(const std::string& path) {
        DWORD attribs = GetFileAttributesA(path.c_str());

        return (attribs != INVALID_FILE_ATTRIBUTES && (attribs & FILE_ATTRIBUTE_DIRECTORY));
    }
}

bool init(const std::string& path){
    std::string git_dir = path + "/.mygit";

    if(dir_exists(git_dir)){
        std::cout << "Reinitialized existing mygit repository in " << git_dir << std::endl;
        return true;
    }

    // 1. create directory structure: .mygit, .mygit/objects, .mygit/refs/heads
    if(!make_dir(git_dir) || !make_dir(git_dir + "/objects") || !make_dir(git_dir + "/refs") || !make_dir(git_dir + "/refs/heads")){

        std::cerr << "FATAL : Cannot Create Repository directories." << std::endl;
        return false;
    }

    // 2. create head file pointing to refs/heads/main
    std::string head_path = git_dir + "/HEAD";
    std::ofstream head_file(head_path.c_str());
    if(!head_file.is_open()){
        std::cerr << "fatal: cannot create HEAD reference file." << std::endl;
    }

    head_file << "ref : refs/heads/main";
    head_file.close();

    std::cout << "Initialized empty mygit repository in " << git_dir << std::endl;

}

std::string find_root(const std::string& start_path){

    std::string current = start_path;

    while (true) {
        if (dir_exists(current + "/.mygit")) {
            return current;
        }

        // Move to parent directory
        std::string parent = current + "/..";
        char resolved[MAX_PATH];
        if (!_fullpath(resolved, parent.c_str(), MAX_PATH)) {
            break;
        }

        // Reached filesystem root
        if (current == resolved) {
            break;
        }
        current = resolved;
    }
    return "";
}

std::string repo_path(const std::string& subpath, const std::string& root){
    
    std::string base = root.empty() ? find_root() : root;
    
    if (base.empty()) {
        throw std::runtime_error("fatal: not a mygit repository (or any of the parent directories)");
    }

    return base + "/.mygit/" + subpath;
}

}
