#include <gtest/gtest.h>
#include <cstdint>
#include <vector>

#include "random.hpp"

using namespace rnd;

static constexpr int kTestCount = 1e5;

Random rnd_def, rnd_det1(777), rnd_det2(777);

TEST(TestRandom, RealRange) {
    for (int i = 0; i < kTestCount; i++) {
        double t = rnd_def.real();
        EXPECT_FALSE(0 > t || 1 <= t);
    }
}

TEST(TestRandom, RealData) {
    for (int i = 0; i < kTestCount; i++) {
        EXPECT_FLOAT_EQ(rnd_det1.real(), rnd_det2.real());
    }
}

TEST(TestRandom, IntegerRange) {
    for (int i = 0; i < kTestCount; i++) {
        int64_t l{-i}, r{2*i};
        int64_t t = rnd_def.integer(l, r);
        EXPECT_FALSE(l > t || r < t);
    }
}

TEST(TestRandom, IntegerData) {
    for (int i = 0; i < kTestCount; i++) {
        int64_t l{-i}, r{2*i};
        EXPECT_EQ(rnd_det1.integer(l, r), rnd_det2.integer(l, r));
    }
}

TEST(TestRandom, IndexRange) {
    for (int i = 0; i < kTestCount; i++) {
        int64_t t = rnd_def.index(kTestCount);
        EXPECT_FALSE(0 > t || kTestCount <= t);
    }
}

TEST(TestRandom, IndexData) {
    for (int i = 0; i < kTestCount; i++) {
        EXPECT_EQ(rnd_det1.index(kTestCount), rnd_det2.index(kTestCount));
    }
}

TEST(TestRandom, ProbabilityData) {
    for (int i = 0; i < kTestCount; i++) {
        EXPECT_EQ(rnd_det1.probability(0.777), rnd_det2.probability(0.777));
    }
}

TEST(TestRandom, MatchesRawGeneratorSequence) {
    const uint64_t SEED = 12345;
    
    std::mt19937_64 raw_gen(SEED);
    std::uniform_int_distribution<int> raw_dist(1, 100);

    Random rnd(SEED);
    std::uniform_int_distribution<int> my_dist(1, 100);

    for (int i = 0; i < 10000; ++i) {
        EXPECT_EQ(raw_dist(raw_gen), rnd.generate(my_dist)) 
            << "Расхождение последовательности на шаге " << i;
    }
}

TEST(RandomTest, PreservesDistributionInternalCache) {
    Random rnd(42);
    std::normal_distribution<double> dist(0.0, 1.0);
    
    double first = rnd.generate(dist);
    double second = rnd.generate(dist);

    EXPECT_NE(first, second);
}

TEST(RandomTest, RespectsBoundaries) {
    Random rnd(999);
    const int min_val = 10;
    const int max_val = 20;
    std::uniform_int_distribution<int> dist(min_val, max_val);

    for (int i = 0; i < 1000; ++i) {
        int val = rnd.generate(dist);
        EXPECT_GE(val, min_val);
        EXPECT_LE(val, max_val);
    }
}

TEST(RandomTest, CorrectReturnTypes) {
    Random rnd(42);
    
    std::uniform_int_distribution<int> i_dist(1, 10);
    std::uniform_real_distribution<double> d_dist(0.0, 1.0);
    std::discrete_distribution<size_t> s_dist({0.5, 0.5});

    EXPECT_TRUE((std::is_same_v<decltype(rnd.generate(i_dist)), int>));
    EXPECT_TRUE((std::is_same_v<decltype(rnd.generate(d_dist)), double>));
    EXPECT_TRUE((std::is_same_v<decltype(rnd.generate(s_dist)), size_t>));
}
