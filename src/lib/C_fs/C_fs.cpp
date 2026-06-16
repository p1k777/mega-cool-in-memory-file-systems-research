#include "C_fs.hpp"

#include <stdexcept>

namespace filesystem {

FlatHashFileSystem::FlatHashFileSystem() {
    nodes_.emplace("/", Node{});
}

IFileSystem::bytes_type FlatHashFileSystem::op_read(
    const path_type& path
) const {
    throw std::runtime_error("FlatHashFileSystem::op_read is not implemented: " + path);
}

void FlatHashFileSystem::op_write(
    const path_type& path,
    const bytes_type& bytes
) {
    (void)bytes;
    throw std::runtime_error("FlatHashFileSystem::op_write is not implemented: " + path);
}

void FlatHashFileSystem::op_mkdir(const path_type& path) {
    throw std::runtime_error("FlatHashFileSystem::op_mkdir is not implemented: " + path);
}

IFileSystem::units_list_type FlatHashFileSystem::op_ls(
    const path_type& path
) const {
    if (path != "/") {
        throw std::runtime_error("FlatHashFileSystem::op_ls is not implemented: " + path);
    }

    return {};
}

void FlatHashFileSystem::op_mv(
    const path_type& from,
    const path_type& to
) {
    throw std::runtime_error(
        "FlatHashFileSystem::op_mv is not implemented: " + from + " -> " + to
    );
}

IFileSystem::units_list_type FlatHashFileSystem::op_find(
    const path_type& root,
    const path_type& pattern
) const {
    throw std::runtime_error(
        "FlatHashFileSystem::op_find is not implemented: " + root + " " + pattern
    );
}

size_t FlatHashFileSystem::get_memory_usage() const noexcept {
    size_t usage = sizeof(*this);

    for (const auto& [path, node] : nodes_) {
        usage += path.capacity() * sizeof(path_type::value_type);
        usage += sizeof(Node);
        usage += node.data.capacity() * sizeof(bytes_type::value_type);
    }

    return usage;
}

} // namespace filesystem
