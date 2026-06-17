#pragma once

#include <cstddef>
#include <cstdint>
#include <random>
#include <vector>

namespace rnd {
    
    class Random {

    public:
        
        Random();
        Random(int64_t seed);

        // вещественный x из [0, 1)
        double real();

        // целый x из [l, r]
        int64_t integer(int64_t l, int64_t r);

        // целый индекс из [0, size)
        size_t index(size_t size);

        // true с вероятностью p
        bool probability(double p);

        // выбирает случайный элемент из вектора
        template <typename T>
        T& choice(std::vector<T>& vct) {
            if (vct.empty()) { std::invalid_argument("vct must not be empty"); }
            return vct[index(vct.size())];
        }

        template <typename Distribution>
        requires requires(Distribution& d, std::mt19937_64& g) { { d(g) }; }
        auto generate(Distribution& d) {
            return d(gen_);
        }

    private:

        std::mt19937_64 gen_;

    };

} // namespace rnd

