// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - Present Romain Augier
// All rights reserved.

#include "stdromano/linalg/dense_matrix.hpp"
#include "stdromano/simd.hpp"

#define STDROMANO_ENABLE_PROFILING
#include "stdromano/profiling.hpp"

int main() noexcept
{
    stdromano::log_info("Starting dense_matrix test");

    std::size_t M = 1492, N = 899, K = 1789;

    stdromano::DenseMatrixF A(M, K);
    A.fill(1);

    stdromano::DenseMatrixF B(K, N);
    B.fill(2);

    SCOPED_PROFILE_START(stdromano::ProfileUnit::MilliSeconds, matmat_mul);
    stdromano::DenseMatrixF C = A * B;
    SCOPED_PROFILE_STOP(matmat_mul);

    // C.debug(10, 10);

    stdromano::log_info("Finished dense_matrix test");

    return 0;
}