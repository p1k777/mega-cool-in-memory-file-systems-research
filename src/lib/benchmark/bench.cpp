#include "bench.hpp"
#include <iostream>

namespace benchmark {

inline auto Now() {
    return std::chrono::high_resolution_clock::now();
}

template <typename FileSystem>
Metrics Benchmark::SingleRun(
        const FileSystem& base_fs,
        const ExperimentConfig& cfg,
        const std::vector<Operation>& operations)
{
    FileSystem fs = base_fs;
    Metrics result{};

    std::vector<double> latencies;
    double total_time = 0.0;

    for (std::size_t op_idx = 0; op_idx < cfg.operations; ++op_idx) {
        // std::cout << op_idx << " ";
        auto opBegin = Now();

        executeOperation(fs, operations[op_idx]);

        auto opEnd = Now();
        double opTime =
            std::chrono::duration<double, std::micro>(opEnd - opBegin).count();

        latencies.push_back(opTime);
        total_time += opTime;
    }

    result.avg_latency_us = AvgLatency(latencies);
    result.p99_latency_us = P99Calc(latencies);
    result.throughput_ops_sec = ThroughputCalc(cfg.operations, total_time);
    result.memory_usage_bytes = fs.get_memory_usage();

    return result;
}


Metrics Benchmark::OverallRun(
        const ExperimentConfig &cfg, const std::vector<OperationType>& op_types)
{
    fsgenerator::FsGenerator fs_generator(cfg.depth, cfg.width, cfg.fill_factor);
    fsgenerator::GeneratedFs file_system = fs_generator.generate();

    std::vector <filesystem::IFileSystem::path_type> paths;
    if (cfg.distribution == benchmark::Distribution::Uniform) {
        pathgen::UniformPathGenerator uni_path_gen(file_system, cfg.locality);
        paths = uni_path_gen.generate(op_types);
    } else {
        pathgen::ZipfPathGenerator zpath_gen(file_system, cfg.zipf_p, cfg.locality);
        paths = zpath_gen.generate(op_types);
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
        }

        ops.push_back(operation);
    }

    switch (cfg.fs_type) {
        case FileSystemType::A: {
            filesystem::TreeFileSystem base_fs;
            file_system.fill(base_fs);

            Metrics avg_result{};
            for (std::size_t repeat = 0; repeat < cfg.repeats; ++repeat) {
                avg_result += SingleRun(base_fs, cfg, ops);
            }
            avg_result /= static_cast<int>(cfg.repeats);
            return avg_result;
        }
        case FileSystemType::B: {
            filesystem::FileSystemB base_fs;
            file_system.fill(base_fs);

            Metrics avg_result{};
            for (std::size_t repeat = 0; repeat < cfg.repeats; ++repeat) {
                avg_result += SingleRun(base_fs, cfg, ops);
            }
            avg_result /= static_cast<int>(cfg.repeats);
            return avg_result;
        }
        case FileSystemType::C: {
            filesystem::FlatHashFileSystem base_fs;
            file_system.fill(base_fs);

            Metrics avg_result{};
            for (std::size_t repeat = 0; repeat < cfg.repeats; ++repeat) {
                avg_result += SingleRun(base_fs, cfg, ops);
            }
            avg_result /= static_cast<int>(cfg.repeats);
            return avg_result;
        }
    }

    throw std::invalid_argument("unknown filesystem type");
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
            filesystem::IFileSystem::bytes_type some_memory(100, 'a');
            fs.op_write(op.path, some_memory);

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
    auto i = std::max_element(v.begin(), v.end());
    return *i;
}

double Benchmark::ThroughputCalc(size_t op_num, double total_time)
{
    double time_sec = total_time / 1'000'000;
    return (op_num / time_sec);
}

void Benchmark::GenerateDataset(filesystem::IFileSystem& fs)
{ }
    // int16_t D_profile[7] = {2, 3, 5, 10, 15, 20};
    // int16_t W_profile[7] = {10, 30, 50, 100, 500};
    // double F_profile[4] = {0.3, 0.6, 0.95};
    
    // rnd::Random random;
    // std::ofstream csv("metrics.csv");

    // csv <<
    //     "D,W,F,"
    //     "profile,"
    //     "system_t,"
    //     "dist,"
    //     "zipf_s,"
    //     "locality,"
    //     "op_read,"
    //     "op_write,"
    //     "op_mkdir,"
    //     "op_ls,"
    //     "op_mv,"
    //     "op_find,"
    //     "avg_latency,"
    //     "p99_latency,"
    //     "throughput,"
    //     "memory\n";

    // for (int d_idx = 0; d_idx < 6; ++d_idx)
    // for (int w_idx = 0; w_idx < 5; ++w_idx)
    // for (int f_idx = 0; f_idx < 3; ++f_idx)

    //     // filesystem type
    //     for (int s = 0; s < 3; s++) {

    //     // distribution profiles
    //     for (int k = 0; k < 3; k++) {

    //     // probability profiles
    //     for (int j = 0; j < 6; j++) {
    //         ExperimentConfig cfg;

    //         cfg.depth = D_profile[d_idx];
    //         cfg.width = W_profile[w_idx];
    //         cfg.fill_factor = F_profile[f_idx];

    //         cfg.operations = 100;
    //         cfg.repeats = 5;

    //         cfg.p_read  = profiles[j].op_read;
    //         cfg.p_write = profiles[j].op_write;
    //         cfg.p_mkdir = profiles[j].op_mkdir;
    //         cfg.p_ls    = profiles[j].op_ls;
    //         cfg.p_mv    = profiles[j].op_mv;
    //         cfg.p_find  = profiles[j].op_find;

    //         cfg.distribution = d_profiles[k].dist;
    //         cfg.locality = d_profiles[k].locality;
    //         cfg.zipf_p = d_profiles[k].zipf_s;

    //         if (s == 0)
    //             cfg.fs_type = FileSystemType::A;
    //         else if (s == 1)
    //             cfg.fs_type = FileSystemType::B;
    //         else 
    //             cfg.fs_type = FileSystemType::C;

    //         Metrics m = OverallRun(cfg);


    //         csv
    //             << cfg.depth << ','
    //             << cfg.width << ','
    //             << cfg.fill_factor << ','

    //             << profiles[j].name << ','

    //             << char('A' + s) << ','

    //             << (cfg.distribution == Distribution::Uniform
    //                     ? "uniform"
    //                     : "zipf")
    //             << ','

    //             << cfg.zipf_p << ','
    //             << cfg.locality << ','

    //             << cfg.p_read << ','
    //             << cfg.p_write << ','
    //             << cfg.p_mkdir << ','
    //             << cfg.p_ls << ','
    //             << cfg.p_mv << ','
    //             << cfg.p_find << ','

    //             << m.avg_latency_us << ','
    //             << m.p99_latency_us << ','
    //             << m.throughput_ops_sec << ','
    //             << m.memory_usage_bytes

    //             << '\n';
    //     }
        
    //     }
// }

}; // namespace benchmark
