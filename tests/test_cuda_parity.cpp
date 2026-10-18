// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - Present Romain Augier
// All rights reserved.

#include "cuda/parity.hpp"

#include "fixtures.hpp"

#include <cmath>
#include <cstring>
#include <vector>

using namespace stdromano;

static std::int64_t ordered_bits(const float f) noexcept
{
    std::int32_t bits;
    std::memcpy(&bits, &f, sizeof(float));
    return bits < 0 ? static_cast<std::int64_t>(INT32_MIN) - bits : static_cast<std::int64_t>(bits);
}

static std::uint64_t ulps_between(const float a, const float b) noexcept
{
    const std::int64_t diff = ordered_bits(a) - ordered_bits(b);
    return static_cast<std::uint64_t>(diff < 0 ? -diff : diff);
}

static bool within_tolerance(const float host, const float device, const parity::Tolerance& tolerance) noexcept
{
    if(maths::isnan(host) || maths::isnan(device))
        return maths::isnan(host) && maths::isnan(device);

    if(maths::isinf(host) || maths::isinf(device))
        return host == device;

    return ulps_between(host, device) <= tolerance.max_ulps || std::fabs(host - device) <= tolerance.abs_error;
}

STDROMANO_TEST_CASE(cuda_host_device_parity)
{
    constexpr std::size_t count = 1 << 16;

    std::vector<parity::Input> inputs(count);
    std::uint64_t state = 0x5EED5EED5EED5EEDULL;

    for(parity::Input& input : inputs)
    {
        input.x = next_random_float_in_range(state, -100.0f, 100.0f);
        input.p = next_random_float_in_range(state, 1e-3f, 100.0f);
        input.n = next_random_float_in_range(state, -1.0f, 1.0f);
        input.u = next_random_u32(state);
    }

    std::vector<float> device_outputs(count * parity::Function_Count);

    if(!parity::evaluate_on_device(inputs.data(), count, device_outputs.data()))
    {
        spdlog::warn("No CUDA device available, only the device compilation was tested");
        return;
    }

    std::vector<float> host_outputs(parity::Function_Count);

    std::uint64_t failures[parity::Function_Count] = {};
    std::uint64_t worst_ulps[parity::Function_Count] = {};

    for(std::size_t i = 0; i < count; ++i)
    {
        parity::evaluate(inputs[i], host_outputs.data());

        for(std::uint32_t f = 0; f < parity::Function_Count; ++f)
        {
            const float host = host_outputs[f];
            const float device = device_outputs[i * parity::Function_Count + f];

            if(!within_tolerance(host, device, parity::tolerances[f]))
            {
                if(failures[f] == 0)
                {
                    spdlog::error("{}: x={} p={} n={} u={} host={} device={} ({} ulps)",
                                  parity::tolerances[f].name,
                                  inputs[i].x,
                                  inputs[i].p,
                                  inputs[i].n,
                                  inputs[i].u,
                                  host,
                                  device,
                                  ulps_between(host, device));
                }

                failures[f]++;
            }

            if(!maths::isnan(host) && !maths::isnan(device) && !maths::isinf(host) && !maths::isinf(device))
                worst_ulps[f] = maths::max(worst_ulps[f], ulps_between(host, device));
        }
    }

    for(std::uint32_t f = 0; f < parity::Function_Count; ++f)
    {
        spdlog::debug("{}: worst {} ulps", parity::tolerances[f].name, worst_ulps[f]);
        STDROMANO_CHECK_MSG(failures[f] == 0, parity::tolerances[f].name);
    }
}

STDROMANO_TEST_MAIN()
