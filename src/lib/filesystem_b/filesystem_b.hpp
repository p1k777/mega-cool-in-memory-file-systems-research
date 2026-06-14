#pragma once
#include "../filesystem.hpp"

#include <memory>
#include <unordered_map>

namespace filesystem {

class FileSystemB final: public IFileSystem {
public:
    FileSystemB();
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
    };

    std::unique_ptr<Node> root_;
    std::unordered_map<path_type, Node*> index_;

    Node* get_node(const path_type& path);
    const Node* get_node(const path_type& path) const;
    static path_type parent_path(const path_type& path);
    static path_type basename(const path_type& path);
    static path_type join_path(const path_type& parent, const path_type& name);


    static bool matches_mask(const path_type& name, const path_type& mask);

    path_type build_path(const Node* node) const;

    void find_dfs(const Node* node, const path_type& pattern, units_list_type& result) const;
   
};

} // namespace filesystem