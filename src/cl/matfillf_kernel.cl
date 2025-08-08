// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - Present Romain Augier
// All rights reserved.

__kernel void matfillf_kernel(__global float* A,
                              float value)
{
    const int tx = get_global_id(0);
    const int ty = get_global_id(1);

    const size_t ncols = get_global_size(1);

    A[ty * ncols + tx] = value;
}