#include "tree.hpp"
#include "utils.hpp"
#include "object.hpp"
#include <algorithm>
#include <stdexcept>
#include <fstream>
#include <sstream>
#include <windows.h>

std::string Tree::serialize() const {
    std::vector<TreeEntry> sorted_entries = entries;
    std::sort(sorted_entries.begin(), sorted_entries.end(), 
        [](const TreeEntry& a, const TreeEntry& b) {
            return a.name < b.name;
        }
    );

    std::string out;
    for (const auto& entry : sorted_entries) {
        out += entry.mode + " " + entry.name;
        out.push_back('\0');
        out += utils::hex_to_bytes(entry.sha_hex);
    }
    return out;
}

void Tree::deserialize(const std::string& data) {
    entries.clear();
    size_t pos = 0;

    while (pos < data.size()) {
        size_t space_pos = data.find(' ', pos);
        if (space_pos == std::string::npos) {
            throw std::runtime_error("Corrupt tree: missing space after mode");
        }
        std::string mode = data.substr(pos, space_pos - pos);

        size_t null_pos = data.find('\0', space_pos);
        if (null_pos == std::string::npos) {
            throw std::runtime_error("Corrupt tree: missing null after filename");
        }
        std::string name = data.substr(space_pos + 1, null_pos - space_pos - 1);

        if (null_pos + 1 + 20 > data.size()) {
            throw std::runtime_error("Corrupt tree: unexpected EOF reading SHA");
        }
        std::string raw_sha = data.substr(null_pos + 1, 20);
        std::string sha_hex = utils::bytes_to_hex(raw_sha);

        entries.push_back({mode, name, sha_hex});
        pos = null_pos + 1 + 20;
    }
}

namespace tree_builder {

std::string write_tree_from_directory(const std::string& dir_path) {
    std::vector<TreeEntry> entries;
    std::string search_pattern = dir_path + "/*";

    WIN32_FIND_DATAA find_data;
    HANDLE hFind = FindFirstFileA(search_pattern.c_str(), &find_data);

    if (hFind == INVALID_HANDLE_VALUE) {
        throw std::runtime_error("fatal: unable to open directory: " + dir_path);
    }

    do {
        std::string filename = find_data.cFileName;

        // Skip current dir, parent dir, .mygit repo, build folder, and compiler test artifacts
        if (filename == "." || filename == ".." || filename == ".mygit" || 
            filename == "build" || filename == "main_test.o" || filename == "mygit.exe") {
            continue;
        }

        std::string full_path = dir_path + "/" + filename;

        if (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            std::string sub_tree_sha = write_tree_from_directory(full_path);
            if (!sub_tree_sha.empty()) {
                entries.push_back({"40000", filename, sub_tree_sha});
            }
        } else {
            std::ifstream file(full_path.c_str(), std::ios::binary);
            if (!file.is_open()) {
                continue;
            }

            std::stringstream buffer;
            buffer << file.rdbuf();
            std::string content = buffer.str();

            std::string blob_sha = object_db::write_object(ObjectType::BLOB, content);
            entries.push_back({"100644", filename, blob_sha});
        }
    } while (FindNextFileA(hFind, &find_data) != 0);

    FindClose(hFind);

    if (entries.empty()) {
        return "";
    }

    Tree tree(entries);
    return tree.write();
}

} // namespace tree_builder