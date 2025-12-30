// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - Present Romain Augier
// All rights reserved.

#if defined(STDROMANO_ENABLE_OPENCL)

#include "stdromano/opencl.hpp"
#include "stdromano/logger.hpp"

#define SIZE 1000

int main()
{
    stdromano::log_info("Starting OpenCL test");

    stdromano::OpenCLConfig config;
    config.preferred_vendor = "NVIDIA";
    config.enable_debug_output = true;
    config.max_cache_size = 50;

    auto& manager = stdromano::OpenCLManager::get_instance();

    if(!manager.initialize(config))
    {
        stdromano::log_error("Error during opencl test");
        return 0;
    }

    auto input_buffer = manager.create_buffer<float>(SIZE, CL_MEM_READ_ONLY);
    auto output_buffer = manager.create_buffer<float>(SIZE, CL_MEM_WRITE_ONLY);

    const stdromano::StringD kernel_source(R"(
    __kernel void vector_add(__global const float* a, __global const float* b, __global float* c) {
        int gid = get_global_id(0);
        c[gid] = a[gid] + b[gid];
    }
    )");

    cl::Event event;

    if(!manager.schedule_task(event,
                              kernel_source,
                              "vector_add",
                              [&](cl::Kernel& kernel) {
                                  kernel.setArg(0, input_buffer);
                                  kernel.setArg(1, input_buffer);
                                  kernel.setArg(2, output_buffer);
                              },
                              cl::NDRange(SIZE)))
    {
        stdromano::log_error("Error during opencl test");
        return 0;
    }

    double exec_time = manager.get_execution_time_ms(event);

    stdromano::log_info("Executed opencl kernel in {} ms", exec_time);

    stdromano::log_info("Finished OpenCL test");

    return 0;
}

#else

int main() { return 0; }

#endif /* defined(STDROMANO_ENABLE_OPENCL) */
