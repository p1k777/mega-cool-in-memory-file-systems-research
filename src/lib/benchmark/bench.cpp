#pragma once

#include "bench.hpp"

namespace benchmark {

inline auto Now() {
    return std::chrono::high_resolution_clock::now();
}


Metrics Benchmark::OverallRun(
        filesystem::IFileSystem &fs, 
        const ExperimentConfig &cfg)
{

    fsgenerator::FsGenerator fs(cfg.depth,
                                cfg.width,
                                cfg.fill_factor);
    

    Metrics avg_result;
    for (int i = 0; i < cfg.repeats; ++i) {
        avg_result += SingleRun(fs, cfg);
    }
    avg_result /= cfg.repeats;

    return avg_result;
}

Metrics Benchmark::SingleRun(
        filesystem::IFileSystem &fs, 
        const ExperimentConfig &cfg)
{
    // FsGenerator(size_t D, size_t W, double F, double file_p=0.5, size_t max_nodes_count=1e7);
    // ну и тут кароче строим дерево файловой системы то сё
    fsgenerator::FsGenerator fs_generator(cfg.depth, cfg.width, cfg.fill_factor);
    fsgenerator::GeneratedFs file_system = fs_generator.generate();


    Metrics result;
    
    std::vector <OperationType> op_types = generate_operations(
        cfg.operations,
        cfg.p_read,
        cfg.p_write,
        cfg.p_mkdir,
        cfg.p_ls,
        cfg.p_mv,
        cfg.p_find
    );

    pathgen::UniformPathGenerator uni_path_gen(file_system, cfg.locality);
    std::vector <filesystem::IFileSystem::path_type> uni_paths = uni_path_gen.generate(op_types);
    std::vector <filesystem::IFileSystem::path_type> uni_paths_2 = uni_path_gen.generate(op_types);


    std::vector <Operation> operations;
    for (int i = 0; i < cfg.operations; ++i) {
        Operation tmp = Operation{
            .type = op_types[i],
            .path = uni_paths[i],
            .second_path = uni_paths_2[i]
        };

        operations.push_back(tmp);
    }


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
    result.memory_usage_bytes = fs.get_memory_usage();

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
    std::vector<double> tmp = v;
    std::sort(tmp.begin(), tmp.end());
    size_t idx = static_cast<size_t>(0.99 * v.size());
    return v[idx];
}

double Benchmark::ThroughputCalc(size_t op_num, double total_time)
{
    double time_sec = total_time / 1'000'000;
    return (op_num / time_sec);
}


}; // namespace benchmark