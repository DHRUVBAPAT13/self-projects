#pragma once

#include <string>
#include <vector>
#include <map>

struct IndexEntry{

    std::string mode;
    std::string sha;
    std::string path;
};

class Index {

public:

    // relative path -> IndexEntry
    std::map<std::string, IndexEntry> entries;

    static Index read();
    void write() const;

    void add_file(const std::string& rel_path);
    void add_path(const std::string& target_path);

    // Removes an entry from the staging index.
    // If delete_working_file is true, also deletes the file from disk.
    bool remove_entry(const std::string& rel_path, bool delete_working_file);

    // Discards uncommitted modifications by copying staged blob back to working tree
    bool restore_file(const std::string& rel_path);

    // creates a root tree object directly from the staged entries
    std::string write_tree() const;

};