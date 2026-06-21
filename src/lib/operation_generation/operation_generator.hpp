#pragma once

#include "common.hpp"

#include <cstddef>
#include <vector>

std::vector<OperationType> generate_operations(
    size_t count,
    double p_read,
    double p_write,
    double p_mkdir,
    double p_ls,
    double p_move,
    double p_find
);