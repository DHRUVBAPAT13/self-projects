#include "commit.hpp"
#include "repository.hpp"

#include <sstream>
#include <fstream>
#include <ctime>
#include <stdexcept>
#include <map>
#include <algorithm>
#include <iostream>
#include <windows.h>

std::string Commit::serialize() const {
    std::ostringstream ss;
    ss << "tree " << tree_sha << "\n";
    for (const auto& parent : parent_shas) {
        ss << "parent " << parent << "\n";
    }
    ss << "author " << author << "\n";
    // Must have an extra newline separating the headers from the commit message
    ss << "committer " << committer << "\n\n";
    ss << message << "\n";

    return ss.str();
}

void Commit::deserialize(const std::string& data) {
    std::istringstream stream(data);
    std::string line;
    parent_shas.clear();
    message.clear();

    // 1. Parse header lines
    while (std::getline(stream, line) && !line.empty()) {
        if (line.rfind("tree ", 0) == 0) {
            tree_sha = line.substr(5);
        } else if (line.rfind("parent ", 0) == 0) {
            parent_shas.push_back(line.substr(7));
        } else if (line.rfind("author ", 0) == 0) {
            author = line.substr(7);
        } else if (line.rfind("committer ", 0) == 0) {
            committer = line.substr(10);
        }
    }

    // 2. Remaining lines form commit message
    std::ostringstream msg_stream;
    while (std::getline(stream, line)) {
        msg_stream << line << "\n";
    }
    message = msg_stream.str();
    if (!message.empty() && message.back() == '\n') {
        message.pop_back();
    }
}

namespace commit_helper {

std::string get_head_commit() {
    std::string head_path = repository::repo_path("HEAD");
    std::ifstream head_file(head_path.c_str());
    if (!head_file.is_open()) {
        return "";
    }

    std::string line;
    std::getline(head_file, line);
    head_file.close();

    // Strip carriage return and trailing spaces
    while (!line.empty() && (line.back() == '\r' || line.back() == '\n' || line.back() == ' ')) {
        line.pop_back();
    }

    // Handle symbolic reference (support both "ref:" and "ref :")
    size_t colon_pos = line.find(':');
    if (line.rfind("ref", 0) == 0 && colon_pos != std::string::npos) {
        std::string ref_subpath = line.substr(colon_pos + 1);
        
        // Trim leading spaces
        size_t first = ref_subpath.find_first_not_of(" \t");
        if (first == std::string::npos) return "";
        ref_subpath = ref_subpath.substr(first);

        std::string ref_full_path = repository::repo_path(ref_subpath);
        std::ifstream ref_file(ref_full_path.c_str());
        
        if (!ref_file.is_open()) {
            return ""; // Initial branch has no commits yet
        }

        std::string commit_sha;
        std::getline(ref_file, commit_sha);
        while (!commit_sha.empty() && (commit_sha.back() == '\r' || commit_sha.back() == '\n' || commit_sha.back() == ' ')) {
            commit_sha.pop_back();
        }
        return commit_sha;
    }

    return line;
}

void update_head(const std::string& new_commit_sha) {
    std::string head_path = repository::repo_path("HEAD");
    std::ifstream head_file(head_path.c_str());
    std::string line;
    if (head_file.is_open()) {
        std::getline(head_file, line);
        head_file.close();
    }

    while (!line.empty() && (line.back() == '\r' || line.back() == '\n' || line.back() == ' ')) {
        line.pop_back();
    }

    size_t colon_pos = line.find(':');
    if (line.rfind("ref", 0) == 0 && colon_pos != std::string::npos) {
        std::string ref_subpath = line.substr(colon_pos + 1);
        size_t first = ref_subpath.find_first_not_of(" \t");
        if (first != std::string::npos) {
            ref_subpath = ref_subpath.substr(first);
        }

        std::string ref_full_path = repository::repo_path(ref_subpath);
        std::ofstream ref_file(ref_full_path.c_str());
        if (!ref_file.is_open()) {
            throw std::runtime_error("fatal: unable to write reference: " + ref_full_path);
        }
        ref_file << new_commit_sha << "\n";
    } else {
        std::ofstream out_head(head_path.c_str());
        out_head << new_commit_sha << "\n";
    }
}

std::string create_commit(const std::string& tree_sha, const std::string& message, const std::string& parent_sha) {
    Commit c;
    c.tree_sha = tree_sha;
    if (!parent_sha.empty()) {
        c.parent_shas.push_back(parent_sha);
    }

    std::time_t now = std::time(nullptr);
    std::string timestamp_str = std::to_string(now) + " +0000";

    std::string ident = "Dhruv <dhruv@example.com> " + timestamp_str;
    c.author = ident;
    c.committer = ident;
    c.message = message;

    std::string commit_sha = c.write();
    update_head(commit_sha);
    return commit_sha;
}

namespace {

    std::string get_current_head_branch(const std::string& root) {
        std::ifstream head_file((root + "/.mygit/HEAD").c_str());
        if (!head_file.is_open()) return "";

        std::string line;
        std::getline(head_file, line);
        while (!line.empty() && (line.back() == '\r' || line.back() == '\n' || line.back() == ' ')) {
            line.pop_back();
        }

        // Support both "ref: refs/heads/..." and "ref : refs/heads/..."
        std::string prefix1 = "ref: refs/heads/";
        std::string prefix2 = "ref : refs/heads/";
        if (line.rfind(prefix1, 0) == 0) {
            return line.substr(prefix1.size());
        } else if (line.rfind(prefix2, 0) == 0) {
            return line.substr(prefix2.size());
        }
        return "";
    }

    std::map<std::string, std::vector<std::string>> collect_decorations(const std::string& root) {
        std::map<std::string, std::vector<std::string>> decorations;
        std::string current_branch = get_current_head_branch(root);

        // 1. Inspect Branches in .mygit/refs/heads
        std::string branches_dir = root + "/.mygit/refs/heads";
        WIN32_FIND_DATAA fd;
        HANDLE h = FindFirstFileA((branches_dir + "/*").c_str(), &fd);
        if (h != INVALID_HANDLE_VALUE) {
            do {
                std::string name = fd.cFileName;
                if (name != "." && name != "..") {
                    std::ifstream f((branches_dir + "/" + name).c_str());
                    std::string sha;
                    if (f >> sha) {
                        if (!current_branch.empty() && name == current_branch) {
                            decorations[sha].insert(decorations[sha].begin(),
                                "\033[1;36mHEAD -> \033[1;32m" + name + "\033[0m");
                        } else {
                            decorations[sha].push_back("\033[1;32m" + name + "\033[0m");
                        }
                    }
                }
            } while (FindNextFileA(h, &fd) != 0);
            FindClose(h);
        }

        // 2. Inspect Tags in .mygit/refs/tags
        std::string tags_dir = root + "/.mygit/refs/tags";
        h = FindFirstFileA((tags_dir + "/*").c_str(), &fd);
        if (h != INVALID_HANDLE_VALUE) {
            do {
                std::string name = fd.cFileName;
                if (name != "." && name != "..") {
                    std::ifstream f((tags_dir + "/" + name).c_str());
                    std::string sha;
                    if (f >> sha) {
                        decorations[sha].push_back("\033[1;33mtag: " + name + "\033[0m");
                    }
                }
            } while (FindNextFileA(h, &fd) != 0);
            FindClose(h);
        }

        return decorations;
    }

} // anonymous namespace

void print_graph_log() {
    std::string root = repository::find_root(".");
    if (root.empty()) {
        std::cerr << "fatal: not a mygit repository\n";
        return;
    }

    std::string current_sha = get_head_commit();
    if (current_sha.empty()) {
        std::cout << "fatal: your current branch does not have any commits yet\n";
        return;
    }

    auto decorations = collect_decorations(root);

    while (!current_sha.empty()) {
        auto obj = object_db::read_object(current_sha);
        if (obj.first != ObjectType::COMMIT) break;

        Commit c;
        c.deserialize(obj.second);

        std::string dec_str = "";
        auto it = decorations.find(current_sha);
        if (it != decorations.end() && !it->second.empty()) {
            dec_str = " (";
            for (size_t i = 0; i < it->second.size(); ++i) {
                dec_str += it->second[i];
                if (i + 1 < it->second.size()) dec_str += ", ";
            }
            dec_str += ")";
        }

        // Print ASCII node with commit hash, decorations, and commit summary
        std::cout << "\033[1;33m* \033[1;33m" << current_sha.substr(0, 7) << "\033[0m"
                  << dec_str << " - " << c.message << "\n";

        if (!c.parent_shas.empty()) {
            std::cout << "\033[1;33m|\033[0m\n";
            current_sha = c.parent_shas[0];
        } else {
            current_sha = "";
        }
    }
}

} // namespace commit_helper