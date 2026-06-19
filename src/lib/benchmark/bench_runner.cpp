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
#include <thread>
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
        << "  " << program << " FS Repeats Ops OutputCsv\n\n"
        << "Arguments:\n"
        << "  FS         A | B | C\n"
        << "  Repeats    number of repeats for each grid point\n"
        << "  Ops        number of operations in each run\n"
        << "  OutputCsv  path to output csv file\n\n"
        << "Example:\n"
        << "  " << program << " B 5 10000 results.csv\n";
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

std::unique_ptr<filesystem::IFileSystem> MakeFileSystem(FileSystemType type) {
    switch (type) {
        case FileSystemType::A:
            return std::make_unique<AFileSystem>();
        case FileSystemType::B:
            return std::make_unique<BFileSystem>();
        case FileSystemType::C:
            return std::make_unique<CFileSystem>();
    }

    throw std::invalid_argument("unknown filesystem type");
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
        << cfg.depth << ','
        << cfg.width << ','
        << cfg.fill_factor << ','
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

struct CsvRow {
    ExperimentConfig cfg;
    std::string profile_name;
    Metrics metrics;
};

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

    cfg.depth = depth;
    cfg.width = width;
    cfg.fill_factor = fill_factor;

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

std::vector<CsvRow> RunProfileConfigs(
    FileSystemType fs_type,
    std::size_t repeats,
    std::size_t operations,
    int depth,
    int width,
    double fill_factor,
    const benchmark::ProbabilityProfile& profile
) {
    Benchmark benchmark;
    std::vector<CsvRow> rows;
    rows.reserve(benchmark::d_profiles.size());

    std::vector<OperationType> op_types = generate_operations(
        operations,
        profile.op_read,
        profile.op_write,
        profile.op_mkdir,
        profile.op_ls,
        profile.op_mv,
        profile.op_find
    );

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

        rows.push_back(CsvRow{
            .cfg = cfg,
            .profile_name = profile.name,
            .metrics = benchmark.OverallRun(cfg, op_types)
        });
    }

    return rows;
}

} // namespace

int main(int argc, char** argv) {
    try {
        if (argc != 5) {
            PrintUsage(argv[0]);
            return EXIT_FAILURE;
        }

        const FileSystemType fs_type = ParseFileSystemType(argv[1]);
        const std::size_t repeats = ParseSize(argv[2], "Repeats");
        const std::size_t operations = ParseSize(argv[3], "Ops");
        const std::string output_csv = argv[4];

        ValidateArguments(repeats, operations);

        std::vector<CsvRow> rows;
        rows.reserve(
            3U
            * 3U
            * 2U
            * benchmark::profiles.size()
            * benchmark::d_profiles.size()
        );

        for (int depth : {2, 5, 10}) {
            for (int width : {10, 30, 50}) {
                for (double fill_factor : {0.3, 0.6, 0.9}) {
                    // std::vector<std::thread> threads;
                    // threads.reserve(benchmark::profiles.size());
                    std::vector<std::vector<CsvRow>> profile_rows(
                        benchmark::profiles.size()
                    );

                    for (std::size_t profile_idx = 0;
                         profile_idx < benchmark::profiles.size();
                         ++profile_idx) {
                                profile_rows[profile_idx] = RunProfileConfigs(
                                    fs_type,
                                    repeats,
                                    operations,
                                    depth,
                                    width,
                                    fill_factor,
                                    std::cref(benchmark::profiles[profile_idx])
                                );
                    }

                    // for (std::thread& thread : threads) {
                    //     thread.join();
                    // }

                    for (const auto& profile_result : profile_rows) {
                        for (const CsvRow& row : profile_result) {
                            rows.push_back(row);
                            std::cout
                                << "[BM RUN] Suit Profile=" << row.profile_name
                                << " D=" << row.cfg.depth
                                << " W=" << row.cfg.width
                                << " F=" << row.cfg.fill_factor
                                << " finished" << '\n';
                        }
                    }
                }
            }
        }

        const bool need_header = CsvNeedsHeader(output_csv);
        std::ofstream output(output_csv, std::ios::app);
        if (!output) {
            throw std::runtime_error("cannot open output csv: " + output_csv);
        }

        if (need_header) {
            WriteCsvHeader(output);
        }

        for (const CsvRow& row : rows) {
            WriteCsvRow(output, row.cfg, row.profile_name, row.metrics);
        }

        return EXIT_SUCCESS;
    } catch (const std::exception& error) {
        std::cerr << "benchmark runner error: " << error.what() << '\n';
        return EXIT_FAILURE;
    }
}
