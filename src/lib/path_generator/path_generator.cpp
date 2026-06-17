#include <algorithm>
#include <cmath>
#include <cstddef>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>

#include "path_generator.hpp"
#include "../common.hpp"
#include "../fs_generator/generated_fs.hpp"

namespace {
    static std::string join_path(const std::string& dir, const std::string& name) {
        if (dir == "/") {
            return "/" + name;
        }
        return dir + "/" + name;
    }
}

namespace pathgen {

    IPathGenerator::IPathGenerator(const fsgenerator::GeneratedFs& fs, double loc)
        : generated_fs_(fs)
        , loc_(loc)
        , last_dir_(-1)
        , children_(fs.get_dirs().size() + fs.get_files().size())
        , rnd_()
        , cnt_(0)
    {
        if (generated_fs_.get_dirs().empty()) { throw std::invalid_argument("requires at least 1 dir"); }
        if (generated_fs_.get_files().empty()) { throw std::invalid_argument("requires at least 1 file"); }
        if (0 > loc_ || loc_ >= 1) { throw std::invalid_argument("loc must be 0 <= loc < 1"); }

        for (size_t i : fs.get_dirs()) {
            const auto& node = fs.get_node(i);
            if (!node.is_root) {
                children_[node.parent].dirs.push_back(i);
            }
        }

        for (size_t i : fs.get_files()) {
            children_[fs.get_node(i).parent].files.push_back(i);
        }
    }

    std::vector<IPathGenerator::path_type>
    IPathGenerator::generate(const std::vector<OperationType>& ops) {
        std::vector<path_type> result;

        for (OperationType op : ops) {
            switch (op) {
                case OperationType::Read:
                    result.push_back(generated_fs_.get_path(gen_file_()));
                    break;

                case OperationType::Write:
                    result.push_back(generated_fs_.get_path(gen_file_()));
                    break;

                case OperationType::Mkdir:
                    result.push_back(join_path(generated_fs_.get_path(gen_dir_()), "new_dir" + std::to_string(cnt_++)));
                    break;

                case OperationType::Ls:
                    result.push_back(generated_fs_.get_path(gen_dir_()));
                    break;

                case OperationType::Move: {
                    size_t file = gen_file_();
                    result.push_back(generated_fs_.get_path(file));
                    result.push_back(join_path(generated_fs_.get_path(gen_dir_()), generated_fs_.get_node(file).name + "_new"));
                    break;
                }

                case OperationType::Find:
                    result.push_back(gen_pattern_());
                    break;
            }
        }

        return result;
    }

    
    UniformPathGenerator::UniformPathGenerator(const fsgenerator::GeneratedFs& fs, double loc)
        : IPathGenerator(fs, loc) {}

    size_t UniformPathGenerator::gen_file_() noexcept {
        size_t cur;
        if (last_dir_ != -1 && !children_[last_dir_].files.empty() && rnd_.probability(loc_)) {
            cur = rnd_.choice(children_[last_dir_].files);
        } else {
            size_t idx = rnd_.index(generated_fs_.get_files().size());
            cur = generated_fs_.get_files()[idx];
        }

        last_dir_ = generated_fs_.get_node(cur).parent;
        return cur;
    }
    size_t UniformPathGenerator::gen_dir_() noexcept {
        size_t cur;
        if (last_dir_ != -1 && !children_[last_dir_].dirs.empty() && rnd_.probability(loc_)) {
            cur = rnd_.choice(children_[last_dir_].dirs);
        } else {
            size_t idx = rnd_.index(generated_fs_.get_dirs().size());
            cur = generated_fs_.get_dirs()[idx];
        }

        last_dir_ = cur;
        return cur;
    }
    IPathGenerator::path_type UniformPathGenerator::gen_pattern_() {
        return generated_fs_.get_path(gen_dir_()) + "/*";
    }

    ZipfPathGenerator::ZipfPathGenerator(const fsgenerator::GeneratedFs& fs, double s, double loc)
        : IPathGenerator(fs, loc)
        , s_(s)
        , files_distr_(0, fs.get_files().size() - 1, s_)
        , dirs_distr_(0, fs.get_dirs().size() - 1, s_)
        , rank_to_file_(fs.get_files())
        , rank_to_dir_(fs.get_dirs())
    {
        if (s <= 0) { throw std::invalid_argument("s must be >= 0"); }

        std::mt19937_64 g;
        std::shuffle(rank_to_file_.begin(), rank_to_file_.end(), g);
        std::shuffle(rank_to_dir_.begin(), rank_to_dir_.end(), g);

        size_t W = 0;
        for (size_t i : generated_fs_.get_dirs()) {
            W = std::max(W, generated_fs_.get_node(i).children);
        }

        small_distrs_.reserve(W + 1);
        small_distrs_.emplace_back(0, 0, s_);
        for (size_t n = 1; n < W + 1; ++n) {
            small_distrs_.emplace_back(0, n-1, s_);
        }
    }

    size_t ZipfPathGenerator::gen_file_() noexcept {
        size_t cur;
        if (last_dir_ != -1 && !children_[last_dir_].files.empty() && rnd_.probability(loc_)) {
            cur = children_[last_dir_].files[
                rnd_.generate(small_distrs_[children_[last_dir_].files.size()])
            ];
        } else {
            cur = rank_to_file_[rnd_.generate(files_distr_)];
        }

        last_dir_ = generated_fs_.get_node(cur).parent;
        return cur;
    }
    size_t ZipfPathGenerator::gen_dir_() noexcept {
        size_t cur;
        if (last_dir_ != -1 && !children_[last_dir_].dirs.empty() && rnd_.probability(loc_)) {
            cur = children_[last_dir_].dirs[
                rnd_.generate(small_distrs_[children_[last_dir_].dirs.size()])
            ];
        } else {
            cur = rank_to_dir_[rnd_.generate(dirs_distr_)];
        }

        last_dir_ = cur;
        return cur;
    }
    IPathGenerator::path_type ZipfPathGenerator::gen_pattern_() {
        return generated_fs_.get_path(gen_dir_()) + "/*";
    }

} // namespace pathgen

