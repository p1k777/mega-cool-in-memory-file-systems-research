#include "filesystem_b.hpp"

#include <stdexcept>
#include <regex>
#include <algorithm>

namespace filesystem {

size_t FileSystemB::CountingMemoryResource::bytes_allocated() const noexcept {
    return bytes_allocated_;
}

void* FileSystemB::CountingMemoryResource::do_allocate(size_t bytes, size_t alignment) {
    bytes_allocated_ += bytes;
    return upstream_->allocate(bytes, alignment);
}

void FileSystemB::CountingMemoryResource::do_deallocate(void* ptr, size_t bytes, size_t alignment) {
    bytes_allocated_ -= bytes;
    upstream_->deallocate(ptr, bytes, alignment);
}

bool FileSystemB::CountingMemoryResource::do_is_equal(const std::pmr::memory_resource& other) const noexcept {
    return this == &other;
}


FileSystemB::PmrString FileSystemB::make_key(const path_type& path) const {
    PmrString result{std::pmr::polymorphic_allocator<char>{const_cast<CountingMemoryResource*>(&memory_)}};
    result.assign(path.begin(), path.end());
    return result;
}

IFileSystem::path_type FileSystemB::to_path(const PmrString& value) {
    return path_type(value.begin(), value.end());
}

std::unique_ptr<FileSystemB::Node> FileSystemB::create_node(std::string_view name, 
                                                                Node* parent, bool is_file) {
    auto node = std::make_unique<Node>(&memory_, name, parent);
    node->is_file = is_file;

    ++node_count_;

    return node;
}

FileSystemB::FileSystemB()
    : root_(nullptr)
    , index_(&memory_) {
    root_ = create_node("", nullptr, false);
    index_.emplace(make_key("/"), root_.get());
}

FileSystemB::Node::Node(std::pmr::memory_resource* memory, std::string_view name_value,
                                                        Node* parent_value)
    : name(std::pmr::polymorphic_allocator<char>{memory})
    , data(std::pmr::polymorphic_allocator<PmrByte>{memory})
    , parent(parent_value)
    , children(std::pmr::polymorphic_allocator<std::unique_ptr<Node>>{memory}) 
{
    name.assign(name_value.begin(), name_value.end());
}

FileSystemB::Node* FileSystemB::get_node(const path_type& path) {
    auto it = index_.find(make_key(path));

    if(it == index_.end()) {
        throw std::runtime_error("path does not exist");
    }

    return it->second;
}

const FileSystemB::Node* FileSystemB::get_node(const path_type& path) const {
    auto it = index_.find(make_key(path));

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
    validate_path(path);

    const Node* node = get_node(path);

    if(!node->is_file) {
        throw std::runtime_error("read from directory");
    }

    return bytes_type(node->data.begin(), node->data.end());
}

void FileSystemB::op_mkdir(const path_type& path) {
    validate_path(path);

    if(path == "/") {
        return;
    }

    if(index_.find(make_key(path)) != index_.end()) {
        throw std::runtime_error("path already exists");
    }

    path_type parent = parent_path(path);
    path_type name = basename(path);

    Node* parent_node = get_node(parent);

    if(parent_node->is_file) {
        throw std::runtime_error("parent is file");
    }

    auto new_node = create_node(name, parent_node, false);

    Node* raw = new_node.get();

    parent_node->children.push_back(std::move(new_node));
    index_.emplace(make_key(path), raw);
}

void FileSystemB::op_write(const path_type& path, const bytes_type& data) {
    validate_path(path); 
    
    auto it = index_.find(make_key(path));

    if(it != index_.end()) {
        Node* node = it->second;

        if(!node->is_file) {
            throw std::runtime_error("write to directory");
        }

        node->data.assign(data.begin(), data.end());
        return;
    }

    path_type parent = parent_path(path);
    path_type name = basename(path);

    Node* parent_node = get_node(parent);

    if(parent_node->is_file) {
        throw std::runtime_error("parent is file");
    }

    auto new_node = create_node(name, parent_node, true);
    new_node->data.assign(data.begin(), data.end());

    Node* raw = new_node.get();

    parent_node->children.push_back(std::move(new_node));
    index_.emplace(make_key(path), raw);
}

IFileSystem::units_list_type FileSystemB::op_ls(const path_type& path) const {
    validate_path(path);

    const Node* node = get_node(path);

    if(node->is_file) {
        throw std::runtime_error("ls from file");
    }

    units_list_type result;

    for(const auto& child : node->children) {
        result.push_back(join_path(path, to_path(child->name)));
    }

    return result;
}

void FileSystemB::op_mv(const path_type& from, const path_type& to) {
    validate_path(from);
    validate_path(to);
    
    if(from == "/") {
        throw std::runtime_error("cannot move root");
    }

    auto from_it = index_.find(make_key(from));

    if(from_it == index_.end()) {
        throw std::runtime_error("source does not exist");
    }

    if(index_.find(make_key(to)) != index_.end()) {
        throw std::runtime_error("destination already exists");
    }

    if(is_inside(from, to)) {
        throw std::runtime_error("cannot move directory inside itself");
    }

    Node* node = from_it->second;

    path_type new_parent_path = parent_path(to);
    path_type new_name = basename(to);

    Node* new_parent = get_node(new_parent_path);

    if(new_parent->is_file) {
        throw std::runtime_error("new parent is file");
    }

    erase_index_for_subtree(node);

    std::unique_ptr<Node> owned = detach_from_parent(node);

    owned->name.assign(new_name.begin(), new_name.end());
    owned->parent = new_parent;

    Node* raw = owned.get();

    new_parent->children.push_back(std::move(owned));

    add_index_for_subtree(raw);
}

IFileSystem::units_list_type FileSystemB::op_find(const path_type& path, const path_type& pattern) const {
    validate_path(path);

    const Node* node = get_node(path);

    if(node->is_file) {
        throw std::runtime_error("find from file");
    }

    units_list_type result;
    find_dfs(node, pattern, result);

    return result;
}

size_t FileSystemB::get_memory_usage() const noexcept {
    return sizeof(*this) + node_count_ * sizeof(Node) + memory_.bytes_allocated();
}

bool FileSystemB::matches_mask(const path_type& name, const path_type& mask) {
    std::string regex_pattern = "^";

    for(char c : mask) {
        if(c == '*') {
            regex_pattern += ".*";
        } else if(c == '?') {
            regex_pattern += ".";
        } else if(c == '.' || c == '\\' || c == '+' || c == '(' || c == ')' ||
                  c == '[' || c == ']' || c == '{' || c == '}' || c == '^' ||
                  c == '$' || c == '|') {
            regex_pattern += '\\';
            regex_pattern += c;
        } else {
            regex_pattern += c;
        }
    }

    regex_pattern += "$";

    return std::regex_match(name, std::regex(regex_pattern));
}

IFileSystem::path_type FileSystemB::build_path(const Node* node) const {
    if(node == root_.get()) {
        return "/";
    }

    std::vector<std::string> parts;

    while(node != nullptr && node != root_.get()) {
        parts.push_back(to_path(node->name));
        node = node->parent;
    }

    path_type result;

    for(auto it = parts.rbegin(); it != parts.rend(); ++it) {
        result += "/";
        result += *it;
    }

    return result.empty() ? "/" : result;
}

void FileSystemB::find_dfs(const Node* node, const path_type& pattern, units_list_type& result) const {
    path_type current_path = build_path(node);

    if(node != root_.get() && matches_mask(to_path(node->name), pattern)) {
        result.push_back(current_path);
    }

    for(const auto& child : node->children) {
        find_dfs(child.get(), pattern, result);
    }
}

bool FileSystemB::is_inside(const path_type& from, const path_type& to) {
    if(to.size() <= from.size()) {
        return false;
    }

    if(to.compare(0, from.size(), from) != 0) {
        return false;
    }

    return to[from.size()] == '/';
}

void FileSystemB::erase_index_for_subtree(Node* node) {
    auto path = build_path(node);
    index_.erase(make_key(path));

    for(auto& child : node->children) {
        erase_index_for_subtree(child.get());
    }
}

void FileSystemB::add_index_for_subtree(Node* node) {
    auto path = build_path(node);
    index_.emplace(make_key(path), node);

    for(auto& child : node->children) {
        add_index_for_subtree(child.get());
    }
}

std::unique_ptr<FileSystemB::Node> FileSystemB::detach_from_parent(Node* node) {
    Node* parent = node->parent;
    auto& children = parent->children;

    auto it = std::find_if(
        children.begin(),
        children.end(),
        [node](const std::unique_ptr<Node>& child) {
            return child.get() == node;
        }
    );

    if(it == children.end()) {
        throw std::runtime_error("broken tree");
    }

    std::unique_ptr<Node> owned = std::move(*it);
    children.erase(it);

    return owned;
}

void FileSystemB::validate_path(const path_type& path) {
    if(path.empty()) {
        throw std::runtime_error("path is empty");
    }

    if(path[0] != '/') {
        throw std::runtime_error("path must be absolute");
    }

    if(path == "/") {
        return;
    }

    if(path.back() == '/') {
        throw std::runtime_error("path has trailing slash");
    }

    size_t start = 1;

    while(start < path.size()) {
        size_t slash_pos = path.find('/', start);
        size_t end = slash_pos == path_type::npos ? path.size() : slash_pos;

        path_type component = path.substr(start, end - start);

        if(component.empty()) {
            throw std::runtime_error("path has empty component");
        }

        if(component == "." || component == "..") {
            throw std::runtime_error("path has invalid component");
        }

        if(slash_pos == path_type::npos) {
            break;
        }

        start = slash_pos + 1;
    }
}

} // namespace filesystem