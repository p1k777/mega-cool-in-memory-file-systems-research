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

    static std::vector<std::string> split_path(const path_type& path);

    Node* find_node(const path_type& path);
    const Node* find_node(const path_type& path) const;

    Node* find_child(Node& dir, const std::string& name);
    const Node* find_child(const Node& dir, const std::string& name) const;

    Node& require_parent_dir(const path_type& path, std::string& leaf_name);
    const Node& require_dir(const path_type& path) const;

    std::unique_ptr<Node> detach_node(const path_type& path);
    void attach_node(Node& new_parent, std::unique_ptr<Node> node, std::string new_name);

    static bool glob_matches(
        const std::string& name,
        const std::string& pattern
    ) noexcept;

    static void collect_find(
        const Node& node,
        const path_type& current_path,
        const std::string& pattern,
        units_list_type& result
    );

    static size_t memory_usage_of(const Node& node) noexcept;
};

} // namespace filesystem
