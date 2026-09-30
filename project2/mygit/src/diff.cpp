#include "diff.hpp"
#include "index.hpp"
#include "repository.hpp"
#include "commit.hpp"
#include "object.hpp"
#include "tree.hpp"
#include "utils.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <algorithm>

namespace diff_ops {

namespace {

    std::string clean_path(std::string p) {
        for (char& c : p) {
            if (c == '\\') c = '/';
        }
        while (p.rfind("./", 0) == 0) {
            p = p.substr(2);
        }
        return p;
    }

    std::vector<std::string> split_lines(const std::string& text) {
        std::vector<std::string> lines;
        std::istringstream stream(text);
        std::string line;
        while (std::getline(stream, line)) {
            while (!line.empty() && (line.back() == '\r' || line.back() == '\n')) {
                line.pop_back();
            }
            lines.push_back(line);
        }
        return lines;
    }

    std::vector<std::vector<int>> compute_lcs(const std::vector<std::string>& a,
                                              const std::vector<std::string>& b) {
        int n = static_cast<int>(a.size());
        int m = static_cast<int>(b.size());
        std::vector<std::vector<int>> dp(n + 1, std::vector<int>(m + 1, 0));

        for (int i = 0; i < n; ++i) {
            for (int j = 0; j < m; ++j) {
                if (a[i] == b[j]) {
                    dp[i + 1][j + 1] = dp[i][j] + 1;
                } else {
                    dp[i + 1][j + 1] = std::max(dp[i + 1][j], dp[i][j + 1]);
                }
            }
        }
        return dp;
    }

    struct DiffLine {
        char tag; // ' ', '+', '-'
        std::string content;
    };

    std::vector<DiffLine> generate_diff_lines(const std::vector<std::string>& a,
                                             const std::vector<std::string>& b) {
        auto dp = compute_lcs(a, b);
        int i = static_cast<int>(a.size());
        int j = static_cast<int>(b.size());
        std::vector<DiffLine> result;

        while (i > 0 || j > 0) {
            if (i > 0 && j > 0 && a[i - 1] == b[j - 1]) {
                result.push_back({' ', a[i - 1]});
                --i;
                --j;
            } else if (j > 0 && (i == 0 || dp[i][j - 1] >= dp[i - 1][j])) {
                result.push_back({'+', b[j - 1]});
                --j;
            } else if (i > 0 && (j == 0 || dp[i][j - 1] < dp[i - 1][j])) {
                result.push_back({'-', a[i - 1]});
                --i;
            } else {
                break;
            }
        }

        std::reverse(result.begin(), result.end());
        return result;
    }

    void flatten_tree(const std::string& tree_sha, const std::string& prefix,
                      std::map<std::string, std::string>& out_files) {
        if (tree_sha.empty()) return;

        auto result = object_db::read_object(tree_sha);
        if (result.first != ObjectType::TREE) return;

        Tree tree;
        tree.deserialize(result.second);

        for (const auto& entry : tree.entries) {
            std::string full_name = prefix.empty() ? entry.name : prefix + "/" + entry.name;
            if (entry.mode == "40000") {
                flatten_tree(entry.sha_hex, full_name, out_files);
            } else {
                out_files[clean_path(full_name)] = entry.sha_hex;
            }
        }
    }

    std::string read_disk_file(const std::string& path) {
        std::ifstream file(path.c_str(), std::ios::binary);
        if (!file.is_open()) return "";
        std::stringstream ss;
        ss << file.rdbuf();
        return ss.str();
    }

} // anonymous namespace

void print_unified_diff(const std::string& old_label, const std::string& old_text,
                        const std::string& new_label, const std::string& new_text) {
    std::vector<std::string> lines_a = split_lines(old_text);
    std::vector<std::string> lines_b = split_lines(new_text);

    if (lines_a == lines_b) return;

    std::vector<DiffLine> diff = generate_diff_lines(lines_a, lines_b);

    std::cout << "\033[1mdiff --mygit a/" << old_label << " b/" << new_label << "\033[0m\n";
    std::cout << "\033[1m--- a/" << old_label << "\033[0m\n";
    std::cout << "\033[1m+++ b/" << new_label << "\033[0m\n";
    std::cout << "\033[36m@@ -1," << lines_a.size() << " +1," << lines_b.size() << " @@\033[0m\n";

    for (const auto& dl : diff) {
        if (dl.tag == '+') {
            std::cout << "\033[32m+" << dl.content << "\033[0m\n";
        } else if (dl.tag == '-') {
            std::cout << "\033[31m-" << dl.content << "\033[0m\n";
        } else {
            std::cout << " " << dl.content << "\n";
        }
    }
}

void show_diff_working_vs_index() {
    Index idx = Index::read();
    for (const auto& pair : idx.entries) {
        std::string path = clean_path(pair.first);
        const std::string& staged_sha = pair.second.sha;

        std::string current_content = read_disk_file(path);
        
        try {
            auto obj = object_db::read_object(staged_sha);
            std::string staged_content = obj.second;
            print_unified_diff(path, staged_content, path, current_content);
        } catch (...) {
            // Ignore missing blobs
        }
    }
}

void show_diff_index_vs_head() {
    std::string head_commit = commit_helper::get_head_commit();
    std::map<std::string, std::string> head_files;

    if (!head_commit.empty()) {
        try {
            auto commit_obj = object_db::read_object(head_commit);
            Commit c;
            c.deserialize(commit_obj.second);
            flatten_tree(c.tree_sha, "", head_files);
        } catch (...) {}
    }

    Index idx = Index::read();
    for (const auto& pair : idx.entries) {
        std::string path = clean_path(pair.first);
        const std::string& staged_sha = pair.second.sha;

        std::string head_content = "";
        auto it = head_files.find(path);
        if (it != head_files.end()) {
            try {
                head_content = object_db::read_object(it->second).second;
            } catch (...) {}
        }

        try {
            std::string staged_content = object_db::read_object(staged_sha).second;
            print_unified_diff(path, head_content, path, staged_content);
        } catch (...) {}
    }
}

} // namespace diff_ops