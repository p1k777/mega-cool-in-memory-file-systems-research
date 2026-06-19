#pragma once

#include <string>
#include <chrono>
#include <algorithm>
#include <fstream>

#include "../common.hpp"
#include "../fs_generator/fs_generator.hpp"
#include "../fs_generator/generated_fs.hpp"
#include "../path_generator/path_generator.hpp"
#include "../operation_generation/operation_generator.hpp"
#include "../A_fs/A_fs.hpp"
#include "../filesystem_b/filesystem_b.hpp"
#include "../C_fs/C_fs.hpp"

namespace benchmark {

struct Operation {
    OperationType type;

    std::string path;
    std::string second_path;
    std::string pattern = "*"; // для поиска 
};

struct ProbabilityProfile {
    std::string name;

    double op_read;
    double op_write;
    double op_mkdir;
    double op_ls;
    double op_mv;
    double op_find;
};

enum class Distribution
{
    Uniform,
    Zipf
};

struct DistributionProfile {
    Distribution dist;
    double locality;
    double zipf_s;
};

enum class FileSystemType 
{
    A,
    B,
    C
};

struct ExperimentConfig
{
    int depth = 0;
    int width = 0;

    double fill_factor = 0.0;

    double p_read = 0.0;
    double p_write = 0.0;
    double p_mkdir = 0.0;
    double p_ls = 0.0;
    double p_mv = 0.0;
    double p_find = 0.0;

    Distribution distribution =
        Distribution::Uniform;

    double locality = 0.0;
    double zipf_p = 0.0;

    size_t operations = 0;
    size_t repeats = 1;

    FileSystemType fs_type =
        FileSystemType::A;
};

const std::vector<ProbabilityProfile> profiles {
    ProbabilityProfile{"build_system", 0.50, 0.25, 0.10, 0.05, 0.05, 0.05},
    ProbabilityProfile{"file_manager", 0.20, 0.05, 0.45, 0.10, 0.10, 0.10},
    // ProbabilityProfile{"backup", 0.1, 0.7, 0.0, 0.0, 0.2, 0.0,},
    // ProbabilityProfile{"refactoring", 0.1, 0.1, 0.1, 0.1, 0.5, 0.1,},
    ProbabilityProfile{"database", 0.55, 0.45, 0.00, 0.00, 0.00, 0.00},
    ProbabilityProfile{"web_server", 0.8, 0.1, 0.0, 0.1, 0.0, 0.0}
};

const std::vector<DistributionProfile> d_profiles {
    DistributionProfile{Distribution::Uniform, 0.0, 0.0},
    DistributionProfile{Distribution::Zipf, 0.3, 1.5},
    DistributionProfile{Distribution::Zipf, 0.8, 2.0}
};


class Benchmark {

public:

    Benchmark() = default;
    ~Benchmark() = default;


    Metrics OverallRun(const ExperimentConfig& cfg, const std::vector<OperationType>& op_types);
    template <typename FileSystem>
    Metrics SingleRun(
        const FileSystem& base_fs,
        const ExperimentConfig& cfg,
        const std::vector<Operation>& operations
    );
  
    void GenerateDataset(filesystem::IFileSystem& fs);

private:

    void executeOperation(
        filesystem::IFileSystem& fs,
        const Operation& op
    );

    // вспомогательные функции для вычислений
    double AvgLatency(const std::vector <double> &v);
    double P99Calc(const std::vector <double> &v);
    double ThroughputCalc(size_t op_num, double total_time);

};


}; // namespace benchmark
