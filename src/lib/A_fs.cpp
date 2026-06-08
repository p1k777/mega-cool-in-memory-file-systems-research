#include "A_fs.hpp"

#include <memory>
#include <stdexcept>
#include <utility>

namespace filesystem {

IFileSystem::bytes_type TreeFileSystem::op_read(const path_type& path) const {
    const Node* node = find_node(path);

    if (node == nullptr) {
        throw std::runtime_error("file does not exist: " + path);
    }
    if (node->is_directory) {
        throw std::runtime_error("cannot read a directory: " + path);
    }

    return node->data;
}

void TreeFileSystem::op_write(
    const path_type& path,
    const bytes_type& bytes
) {
    if (Node* node = find_node(path); node != nullptr) {
        if (node->is_directory) {
            throw std::runtime_error("cannot write to a directory: " + path);
        }

        node->data = bytes;
        return;
    }

    std::string file_name;
    Node& parent = require_parent_dir(path, file_name);

    auto file = std::make_unique<Node>();
    file->is_directory = false;
    file->data = bytes;
    attach_node(parent, std::move(file), std::move(file_name));
}

void TreeFileSystem::op_mkdir(const path_type& path) {
    if (find_node(path) != nullptr) {
        throw std::runtime_error("path already exists: " + path);
    }

    std::string directory_name;
    Node& parent = require_parent_dir(path, directory_name);

    auto directory = std::make_unique<Node>();
    attach_node(parent, std::move(directory), std::move(directory_name));
}

IFileSystem::units_list_type TreeFileSystem::op_ls(
    const path_type& path
) const {
    const Node& directory = require_dir(path);

    units_list_type result;
    result.reserve(directory.children.size());

    for (const auto& child : directory.children) {
        result.push_back(child->name);
    }

    return result;
}

void TreeFileSystem::op_mv(
    const path_type& from,
    const path_type& to
) {
    if (from == to) {
        return;
    }
    if (from == "/") {
        throw std::runtime_error("cannot move the root directory");
    }

    Node* source = find_node(from);
    if (source == nullptr) {
        throw std::runtime_error("source path does not exist: " + from);
    }

    std::string new_name;
    Node& new_parent = require_parent_dir(to, new_name);

    if (find_child(new_parent, new_name) != nullptr) {
        throw std::runtime_error("destination path already exists: " + to);
    }

    if (source->is_directory) {
        for (Node* ancestor = &new_parent;
             ancestor != nullptr;
             ancestor = ancestor->parent) {
            if (ancestor == source) {
                throw std::runtime_error(
                    "cannot move a directory into its own subtree"
                );
            }
        }
    }

    std::unique_ptr<Node> node = detach_node(from);
    attach_node(new_parent, std::move(node), std::move(new_name));
}

IFileSystem::units_list_type TreeFileSystem::op_find(
    const path_type& root,
    const path_type& pattern
) const {
    const Node& subtree_root = require_dir(root);

    units_list_type result;
    collect_find(subtree_root, root, pattern, result);
    return result;
}

size_t TreeFileSystem::get_memory_usage() const noexcept {
    return memory_usage_of(root_);
}

} // namespace filesystem
