// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - Present Romain Augier
// All rights reserved.

#include "stdromano/opencl.hpp"

#include "fixtures.hpp"

#include <vector>

using namespace stdromano;

static const char* VECTOR_ADD_SOURCE = R"(
__kernel void vector_add(__global const float* a, __global const float* b, __global float* c, uint n)
{
    const uint gid = get_global_id(0);

    if(gid < n)
        c[gid] = a[gid] + b[gid];
}
)";

static bool initialize_any_device()
{
    OpenCLConfig config;
    config.device_type = CL_DEVICE_TYPE_ALL;
    config.prefer_dedicated_gpu = false;

    OpenCLManager& manager = OpenCLManager::get_instance();

    return manager.is_initialized() || manager.initialize(config);
}

static bool vector_add(const std::vector<float>& a, const std::vector<float>& b, std::vector<float>& c)
{
    OpenCLManager& manager = OpenCLManager::get_instance();

    const std::size_t count = a.size();

    cl::Buffer a_buffer = manager.create_buffer<float>(count, CL_MEM_READ_ONLY);
    cl::Buffer b_buffer = manager.create_buffer<float>(count, CL_MEM_READ_ONLY);
    cl::Buffer c_buffer = manager.create_buffer<float>(count, CL_MEM_WRITE_ONLY);

    manager.write_buffer(a_buffer, a.data(), count);
    manager.write_buffer(b_buffer, b.data(), count);

    const std::size_t global_size = ((count + 63) / 64) * 64;

    cl::Event event;

    if(!manager.schedule_task(event,
                              StringD(VECTOR_ADD_SOURCE),
                              "vector_add",
                              [&](cl::Kernel& kernel) {
                                  kernel.setArg(0, a_buffer);
                                  kernel.setArg(1, b_buffer);
                                  kernel.setArg(2, c_buffer);
                                  kernel.setArg(3, static_cast<cl_uint>(count));
                              },
                              cl::NDRange(global_size)))
        return false;

    event.wait();

    c.resize(count);

    return manager.read_buffer(c_buffer, c.data(), count).has_value();
}

STDROMANO_TEST_CASE(initializes_a_device)
{
    if(!initialize_any_device())
        return;

    OpenCLManager& manager = OpenCLManager::get_instance();

    STDROMANO_CHECK(manager.is_initialized());
    STDROMANO_CHECK_GT(manager.get_num_devices(), 0u);
    STDROMANO_CHECK(manager.get_context() != nullptr);
    STDROMANO_CHECK(manager.get_queue(0) != nullptr);
}

STDROMANO_TEST_CASE(vector_add_kernel)
{
    if(!initialize_any_device())
        return;

    std::vector<float> a(1000);
    std::vector<float> b(1000);
    std::vector<float> c;

    for(std::size_t i = 0; i < a.size(); ++i)
    {
        a[i] = static_cast<float>(i);
        b[i] = static_cast<float>(2 * i);
    }

    STDROMANO_REQUIRE(vector_add(a, b, c));

    for(std::size_t i = 0; i < c.size(); ++i)
        STDROMANO_REQUIRE_EQ(c[i], static_cast<float>(3 * i));

    STDROMANO_CHECK_GT(OpenCLManager::get_instance().get_cache_size(), 0u);
}

STDROMANO_TEST_CASE(copy_buffer)
{
    if(!initialize_any_device())
        return;

    OpenCLManager& manager = OpenCLManager::get_instance();

    const std::vector<std::int32_t> source = {1, 2, 3, 4, 5};
    std::vector<std::int32_t> result(source.size(), 0);

    cl::Buffer from = manager.create_buffer<std::int32_t>(source.size());
    cl::Buffer to = manager.create_buffer<std::int32_t>(source.size());

    manager.write_buffer(from, source.data(), source.size());

    STDROMANO_REQUIRE(manager.copy_buffer(to, from, source.size() * sizeof(std::int32_t)));
    STDROMANO_REQUIRE(manager.read_buffer(to, result.data(), result.size()));
    STDROMANO_CHECK(result == source);
}

STDROMANO_TEST_CASE(fuzz_vector_add_matches_the_cpu)
{
    if(!initialize_any_device())
        return;

    const auto report = fuzz::run_property(fixtures::options("opencl_vector_add", 30), [](fuzz::Source& source) {
        const std::size_t count = source.range<std::size_t>(1, 5000);

        std::vector<float> a(count);
        std::vector<float> b(count);
        std::vector<float> c;

        for(std::size_t i = 0; i < count; ++i)
        {
            a[i] = source.finite(-1e6f, 1e6f);
            b[i] = source.finite(-1e6f, 1e6f);
        }

        STDROMANO_FUZZ_CHECK(vector_add(a, b, c));

        for(std::size_t i = 0; i < count; ++i)
            STDROMANO_FUZZ_CHECK_EQ(c[i], a[i] + b[i]);

        return true;
    });

    TESTS_REQUIRE_PROPERTY(report);
}

STDROMANO_TEST_MAIN()
