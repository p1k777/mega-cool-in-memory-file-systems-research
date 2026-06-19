#pragma once
#include "../filesystem.hpp"

#include <memory>
#include <memory_resource>
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
    class CountingMemoryResource : public std::pmr::memory_resource {
    public:
        size_t bytes_allocated() const noexcept;

    private:
        std::pmr::memory_resource* upstream_ = std::pmr::get_default_resource();
        size_t bytes_allocated_ = 0;

        void* do_allocate(size_t bytes, size_t alignment) override;
        void do_deallocate(void* ptr, size_t bytes, size_t alignment) override;
        bool do_is_equal(const std::pmr::memory_resource& other) const noexcept override;
    };
    
    using PmrString = std::pmr::string;
    using PmrByte = bytes_type::value_type;
    using PmrBytes = std::pmr::vector<PmrByte>;

    struct Node {
        PmrString name;
        bool is_file = false;
        PmrBytes data;
        Node* parent = nullptr;
        std::pmr::vector<std::unique_ptr<Node>> children;

        Node(
            std::pmr::memory_resource* memory,
            std::string_view name_value,
            Node* parent_value
        );
    };

    CountingMemoryResource memory_;

    std::unique_ptr<Node> root_;

    std::pmr::unordered_map<PmrString, Node*> index_;

    size_t node_count_ = 0;

    std::unique_ptr<Node> create_node(std::string_view name, Node* parent, bool is_file);

    static path_type to_path(const PmrString& value);

    Node* get_node(const path_type& path);
    const Node* get_node(const path_type& path) const;

    static void validate_path(const path_type& path);

    static path_type parent_path(const path_type& path);
    static path_type basename(const path_type& path);
    static path_type join_path(const path_type& parent, const path_type& name);


    static bool matches_mask(const path_type& name, const path_type& mask);

    path_type build_path(const Node* node) const;

    void find_dfs(const Node* node, const path_type& pattern, units_list_type& result) const;

    static bool is_inside(const path_type& from, const path_type& to);

    void erase_index_for_subtree(Node* node);
    void add_index_for_subtree(Node* node);

    std::unique_ptr<Node> detach_from_parent(Node* node);

    PmrString make_key(const path_type& path) const;
    std::unique_ptr<Node> clone_subtree(const Node& node, Node* parent);
   
};

} // namespace filesystem
