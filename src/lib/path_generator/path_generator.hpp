#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "../common.hpp"
#include "../filesystem.hpp"
#include "../fs_generator/generated_fs.hpp"
#include "../random/random.hpp"
#include "zipf_distribution.hpp"

namespace pathgen {
    
    class IPathGenerator {
    public:
        using path_type = filesystem::IFileSystem::path_type;

        IPathGenerator(const fsgenerator::GeneratedFs& fs, double loc);

        std::vector<path_type> generate(const std::vector<OperationType>&);

        virtual ~IPathGenerator() = default;

    protected:

        virtual size_t gen_file_() noexcept = 0;
        virtual size_t gen_dir_() noexcept = 0;

        const fsgenerator::GeneratedFs& generated_fs_;
        double loc_;
        size_t last_dir_;

        struct Children {
            std::vector<size_t> dirs;
            std::vector<size_t> files;
        };
        std::vector<Children> children_;

        rnd::Random rnd_;

        size_t cnt_;
        size_t move_cnt_;

        std::vector<path_type> current_paths_;
    };


    class UniformPathGenerator : public IPathGenerator {
    public:

        UniformPathGenerator(const fsgenerator::GeneratedFs& fs, double loc);
        
    private:
        size_t gen_file_() noexcept override;
        size_t gen_dir_() noexcept override;
    };


    class ZipfPathGenerator : public IPathGenerator {
    public:

        ZipfPathGenerator(const fsgenerator::GeneratedFs& fs, double s, double loc);

    private:
        size_t gen_file_() noexcept override;
        size_t gen_dir_() noexcept override;

        double s_;
        ZipfDistribution<size_t> files_distr_;
        ZipfDistribution<size_t> dirs_distr_;

        std::vector<size_t> rank_to_file_;
        std::vector<size_t> rank_to_dir_;

        std::vector<ZipfDistribution<size_t>> small_distrs_;
    };

} // namespace pathgen
