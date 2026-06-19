#include "A_fs.hpp"

#include <algorithm>
#include <memory>
#include <stdexcept>
#include <utility>

namespace filesystem {

TreeFileSystem::TreeFileSystem(const TreeFileSystem& other) {
    root_.name = other.root_.name;
    root_.is_directory = other.root_.is_directory;
    root_.data = other.root_.data;
    root_.parent = nullptr;
    root_.children.clear();
    root_.children.reserve(other.root_.children.size());

    for (const auto& child : other.root_.children) {
        root_.children.push_back(clone_node(*child, &root_));
    }
}

TreeFileSystem& TreeFileSystem::operator=(const TreeFileSystem& other) {
    if (this == &other) {
        return *this;
    }

    root_.name = other.root_.name;
    root_.is_directory = other.root_.is_directory;
    root_.data = other.root_.data;
    root_.parent = nullptr;
    root_.children.clear();
    root_.children.reserve(other.root_.children.size());

    for (const auto& child : other.root_.children) {
        root_.children.push_back(clone_node(*child, &root_));
    }

    return *this;
}

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

std::vector<std::string> TreeFileSystem::split_path(
    const path_type& path
) {
    if (path.empty() || path.front() != '/') {
        throw std::invalid_argument("path must be absolute: " + path);
    }
    if (path.size() > 1 && path.back() == '/') {
        throw std::invalid_argument("path must not end with '/': " + path);
    }

    std::vector<std::string> components;
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

        std::string component =
            path.substr(component_begin, component_end - component_begin);
        if (component == "." || component == "..") {
            throw std::invalid_argument(
                "'.' and '..' are not supported in paths: " + path
            );
        }

        components.push_back(std::move(component));

        if (separator == std::string::npos) {
            break;
        }
        component_begin = separator + 1;
    }

    return components;
}

TreeFileSystem::Node* TreeFileSystem::find_node(const path_type& path) {
    Node* current = &root_;

    for (const std::string& component : split_path(path)) {
        current = find_child(*current, component);
        if (current == nullptr) {
            return nullptr;
        }
    }

    return current;
}

const TreeFileSystem::Node* TreeFileSystem::find_node(
    const path_type& path
) const {
    const Node* current = &root_;

    for (const std::string& component : split_path(path)) {
        current = find_child(*current, component);
        if (current == nullptr) {
            return nullptr;
        }
    }

    return current;
}

TreeFileSystem::Node* TreeFileSystem::find_child(
    Node& dir,
    const std::string& name
) {
    if (!dir.is_directory) {
        return nullptr;
    }

    const auto child = std::find_if(
        dir.children.begin(),
        dir.children.end(),
        [&name](const std::unique_ptr<Node>& candidate) {
            return candidate->name == name;
        }
    );

    return child == dir.children.end() ? nullptr : child->get();
}

const TreeFileSystem::Node* TreeFileSystem::find_child(
    const Node& dir,
    const std::string& name
) const {
    if (!dir.is_directory) {
        return nullptr;
    }

    const auto child = std::find_if(
        dir.children.begin(),
        dir.children.end(),
        [&name](const std::unique_ptr<Node>& candidate) {
            return candidate->name == name;
        }
    );

    return child == dir.children.end() ? nullptr : child->get();
}

TreeFileSystem::Node& TreeFileSystem::require_parent_dir(
    const path_type& path,
    std::string& leaf_name
) {
    std::vector<std::string> components = split_path(path);
    if (components.empty()) {
        throw std::invalid_argument("the root directory has no parent");
    }

    leaf_name = std::move(components.back());
    components.pop_back();

    Node* current = &root_;
    for (const std::string& component : components) {
        current = find_child(*current, component);
        if (current == nullptr) {
            throw std::runtime_error(
                "parent directory does not exist: " + path
            );
        }
        if (!current->is_directory) {
            throw std::runtime_error(
                "parent path contains a file: " + path
            );
        }
    }

    return *current;
}

const TreeFileSystem::Node& TreeFileSystem::require_dir(
    const path_type& path
) const {
    const Node* node = find_node(path);
    if (node == nullptr) {
        throw std::runtime_error("directory does not exist: " + path);
    }
    if (!node->is_directory) {
        throw std::runtime_error("path is not a directory: " + path);
    }

    return *node;
}

std::unique_ptr<TreeFileSystem::Node> TreeFileSystem::detach_node(
    const path_type& path
) {
    std::string node_name;
    Node& parent = require_parent_dir(path, node_name);

    const auto node = std::find_if(
        parent.children.begin(),
        parent.children.end(),
        [&node_name](const std::unique_ptr<Node>& candidate) {
            return candidate->name == node_name;
        }
    );

    if (node == parent.children.end()) {
        throw std::runtime_error("path does not exist: " + path);
    }

    std::unique_ptr<Node> detached = std::move(*node);
    parent.children.erase(node);
    detached->parent = nullptr;
    return detached;
}

void TreeFileSystem::attach_node(
    Node& new_parent,
    std::unique_ptr<Node> node,
    std::string new_name
) {
    if (!new_parent.is_directory) {
        throw std::runtime_error("cannot attach a node to a file");
    }
    if (node == nullptr) {
        throw std::invalid_argument("cannot attach an empty node");
    }
    if (new_name.empty() || new_name == "." || new_name == ".."
        || new_name.find('/') != std::string::npos) {
        throw std::invalid_argument("invalid node name: " + new_name);
    }
    if (find_child(new_parent, new_name) != nullptr) {
        throw std::runtime_error("a node with this name already exists");
    }

    node->name = std::move(new_name);
    node->parent = &new_parent;
    new_parent.children.push_back(std::move(node));
}

bool TreeFileSystem::glob_matches(
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

void TreeFileSystem::collect_find(
    const Node& node,
    const path_type& current_path,
    const std::string& pattern,
    units_list_type& result
) {
    if (glob_matches(node.name, pattern)) {
        result.push_back(current_path);
    }

    for (const auto& child : node.children) {
        const path_type child_path =
            current_path == "/"
                ? current_path + child->name
                : current_path + "/" + child->name;
        collect_find(*child, child_path, pattern, result);
    }
}

std::unique_ptr<TreeFileSystem::Node> TreeFileSystem::clone_node(
    const Node& node,
    Node* parent
) {
    auto copy = std::make_unique<Node>();
    copy->name = node.name;
    copy->is_directory = node.is_directory;
    copy->data = node.data;
    copy->parent = parent;
    copy->children.reserve(node.children.size());

    for (const auto& child : node.children) {
        copy->children.push_back(clone_node(*child, copy.get()));
    }

    return copy;
}

size_t TreeFileSystem::memory_usage_of(const Node& node) noexcept {
    size_t usage =
        sizeof(Node)
        + node.name.capacity() * sizeof(char)
        + node.data.capacity() * sizeof(bytes_type::value_type)
        + node.children.capacity() * sizeof(std::unique_ptr<Node>);

    for (const auto& child : node.children) {
        usage += memory_usage_of(*child);
    }

    return usage;
}

} // namespace filesystem
