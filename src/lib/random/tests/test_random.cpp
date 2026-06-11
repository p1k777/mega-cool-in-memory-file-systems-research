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
