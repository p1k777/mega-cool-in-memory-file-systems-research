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
    
    filesystem::IFileSystem *ptr;
    if (cfg.fs_type == FileSystemType::A) {
        ptr = new filesystem::TreeFileSystem;
    } else if (cfg.fs_type == FileSystemType::B) {
        ptr = new filesystem::FileSystemB;
    } else {
        ptr = new filesystem::FlatHashFileSystem;
    }
    
    fsgenerator::FsGenerator fs_generator(cfg.depth, cfg.width, cfg.fill_factor);
    fsgenerator::GeneratedFs file_system = fs_generator.generate();
    file_system.fill(fs);

    std::vector <OperationType> op_types = generate_operations(
        cfg.operations,
        cfg.p_read,
        cfg.p_write,
        cfg.p_mkdir,
        cfg.p_ls,
        cfg.p_mv,
        cfg.p_find
    );

    std::vector <filesystem::IFileSystem::path_type> paths;
    pathgen::IPathGenerator* path_gen = nullptr;
    if (cfg.distribution == benchmark::Distribution::Uniform) {
        pathgen::UniformPathGenerator uni_path_gen(file_system, cfg.locality);
        paths = uni_path_gen.generate(op_types);
        path_gen = &uni_path_gen;
    } else {
        pathgen::ZipfPathGenerator zpath_gen(file_system, cfg.zipf_p, cfg.locality);
        paths = zpath_gen.generate(op_types);
        path_gen = &zpath_gen;
    }

    int p_idx = 0;
    std::vector <Operation> ops;
    for (int i = 0; i < cfg.operations; ++i) {
        Operation operation;
        operation.type = op_types[i];
        operation.path = paths[p_idx];
        p_idx++;

        if (op_types[i] == OperationType::Move) {
            operation.second_path = paths[p_idx];
            p_idx++;
        } else if (op_types[i] == OperationType::Find) {
            swap(operation.pattern, operation.path);
        }

        ops.push_back(operation);
    }

    Metrics avg_result;
    for (int i = 0; i < cfg.repeats; ++i) {
        avg_result += SingleRun(*ptr, file_system, cfg, ops);
    }
    avg_result /= cfg.repeats;

    return avg_result;
}

Metrics Benchmark::SingleRun(
        filesystem::IFileSystem &fs,
        fsgenerator::GeneratedFs &generator,
        const ExperimentConfig &cfg,
        const std::vector <Operation> &operations)
{

    generator.fill(fs);
    Metrics result;


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

void Benchmark::WriteMetrics(const Metrics& metrics,  const ExperimentConfig& cfg, const std::string& path) 
{
    std::ofstream file(path + "/metrics.txt");

    // Параметры дерева
    file << "depth " << cfg.depth << '\n';
    file << "width " << cfg.width << '\n';
    file << "fill_factor " << cfg.fill_factor << '\n';

    file << "p_read " << cfg.p_read << '\n';
    file << "p_write " << cfg.p_write << '\n';
    file << "p_mkdir " << cfg.p_mkdir << '\n';
    file << "p_ls " << cfg.p_ls << '\n';
    file << "p_mv " << cfg.p_mv << '\n';
    file << "p_find " << cfg.p_find << '\n';
    
    file << "distribution ";
    if (cfg.distribution == Distribution::Zipf)
        file << "zipf " << cfg.zipf_p;
    else 
        file << "uniform";
    file << '\n';
    
    // Результат работы
    file << "avg_latency_us " << metrics.avg_latency_us << '\n';    
    file << "p99_latency_us " << metrics.p99_latency_us << '\n';    
    file << "throughput_ops_sec " << metrics.throughput_ops_sec << '\n';    
    file << "memory_usage_bytes " << metrics.memory_usage_bytes << '\n';
}


}; // namespace benchmark