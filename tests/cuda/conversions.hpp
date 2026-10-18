// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - Present Romain Augier
// All rights reserved.

#pragma once

#if !defined(__STDROMANO_TESTS_CUDA_CONVERSIONS)
#define __STDROMANO_TESTS_CUDA_CONVERSIONS

namespace conversions {

enum DeviceResult_ : int
{
    DeviceResult_NoDevice,
    DeviceResult_Failed,
    DeviceResult_Passed,
};

/* Compiled in the host pass of a .cu file, where the CUDA vector types exist, runs without a GPU */
bool check_on_host() noexcept;

int check_on_device() noexcept;

} // namespace conversions

#endif /* !defined(__STDROMANO_TESTS_CUDA_CONVERSIONS) */
