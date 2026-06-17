#include "operation_generator.hpp"

#include <cmath>
#include <random>
#include <stdexcept>

namespace {

void validate_probabilities(
    double p_read,
    double p_write,
    double p_mkdir,
    double p_ls,
    double p_move,
    double p_find
) {
    if(p_read < 0.0 ||
       p_write < 0.0 ||
       p_mkdir < 0.0 ||
       p_ls < 0.0 ||
       p_move < 0.0 ||
       p_find < 0.0) {
        throw std::runtime_error("operation probability is negative");
    }

    double sum = p_read + p_write + p_mkdir + p_ls + p_move + p_find;

    if(std::abs(sum - 1.0) > 1e-9) {
        throw std::runtime_error("operation probabilities sum is not 1");
    }
}

OperationType operation_from_index(size_t index) {
    switch(index) {
        case 0:
            return OperationType::Read;
        case 1:
            return OperationType::Write;
        case 2:
            return OperationType::Mkdir;
        case 3:
            return OperationType::Ls;
        case 4:
            return OperationType::Move;
        case 5:
            return OperationType::Find;
        default:
            throw std::runtime_error("invalid operation index");
    }
}

} // namespace

std::vector<OperationType> generate_operations(
    size_t count,
    double p_read,
    double p_write,
    double p_mkdir,
    double p_ls,
    double p_move,
    double p_find
) {
    validate_probabilities(p_read, p_write, p_mkdir, p_ls, p_move, p_find);

    std::random_device random_device;
    std::mt19937 generator(random_device());

    std::discrete_distribution<size_t> distribution({p_read, p_write, p_mkdir, p_ls, p_move, p_find});

    std::vector<OperationType> operations;
    operations.reserve(count);

    for(size_t i = 0; i < count; ++i) {
        size_t index = distribution(generator);
        operations.push_back(operation_from_index(index));
    }

    return operations;
}