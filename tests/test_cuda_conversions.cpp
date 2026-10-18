// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - Present Romain Augier
// All rights reserved.

#include "cuda/conversions.hpp"

#include "fixtures.hpp"

STDROMANO_TEST_CASE(cuda_conversions_host)
{
    STDROMANO_CHECK(conversions::check_on_host());
}

STDROMANO_TEST_CASE(cuda_conversions_device)
{
    const int result = conversions::check_on_device();

    if(result == conversions::DeviceResult_NoDevice)
    {
        spdlog::warn("No CUDA device available, only the device compilation was tested");
        return;
    }

    STDROMANO_CHECK_EQ(result, static_cast<int>(conversions::DeviceResult_Passed));
}

STDROMANO_TEST_MAIN()
