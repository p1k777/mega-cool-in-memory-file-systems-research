#pragma once
#include "../filesystem.hpp"

#include <memory>
#include <string>
#include <vector>
#include <string_view>
#include <unordered_map>

namespace filesystem {

class FileSystemB : public IFileSystem {
public:
    FileSystemB();
    FileSystemB(const FileSystemB& other);
    FileSystemB& operator=(const FileSystemB& other);

    FileSystemB(FileSystemB&&) noexcept = default;
    FileSystemB& operator=(FileSystemB&&) noexcept = default;

    bytes_type op_read(const path_type&) const override;
    void op_write(const path_type&, const bytes_type&) override;

    void op_mkdir(const path_type&) override;
    units_list_type op_ls(const path_type&) const override;

    void op_mv(const path_type&, const path_type&) override;
    units_list_type op_find(const path_type&, const path_type&) const override;

    size_t get_memory_usage() const noexcept override;

private:

    struct Node {
        std::string name;
        bool is_file = false;
        bytes_type data;
        Node* parent = nullptr;
        std::vector<std::unique_ptr<Node>> children;

        Node(
            std::string_view name_value,
            Node* parent_value
        );
    };

    std::unique_ptr<Node> root_;

    std::unordered_map<std::string, Node*> index_;

    std::unique_ptr<Node> create_node(std::string_view name, Node* parent, bool is_file);

    Node* get_node(const path_type& path);
    const Node* get_node(const path_type& path) const;

    static void validate_path(const path_type& path);

    static path_type parent_path(const path_type& path);
    static path_type basename(const path_type& path);
    static path_type join_path(const path_type& parent, const path_type& name);

    static bool matches_mask(std::string_view name, std::string_view mask);

    path_type build_path(const Node* node) const;

    void find_dfs(const Node* node, const path_type& pattern, 
                        path_type& current_path, units_list_type& result) const;

    static bool is_inside(const path_type& from, const path_type& to);

    void erase_index_for_subtree(Node* node);
    void add_index_for_subtree(Node* node);

    std::unique_ptr<Node> detach_from_parent(Node* node);

    std::unique_ptr<Node> clone_node(const Node& node, Node* parent, const path_type& path);

    static size_t memory_usage_of(const Node& node) noexcept;
};

} // namespace filesystem