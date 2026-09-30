#pragma once

#include <string>
#include <vector>

class IgnoreMatcher{
    public:

        IgnoreMatcher();

        // loads patterns from .mygitignore located in the repo root
        void load_rules();

        // returns true if a path matches any ignore rule
        bool is_ignored(const std::string& rel_path, bool is_dir) const;

    private:

        std::vector<std::string> patterns;

        // checks wildcards like *.exe, logs/*, or simple string matching
        bool match_pattern(const std::string& pat, const std::string& path, bool is_dir) const;
};