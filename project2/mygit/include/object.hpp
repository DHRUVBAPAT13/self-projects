#pragma once

#include <string>
#include <vector>
#include <cstdint>

enum class ObjectType{
    BLOB,
    TREE,
    COMMIT
};

// converts ObjectType enum into its Git-standard string equivalent
std::string object_type_to_string(ObjectType type);

// coverts Git-standard string back to ObjectType enum
ObjectType string_to_object_type(const std::string& type_str);

class GitObject{

public:
    virtual ~GitObject() = default;

    virtual ObjectType get_type() const = 0;
    virtual std::string serialize() const = 0;
    virtual void deserialize(const std::string& data) = 0;

    // hashes and writes this object to .mygit/objects/<dir>/<file>
    // returns the 40-character SHA-1 hex string
    std::string write();

};

namespace object_db{

    // writes raw payload directly with given type and returns its SHA-1 hash
    std::string write_object(ObjectType type, const std::string& content);

    // reads and uncompresses an object by its 40-character SHA-1 hash
    // returns a pair -> {ObjectType, raw_payload}
    std::pair<ObjectType, std::string> read_object(const std::string& sha); 

}   // namespace object_db