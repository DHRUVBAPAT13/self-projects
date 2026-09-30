#pragma once

#include <string>

namespace repository{

    // Initializes a new .mygit repository in the specified directory (default: current directory)
    bool init(const std::string& path = ".");

    // finds root folder containing .mygit by walking up parent directories
    std::string find_root(const std::string& start_path = ".");

    // resolves a subpathrelative to .mygit (e.g. repo_path("objects/4b"))
    std::string repo_path(const std::string& subpath, const std::string& root = "");

} // namespace repository