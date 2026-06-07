#include <algorithm>
#include <cstddef>
#include <numeric>
#include <utility>
#include <vector>

#include "../filesystem.hpp"
#include "generated-fs.hpp"

namespace fsgenerator {



    GeneratedFs::GeneratedFs(std::vector<tree_node_type> tree) {
        std::vector<size_t> order(tree.size());
        std::iota(order.begin(), order.end(), 0);

        std::sort(order.begin(), order.end(),
            [&](size_t a, size_t b) {
                return tree[a].depth < tree[b].depth;
            }
        );

        fs_.reserve(tree.size());

        std::vector<size_t> tree_to_fs(tree.size());

        bool root_found = false;

        for (size_t tree_idx : order) {
            const auto& node = tree[tree_idx];

            path_type path;

            if (node.is_root) {
                path = "/";

                if (root_found) {
                    /* ... */ // два корня — ошибка
                }

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
        }

        if (!root_found) {
            /* ... */ // нет корня — ошибка
        }
    }

    GeneratedFs& GeneratedFs::operator=(GeneratedFs tmp) {
        std::swap(*this, tmp);
        return *this;
    }

    void GeneratedFs::fill(filesystem::IFileSystem& fs_impl) {
        for (auto& node : fs_) {
            if (node.is_file) {
                fs_impl.op_write(node.path, {'p', 'a', 'y', 'l', 'o', 'a', 'd'});
            } else {
                fs_impl.op_mkdir(node.path);
            }
        }
    }

}; // namespace fsgenerator