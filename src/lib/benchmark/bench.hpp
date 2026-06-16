#pragma once

#include <string>
#include <chrono>
#include <algorithm>

#include "filesystem.hpp"
#include "../common.hpp"

namespace benchmark {

enum class OperationType {
    Read,    // файл
    Write,   // файл
    Mkdir,   // папка
    Ls,      // папка
    Move,    // файл Х2
    Find     // похуй похуй
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

    Benchmark() = default;
    ~Benchmark() = default;


    Metrics run(
        filesystem::IFileSystem& fs,
        const ExperimentConfig& cfg
    );
    
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


/*
if(random() < locality)
{
    return randomPathInsideSubtree(
        lastPath
    );
}
else
{
    return randomPathGlobal();
}
*/