#pragma once

#include <string>
#include <vector>

namespace diff_ops
{
    // Computes and prints unified diff between two text strings
    void print_unified_diff(const std::string& old_label, const std::string& old_text, const std::string& new_label, const std::string& new_text);

    // Diffs working directory files against the staging index (unstaged modifications)
    void show_diff_working_vs_index();

    // Diffs staging index entries against HEAD commit (staged modifications)
    void show_diff_index_vs_head();
    
} // namespace diff_ops
