#include "operation_generator.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <stdexcept>
#include <vector>

namespace {

bool all_are(const std::vector<OperationType>& operations, OperationType expected) {
    return std::all_of(operations.begin(), operations.end(),
        [expected](OperationType operation) {
            return operation == expected;
        }
    );
}

} // namespace

TEST(OperationGenerator, ReturnsRequestedCount) {
    auto operations = generate_operations(
        100,
        0.4,
        0.2,
        0.1,
        0.1,
        0.1,
        0.1
    );
    EXPECT_EQ(operations.size(), 100);
}

TEST(OperationGenerator, ReturnsEmptyVectorForZeroCount) {
    auto operations = generate_operations(
        0,
        0.4,
        0.2,
        0.1,
        0.1,
        0.1,
        0.1
    );
    EXPECT_TRUE(operations.empty());
}

TEST(OperationGenerator, GeneratesOnlyReadWhenReadProbabilityIsOne) {
    EXPECT_TRUE(all_are(generate_operations(20, 1.0, 0.0, 0.0, 0.0, 0.0, 0.0),OperationType::Read));
}

TEST(OperationGenerator, GeneratesOnlyWriteWhenWriteProbabilityIsOne) {
    EXPECT_TRUE(all_are(
        generate_operations(20, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0),
        OperationType::Write
    ));
}


TEST(OperationGenerator, ThrowsOnNegativeProbability) {
    EXPECT_THROW(
        generate_operations(10, -0.1, 0.2, 0.2, 0.2, 0.2, 0.3),
        std::runtime_error
    );
}

TEST(OperationGenerator, ThrowsWhenProbabilitiesSumIsNotOne) {
    EXPECT_THROW(
        generate_operations(10, 0.4, 0.2, 0.1, 0.1, 0.1, 0.2),
        std::runtime_error
    );
}

TEST(OperationGenerator, ThrowsWhenAllProbabilitiesAreZero) {
    EXPECT_THROW(
        generate_operations(10, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0),
        std::runtime_error
    );
}