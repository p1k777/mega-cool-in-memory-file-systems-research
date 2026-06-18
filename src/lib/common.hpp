#pragma once

#include <cstddef>

enum class OperationType {
    Read,
    Write,
    Mkdir,
    Ls,
    Move,
    Find
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