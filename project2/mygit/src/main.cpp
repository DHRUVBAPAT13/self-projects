#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

#include "repository.hpp"
#include "object.hpp"
#include "tree.hpp"
#include "commit.hpp"
#include "branch.hpp"
#include "index.hpp"
#include "status.hpp"
#include "diff.hpp"
#include "tag.hpp"
#include "utils.hpp"

int main(int argc, char* argv[]) {
    if (argc < 2) {
    std::cout << "Usage: mygit <command> [<args>]\n\n"
              << "Start a working area:\n"
              << "   init [<dir>]                 Initialize a new repository\n\n"
              << "Work on the current change:\n"
              << "   add <file|dir>...            Add file contents to the staging area\n"
              << "   rm [--cached] <file>...      Remove files from index (and disk if not --cached)\n"
              << "   restore <file>...            Discard working tree changes using staged copy\n\n"
              << "Examine the history and state:\n"
              << "   status                       Show the working tree status\n"
              << "   diff [--staged]              Show changes between commits, index, and workspace\n"
              << "   log                          Show commit logs\n"
              << "   log [--graph]                Show commit logs (with optional ASCII DAG graph)\n\n"
              << "Grow, mark and tweak your common history:\n"
              << "   commit -m \"<msg>\"            Record staged snapshot to repository\n"
              << "   branch [<name>]              List or create branches\n"
              << "   checkout <branch|commit>     Switch branches or restore tree snapshot\n\n"
              << "Low-level plumbing:\n"
              << "   hash-object [-w] <file>      Compute object ID and optionally create blob\n"
              << "   cat-file -p <object>         Print object content\n"
              << "   ls-tree <tree-sha>           List contents of a tree object\n"
              << "   tag [-d] <name> [<commit>]   Create, list, or delete lightweight tags\n";
    return 1;
    }

    std::string command = argv[1];

    try {
        if (command == "init") {
            std::string target_dir = (argc > 2) ? argv[2] : ".";
            return repository::init(target_dir) ? 0 : 1;
        }
        else if (command == "add") {
            if (argc < 3) {
                std::cerr << "fatal: nothing specified, nothing added.\n";
                return 1;
            }

            Index idx = Index::read();
            for (int i = 2; i < argc; ++i) {
                idx.add_path(argv[i]);
            }
            idx.write();
            return 0;
        }
        else if (command == "commit") {
            if (argc < 4 || std::string(argv[2]) != "-m") {
                std::cerr << "Usage: mygit commit -m \"<commit message>\"\n";
                return 1;
            }

            std::string message = argv[3];
            Index idx = Index::read();
            if (idx.entries.empty()) {
                std::cerr << "fatal: nothing to commit (create/stage files and use 'mygit add')\n";
                return 1;
            }

            // Snapshot staged index directly to root tree
            std::string tree_sha = idx.write_tree();
            std::string parent_sha = commit_helper::get_head_commit();

            std::string commit_sha = commit_helper::create_commit(tree_sha, message, parent_sha);
            std::cout << "[" << commit_sha.substr(0, 7) << "] " << message << std::endl;
            return 0;
        }
        else if (command == "branch") {
            if (argc == 2) {
                branch_ops::list_branches();
            } else {
                return branch_ops::create_branch(argv[2]) ? 0 : 1;
            }
            return 0;
        }
        else if (command == "checkout") {
            if (argc < 3) {
                std::cerr << "Usage: mygit checkout <branch | commit-sha>\n";
                return 1;
            }
            return branch_ops::checkout(argv[2]) ? 0 : 1;
        }
        else if (command == "log") {
            if (argc > 2 && std::string(argv[2]) == "--graph") {
                commit_helper::print_graph_log();
                return 0;
            }

            std::string commit_sha = commit_helper::get_head_commit();
            if (commit_sha.empty()) {
                std::cout << "fatal: your current branch does not have any commits yet\n";
                return 1;
            }

            while (!commit_sha.empty()) {
                auto result = object_db::read_object(commit_sha);
                Commit c;
                c.deserialize(result.second);

                std::cout << "\033[33mcommit " << commit_sha << "\033[0m\n";
                std::cout << "Author: " << c.author << "\n";
                std::cout << "Tree:   " << c.tree_sha << "\n\n";
                std::cout << "    " << c.message << "\n\n";

                commit_sha = c.parent_shas.empty() ? "" : c.parent_shas[0];
            }
            return 0;
        }
        else if (command == "hash-object") {
            if (argc < 3) return 1;
            bool write_flag = (std::string(argv[2]) == "-w");
            std::string filepath = write_flag ? argv[3] : argv[2];

            std::ifstream file(filepath.c_str(), std::ios::binary);
            std::stringstream buffer;
            buffer << file.rdbuf();
            std::string content = buffer.str();

            if (write_flag) {
                std::cout << object_db::write_object(ObjectType::BLOB, content) << std::endl;
            } else {
                std::string header = "blob " + std::to_string(content.size()) + '\0';
                std::cout << utils::sha1(header + content) << std::endl;
            }
            return 0;
        }
        else if (command == "cat-file") {
            if (argc < 4 || std::string(argv[2]) != "-p") return 1;
            auto result = object_db::read_object(argv[3]);
            if (result.first == ObjectType::TREE) {
                Tree tree;
                tree.deserialize(result.second);
                for (const auto& entry : tree.entries) {
                    std::cout << entry.mode << " " << (entry.mode == "40000" ? "tree" : "blob")
                              << " " << entry.sha_hex << "\t" << entry.name << "\n";
                }
            } else {
                std::cout << result.second;
            }
            return 0;
        }
        else if (command == "ls-tree") {
            if (argc < 3) return 1;
            auto result = object_db::read_object(argv[2]);
            Tree tree;
            tree.deserialize(result.second);
            for (const auto& entry : tree.entries) {
                std::cout << entry.mode << " " << (entry.mode == "40000" ? "tree" : "blob")
                          << " " << entry.sha_hex << "\t" << entry.name << "\n";
            }
            return 0;
        }
        else if (command == "rm") {
            if (argc < 3) {
                std::cerr << "Usage: mygit rm [--cached] <file>...\n";
                return 1;
            }

            bool cached = false;
            int start_idx = 2;
            if (std::string(argv[2]) == "--cached") {
                cached = true;
                start_idx = 3;
                if (argc < 4) {
                    std::cerr << "Usage: mygit rm --cached <file>...\n";
                    return 1;
                }
            }

            Index idx = Index::read();
            bool modified = false;
            for (int i = start_idx; i < argc; ++i) {
                if (idx.remove_entry(argv[i], !cached)) {
                    std::cout << "rm '" << argv[i] << "'\n";
                    modified = true;
                }
            }

            if (modified) {
                idx.write();
            }
            return 0;
        }
        else if (command == "restore") {
            if (argc < 3) {
                std::cerr << "Usage: mygit restore <file>...\n";
                return 1;
            }

            Index idx = Index::read();
            for (int i = 2; i < argc; ++i) {
                idx.restore_file(argv[i]);
            }
            return 0;
        }
        else if (command == "status") {
            status_ops::show_status();
            return 0;
        }
        else if (command == "diff") {
            bool staged = false;
            if (argc > 2 && (std::string(argv[2]) == "--staged" || std::string(argv[2]) == "--cached")) {
                staged = true;
            }

            if (staged) {
                diff_ops::show_diff_index_vs_head();
            } else {
                diff_ops::show_diff_working_vs_index();
            }
            return 0;
        }
        else if (command == "tag") {
            if (argc == 2) {
                // 'mygit tag' -> list all tags
                tag_ops::list_tags();
            } else if (argc == 3) {
                // 'mygit tag <name>'
                tag_ops::create_tag(argv[2]);
            } else if (argc == 4) {
                if (std::string(argv[2]) == "-d") {
                    // 'mygit tag -d <name>'
                    tag_ops::delete_tag(argv[3]);
                } else {
                    // 'mygit tag <name> <commit-sha>'
                    tag_ops::create_tag(argv[2], argv[3]);
                }
            } else {
                std::cerr << "Usage: mygit tag [-d] <tagname> [<commit>]\n";
                return 1;
            }
            return 0;
        }
        else {
            std::cerr << "mygit: '" << command << "' is not a mygit command.\n";
            return 1;
        }
    } 
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }

    return 0;
}