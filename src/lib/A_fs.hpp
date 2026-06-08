#pragma once

#include "filesystem.hpp"

#include <memory>
#include <string>
#include <vector>

namespace filesystem {

class TreeFileSystem final : public IFileSystem {
public:
    bytes_type op_read(const path_type& path) const override;
    void op_write(const path_type& path, const bytes_type& bytes) override;

    void op_mkdir(const path_type& path) override;
    units_list_type op_ls(const path_type& path) const override;

    void op_mv(const path_type& from, const path_type& to) override;
    units_list_type op_find(const path_type& root, const path_type& pattern) const override;

    size_t get_memory_usage() const noexcept override;

private:
    struct Node {
        std::string name;
        bool is_directory = true;
        bytes_type data;

        Node* parent = nullptr;
        std::vector<std::unique_ptr<Node>> children;
    };

    Node root_{"", true, {}, nullptr, {}};
};

} // namespace filesystem