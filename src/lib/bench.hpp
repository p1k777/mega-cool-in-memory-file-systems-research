#pragma once

#include <string>
#include "filesystem.hpp"
#include "fs_generator.hpp"

namespace benchmark {

enum class OperationType {
    Read,
    Write,
    Mkdir,
    Ls,
    Move,
    Find
};

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
    double latency_stddev_us;      
};


enum class Distribution
{
    Uniform,
    Zipf15,
    Zipf20
};

struct ExperimentConfig
{
    // дерево

    int depth;
    int width;
    double fill_factor;

    // probability jgthfwbazqqfwd;mgwf;gvm

    double p_read;
    double p_write;
    double p_mkdir;
    double p_ls;
    double p_mv;
    double p_find;

    // параметры доступа 

    Distribution distribution;

    double locality;

    // параметры эксперимента

    size_t operations;

    size_t repeats;
};



class Benchmark {

public:



    Metrics run(
        filesystem::IFileSystem& fs,
        const ExperimentConfig& cfg);
    
    

};


}; // namespace benchmark