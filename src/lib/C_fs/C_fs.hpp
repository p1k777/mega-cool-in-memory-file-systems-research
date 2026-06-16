#pragma once

#include "../filesystem.hpp"

#include <string>
#include <unordered_map>

namespace filesystem {

class FlatHashFileSystem final : public IFileSystem {
public:
    FlatHashFileSystem();

    bytes_type op_read(const path_type& path) const override;
    void op_write(const path_type& path, const bytes_type& bytes) override;

    void op_mkdir(const path_type& path) override;
    units_list_type op_ls(const path_type& path) const override;

    void op_mv(const path_type& from, const path_type& to) override;
    units_list_type op_find(const path_type& root, const path_type& pattern) const override;

    size_t get_memory_usage() const noexcept override;

private:
    struct Node {
        bool is_directory = true;
        bytes_type data;
    };

    std::unordered_map<path_type, Node> nodes_;
};

} // namespace filesystem
