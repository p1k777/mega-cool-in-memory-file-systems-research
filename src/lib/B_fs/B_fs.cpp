#include "B_fs.hpp"

#include <stdexcept>
#include <algorithm>
#include <utility>

namespace filesystem {

std::unique_ptr<FileSystemB::Node> FileSystemB::create_node(std::string_view name, 
                                                                Node* parent, bool is_file) {
    auto node = std::make_unique<Node>(name, parent);
    node->is_file = is_file;
    return node;
}

FileSystemB::FileSystemB() {
    root_ = create_node("", nullptr, false);
    index_.emplace("/", root_.get());
}

FileSystemB::FileSystemB(const FileSystemB& other) {
    index_.reserve(other.index_.size());
    root_ = clone_node(*other.root_, nullptr, "/");
}

FileSystemB& FileSystemB::operator=(const FileSystemB& other) {
    if(this == &other) {
        return *this;
    }

    FileSystemB copy(other);
    *this = std::move(copy);

    return *this;
}

FileSystemB::Node::Node(std::string_view name_value, Node* parent_value)
    : name(name_value)
    , parent(parent_value){}

std::unique_ptr<FileSystemB::Node> FileSystemB::clone_node(const Node& node, 
                                                            Node* parent,const path_type& path) {
    auto copy = std::make_unique<Node>(node.name, parent);

    copy->is_file = node.is_file;
    copy->data = node.data;
    copy->children.reserve(node.children.size());

    Node* raw = copy.get();
    index_.emplace(path, raw);

    for(const auto& child : node.children) {
        const path_type child_path =
            join_path(path, child->name);

        copy->children.push_back(
            clone_node(*child, raw, child_path)
        );
    }

    return copy;
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

IFileSystem::bytes_type FileSystemB::op_read(const path_type& path) const {
    validate_path(path);

    const Node* node = get_node(path);

    if(!node->is_file) {
        throw std::runtime_error("read from directory");
    }

    return node->data;
}

void FileSystemB::op_mkdir(const path_type& path) {
    validate_path(path);

    if(index_.find(path) != index_.end()) {
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
    index_.emplace(path, raw);
}

void FileSystemB::op_write(const path_type& path, const bytes_type& data) {
    validate_path(path); 
    
    auto it = index_.find(path);

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
    index_.emplace(path, raw);
}

IFileSystem::units_list_type FileSystemB::op_ls(const path_type& path) const {
    validate_path(path);

    const Node* node = get_node(path);

    if(node->is_file) {
        throw std::runtime_error("ls from file");
    }

    units_list_type result;

    for(const auto& child : node->children) {
        result.push_back(join_path(path, child->name));
    }

    return result;
}

void FileSystemB::op_mv(const path_type& from, const path_type& to) {
    validate_path(from);
    validate_path(to);
    
    if(from == "/") {
        throw std::runtime_error("cannot move root");
    }

    auto from_it = index_.find(from);

    if(from_it == index_.end()) {
        throw std::runtime_error("source does not exist");
    }

    if(index_.find(to) != index_.end()) {
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
    path_type current_path = path;

    find_dfs(node, pattern, current_path, result);

    return result;
}

size_t FileSystemB::memory_usage_of(const Node& node) noexcept {
    size_t usage =
        sizeof(Node)
        + node.name.capacity() * sizeof(char)
        + node.data.capacity() * sizeof(bytes_type::value_type)
        + node.children.capacity()
              * sizeof(std::unique_ptr<Node>);

    for(const auto& child : node.children) {
        usage += memory_usage_of(*child);
    }

    return usage;
}

size_t FileSystemB::get_memory_usage() const noexcept {
    size_t usage = sizeof(*this) + memory_usage_of(*root_);

    usage += index_.bucket_count() * sizeof(void*);

    for(const auto& [path, node] : index_) {
        (void)node;

        usage += sizeof(std::pair<const std::string, Node*>);
        usage += path.capacity() * sizeof(char);
    }

    return usage;
}

bool FileSystemB::matches_mask(std::string_view name, std::string_view mask) {
    if(mask == "*") {
        return true;
    }

    size_t name_pos = 0;
    size_t mask_pos = 0;
    size_t star_pos = std::string_view::npos;
    size_t match_after_star = 0;

    while(name_pos < name.size()) {
        if(mask_pos < mask.size() &&
           (mask[mask_pos] == '?' || mask[mask_pos] == name[name_pos])) {
            ++name_pos;
            ++mask_pos;
        } else if(mask_pos < mask.size() && mask[mask_pos] == '*') {
            star_pos = mask_pos++;
            match_after_star = name_pos;
        } else if(star_pos != std::string_view::npos) {
            mask_pos = star_pos + 1;
            name_pos = ++match_after_star;
        } else {
            return false;
        }
    }

    while(mask_pos < mask.size() && mask[mask_pos] == '*') {
        ++mask_pos;
    }

    return mask_pos == mask.size();
}

IFileSystem::path_type FileSystemB::build_path(const Node* node) const {
    if(node == root_.get()) {
        return "/";
    }

    std::vector<std::string> parts;

    while(node != nullptr && node != root_.get()) {
        parts.push_back(node->name);
        node = node->parent;
    }

    path_type result;

    for(auto it = parts.rbegin(); it != parts.rend(); ++it) {
        result += "/";
        result += *it;
    }

    return result.empty() ? "/" : result;
}

void FileSystemB::find_dfs(const Node* node, const path_type& pattern,
                                path_type& current_path, units_list_type& result) const {
    if(node != root_.get() && matches_mask(node->name, pattern)) {
        result.push_back(current_path);
    }

    for(const auto& child : node->children) {
        size_t old_size = current_path.size();

        if(current_path.back() != '/') {
            current_path.push_back('/');
        }

        current_path.append(child->name.data(), child->name.size());

        find_dfs(child.get(), pattern, current_path, result);

        current_path.resize(old_size);
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
    auto it = index_.find(path);

    if(it != index_.end()) {
        index_.erase(it);
    }

    for(auto& child : node->children) {
        erase_index_for_subtree(child.get());
    }
}

void FileSystemB::add_index_for_subtree(Node* node) {
    auto path = build_path(node);
    index_.emplace(path, node);

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
