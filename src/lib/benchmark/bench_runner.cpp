#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <exception>
#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include "bench.hpp"
#include "../common.hpp"
#include "../A_fs/A_fs.hpp"
#include "../filesystem_b/filesystem_b.hpp"
#include "../C_fs/C_fs.hpp"

namespace {

using benchmark::Benchmark;
using benchmark::Distribution;
using benchmark::ExperimentConfig;
using benchmark::FileSystemType;

using AFileSystem = filesystem::TreeFileSystem;
using BFileSystem = filesystem::FileSystemB;
using CFileSystem = filesystem::FlatHashFileSystem;

void PrintUsage(const char* program) {
    std::cerr
        << "Usage:\n"
        << "  " << program << " FS Repeats Ops Profile OutputCsv\n\n"
        << "Arguments:\n"
        << "  FS         A | B | C\n"
        << "  Repeats    number of repeats for each grid point\n"
        << "  Ops        number of operations in each run\n"
        << "  Profile    build_system|build|bs | "
           "file_manager|file|fm | "
           "backup|bu | "
           "refactoring|ref | "
           "database|db | "
           "web_server|web|ws\n"
        << "  OutputCsv  path to output csv file\n\n"
        << "Example:\n"
        << "  " << program << " B 5 10000 db results.csv\n";
}

std::size_t ParseSize(const char* value, std::string_view name) {
    try {
        std::size_t pos = 0;
        const unsigned long long parsed = std::stoull(value, &pos);

        if (value[pos] != '\0') {
            throw std::invalid_argument("trailing characters");
        }

        return static_cast<std::size_t>(parsed);
    } catch (...) {
        throw std::invalid_argument("bad size_t argument: " + std::string(name));
    }
}

FileSystemType ParseFileSystemType(std::string_view value) {
    if (value == "A" || value == "a") {
        return FileSystemType::A;
    }

    if (value == "B" || value == "b") {
        return FileSystemType::B;
    }

    if (value == "C" || value == "c") {
        return FileSystemType::C;
    }

    throw std::invalid_argument("FS must be one of: A, B, C");
}

const benchmark::ProbabilityProfile& ParseProfile(std::string_view value) {
    auto matches = [value](std::string_view full_name,
                           std::initializer_list<std::string_view> aliases) {
        if (value == full_name) {
            return true;
        }

        for (const std::string_view alias : aliases) {
            if (value == alias) {
                return true;
            }
        }

        return false;
    };

    for (const auto& profile : benchmark::profiles) {
        if (matches(profile.name, {"build", "bs"}) &&
            profile.name == "build_system") {
            return profile;
        }

        if (matches(profile.name, {"file", "fm"}) &&
            profile.name == "file_manager") {
            return profile;
        }

        if (matches(profile.name, {"bu"}) &&
            profile.name == "backup") {
            return profile;
        }

        if (matches(profile.name, {"ref"}) &&
            profile.name == "refactoring") {
            return profile;
        }

        if (matches(profile.name, {"db"}) &&
            profile.name == "database") {
            return profile;
        }

        if (matches(profile.name, {"web", "ws"}) &&
            profile.name == "web_server") {
            return profile;
        }
    }

    throw std::invalid_argument(
        "Profile must be one of: build_system/build/bs, "
        "file_manager/file/fm, backup/bu, refactoring/ref, "
        "database/db, web_server/web/ws"
    );
}

std::string FileSystemTypeToString(FileSystemType type) {
    switch (type) {
        case FileSystemType::A:
            return "A";
        case FileSystemType::B:
            return "B";
        case FileSystemType::C:
            return "C";
    }

    return "unknown";
}

std::string DistributionToString(Distribution dist) {
    switch (dist) {
        case Distribution::Uniform:
            return "uniform";
        case Distribution::Zipf:
            return "zipf";
    }

    return "unknown";
}

bool CsvNeedsHeader(const std::string& path) {
    std::ifstream input(path);
    return !input.good() || input.peek() == std::ifstream::traits_type::eof();
}

void WriteCsvHeader(std::ofstream& output) {
    output
        << "fs_type,"
        << "depth,width,fill_factor,"
        << "profile_name,"
        << "p_read,p_write,p_mkdir,p_ls,p_mv,p_find,"
        << "distribution,locality,zipf_s,"
        << "operations,repeats,"
        << "avg_latency_us,p99_latency_us,throughput_ops_sec,memory_usage_bytes\n";
}

void WriteCsvRow(
    std::ofstream& output,
    const ExperimentConfig& cfg,
    const std::string& profile_name,
    const Metrics& metrics
) {
    output
        << FileSystemTypeToString(cfg.fs_type) << ','
        << cfg.D << ','
        << cfg.W << ','
        << cfg.F << ','
        << profile_name << ','
        << cfg.p_read << ','
        << cfg.p_write << ','
        << cfg.p_mkdir << ','
        << cfg.p_ls << ','
        << cfg.p_mv << ','
        << cfg.p_find << ','
        << DistributionToString(cfg.distribution) << ','
        << cfg.locality << ','
        << cfg.zipf_p << ','
        << cfg.operations << ','
        << cfg.repeats << ','
        << metrics.avg_latency_us << ','
        << metrics.p99_latency_us << ','
        << metrics.throughput_ops_sec << ','
        << metrics.memory_usage_bytes << '\n';
}

void ValidateArguments(std::size_t repeats, std::size_t operations) {
    if (repeats == 0) {
        throw std::invalid_argument("Repeats must be > 0");
    }

    if (operations == 0) {
        throw std::invalid_argument("Ops must be > 0");
    }
}

ExperimentConfig MakeConfig(
    FileSystemType fs_type,
    std::size_t repeats,
    std::size_t operations,
    int depth,
    int width,
    double fill_factor,
    const benchmark::ProbabilityProfile& profile,
    const benchmark::DistributionProfile& distribution_profile
) {
    ExperimentConfig cfg{};

    cfg.D = depth;
    cfg.W = width;
    cfg.F = fill_factor;

    cfg.p_read = profile.op_read;
    cfg.p_write = profile.op_write;
    cfg.p_mkdir = profile.op_mkdir;
    cfg.p_ls = profile.op_ls;
    cfg.p_mv = profile.op_mv;
    cfg.p_find = profile.op_find;

    cfg.distribution = distribution_profile.dist;
    cfg.locality = distribution_profile.locality;
    cfg.zipf_p = distribution_profile.zipf_s;

    cfg.operations = operations;
    cfg.repeats = repeats;
    cfg.fs_type = fs_type;

    return cfg;
}

void RunProfileConfigs(
    std::ofstream& output,
    const fsgenerator::GeneratedFs& file_system,
    FileSystemType fs_type,
    std::size_t repeats,
    std::size_t operations,
    int depth,
    int width,
    double fill_factor,
    const benchmark::ProbabilityProfile& profile
) {
    Benchmark benchmark;

    for (const auto& distribution_profile : benchmark::d_profiles) {
        ExperimentConfig cfg = MakeConfig(
            fs_type,
            repeats,
            operations,
            depth,
            width,
            fill_factor,
            profile,
            distribution_profile
        );

        const std::vector<benchmark::Operation> operations =
            benchmark.BuildOperations(file_system, cfg);
        const Metrics metrics =
            benchmark.RunPrepared(file_system, cfg, operations);
        WriteCsvRow(output, cfg, profile.name, metrics);

        std::cout
            << "[BM RUN] Suit Profile=" << profile.name
            << " D=" << cfg.D
            << " W=" << cfg.W
            << " F=" << cfg.F
            << " Dist=" << DistributionToString(cfg.distribution)
            << " finished" << '\n';
    }
}

} // namespace

int main(int argc, char** argv) {
    try {
        if (argc != 6) {
            PrintUsage(argv[0]);
            return EXIT_FAILURE;
        }

        const FileSystemType fs_type = ParseFileSystemType(argv[1]);
        const std::size_t repeats = ParseSize(argv[2], "Repeats");
        const std::size_t operations = ParseSize(argv[3], "Ops");
        const benchmark::ProbabilityProfile& profile = ParseProfile(argv[4]);
        const std::string output_csv = argv[5];

        ValidateArguments(repeats, operations);

        const bool need_header = CsvNeedsHeader(output_csv);
        std::ofstream output(output_csv, std::ios::app);
        if (!output) {
            throw std::runtime_error("cannot open output csv: " + output_csv);
        }

        if (need_header) {
            WriteCsvHeader(output);
        }

        for (int depth : {2, 5, 10}) {
            for (int width : {5, 10, 15}) {
                for (double fill_factor : {0.3, 0.6, 0.95}) {
                    fsgenerator::FsGenerator fs_generator(
                        static_cast<std::size_t>(depth),
                        static_cast<std::size_t>(width),
                        fill_factor
                    );
                    const fsgenerator::GeneratedFs file_system =
                        fs_generator.generate();

                    RunProfileConfigs(
                        output,
                        file_system,
                        fs_type,
                        repeats,
                        operations,
                        depth,
                        width,
                        fill_factor,
                        profile
                    );
                }
            }
        }

        return EXIT_SUCCESS;
    } catch (const std::exception& error) {
        std::cerr << "benchmark runner error: " << error.what() << '\n';
        return EXIT_FAILURE;
    }
}
