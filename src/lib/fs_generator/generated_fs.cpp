#include <algorithm>
#include <cstddef>
#include <numeric>
#include <stdexcept>
#include <utility>
#include <vector>

#include "../filesystem.hpp"
#include "generated_fs.hpp"

#include <iostream>

namespace fsgenerator {



    GeneratedFs::GeneratedFs(std::vector<tree_node_type> tree)
        : tree_(std::move(tree))
    {
        std::vector<size_t> order(tree_.size());
        std::iota(order.begin(), order.end(), 0);

        std::sort(order.begin(), order.end(),
            [&](size_t a, size_t b) {
                return tree_[a].depth < tree_[b].depth;
            }
        );

        fs_.reserve(tree_.size());
        fs_to_tree_.reserve(tree_.size());

        std::vector<size_t> tree_to_fs(tree_.size());

        bool root_found = false;

        for (size_t tree_idx : order) {
            const auto& node = tree_[tree_idx];

            path_type path;

            if (node.is_root) {
                path = "/";

                if (root_found) { throw std::invalid_argument("Provided tree must contain only 1 root"); }

                root_found = true;
            } else {
                const size_t parent_fs_idx = tree_to_fs[node.parent];
                const auto& parent_path = fs_[parent_fs_idx].path;

                if (parent_path == "/") {
                    path = parent_path + node.name;
                } else {
                    path = parent_path + "/" + node.name;
                }
            }

            const size_t fs_idx = fs_.size();
            tree_to_fs[tree_idx] = fs_idx;

            if (node.is_file) {
                files_.push_back(fs_idx);
            } else {
                dirs_.push_back(fs_idx);
            }

            fs_.push_back(FsNode{
                .path = std::move(path),
                .is_file = node.is_file
            });
            fs_to_tree_.push_back(tree_idx);
        }

        if (!root_found) { throw std::invalid_argument("Provided tree must contain the root"); }
    }

    GeneratedFs& GeneratedFs::operator=(GeneratedFs tmp) {
        std::swap(*this, tmp);
        return *this;
    }

    void GeneratedFs::fill(filesystem::IFileSystem& fs_impl) const {
        for (const auto& node : fs_) {
            if (node.path == "/") { continue; }

            if (node.is_file) {
                fs_impl.op_write(node.path, {'p', 'a', 'y', 'l', 'o', 'a', 'd'});
            } else {
                fs_impl.op_mkdir(node.path);
            }

            // std::cout << "[GEN_FS] Added '" << node.path << "'\n";
        }
    }
    
    const GeneratedFs::path_type& GeneratedFs::get_path(size_t idx) const {
        return fs_.at(idx).path;
    }

    const std::vector<size_t>& GeneratedFs::get_files() const {
        return files_;
    }
    const std::vector<size_t>& GeneratedFs::get_dirs() const {
        return dirs_;
    }
    const GeneratedFs::tree_node_type& GeneratedFs::get_node(size_t idx) const {
        return tree_.at(idx);
    }

    const GeneratedFs::tree_node_type& GeneratedFs::get_tree_node_for_fs_index(size_t idx) const {
        return tree_.at(fs_to_tree_.at(idx));
    }

}; // namespace fsgenerator
