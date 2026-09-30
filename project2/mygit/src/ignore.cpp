#include "ignore.hpp"
#include "repository.hpp"
#include <fstream>
#include <sstream>
#include <algorithm>

namespace {

    std::string clean_str(std::string s) {
        while (!s.empty() && (s.back() == '\r' || s.back() == '\n' || s.back() == ' ' || s.back() == '\t')) {
            s.pop_back();
        }
        size_t start = s.find_first_not_of(" \t");
        if (start == std::string::npos) return "";
        s = s.substr(start);

        for (char& c : s) {
            if (c == '\\') c = '/';
        }
        return s;
    }

    bool wildcmp(const char* pattern, const char* str) {
        const char* cp = nullptr;
        const char* mp = nullptr;

        while (*str) {
            if (*pattern == '*') {
                if (!*++pattern) return true;
                mp = pattern;
                cp = str + 1;
            } else if (*pattern == *str || *pattern == '?') {
                pattern++;
                str++;
            } else if (cp) {
                pattern = mp;
                str = cp++;
            } else {
                return false;
            }
        }

        while (*pattern == '*') {
            pattern++;
        }
        return !*pattern;
    }

} // anonymous namespace

IgnoreMatcher::IgnoreMatcher() {
    // Default core patterns
    patterns.push_back(".mygit");
}

void IgnoreMatcher::load_rules() {
    std::string root = repository::find_root(".");
    if (root.empty()) return;

    std::string ignore_file = root + "/.mygitignore";
    std::ifstream file(ignore_file.c_str());
    if (!file.is_open()) return;

    std::string line;
    while (std::getline(file, line)) {
        std::string p = clean_str(line);
        if (p.empty() || p[0] == '#') continue;
        patterns.push_back(p);
    }
}

bool IgnoreMatcher::match_pattern(const std::string& pat, const std::string& path, bool is_dir) const {
    if (pat.empty() || path.empty()) return false;

    std::string pattern = pat;

    bool dir_only = false;
    if (!pattern.empty() && pattern.back() == '/') {
        dir_only = true;
        pattern.pop_back();
    }

    if (dir_only && !is_dir) {
        return false;
    }

    size_t last_slash = path.find_last_of('/');
    std::string basename = (last_slash == std::string::npos) ? path : path.substr(last_slash + 1);

    if (pattern == basename || pattern == path) {
        return true;
    }

    if (wildcmp(pattern.c_str(), basename.c_str())) {
        return true;
    }

    if (wildcmp(pattern.c_str(), path.c_str())) {
        return true;
    }

    return false;
}

bool IgnoreMatcher::is_ignored(const std::string& rel_path, bool is_dir) const {
    std::string norm = clean_str(rel_path);
    while (norm.rfind("./", 0) == 0) {
        norm = norm.substr(2);
    }

    // Never ignore root or empty path
    if (norm.empty() || norm == ".") return false;

    for (const auto& pat : patterns) {
        if (match_pattern(pat, norm, is_dir)) {
            return true;
        }
    }
    return false;
}