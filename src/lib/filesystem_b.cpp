#include "filesystem_b.hpp"

#include <stdexcept>

namespace filesystem {

FileSystemB::FileSystemB() {
    root_ = std::make_unique<Node>();
    root_->name = "";
    root_->is_file = false;
    root_->parent = nullptr;

    index_["/"] = root_.get();
}

FileSystemB::Node* FileSystemB::get_node(const path_type& path) {
    auto it = index_.find(path);

    if(it == index_.end()) {
        throw std::runtime_error("path does not exist");
    }

    return it->second;
}

const FileSystemB::Node* FileSystemB::get_node(const path_type& path) const {
    auto it = index_.find(path);

    if(it == index_.end()) {
        throw std::runtime_error("path does not exist");
    }

    return it->second;
}

IFileSystem::path_type FileSystemB::parent_path(const path_type& path) {
    if(path == "/") {
        return "/";
    }

    size_t pos = path.find_last_of('/');

    if(pos == 0) {
        return "/";
    }

    return path.substr(0, pos);
}

IFileSystem::path_type FileSystemB::basename(const path_type& path) {
    if(path == "/") {
        return "";
    }

    size_t pos = path.find_last_of('/');
    return path.substr(pos + 1);
}

IFileSystem::path_type FileSystemB::join_path(const path_type& parent, const path_type& name) {
    if(parent == "/") {
        return "/" + name;
    }

    return parent + "/" + name;
}

IFileSystem::bytes_type FileSystemB::op_read(const path_type& path) const {
    const Node* node = get_node(path);

    if(!node->is_file) {
        throw std::runtime_error("read from directory");
    }

    return node->data;
}

void FileSystemB::op_mkdir(const path_type& path) {
    if(path == "/") {
        return;
    }

    if(index_.find(path) != index_.end()) {
        throw std::runtime_error("path already exists");
    }

    path_type parent = parent_path(path);
    path_type name = basename(path);

    Node* parent_node = get_node(parent);

    if(parent_node->is_file) {
        throw std::runtime_error("parent is file");
    }

    auto new_node = std::make_unique<Node>();
    new_node->name = name;
    new_node->is_file = false;
    new_node->parent = parent_node;

    Node* raw = new_node.get();

    parent_node->children.push_back(std::move(new_node));
    index_[path] = raw;
}

void FileSystemB::op_write(const path_type& path, const bytes_type& data) {
    auto it = index_.find(path);

    if(it != index_.end()) {
        Node* node = it->second;

        if(!node->is_file) {
            throw std::runtime_error("write to directory");
        }

        node->data = data;
        return;
    }

    path_type parent = parent_path(path);
    path_type name = basename(path);

    Node* parent_node = get_node(parent);

    if(parent_node->is_file) {
        throw std::runtime_error("parent is file");
    }

    auto new_node = std::make_unique<Node>();
    new_node->name = name;
    new_node->is_file = true;
    new_node->data = data;
    new_node->parent = parent_node;

    Node* raw = new_node.get();

    parent_node->children.push_back(std::move(new_node));
    index_[path] = raw;
}

IFileSystem::units_list_type FileSystemB::op_ls(const path_type& path) const {
    const Node* node = get_node(path);

    if(node->is_file) {
        throw std::runtime_error("ls from file");
    }

    units_list_type result;

    for(const auto& child : node->children) {
        result.push_back(join_path(path, child->name));
    }

    return result;
}

void FileSystemB::op_mv(const path_type&, const path_type&) {
    throw std::runtime_error("op_mv is not implemented");
}

IFileSystem::units_list_type FileSystemB::op_find(const path_type&, const path_type&) const {
    throw std::runtime_error("op_find is not implemented");
}

size_t FileSystemB::get_memory_usage() const noexcept {
    return 0;
}

} // namespace filesystem