#pragma once

#include "bench.hpp"

namespace benchmark {

inline auto Now() {
    return std::chrono::high_resolution_clock::now();
}


Metrics Benchmark::run(
        filesystem::IFileSystem &fs, 
        const ExperimentConfig &cfg)
{
    // FsGenerator(size_t D, size_t W, double F, double file_p=0.5, size_t max_nodes_count=1e7);
    // ну и тут кароче строим дерево файловой системы то сё
    fsgenerator::FsGenerator fs(cfg.depth,
                                cfg.width,
                                cfg.fill_factor);
    
    Metrics result;
    
    // пусть есть операции
    std::vector <Operation> operations(100);
    std::vector <double> latencies;
    double total_time = 0.0;

    for (int i = 0; i < cfg.operations; ++i) {
        auto opBegin = Now();

        executeOperation(fs, operations[i]);

        auto opEnd = Now();
        double opTime = std::chrono::duration<double, std::micro> (opEnd - opBegin).count();
        
        latencies.push_back(opTime);
        total_time += opTime;
    }

    result.avg_latency_us = AvgLatency(latencies);
    result.p99_latency_us = P99Calc(latencies);
    result.throughput_ops_sec = ThroughputCalc(cfg.operations, total_time);

    return result;
}

void Benchmark::executeOperation(
        filesystem::IFileSystem &fs, 
        const Operation &op) 
{

    switch(op.type) {

        case OperationType::Find:
            fs.op_find(op.path, op.pattern);
            break;
        
        case OperationType::Ls:
            fs.op_ls(op.path);

            break;

        case OperationType::Mkdir:
            fs.op_mkdir(op.path);

            break;

        case OperationType::Move:
            fs.op_mv(op.path, op.second_path);

            break;

        case OperationType::Read:
            fs.op_read(op.path);

            break;

        case OperationType::Write:
            filesystem::IFileSystem::bytes_type some_bullshit;
            fs.op_write(op.path, some_bullshit);

            break;


    }

}

double Benchmark::AvgLatency(const std::vector<double> &v)
{
    double sum = 0.0;
    for (auto d : v) {
        sum += d;
    }
    
    return sum / v.size();
}

double Benchmark::P99Calc(const std::vector<double> &v)
{
    sort(v.begin(), v.end());
    size_t idx = static_cast<size_t>(0.99 * v.size());
    return v[idx];
}

double Benchmark::ThroughputCalc(size_t op_num, double total_time)
{
    return (op_num / total_time);
}


}; // namespace benchmark