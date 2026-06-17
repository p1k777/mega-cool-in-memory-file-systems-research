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
        std::uniform_real_distribution<double> d(0, 1);
        return generate(d);
    }

    int64_t Random::integer(int64_t l, int64_t r) {
        if (l > r) { std::invalid_argument("l > r must be false"); }
        std::uniform_int_distribution<int64_t> d(l, r);
        return generate(d);
    }

    size_t Random::index(size_t size) {
        if (size == 0) { std::invalid_argument("size must be positive"); }
        std::uniform_int_distribution<size_t> d(0, size-1);
        return generate(d);
    }

    bool Random::probability(double p) {
        return real() < p;
    }

} // namespace rnd

