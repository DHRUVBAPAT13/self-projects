#include "repository.hpp"
#include "utils.hpp"
#include "object.hpp"

#include <fstream>
#include <sstream>
#include <stdexcept>
#include <windows.h>

namespace{

    // windows directory creator
    bool make_directory(const std::string& path){
        if(CreateDirectoryA(path.c_str(), NULL)){
            return true;
        }

        return (GetLastError() == ERROR_ALREADY_EXISTS);
    }
}

std::string object_type_to_string(ObjectType type){
    switch(type){
        case ObjectType::BLOB : return "blob";

        case ObjectType::TREE : return "tree";

        case ObjectType::COMMIT : return "commit";

        default : throw std::runtime_error("Invalid object type");

    }
}

ObjectType string_to_object_type(const std::string& type_str){
    if(type_str == "blob") return ObjectType::BLOB;

    if(type_str == "tree") return ObjectType::TREE;

    if(type_str == "commit") return ObjectType::COMMIT;

    throw std::runtime_error("Unknown object type string: " + type_str);
}

std::string GitObject::write(){
    return object_db::write_object(get_type(), serialize());
}

namespace object_db{
    
std::string write_object(ObjectType type, const std::string& content){

    // 1. Git format: "<type> <size>\0<content>"
    std::string header = object_type_to_string(type) + " " + std::to_string(content.size());
    std::string store;
    store.reserve(header.size() + 1 + content.size());
    store += header;
    store.push_back('\0');
    store += content;

    // 2. Compute SHA-1 hash of the uncompressed data
    std::string sha = utils::sha1(store);

    // 3. Determine storage location: .mygit/objects/<first-2-chars>/<remaining-38-chars>
    std::string dir_name = sha.substr(0, 2);
    std::string file_name = sha.substr(2);

    std::string obj_dir = repository::repo_path("objects/" + dir_name);
    make_directory(obj_dir);

    std::string full_path = obj_dir + "/" + file_name;

    // Avoid rewriting if the object already exists (content-addressable and immutable)
    DWORD attribs = GetFileAttributesA(full_path.c_str());
    if (attribs != INVALID_FILE_ATTRIBUTES) {
        return sha;
    }

    // 4. Compress the entire object (header + payload)
    std::vector<uint8_t> compressed = utils::compress(store);

    // 5. Write to disk in binary mode
    std::ofstream out_file(full_path.c_str(), std::ios::binary);
    if (!out_file.is_open()) {
        throw std::runtime_error("fatal: unable to write object to " + full_path);
    }

    out_file.write(reinterpret_cast<const char*>(compressed.data()), compressed.size());
    out_file.close();

    return sha;
}

std::pair<ObjectType, std::string> read_object(const std::string& sha){

    if(sha.size() != 40){
        throw std::runtime_error("FATAL : Invalid SHA-1 length: "+sha);
    }

    std::string dir_name = sha.substr(0,2);
    std::string file_name = sha.substr(2);
    std::string full_path = repository::repo_path("objects/" + dir_name + "/" + file_name);

    // 1. read compressed bytes
    std::ifstream in_file(full_path.c_str(), std::ios::binary);
    if(!in_file.is_open()){
        throw std::runtime_error("FATAL : Object Not Found : " + sha);
    }

    std::vector<uint8_t> compressed((std::istreambuf_iterator<char>(in_file)), std::istreambuf_iterator<char>());

    in_file.close();

    // 2. decompress
    std::string raw  = utils::decompress(compressed);

    // 3. parse header : "<type> <szie>\0<payload>"
    size_t space_pos = raw.find(' ');
    if(space_pos == std::string::npos){
        throw std::runtime_error("FATAL : Corrupt object header : missing space");
    }

    size_t null_pos = raw.find('\0', space_pos);
    if(null_pos == std::string::npos){
        throw std::runtime_error("FATAL : Corrupt object header : missing null byte");
    }

    std::string type_str = raw.substr(0, space_pos);
    std::string size_str = raw.substr(space_pos + 1, null_pos - space_pos -1);
    size_t size = std::stoul(size_str);

    std::string payload = raw.substr(null_pos + 1);
    if(payload.size()!= size){
        throw std::runtime_error("FATAL : Object size Mismatch");
    }

    return {string_to_object_type(type_str), payload};

}

} // namespace object_db
