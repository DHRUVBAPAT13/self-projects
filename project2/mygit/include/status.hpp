#pragma once

#include <string>
#include <vector>
#include <map>

namespace status_ops
{
    struct FileStatus{
        std::vector<std::string> staged_new;
        std::vector<std::string> staged_modified;
        std::vector<std::string> staged_deleted;

        std::vector<std::string> unstaged_modified;
        std::vector<std::string> unstaged_deleted;

        std::vector<std::string> untracked;
    };

    // computes and prints the full Git status output
    void show_status();

} // namespace status_ops
