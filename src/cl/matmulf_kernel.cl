// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - Present Romain Augier
// All rights reserved.

#define TILE_SIZE 16

__kernel void matmulf_kernel(__global const float* A,
                             __global const float* B,
                             __global float* C,
                             uint M,
                             uint K,
                             uint N)
{
    const int row = get_global_id(1);
    const int col = get_global_id(0);

    __local float Asub[TILE_SIZE][TILE_SIZE];
    __local float Bsub[TILE_SIZE][TILE_SIZE];

    float sum = 0.0f;

    for(int tile = 0; tile < (K + TILE_SIZE - 1) / TILE_SIZE; ++tile) 
    {
        int a_col = tile * TILE_SIZE + get_local_id(0);
        int a_row = row;

        if(a_row < M && a_col < K)
        {
            Asub[get_local_id(1)][get_local_id(0)] = A[a_col * (M) + a_row];
        }
        else
        {
            Asub[get_local_id(1)][get_local_id(0)] = 0.0f;
        }

        int b_col = col;
        int b_row = tile * TILE_SIZE + get_local_id(1);

        if (b_row < K && b_col < N)
        {
            Bsub[get_local_id(1)][get_local_id(0)] = B[b_col * (K) + b_row];
        }
        else
        {
            Bsub[get_local_id(1)][get_local_id(0)] = 0.0f;
        }

        barrier(CLK_LOCAL_MEM_FENCE);

        for(int i = 0; i < TILE_SIZE; ++i)
        {
            sum += Asub[get_local_id(1)][i] * Bsub[i][get_local_id(0)];
        }

        barrier(CLK_LOCAL_MEM_FENCE);
    }

    if (row < M && col < N)
    {
        C[col * (M) + row] = sum;
    }
}