#pragma once

#include "../filesystem.hpp"

#include <string>
#include <unordered_map>
#include <vector>

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

    static void validate_path(const path_type& path);
    static path_type parent_path(const path_type& path);
    static std::string leaf_name(const path_type& path);

    Node* find_node(const path_type& path);
    const Node* find_node(const path_type& path) const;

    const Node& require_node(const path_type& path) const;
    const Node& require_dir(const path_type& path) const;
    void ensure_missing(const path_type& path) const;

    units_list_type collect_children(const path_type& path) const;
    std::vector<path_type> collect_subtree_paths(const path_type& root) const;

    static bool is_under_or_equal(
        const path_type& maybe_parent,
        const path_type& path
    ) noexcept;
    static path_type replace_prefix(
        const path_type& path,
        const path_type& old_prefix,
        const path_type& new_prefix
    );
    static bool glob_matches(
        const std::string& name,
        const std::string& pattern
    ) noexcept;
    static size_t memory_usage_of(
        const path_type& path,
        const Node& node
    ) noexcept;

    std::unordered_map<path_type, Node> nodes_;
};

} // namespace filesystem
