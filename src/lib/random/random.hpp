#pragma once

#include <cstddef>
#include <cstdint>
#include <random>
#include <vector>

namespace random {
    
    class Random {

    public:
        
        Random();
        Random(int64_t seed);

        // вущественный x из [0, 1)
        double real() const noexcept;

        // целый x из [l, r]
        int64_t integer(int64_t l, int64_t r) const noexcept;

        // целый индекс из [0, size)
        size_t index(size_t size) const noexcept;

        // true с вероятностью p
        bool probability(double p) const noexcept;

        // выбирает случайный элемент из вектора
        template <typename T>
        T& choice(std::vector<T>&) const noexcept;

    private:

        std::mt19937_64 gen_;

    };

} // namespace random

