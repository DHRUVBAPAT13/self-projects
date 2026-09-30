#include "tag.hpp"
#include "repository.hpp"
#include "commit.hpp"
#include "utils.hpp"
#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <windows.h>

namespace tag_ops {

namespace {

    std::string get_tags_dir() {
        std::string root = repository::find_root(".");
        if (root.empty()) {
            throw std::runtime_error("fatal: not a mygit repository");
        }
        std::string tags_dir = root + "/.mygit/refs/tags";
        // Ensure .mygit/refs/tags directory exists
        CreateDirectoryA((root + "/.mygit/refs").c_str(), NULL);
        CreateDirectoryA(tags_dir.c_str(), NULL);
        return tags_dir;
    }

} // anonymous namespace

void list_tags() {
    std::string tags_dir = get_tags_dir();
    std::string search = tags_dir + "/*";

    WIN32_FIND_DATAA fd;
    HANDLE h = FindFirstFileA(search.c_str(), &fd);
    if (h == INVALID_HANDLE_VALUE) {
        return;
    }

    std::vector<std::string> tags;
    do {
        std::string name = fd.cFileName;
        if (name != "." && name != "..") {
            tags.push_back(name);
        }
    } while (FindNextFileA(h, &fd) != 0);

    FindClose(h);

    std::sort(tags.begin(), tags.end());
    for (const auto& tag : tags) {
        std::cout << tag << "\n";
    }
}

void create_tag(const std::string& name, const std::string& commit_sha) {
    std::string target_sha = commit_sha;

    if (target_sha.empty()) {
        target_sha = commit_helper::get_head_commit();
        if (target_sha.empty()) {
            std::cerr << "fatal: cannot create tag without any commits\n";
            return;
        }
    }

    if (target_sha.length() != 40) {
        std::cerr << "fatal: invalid object name '" << target_sha << "'\n";
        return;
    }

    std::string tags_dir = get_tags_dir();
    std::string tag_path = tags_dir + "/" + name;

    // Check if tag already exists
    DWORD attr = GetFileAttributesA(tag_path.c_str());
    if (attr != INVALID_FILE_ATTRIBUTES) {
        std::cerr << "fatal: tag '" << name << "' already exists\n";
        return;
    }

    std::ofstream out(tag_path.c_str());
    if (!out.is_open()) {
        std::cerr << "fatal: unable to write tag file\n";
        return;
    }

    out << target_sha << "\n";
    out.close();

    std::cout << "Created tag '" << name << "' pointing to " << target_sha.substr(0, 7) << "\n";
}

void delete_tag(const std::string& name) {
    std::string tags_dir = get_tags_dir();
    std::string tag_path = tags_dir + "/" + name;

    if (!DeleteFileA(tag_path.c_str())) {
        std::cerr << "error: tag '" << name << "' not found.\n";
        return;
    }

    std::cout << "Deleted tag '" << name << "'\n";
}

} // namespace tag_ops