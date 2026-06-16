#include <random>
#include <string>
#include <vector>

#include "path_generator.hpp"
#include "../common.hpp"
#include "../fs_generator/generated_fs.hpp"

namespace pathgen {

    IPathGenerator::IPathGenerator(const fsgenerator::GeneratedFs& fs, double loc)
        : generated_fs_(fs)
        , loc_(loc)
        , last_dir_(-1)
        , children_(fs.get_dirs().size() + fs.get_files().size())
        , rnd_()
    {
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
    IPathGenerator::generate(const std::vector<OperationType>& ops) noexcept {
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
                    result.push_back("new_" + generated_fs_.get_path(gen_dir_()));
                    break;

                case OperationType::Ls:
                    result.push_back(generated_fs_.get_path(gen_dir_()));
                    break;

                case OperationType::Move:
                    result.push_back(generated_fs_.get_path(gen_file_()));
                    result.push_back(generated_fs_.get_path(gen_file_()));
                    break;

                case OperationType::Find:
                    result.push_back(generated_fs_.get_path(gen_dir_()) + "/*");
                    break;
            }
        }

        return result;
    }

    
    UniformPathGenerator::UniformPathGenerator(const fsgenerator::GeneratedFs& fs, double loc)
        : IPathGenerator(fs, loc) {}


    size_t UniformPathGenerator::gen_file_() noexcept {
        size_t cur = -1;
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
        size_t cur = -1;
        if (last_dir_ != -1 && !children_[last_dir_].dirs.empty() && rnd_.probability(loc_)) {
            cur = rnd_.choice(children_[last_dir_].dirs);
        } else {
            size_t idx = rnd_.index(generated_fs_.get_dirs().size());
            cur = generated_fs_.get_dirs()[idx];
        }

        last_dir_ = generated_fs_.get_node(cur).parent;
        return cur;
    }

} // namespace pathgen

