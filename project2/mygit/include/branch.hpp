#pragma once

#include <vector>
#include <string>

namespace branch_ops{

    // lists all branches, highlighting current active branch
    void list_branches();

    // creates a new branch pointing to currentHEAD
    bool create_branch(const std::string& branch_name);

    // switches HEAD and restores working tree to match target branch or commit SHA
    bool checkout(const std::string& target); 

}