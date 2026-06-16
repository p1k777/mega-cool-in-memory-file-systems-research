#include "C_fs.hpp"

#include <algorithm>
#include <stdexcept>
#include <utility>

namespace filesystem {

FlatHashFileSystem::FlatHashFileSystem() {
    nodes_.emplace("/", Node{});
}

IFileSystem::bytes_type FlatHashFileSystem::op_read(
    const path_type& path
) const {
    const Node& node = require_node(path);

    if (node.is_directory) {
        throw std::runtime_error("cannot read a directory: " + path);
    }

    return node.data;
}

void FlatHashFileSystem::op_write(
    const path_type& path,
    const bytes_type& bytes
) {
    validate_path(path);

    if (Node* node = find_node(path); node != nullptr) {
        if (node->is_directory) {
            throw std::runtime_error("cannot write to a directory: " + path);
        }

        node->data = bytes;
        return;
    }

    require_dir(parent_path(path));

    Node file;
    file.is_directory = false;
    file.data = bytes;
    nodes_.emplace(path, std::move(file));
}

void FlatHashFileSystem::op_mkdir(const path_type& path) {
    validate_path(path);
    ensure_missing(path);
    require_dir(parent_path(path));

    nodes_.emplace(path, Node{});
}

IFileSystem::units_list_type FlatHashFileSystem::op_ls(
    const path_type& path
) const {
    require_dir(path);
    return collect_children(path);
}

void FlatHashFileSystem::op_mv(
    const path_type& from,
    const path_type& to
) {
    if (from == to) {
        return;
    }
    if (from == "/") {
        throw std::runtime_error("cannot move the root directory");
    }

    const Node& source = require_node(from);
    const path_type destination_parent = parent_path(to);
    require_dir(destination_parent);
    ensure_missing(to);

    if (source.is_directory && is_under_or_equal(from, destination_parent)) {
        throw std::runtime_error("cannot move a directory into its own subtree");
    }

    std::vector<path_type> subtree_paths = collect_subtree_paths(from);
    std::sort(
        subtree_paths.begin(),
        subtree_paths.end(),
        [](const path_type& left, const path_type& right) {
            return left.size() > right.size();
        }
    );

    for (const path_type& old_path : subtree_paths) {
        const path_type new_path = replace_prefix(old_path, from, to);
        Node node = std::move(nodes_.at(old_path));
        nodes_.erase(old_path);
        nodes_.emplace(new_path, std::move(node));
    }
}

IFileSystem::units_list_type FlatHashFileSystem::op_find(
    const path_type& root,
    const path_type& pattern
) const {
    require_dir(root);

    units_list_type result;
    for (const path_type& path : collect_subtree_paths(root)) {
        if (glob_matches(leaf_name(path), pattern)) {
            result.push_back(path);
        }
    }

    return result;
}

size_t FlatHashFileSystem::get_memory_usage() const noexcept {
    size_t usage = sizeof(*this);

    for (const auto& [path, node] : nodes_) {
        usage += memory_usage_of(path, node);
    }

    return usage;
}

} // namespace filesystem
