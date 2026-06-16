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

void FlatHashFileSystem::validate_path(const path_type& path) {
    if (path.empty() || path.front() != '/') {
        throw std::invalid_argument("path must be absolute: " + path);
    }
    if (path.size() > 1 && path.back() == '/') {
        throw std::invalid_argument("path must not end with '/': " + path);
    }

    size_t component_begin = 1;
    while (component_begin < path.size()) {
        const size_t separator = path.find('/', component_begin);
        const size_t component_end =
            separator == std::string::npos ? path.size() : separator;

        if (component_end == component_begin) {
            throw std::invalid_argument(
                "path must not contain empty components: " + path
            );
        }

        const std::string component =
            path.substr(component_begin, component_end - component_begin);
        if (component == "." || component == "..") {
            throw std::invalid_argument(
                "'.' and '..' are not supported in paths: " + path
            );
        }

        if (separator == std::string::npos) {
            break;
        }
        component_begin = separator + 1;
    }
}

IFileSystem::path_type FlatHashFileSystem::parent_path(
    const path_type& path
) {
    validate_path(path);

    if (path == "/") {
        throw std::invalid_argument("the root directory has no parent");
    }

    const size_t separator = path.rfind('/');
    if (separator == 0) {
        return "/";
    }

    return path.substr(0, separator);
}

std::string FlatHashFileSystem::leaf_name(const path_type& path) {
    validate_path(path);

    if (path == "/") {
        return "";
    }

    return path.substr(path.rfind('/') + 1);
}

FlatHashFileSystem::Node* FlatHashFileSystem::find_node(
    const path_type& path
) {
    validate_path(path);

    const auto node = nodes_.find(path);
    return node == nodes_.end() ? nullptr : &node->second;
}

const FlatHashFileSystem::Node* FlatHashFileSystem::find_node(
    const path_type& path
) const {
    validate_path(path);

    const auto node = nodes_.find(path);
    return node == nodes_.end() ? nullptr : &node->second;
}

const FlatHashFileSystem::Node& FlatHashFileSystem::require_node(
    const path_type& path
) const {
    const Node* node = find_node(path);
    if (node == nullptr) {
        throw std::runtime_error("path does not exist: " + path);
    }

    return *node;
}

const FlatHashFileSystem::Node& FlatHashFileSystem::require_dir(
    const path_type& path
) const {
    const Node& node = require_node(path);
    if (!node.is_directory) {
        throw std::runtime_error("path is not a directory: " + path);
    }

    return node;
}

void FlatHashFileSystem::ensure_missing(const path_type& path) const {
    if (find_node(path) != nullptr) {
        throw std::runtime_error("path already exists: " + path);
    }
}

IFileSystem::units_list_type FlatHashFileSystem::collect_children(
    const path_type& path
) const {
    units_list_type result;

    for (const auto& [candidate_path, node] : nodes_) {
        (void)node;
        if (candidate_path != "/" && parent_path(candidate_path) == path) {
            result.push_back(leaf_name(candidate_path));
        }
    }

    return result;
}

std::vector<IFileSystem::path_type> FlatHashFileSystem::collect_subtree_paths(
    const path_type& root
) const {
    validate_path(root);

    std::vector<path_type> result;
    for (const auto& [path, node] : nodes_) {
        (void)node;
        if (is_under_or_equal(root, path)) {
            result.push_back(path);
        }
    }

    return result;
}

bool FlatHashFileSystem::is_under_or_equal(
    const path_type& maybe_parent,
    const path_type& path
) noexcept {
    if (maybe_parent == "/") {
        return !path.empty() && path.front() == '/';
    }
    if (path == maybe_parent) {
        return true;
    }
    if (path.size() <= maybe_parent.size()) {
        return false;
    }

    return path.compare(0, maybe_parent.size(), maybe_parent) == 0
           && path[maybe_parent.size()] == '/';
}

IFileSystem::path_type FlatHashFileSystem::replace_prefix(
    const path_type& path,
    const path_type& old_prefix,
    const path_type& new_prefix
) {
    if (!is_under_or_equal(old_prefix, path)) {
        throw std::invalid_argument("path is outside the old prefix: " + path);
    }
    if (path == old_prefix) {
        return new_prefix;
    }

    return new_prefix + path.substr(old_prefix.size());
}

bool FlatHashFileSystem::glob_matches(
    const std::string& name,
    const std::string& pattern
) noexcept {
    size_t name_index = 0;
    size_t pattern_index = 0;
    size_t star_index = std::string::npos;
    size_t star_match_begin = 0;

    while (name_index < name.size()) {
        if (pattern_index < pattern.size()
            && (pattern[pattern_index] == '?'
                || pattern[pattern_index] == name[name_index])) {
            ++name_index;
            ++pattern_index;
        } else if (pattern_index < pattern.size()
                   && pattern[pattern_index] == '*') {
            star_index = pattern_index++;
            star_match_begin = name_index;
        } else if (star_index != std::string::npos) {
            pattern_index = star_index + 1;
            name_index = ++star_match_begin;
        } else {
            return false;
        }
    }

    while (pattern_index < pattern.size()
           && pattern[pattern_index] == '*') {
        ++pattern_index;
    }

    return pattern_index == pattern.size();
}

size_t FlatHashFileSystem::memory_usage_of(
    const path_type& path,
    const Node& node
) noexcept {
    return sizeof(std::pair<const path_type, Node>)
           + path.capacity() * sizeof(path_type::value_type)
           + node.data.capacity() * sizeof(bytes_type::value_type);
}

} // namespace filesystem
