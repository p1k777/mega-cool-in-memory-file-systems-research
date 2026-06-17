#pragma once

#include <algorithm>
#include <cmath>
#include <concepts>
#include <cstddef>
#include <functional>
#include <random>
#include <stdexcept>

namespace pathgen {

    template <std::integral int_ = size_t>
    class ZipfDistribution {
    public:
        
        ZipfDistribution(int_ l, int_ r, double s) : l_(l), r_(r), u_distr_(0, 1) {
            if (r < l) { throw std::invalid_argument("r must be >= l"); }
            if (s <= 0) { throw std::invalid_argument("s must be > 0"); }

            int_ n = static_cast<int_>(r - l + 1);
            if (abs(s - 1) < 1e10) {
                approx_ = [n](double u) -> double {return std::pow(n, u) - 1; };
            } else {
                approx_ = [s, n](double u) -> double {
                    return std::pow(
                        1.0 + u * (std::pow(n, 1.0 - s) - 1.0),
                        1.0 / (1.0 - s)
                    ) - 1.0;
                };
            }
        }

        template <typename Generator>
        requires requires(Generator& g) { {g()} -> std::convertible_to<size_t>; }
        int_ operator()(Generator& g) {
            int_ offset = static_cast<int_>(std::floor(approx_(u_distr_(g))));
            return std::min<int_>(l_ + offset, r_);
        }
    
    private:

        int_ l_, r_;
        std::uniform_real_distribution<double> u_distr_;
        std::function<double(double)> approx_;

    };

};