#include <cstddef>
#include <cstdint>
#include <ctime>
#include <stdexcept>
#include <random>
#include <vector>

#include "random.hpp"

namespace rnd {
    
    Random::Random(int64_t seed) : gen_(seed) {}
    Random::Random() : Random(std::time(nullptr)) {}

    double Random::real() {
        return std::uniform_real_distribution<double>(0, 1)(gen_);
    }

    int64_t Random::integer(int64_t l, int64_t r) {
        if (l > r) { std::invalid_argument("l > r must be false"); }
        return std::uniform_int_distribution<int64_t>(l, r)(gen_);
    }

    size_t Random::index(size_t size) {
        if (size == 0) { std::invalid_argument("size must be positive"); }
        return std::uniform_int_distribution<size_t>(0, size-1)(gen_);
    }

    bool Random::probability(double p) {
        return real() < p;
    }

} // namespace rnd

