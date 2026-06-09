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

} // namespace filesystem