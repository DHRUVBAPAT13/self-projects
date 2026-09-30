#pragma once

#include <string>
#include <vector>

namespace tag_ops {

    // Lists all tags sorted alphabetically
    void list_tags();

    // Creates a lightweight tag pointing to a commit (or HEAD if commit_sha is empty)
    void create_tag(const std::string& name, const std::string& commit_sha = "");

    // Deletes an existing tag
    void delete_tag(const std::string& name);

} // namespace tag_ops