// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - Present Romain Augier
// All rights reserved.

#include "stdromano/linalg/dense_matrix.hpp"
#include "stdromano/simd.hpp"

#define STDROMANO_ENABLE_PROFILING
#include "stdromano/profiling.hpp"

#define NUM_TESTS 10

int main() noexcept
{
    stdromano::log_info("Starting dense_matrix test");

#if STDROMANO_DEBUG
    std::size_t M = 644, N = 766, K = 512;
#else
    std::size_t M = 2048, N = 2048, K = 2048;
#endif /* defined(STDROMANO_DEBUG) */

    stdromano::DenseMatrixF A(M, K);
    A.fill(1);

    stdromano::DenseMatrixF B(K, N);
    B.fill(2);

    for(std::size_t i = 0; i < NUM_TESTS; ++i)
    {
        SCOPED_PROFILE_START(stdromano::ProfileUnit::MilliSeconds, matmat_mul);
        stdromano::DenseMatrixF C = A * B;
        SCOPED_PROFILE_STOP(matmat_mul);
    }

    // C.debug(10, 10);

    stdromano::log_info("Finished dense_matrix test");

    return 0;
}