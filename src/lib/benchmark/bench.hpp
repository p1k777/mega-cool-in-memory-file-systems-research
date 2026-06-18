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
    std::string pattern; // для поиска 
};


struct Metrics {
    double avg_latency_us;         // средняя задержка 
    double p99_latency_us;         // процентиль скорости операций
    double throughput_ops_sec;     // операций в секунду
    size_t memory_usage_bytes;

    Metrics& operator+=(const Metrics& other) {
        avg_latency_us += other.avg_latency_us;
        p99_latency_us += other.p99_latency_us;
        throughput_ops_sec += other.throughput_ops_sec;
        memory_usage_bytes += other.memory_usage_bytes;

        return *this;
    }

    Metrics& operator/=(const int& div) {
        avg_latency_us /= div;
        p99_latency_us /= div;
        throughput_ops_sec /= div;
        memory_usage_bytes /= div;

        return *this;
    }
};


enum class Distribution
{
    Uniform,
    Zipf
};

enum class FileSystemType 
{
    A,
    B,
    C
};

struct ExperimentConfig
{
    // дерево

    int depth;
    int width;
    double fill_factor;

    // probability 

    double p_read;
    double p_write;
    double p_mkdir;
    double p_ls;
    double p_mv;
    double p_find;

    // параметры доступа 

    Distribution distribution;
    double locality;
    double zipf_p;

    // параметры эксперимента

    size_t operations;
    size_t repeats;
    FileSystemType fs_type;
};



class Benchmark {

public:

    Benchmark() = default;
    ~Benchmark() = default;


    Metrics OverallRun(filesystem::IFileSystem& fs, const ExperimentConfig& cfg);
    Metrics SingleRun(filesystem::IFileSystem& fs, fsgenerator::GeneratedFs&, const ExperimentConfig& cfg, const std::vector <Operation> &);
  
    void WriteMetrics(const Metrics&, const ExperimentConfig&, const std::string&);

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