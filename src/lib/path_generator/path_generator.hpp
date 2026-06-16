#pragma once

#include <cstddef>
#include <vector>

#include "../common.hpp"
#include "../filesystem.hpp"
#include "../fs_generator/generated_fs.hpp"
#include "../random/random.hpp"
#include "external/zipfian_int_distribution.h"

namespace pathgen {
    
    class IPathGenerator {
    public:
        using path_type = filesystem::IFileSystem::path_type;

        IPathGenerator(const fsgenerator::GeneratedFs& fs, double loc);

        std::vector<path_type> generate(const std::vector<OperationType>&) noexcept;

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
    };


    class UniformPathGenerator : public IPathGenerator {
    public:

        UniformPathGenerator(const fsgenerator::GeneratedFs& fs, double loc);
        // std::vector<path_type> generate(const std::vector<OperationType>&) noexcept override;
        
    private:
        size_t gen_file_() noexcept override;
        size_t gen_dir_() noexcept override;
    };


    class ZipfPathgenerator : public IPathGenerator {
    public:

        ZipfPathgenerator(const fsgenerator::GeneratedFs& fs, double s, double loc);
        // std::vector<path_type> generate(const std::vector<OperationType>&) noexcept override;

    private:
        size_t gen_file_() noexcept override;
        size_t gen_dir_() noexcept override;
    };

} // namespace pathgen

