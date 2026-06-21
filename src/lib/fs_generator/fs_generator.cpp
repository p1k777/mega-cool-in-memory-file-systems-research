#include <algorithm>
#include <cstddef>
#include <stdexcept>
#include <string>
#include <utility>

#include "fs_generator.hpp"
#include "generated_fs.hpp"
#include "../random/random.hpp"

namespace fsgenerator {
    
    FsGenerator::FsGenerator(size_t D, size_t W, double F, double file_p, size_t max_nodes_count)
        : target_depth_(D)
        , target_width_(W)
        , file_p_(file_p)
        , target_nodes_count_(1)
        , files_count_(0)
        , dirs_count_(0)
    {

        if (W == 0) { throw std::invalid_argument("W must be positive"); }
        if (F <= 0 || F > 1) { throw std::invalid_argument("F must be in (0, 1]"); }
        if (file_p_ < 0 || file_p > 1) { throw std::invalid_argument("file_p must be in [0, 1]"); }

        size_t complete_tree_size = 0;

        size_t cur_power = 1;
        for (size_t i = 0; i <= D; i++) {
            complete_tree_size += cur_power;

            if (static_cast<double>(complete_tree_size) * F >= max_nodes_count) {
                target_nodes_count_ = max_nodes_count;
                return;
            }

            if (cur_power >= max_nodes_count) {
                target_nodes_count_ = max_nodes_count;
                return;
            }

            cur_power *= W;

        }

        target_nodes_count_ = std::max((size_t)1, std::min(
            static_cast<size_t>(complete_tree_size * F),
            max_nodes_count
        ));
    }



    GeneratedFs FsGenerator::generate() {
        if (!tree_.empty()) { return GeneratedFs(tree_); }

        add_root_();
        grow_spine_();
        
        size_t dirs_count{0}, files_count{0};
        rnd::Random rnd;
        while(!fillable_dirs_.empty() && tree_.size() < target_nodes_count_) {
            size_t idx = rnd.index(fillable_dirs_.size());
            size_t node_idx = fillable_dirs_[idx];

            add_node_(node_idx, rnd);

            if (tree_[node_idx].children >= target_width_) {
                std::swap(fillable_dirs_[idx], fillable_dirs_.back());
                fillable_dirs_.pop_back();
            }
        }

        ensure_file_exists_();
        return GeneratedFs(tree_);
    }

    void FsGenerator::clear() noexcept {
        tree_.clear();
        fillable_dirs_.clear();
    }

    void FsGenerator::add_root_() {
        tree_.push_back({
            .name = "",
            .parent = 0,
            .depth = 0,
            .children = 0,
            .is_file = false,
            .is_root = true
        });

        if (tree_.back().depth < target_depth_) {
            fillable_dirs_.push_back(tree_.size() - 1);
        }
    }

    void FsGenerator::add_node_(size_t parent, rnd::Random& rnd) {
        if (rnd.probability(file_p_)) {
            add_file_(
                std::string("file") + std::to_string(files_count_++),
                parent
            );
        } else {
            add_dir_(
                std::string("dir") + std::to_string(dirs_count_++),
                parent
            );
        }
    }

    void FsGenerator::ensure_file_exists_() {
        if (files_count_ != 0) {
            return;
        }

        for (size_t node_idx = tree_.size(); node_idx-- > 0;) {
            const auto& node = tree_[node_idx];
            if (!node.is_file
                && node.depth < target_depth_
                && node.children < target_width_) {
                add_file_(std::string("file") + std::to_string(files_count_++), node_idx);
                return;
            }
        }

        for (size_t node_idx = tree_.size(); node_idx-- > 0;) {
            auto& node = tree_[node_idx];
            if (!node.is_root && !node.is_file && node.children == 0) {
                node.is_file = true;
                ++files_count_;
                if (dirs_count_ != 0) {
                    --dirs_count_;
                }
                return;
            }
        }

        throw std::runtime_error("failed to generate a filesystem with at least one file");
    }

    void FsGenerator::add_dir_(std::string name, size_t parent, bool is_root) {
        tree_.push_back({
            .name = std::move(name),
            .parent = parent,
            .depth = is_root ? 0 : tree_[parent].depth + 1,
            .children = 0,
            .is_file = false,
            .is_root = is_root
        });

        if (tree_.back().depth < target_depth_) {
            fillable_dirs_.push_back(tree_.size() - 1);
        }

        ++tree_[parent].children;
    }

    void FsGenerator::add_file_(std::string name, size_t parent) {
        tree_.push_back({
            .name = std::move(name),
            .parent = parent,
            .depth = tree_[parent].depth + 1,
            .children = 0,
            .is_file = true,
            .is_root = false
        });
        ++tree_[parent].children;
    }

    void FsGenerator::grow_spine_() {
        if (tree_.empty()) { add_root_(); }
        size_t d = tree_.back().depth;

        while (d < target_depth_) {
            add_dir_("spine" + std::to_string(d), tree_.size() - 1);
            d = tree_.back().depth;
        }
    }

} // namespace fsgenerator
