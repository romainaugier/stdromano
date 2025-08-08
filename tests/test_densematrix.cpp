// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - Present Romain Augier
// All rights reserved.

#include "stdromano/linalg/dense_matrix.hpp"
#include "stdromano/opencl.hpp"

#define STDROMANO_ENABLE_PROFILING
#include "stdromano/profiling.hpp"

#if STDROMANO_DEBUG
#define NUM_TESTS 1
#else
#define NUM_TESTS 10
#endif /* STDROMANO_DEBUG */

int main() noexcept
{
    stdromano::log_info("Starting dense_matrix test");

    stdromano::OpenCLConfig config;
    config.preferred_vendor = "NVIDIA";
    config.enable_debug_output = true;
    config.max_cache_size = 50;
    config.enable_profiling = true;

    auto& manager = stdromano::OpenCLManager::get_instance();

    if(!manager.initialize(config))
    {
        stdromano::log_error("Error initializing OpenCL");
        return 0;
    }

#if STDROMANO_DEBUG
    std::size_t M = 644, N = 766, K = 512;
#else
    std::size_t M = 1024, N = 1024, K = 1024;
#endif /* defined(STDROMANO_DEBUG) */

    stdromano::DenseMatrixF A(M, K);
    A.fill(1);

    stdromano::DenseMatrixF B(K, N);
    B.fill(2);

    for(std::size_t i = 0; i < NUM_TESTS; i++)
    {
        SCOPED_PROFILE_START(stdromano::ProfileUnit::MilliSeconds, matmat_mul_cpu);
        stdromano::DenseMatrixF C = A * B;
        SCOPED_PROFILE_STOP(matmat_mul_cpu);

#if STDROMANO_DEBUG
        C.debug(10, 10);
#endif /* defined(STDROMANO_DEBUG) */
    }

    stdromano::DenseMatrixF A_gpu = A.to_backend(stdromano::LinAlgBackend_GPU);
    stdromano::DenseMatrixF B_gpu = B.to_backend(stdromano::LinAlgBackend_GPU);

    for(std::size_t i = 0; i < NUM_TESTS; i++)
    {
        SCOPED_PROFILE_START(stdromano::ProfileUnit::MilliSeconds, matmat_mul_gpu);
        stdromano::DenseMatrixF C_gpu = A_gpu * B_gpu;
        SCOPED_PROFILE_STOP(matmat_mul_gpu);

#if STDROMANO_DEBUG
        stdromano::DenseMatrixF C = C_gpu.to_backend(stdromano::LinAlgBackend_CPU);
        C.debug(10, 10);
#endif /* defined(STDROMANO_DEBUG) */
    }

    stdromano::log_info("Finished dense_matrix test");

    return 0;
}