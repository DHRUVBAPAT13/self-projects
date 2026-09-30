#pragma once

#include "object.hpp"
#include <string>
#include <vector>

struct TreeEntry{
    std::string mode;
    std::string name;
    std::string sha_hex;
};

class Tree : public GitObject {

public:

    std::vector<TreeEntry> entries;

    Tree() = default;
    explicit Tree(const std::vector<TreeEntry>& entries) : entries(entries) {}

    ObjectType get_type() const override{
        return ObjectType::TREE;
    }

    std::string serialize() const override;
    void deserialize(const std::string& data) override;
};

namespace tree_builder {

    // Recursively scans a directory, writes blobs & subtrees, and returns the root tree's SHA-1
    std::string write_tree_from_directory(const std::string& dir_path = ".");

} // namespace tree_builder