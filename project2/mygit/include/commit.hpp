#pragma once

#include "object.hpp"
#include <string>
#include <vector>

class Commit : public GitObject{
public:
    std::string tree_sha;
    std::vector<std::string> parent_shas;
    std::string author;
    std::string committer;
    std::string message;

    Commit() = default;

    ObjectType get_type() const override{
        return ObjectType::COMMIT;
    }

    std::string serialize() const override;
    void deserialize(const std::string& data) override;

};

namespace commit_helper
{
    // reads HEAD to determine current branch and its latest commit SHA
    std::string get_head_commit();

    // updates the current branch pointed to by HEAD with a new commit SHA
    void update_head(const std::string& new_commit_sha);

    // creates and records a commit given a tree SHA, message, and optional parent
    std::string create_commit(const std::string& tree_sha, const std::string& message, const std::string& parent_sha = "");

    void print_graph_log(); // Visual DAG commit history
    
} // namespace commit_helper
