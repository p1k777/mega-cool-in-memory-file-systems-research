#include <cstddef>
#include <cstdint>
#include <ctime>
#include <stdexcept>
#include <random>
#include <vector>

#include "random.hpp"

namespace random {
    
    Random::Random(int64_t seed) : gen_(seed) {}
    Random::Random() : Random(std::time(nullptr)) {}

    double Random::real() const noexcept {
        return std::uniform_real_distribution<double>(0, 1)(gen_);
    }

    int64_t Random::integer(int64_t l, int64_t r) const noexcept {
        if (l > r) { std::invalid_argument("l > r must be false"); }
        return std::uniform_int_distribution<int64_t>(l, r)(gen_);
    }

    size_t Random::index(size_t size) const noexcept {
        if (size == 0) { std::invalid_argument("size must be positive"); }
        return integer(0, size-1);
    }

    bool Random::probability(double p) const noexcept {
        return real() < p;
    }

    template <typename T>
    T& Random::choice(std::vector<T>& vct) const noexcept {
        if (vct.empty()) { std::invalid_argument("vct must not be empty"); }
        return vct[index(vct.size())];
    }

} // namespace random

