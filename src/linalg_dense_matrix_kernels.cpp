// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - Present Romain Augier
// All rights reserved.

#include "stdromano/linalg/dense_matrix.hpp"
#include "stdromano/simd.hpp"
#include "stdromano/threading.hpp"

STDROMANO_NAMESPACE_BEGIN

/********************************/
/* Mat-Mat mul for float */
/********************************/

void matmat_mulf_scalar_kernel(const float* __restrict A,
                               const float* __restrict B,
                               float* __restrict C,
                               std::size_t M,
                               std::size_t K,
                               std::size_t N) noexcept
{
    for(std::size_t i = 0; i < M; i++)
    {
        for(std::size_t j = 0; j < K; j++)
        {
            float sum = 0.0f;

            for(std::size_t k = 0; k < N; k++)
            {
                sum += A[i + k * M] * B[k + j * K];
            }

            C[i * j * M] = sum;
        }
    }
}

/* AVX2 optimized version borrowed from https://github.com/salykova/sgemm.c */

alignas(32) static const std::int8_t mask[32] = {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
                                                 -1, -1, -1, -1, -1, 0,  0,  0,  0,  0,  0,
                                                 0,  0,  0,  0,  0,  0,  0,  0,  0,  0};

inline void fma_loop_00(const float* __restrict blockA_packed,
                        const float* __restrict blockB_packed,
                        __m256* C_accum_00,
                        __m256* C_accum_01,
                        __m256* a0_packFloat8,
                        __m256* a1_packFloat8,
                        __m256* b_packFloat8,
                        std::size_t kc)
{

    for(std::size_t p = 0; p < kc; p++)
    {
        *a0_packFloat8 = _mm256_loadu_ps(blockA_packed);
        *a1_packFloat8 = _mm256_loadu_ps(blockA_packed + 8);

        *b_packFloat8 = _mm256_broadcast_ss(blockB_packed);
        *C_accum_00 = _mm256_fmadd_ps(*a0_packFloat8, *b_packFloat8, *C_accum_00);
        *C_accum_01 = _mm256_fmadd_ps(*a1_packFloat8, *b_packFloat8, *C_accum_01);

        blockA_packed += 16;
        blockB_packed += 6;
    }
}

inline void fma_loop_01(const float* __restrict blockA_packed,
                        const float* __restrict blockB_packed,
                        __m256* C_accum_00,
                        __m256* C_accum_01,
                        __m256* C_accum_10,
                        __m256* C_accum_11,
                        __m256* a0_packFloat8,
                        __m256* a1_packFloat8,
                        __m256* b_packFloat8,
                        std::size_t kc)
{

    for(std::size_t p = 0; p < kc; p++)
    {
        *a0_packFloat8 = _mm256_loadu_ps(blockA_packed);
        *a1_packFloat8 = _mm256_loadu_ps(blockA_packed + 8);

        *b_packFloat8 = _mm256_broadcast_ss(blockB_packed);
        *C_accum_00 = _mm256_fmadd_ps(*a0_packFloat8, *b_packFloat8, *C_accum_00);
        *C_accum_01 = _mm256_fmadd_ps(*a1_packFloat8, *b_packFloat8, *C_accum_01);

        *b_packFloat8 = _mm256_broadcast_ss(blockB_packed + 1);
        *C_accum_10 = _mm256_fmadd_ps(*a0_packFloat8, *b_packFloat8, *C_accum_10);
        *C_accum_11 = _mm256_fmadd_ps(*a1_packFloat8, *b_packFloat8, *C_accum_11);

        blockA_packed += 16;
        blockB_packed += 6;
    }
}

inline void fma_loop_02(const float* __restrict blockA_packed,
                        const float* __restrict blockB_packed,
                        __m256* C_accum_00,
                        __m256* C_accum_01,
                        __m256* C_accum_10,
                        __m256* C_accum_11,
                        __m256* C_accum_20,
                        __m256* C_accum_21,
                        __m256* a0_packFloat8,
                        __m256* a1_packFloat8,
                        __m256* b_packFloat8,
                        std::size_t kc)
{

    for(std::size_t p = 0; p < kc; p++)
    {
        *a0_packFloat8 = _mm256_loadu_ps(blockA_packed);
        *a1_packFloat8 = _mm256_loadu_ps(blockA_packed + 8);

        *b_packFloat8 = _mm256_broadcast_ss(blockB_packed);
        *C_accum_00 = _mm256_fmadd_ps(*a0_packFloat8, *b_packFloat8, *C_accum_00);
        *C_accum_01 = _mm256_fmadd_ps(*a1_packFloat8, *b_packFloat8, *C_accum_01);

        *b_packFloat8 = _mm256_broadcast_ss(blockB_packed + 1);
        *C_accum_10 = _mm256_fmadd_ps(*a0_packFloat8, *b_packFloat8, *C_accum_10);
        *C_accum_11 = _mm256_fmadd_ps(*a1_packFloat8, *b_packFloat8, *C_accum_11);

        *b_packFloat8 = _mm256_broadcast_ss(blockB_packed + 2);
        *C_accum_20 = _mm256_fmadd_ps(*a0_packFloat8, *b_packFloat8, *C_accum_20);
        *C_accum_21 = _mm256_fmadd_ps(*a1_packFloat8, *b_packFloat8, *C_accum_21);

        blockA_packed += 16;
        blockB_packed += 6;
    }
}

inline void fma_loop_03(const float* __restrict blockA_packed,
                        const float* __restrict blockB_packed,
                        __m256* C_accum_00,
                        __m256* C_accum_01,
                        __m256* C_accum_10,
                        __m256* C_accum_11,
                        __m256* C_accum_20,
                        __m256* C_accum_21,
                        __m256* C_accum_30,
                        __m256* C_accum_31,
                        __m256* a0_packFloat8,
                        __m256* a1_packFloat8,
                        __m256* b_packFloat8,
                        std::size_t kc)
{

    for(std::size_t p = 0; p < kc; p++)
    {
        *a0_packFloat8 = _mm256_loadu_ps(blockA_packed);
        *a1_packFloat8 = _mm256_loadu_ps(blockA_packed + 8);

        *b_packFloat8 = _mm256_broadcast_ss(blockB_packed);
        *C_accum_00 = _mm256_fmadd_ps(*a0_packFloat8, *b_packFloat8, *C_accum_00);
        *C_accum_01 = _mm256_fmadd_ps(*a1_packFloat8, *b_packFloat8, *C_accum_01);

        *b_packFloat8 = _mm256_broadcast_ss(blockB_packed + 1);
        *C_accum_10 = _mm256_fmadd_ps(*a0_packFloat8, *b_packFloat8, *C_accum_10);
        *C_accum_11 = _mm256_fmadd_ps(*a1_packFloat8, *b_packFloat8, *C_accum_11);

        *b_packFloat8 = _mm256_broadcast_ss(blockB_packed + 2);
        *C_accum_20 = _mm256_fmadd_ps(*a0_packFloat8, *b_packFloat8, *C_accum_20);
        *C_accum_21 = _mm256_fmadd_ps(*a1_packFloat8, *b_packFloat8, *C_accum_21);

        *b_packFloat8 = _mm256_broadcast_ss(blockB_packed + 3);
        *C_accum_30 = _mm256_fmadd_ps(*a0_packFloat8, *b_packFloat8, *C_accum_30);
        *C_accum_31 = _mm256_fmadd_ps(*a1_packFloat8, *b_packFloat8, *C_accum_31);

        blockA_packed += 16;
        blockB_packed += 6;
    }
}

inline void fma_loop_04(const float* __restrict blockA_packed,
                        const float* __restrict blockB_packed,
                        __m256* C_accum_00,
                        __m256* C_accum_01,
                        __m256* C_accum_10,
                        __m256* C_accum_11,
                        __m256* C_accum_20,
                        __m256* C_accum_21,
                        __m256* C_accum_30,
                        __m256* C_accum_31,
                        __m256* C_accum_40,
                        __m256* C_accum_41,
                        __m256* a0_packFloat8,
                        __m256* a1_packFloat8,
                        __m256* b_packFloat8,
                        std::size_t kc)
{

    for(std::size_t p = 0; p < kc; p++)
    {
        *a0_packFloat8 = _mm256_loadu_ps(blockA_packed);
        *a1_packFloat8 = _mm256_loadu_ps(blockA_packed + 8);

        *b_packFloat8 = _mm256_broadcast_ss(blockB_packed);
        *C_accum_00 = _mm256_fmadd_ps(*a0_packFloat8, *b_packFloat8, *C_accum_00);
        *C_accum_01 = _mm256_fmadd_ps(*a1_packFloat8, *b_packFloat8, *C_accum_01);

        *b_packFloat8 = _mm256_broadcast_ss(blockB_packed + 1);
        *C_accum_10 = _mm256_fmadd_ps(*a0_packFloat8, *b_packFloat8, *C_accum_10);
        *C_accum_11 = _mm256_fmadd_ps(*a1_packFloat8, *b_packFloat8, *C_accum_11);

        *b_packFloat8 = _mm256_broadcast_ss(blockB_packed + 2);
        *C_accum_20 = _mm256_fmadd_ps(*a0_packFloat8, *b_packFloat8, *C_accum_20);
        *C_accum_21 = _mm256_fmadd_ps(*a1_packFloat8, *b_packFloat8, *C_accum_21);

        *b_packFloat8 = _mm256_broadcast_ss(blockB_packed + 3);
        *C_accum_30 = _mm256_fmadd_ps(*a0_packFloat8, *b_packFloat8, *C_accum_30);
        *C_accum_31 = _mm256_fmadd_ps(*a1_packFloat8, *b_packFloat8, *C_accum_31);

        *b_packFloat8 = _mm256_broadcast_ss(blockB_packed + 4);
        *C_accum_40 = _mm256_fmadd_ps(*a0_packFloat8, *b_packFloat8, *C_accum_40);
        *C_accum_41 = _mm256_fmadd_ps(*a1_packFloat8, *b_packFloat8, *C_accum_41);

        blockA_packed += 16;
        blockB_packed += 6;
    }
}

inline void fma_loop_05(const float* __restrict blockA_packed,
                        const float* __restrict blockB_packed,
                        __m256* C_accum_00,
                        __m256* C_accum_01,
                        __m256* C_accum_10,
                        __m256* C_accum_11,
                        __m256* C_accum_20,
                        __m256* C_accum_21,
                        __m256* C_accum_30,
                        __m256* C_accum_31,
                        __m256* C_accum_40,
                        __m256* C_accum_41,
                        __m256* C_accum_50,
                        __m256* C_accum_51,
                        __m256* a0_packFloat8,
                        __m256* a1_packFloat8,
                        __m256* b_packFloat8,
                        std::size_t kc)
{

    for(std::size_t p = 0; p < kc; p++)
    {
        *a0_packFloat8 = _mm256_loadu_ps(blockA_packed);
        *a1_packFloat8 = _mm256_loadu_ps(blockA_packed + 8);

        *b_packFloat8 = _mm256_broadcast_ss(blockB_packed);
        *C_accum_00 = _mm256_fmadd_ps(*a0_packFloat8, *b_packFloat8, *C_accum_00);
        *C_accum_01 = _mm256_fmadd_ps(*a1_packFloat8, *b_packFloat8, *C_accum_01);

        *b_packFloat8 = _mm256_broadcast_ss(blockB_packed + 1);
        *C_accum_10 = _mm256_fmadd_ps(*a0_packFloat8, *b_packFloat8, *C_accum_10);
        *C_accum_11 = _mm256_fmadd_ps(*a1_packFloat8, *b_packFloat8, *C_accum_11);

        *b_packFloat8 = _mm256_broadcast_ss(blockB_packed + 2);
        *C_accum_20 = _mm256_fmadd_ps(*a0_packFloat8, *b_packFloat8, *C_accum_20);
        *C_accum_21 = _mm256_fmadd_ps(*a1_packFloat8, *b_packFloat8, *C_accum_21);

        *b_packFloat8 = _mm256_broadcast_ss(blockB_packed + 3);
        *C_accum_30 = _mm256_fmadd_ps(*a0_packFloat8, *b_packFloat8, *C_accum_30);
        *C_accum_31 = _mm256_fmadd_ps(*a1_packFloat8, *b_packFloat8, *C_accum_31);

        *b_packFloat8 = _mm256_broadcast_ss(blockB_packed + 4);
        *C_accum_40 = _mm256_fmadd_ps(*a0_packFloat8, *b_packFloat8, *C_accum_40);
        *C_accum_41 = _mm256_fmadd_ps(*a1_packFloat8, *b_packFloat8, *C_accum_41);

        *b_packFloat8 = _mm256_broadcast_ss(blockB_packed + 5);
        *C_accum_50 = _mm256_fmadd_ps(*a0_packFloat8, *b_packFloat8, *C_accum_50);
        *C_accum_51 = _mm256_fmadd_ps(*a1_packFloat8, *b_packFloat8, *C_accum_51);

        blockA_packed += 16;
        blockB_packed += 6;
    }
}

inline static void build_masks(__m256i* packed_mask_0, __m256i* packed_mask_1, std::size_t mr)
{
    *packed_mask_0 = _mm256_cvtepi8_epi32(_mm_loadu_si64(&mask[16 - mr]));
    *packed_mask_1 = _mm256_cvtepi8_epi32(_mm_loadu_si64(&mask[16 - mr + 8]));
}

inline void maskload_accum_00(const float* __restrict C,
                              __m256* C_accum_00,
                              __m256* C_accum_01,
                              __m256i packed_mask_0,
                              __m256i packed_mask_1,
                              STDROMANO_MAYBE_UNUSED std::size_t M)
{
    *C_accum_00 = _mm256_maskload_ps(C, packed_mask_0);
    *C_accum_01 = _mm256_maskload_ps(&C[8], packed_mask_1);
}

inline void maskload_accum_01(const float* __restrict C,
                              __m256* C_accum_00,
                              __m256* C_accum_01,
                              __m256* C_accum_10,
                              __m256* C_accum_11,
                              __m256i packed_mask_0,
                              __m256i packed_mask_1,
                              std::size_t M)
{
    *C_accum_00 = _mm256_maskload_ps(C, packed_mask_0);
    *C_accum_01 = _mm256_maskload_ps(&C[8], packed_mask_1);
    *C_accum_10 = _mm256_maskload_ps(&C[M], packed_mask_0);
    *C_accum_11 = _mm256_maskload_ps(&C[M + 8], packed_mask_1);
}

inline void maskload_accum_02(const float* __restrict C,
                              __m256* C_accum_00,
                              __m256* C_accum_01,
                              __m256* C_accum_10,
                              __m256* C_accum_11,
                              __m256* C_accum_20,
                              __m256* C_accum_21,
                              __m256i packed_mask_0,
                              __m256i packed_mask_1,
                              std::size_t M)
{
    *C_accum_00 = _mm256_maskload_ps(C, packed_mask_0);
    *C_accum_01 = _mm256_maskload_ps(&C[8], packed_mask_1);
    *C_accum_10 = _mm256_maskload_ps(&C[M], packed_mask_0);
    *C_accum_11 = _mm256_maskload_ps(&C[M + 8], packed_mask_1);
    *C_accum_20 = _mm256_maskload_ps(&C[2 * M], packed_mask_0);
    *C_accum_21 = _mm256_maskload_ps(&C[2 * M + 8], packed_mask_1);
}

inline void maskload_accum_03(const float* __restrict C,
                              __m256* C_accum_00,
                              __m256* C_accum_01,
                              __m256* C_accum_10,
                              __m256* C_accum_11,
                              __m256* C_accum_20,
                              __m256* C_accum_21,
                              __m256* C_accum_30,
                              __m256* C_accum_31,
                              __m256i packed_mask_0,
                              __m256i packed_mask_1,
                              std::size_t M)
{
    *C_accum_00 = _mm256_maskload_ps(C, packed_mask_0);
    *C_accum_01 = _mm256_maskload_ps(&C[8], packed_mask_1);
    *C_accum_10 = _mm256_maskload_ps(&C[M], packed_mask_0);
    *C_accum_11 = _mm256_maskload_ps(&C[M + 8], packed_mask_1);
    *C_accum_20 = _mm256_maskload_ps(&C[2 * M], packed_mask_0);
    *C_accum_21 = _mm256_maskload_ps(&C[2 * M + 8], packed_mask_1);
    *C_accum_30 = _mm256_maskload_ps(&C[3 * M], packed_mask_0);
    *C_accum_31 = _mm256_maskload_ps(&C[3 * M + 8], packed_mask_1);
}

inline void maskload_accum_04(const float* __restrict C,
                              __m256* C_accum_00,
                              __m256* C_accum_01,
                              __m256* C_accum_10,
                              __m256* C_accum_11,
                              __m256* C_accum_20,
                              __m256* C_accum_21,
                              __m256* C_accum_30,
                              __m256* C_accum_31,
                              __m256* C_accum_40,
                              __m256* C_accum_41,
                              __m256i packed_mask_0,
                              __m256i packed_mask_1,
                              std::size_t M)
{
    *C_accum_00 = _mm256_maskload_ps(C, packed_mask_0);
    *C_accum_01 = _mm256_maskload_ps(&C[8], packed_mask_1);
    *C_accum_10 = _mm256_maskload_ps(&C[M], packed_mask_0);
    *C_accum_11 = _mm256_maskload_ps(&C[M + 8], packed_mask_1);
    *C_accum_20 = _mm256_maskload_ps(&C[2 * M], packed_mask_0);
    *C_accum_21 = _mm256_maskload_ps(&C[2 * M + 8], packed_mask_1);
    *C_accum_30 = _mm256_maskload_ps(&C[3 * M], packed_mask_0);
    *C_accum_31 = _mm256_maskload_ps(&C[3 * M + 8], packed_mask_1);
    *C_accum_40 = _mm256_maskload_ps(&C[4 * M], packed_mask_0);
    *C_accum_41 = _mm256_maskload_ps(&C[4 * M + 8], packed_mask_1);
}

inline void maskload_accum_05(const float* __restrict C,
                              __m256* C_accum_00,
                              __m256* C_accum_01,
                              __m256* C_accum_10,
                              __m256* C_accum_11,
                              __m256* C_accum_20,
                              __m256* C_accum_21,
                              __m256* C_accum_30,
                              __m256* C_accum_31,
                              __m256* C_accum_40,
                              __m256* C_accum_41,
                              __m256* C_accum_50,
                              __m256* C_accum_51,
                              __m256i packed_mask_0,
                              __m256i packed_mask_1,
                              std::size_t M)
{
    *C_accum_00 = _mm256_maskload_ps(C, packed_mask_0);
    *C_accum_01 = _mm256_maskload_ps(&C[8], packed_mask_1);
    *C_accum_10 = _mm256_maskload_ps(&C[M], packed_mask_0);
    *C_accum_11 = _mm256_maskload_ps(&C[M + 8], packed_mask_1);
    *C_accum_20 = _mm256_maskload_ps(&C[2 * M], packed_mask_0);
    *C_accum_21 = _mm256_maskload_ps(&C[2 * M + 8], packed_mask_1);
    *C_accum_30 = _mm256_maskload_ps(&C[3 * M], packed_mask_0);
    *C_accum_31 = _mm256_maskload_ps(&C[3 * M + 8], packed_mask_1);
    *C_accum_40 = _mm256_maskload_ps(&C[4 * M], packed_mask_0);
    *C_accum_41 = _mm256_maskload_ps(&C[4 * M + 8], packed_mask_1);
    *C_accum_50 = _mm256_maskload_ps(&C[5 * M], packed_mask_0);
    *C_accum_51 = _mm256_maskload_ps(&C[5 * M + 8], packed_mask_1);
}

inline void load_accum_00(const float* __restrict C,
                          __m256* C_accum_00,
                          __m256* C_accum_01,
                          STDROMANO_MAYBE_UNUSED std::size_t M)
{
    *C_accum_00 = _mm256_loadu_ps(C);
    *C_accum_01 = _mm256_loadu_ps(&C[8]);
}

inline void load_accum_01(const float* __restrict C,
                          __m256* C_accum_00,
                          __m256* C_accum_01,
                          __m256* C_accum_10,
                          __m256* C_accum_11,
                          std::size_t M)
{
    *C_accum_00 = _mm256_loadu_ps(C);
    *C_accum_01 = _mm256_loadu_ps(&C[8]);
    *C_accum_10 = _mm256_loadu_ps(&C[M]);
    *C_accum_11 = _mm256_loadu_ps(&C[M + 8]);
}

inline void load_accum_02(const float* __restrict C,
                          __m256* C_accum_00,
                          __m256* C_accum_01,
                          __m256* C_accum_10,
                          __m256* C_accum_11,
                          __m256* C_accum_20,
                          __m256* C_accum_21,
                          std::size_t M)
{
    *C_accum_00 = _mm256_loadu_ps(C);
    *C_accum_01 = _mm256_loadu_ps(&C[8]);
    *C_accum_10 = _mm256_loadu_ps(&C[M]);
    *C_accum_11 = _mm256_loadu_ps(&C[M + 8]);
    *C_accum_20 = _mm256_loadu_ps(&C[2 * M]);
    *C_accum_21 = _mm256_loadu_ps(&C[2 * M + 8]);
}

inline void load_accum_03(const float* __restrict C,
                          __m256* C_accum_00,
                          __m256* C_accum_01,
                          __m256* C_accum_10,
                          __m256* C_accum_11,
                          __m256* C_accum_20,
                          __m256* C_accum_21,
                          __m256* C_accum_30,
                          __m256* C_accum_31,
                          std::size_t M)
{
    *C_accum_00 = _mm256_loadu_ps(C);
    *C_accum_01 = _mm256_loadu_ps(&C[8]);
    *C_accum_10 = _mm256_loadu_ps(&C[M]);
    *C_accum_11 = _mm256_loadu_ps(&C[M + 8]);
    *C_accum_20 = _mm256_loadu_ps(&C[2 * M]);
    *C_accum_21 = _mm256_loadu_ps(&C[2 * M + 8]);
    *C_accum_30 = _mm256_loadu_ps(&C[3 * M]);
    *C_accum_31 = _mm256_loadu_ps(&C[3 * M + 8]);
}

inline void load_accum_04(const float* __restrict C,
                          __m256* C_accum_00,
                          __m256* C_accum_01,
                          __m256* C_accum_10,
                          __m256* C_accum_11,
                          __m256* C_accum_20,
                          __m256* C_accum_21,
                          __m256* C_accum_30,
                          __m256* C_accum_31,
                          __m256* C_accum_40,
                          __m256* C_accum_41,
                          std::size_t M)
{
    *C_accum_00 = _mm256_loadu_ps(C);
    *C_accum_01 = _mm256_loadu_ps(&C[8]);
    *C_accum_10 = _mm256_loadu_ps(&C[M]);
    *C_accum_11 = _mm256_loadu_ps(&C[M + 8]);
    *C_accum_20 = _mm256_loadu_ps(&C[2 * M]);
    *C_accum_21 = _mm256_loadu_ps(&C[2 * M + 8]);
    *C_accum_30 = _mm256_loadu_ps(&C[3 * M]);
    *C_accum_31 = _mm256_loadu_ps(&C[3 * M + 8]);
    *C_accum_40 = _mm256_loadu_ps(&C[4 * M]);
    *C_accum_41 = _mm256_loadu_ps(&C[4 * M + 8]);
}

inline void load_accum_05(const float* __restrict C,
                          __m256* C_accum_00,
                          __m256* C_accum_01,
                          __m256* C_accum_10,
                          __m256* C_accum_11,
                          __m256* C_accum_20,
                          __m256* C_accum_21,
                          __m256* C_accum_30,
                          __m256* C_accum_31,
                          __m256* C_accum_40,
                          __m256* C_accum_41,
                          __m256* C_accum_50,
                          __m256* C_accum_51,
                          std::size_t M)
{
    *C_accum_00 = _mm256_loadu_ps(C);
    *C_accum_01 = _mm256_loadu_ps(&C[8]);
    *C_accum_10 = _mm256_loadu_ps(&C[M]);
    *C_accum_11 = _mm256_loadu_ps(&C[M + 8]);
    *C_accum_20 = _mm256_loadu_ps(&C[2 * M]);
    *C_accum_21 = _mm256_loadu_ps(&C[2 * M + 8]);
    *C_accum_30 = _mm256_loadu_ps(&C[3 * M]);
    *C_accum_31 = _mm256_loadu_ps(&C[3 * M + 8]);
    *C_accum_40 = _mm256_loadu_ps(&C[4 * M]);
    *C_accum_41 = _mm256_loadu_ps(&C[4 * M + 8]);
    *C_accum_50 = _mm256_loadu_ps(&C[5 * M]);
    *C_accum_51 = _mm256_loadu_ps(&C[5 * M + 8]);
}

inline void store_accum_00(float* __restrict C,
                           __m256* C_accum_00,
                           __m256* C_accum_01,
                           STDROMANO_MAYBE_UNUSED std::size_t M)
{
    _mm256_storeu_ps(C, *C_accum_00);
    _mm256_storeu_ps(&C[8], *C_accum_01);
}

inline void store_accum_01(float* __restrict C,
                           __m256* C_accum_00,
                           __m256* C_accum_01,
                           __m256* C_accum_10,
                           __m256* C_accum_11,
                           std::size_t M)
{
    _mm256_storeu_ps(C, *C_accum_00);
    _mm256_storeu_ps(&C[8], *C_accum_01);
    _mm256_storeu_ps(&C[M], *C_accum_10);
    _mm256_storeu_ps(&C[M + 8], *C_accum_11);
}

inline void store_accum_02(float* __restrict C,
                           __m256* C_accum_00,
                           __m256* C_accum_01,
                           __m256* C_accum_10,
                           __m256* C_accum_11,
                           __m256* C_accum_20,
                           __m256* C_accum_21,
                           std::size_t M)
{
    _mm256_storeu_ps(C, *C_accum_00);
    _mm256_storeu_ps(&C[8], *C_accum_01);
    _mm256_storeu_ps(&C[M], *C_accum_10);
    _mm256_storeu_ps(&C[M + 8], *C_accum_11);
    _mm256_storeu_ps(&C[2 * M], *C_accum_20);
    _mm256_storeu_ps(&C[2 * M + 8], *C_accum_21);
}

inline void store_accum_03(float* __restrict C,
                           __m256* C_accum_00,
                           __m256* C_accum_01,
                           __m256* C_accum_10,
                           __m256* C_accum_11,
                           __m256* C_accum_20,
                           __m256* C_accum_21,
                           __m256* C_accum_30,
                           __m256* C_accum_31,
                           std::size_t M)
{
    _mm256_storeu_ps(C, *C_accum_00);
    _mm256_storeu_ps(&C[8], *C_accum_01);
    _mm256_storeu_ps(&C[M], *C_accum_10);
    _mm256_storeu_ps(&C[M + 8], *C_accum_11);
    _mm256_storeu_ps(&C[2 * M], *C_accum_20);
    _mm256_storeu_ps(&C[2 * M + 8], *C_accum_21);
    _mm256_storeu_ps(&C[3 * M], *C_accum_30);
    _mm256_storeu_ps(&C[3 * M + 8], *C_accum_31);
}

inline void store_accum_04(float* __restrict C,
                           __m256* C_accum_00,
                           __m256* C_accum_01,
                           __m256* C_accum_10,
                           __m256* C_accum_11,
                           __m256* C_accum_20,
                           __m256* C_accum_21,
                           __m256* C_accum_30,
                           __m256* C_accum_31,
                           __m256* C_accum_40,
                           __m256* C_accum_41,
                           std::size_t M)
{
    _mm256_storeu_ps(C, *C_accum_00);
    _mm256_storeu_ps(&C[8], *C_accum_01);
    _mm256_storeu_ps(&C[M], *C_accum_10);
    _mm256_storeu_ps(&C[M + 8], *C_accum_11);
    _mm256_storeu_ps(&C[2 * M], *C_accum_20);
    _mm256_storeu_ps(&C[2 * M + 8], *C_accum_21);
    _mm256_storeu_ps(&C[3 * M], *C_accum_30);
    _mm256_storeu_ps(&C[3 * M + 8], *C_accum_31);
    _mm256_storeu_ps(&C[4 * M], *C_accum_40);
    _mm256_storeu_ps(&C[4 * M + 8], *C_accum_41);
}

inline void store_accum_05(float* __restrict C,
                           __m256* C_accum_00,
                           __m256* C_accum_01,
                           __m256* C_accum_10,
                           __m256* C_accum_11,
                           __m256* C_accum_20,
                           __m256* C_accum_21,
                           __m256* C_accum_30,
                           __m256* C_accum_31,
                           __m256* C_accum_40,
                           __m256* C_accum_41,
                           __m256* C_accum_50,
                           __m256* C_accum_51,
                           std::size_t M)
{
    _mm256_storeu_ps(C, *C_accum_00);
    _mm256_storeu_ps(&C[8], *C_accum_01);
    _mm256_storeu_ps(&C[M], *C_accum_10);
    _mm256_storeu_ps(&C[M + 8], *C_accum_11);
    _mm256_storeu_ps(&C[2 * M], *C_accum_20);
    _mm256_storeu_ps(&C[2 * M + 8], *C_accum_21);
    _mm256_storeu_ps(&C[3 * M], *C_accum_30);
    _mm256_storeu_ps(&C[3 * M + 8], *C_accum_31);
    _mm256_storeu_ps(&C[4 * M], *C_accum_40);
    _mm256_storeu_ps(&C[4 * M + 8], *C_accum_41);
    _mm256_storeu_ps(&C[5 * M], *C_accum_50);
    _mm256_storeu_ps(&C[5 * M + 8], *C_accum_51);
}

inline void maskstore_accum_00(float* __restrict C,
                               __m256* C_accum_00,
                               __m256* C_accum_01,
                               __m256i packed_mask_0,
                               __m256i packed_mask_1,
                               STDROMANO_MAYBE_UNUSED std::size_t M)
{
    _mm256_maskstore_ps(C, packed_mask_0, *C_accum_00);
    _mm256_maskstore_ps(&C[8], packed_mask_1, *C_accum_01);
}

inline void maskstore_accum_01(float* __restrict C,
                               __m256* C_accum_00,
                               __m256* C_accum_01,
                               __m256* C_accum_10,
                               __m256* C_accum_11,
                               __m256i packed_mask_0,
                               __m256i packed_mask_1,
                               std::size_t M)
{
    _mm256_maskstore_ps(C, packed_mask_0, *C_accum_00);
    _mm256_maskstore_ps(&C[8], packed_mask_1, *C_accum_01);
    _mm256_maskstore_ps(&C[M], packed_mask_0, *C_accum_10);
    _mm256_maskstore_ps(&C[M + 8], packed_mask_1, *C_accum_11);
}

inline void maskstore_accum_02(float* __restrict C,
                               __m256* C_accum_00,
                               __m256* C_accum_01,
                               __m256* C_accum_10,
                               __m256* C_accum_11,
                               __m256* C_accum_20,
                               __m256* C_accum_21,
                               __m256i packed_mask_0,
                               __m256i packed_mask_1,
                               std::size_t M)
{
    _mm256_maskstore_ps(C, packed_mask_0, *C_accum_00);
    _mm256_maskstore_ps(&C[8], packed_mask_1, *C_accum_01);
    _mm256_maskstore_ps(&C[M], packed_mask_0, *C_accum_10);
    _mm256_maskstore_ps(&C[M + 8], packed_mask_1, *C_accum_11);
    _mm256_maskstore_ps(&C[2 * M], packed_mask_0, *C_accum_20);
    _mm256_maskstore_ps(&C[2 * M + 8], packed_mask_1, *C_accum_21);
}

inline void maskstore_accum_03(float* __restrict C,
                               __m256* C_accum_00,
                               __m256* C_accum_01,
                               __m256* C_accum_10,
                               __m256* C_accum_11,
                               __m256* C_accum_20,
                               __m256* C_accum_21,
                               __m256* C_accum_30,
                               __m256* C_accum_31,
                               __m256i packed_mask_0,
                               __m256i packed_mask_1,
                               std::size_t M)
{
    _mm256_maskstore_ps(C, packed_mask_0, *C_accum_00);
    _mm256_maskstore_ps(&C[8], packed_mask_1, *C_accum_01);
    _mm256_maskstore_ps(&C[M], packed_mask_0, *C_accum_10);
    _mm256_maskstore_ps(&C[M + 8], packed_mask_1, *C_accum_11);
    _mm256_maskstore_ps(&C[2 * M], packed_mask_0, *C_accum_20);
    _mm256_maskstore_ps(&C[2 * M + 8], packed_mask_1, *C_accum_21);
    _mm256_maskstore_ps(&C[3 * M], packed_mask_0, *C_accum_30);
    _mm256_maskstore_ps(&C[3 * M + 8], packed_mask_1, *C_accum_31);
}

inline void maskstore_accum_04(float* __restrict C,
                               __m256* C_accum_00,
                               __m256* C_accum_01,
                               __m256* C_accum_10,
                               __m256* C_accum_11,
                               __m256* C_accum_20,
                               __m256* C_accum_21,
                               __m256* C_accum_30,
                               __m256* C_accum_31,
                               __m256* C_accum_40,
                               __m256* C_accum_41,
                               __m256i packed_mask_0,
                               __m256i packed_mask_1,
                               std::size_t M)
{
    _mm256_maskstore_ps(C, packed_mask_0, *C_accum_00);
    _mm256_maskstore_ps(&C[8], packed_mask_1, *C_accum_01);
    _mm256_maskstore_ps(&C[M], packed_mask_0, *C_accum_10);
    _mm256_maskstore_ps(&C[M + 8], packed_mask_1, *C_accum_11);
    _mm256_maskstore_ps(&C[2 * M], packed_mask_0, *C_accum_20);
    _mm256_maskstore_ps(&C[2 * M + 8], packed_mask_1, *C_accum_21);
    _mm256_maskstore_ps(&C[3 * M], packed_mask_0, *C_accum_30);
    _mm256_maskstore_ps(&C[3 * M + 8], packed_mask_1, *C_accum_31);
    _mm256_maskstore_ps(&C[4 * M], packed_mask_0, *C_accum_40);
    _mm256_maskstore_ps(&C[4 * M + 8], packed_mask_1, *C_accum_41);
}

inline void maskstore_accum_05(float* __restrict C,
                               __m256* C_accum_00,
                               __m256* C_accum_01,
                               __m256* C_accum_10,
                               __m256* C_accum_11,
                               __m256* C_accum_20,
                               __m256* C_accum_21,
                               __m256* C_accum_30,
                               __m256* C_accum_31,
                               __m256* C_accum_40,
                               __m256* C_accum_41,
                               __m256* C_accum_50,
                               __m256* C_accum_51,
                               __m256i packed_mask_0,
                               __m256i packed_mask_1,
                               std::size_t M)
{
    _mm256_maskstore_ps(C, packed_mask_0, *C_accum_00);
    _mm256_maskstore_ps(&C[8], packed_mask_1, *C_accum_01);
    _mm256_maskstore_ps(&C[M], packed_mask_0, *C_accum_10);
    _mm256_maskstore_ps(&C[M + 8], packed_mask_1, *C_accum_11);
    _mm256_maskstore_ps(&C[2 * M], packed_mask_0, *C_accum_20);
    _mm256_maskstore_ps(&C[2 * M + 8], packed_mask_1, *C_accum_21);
    _mm256_maskstore_ps(&C[3 * M], packed_mask_0, *C_accum_30);
    _mm256_maskstore_ps(&C[3 * M + 8], packed_mask_1, *C_accum_31);
    _mm256_maskstore_ps(&C[4 * M], packed_mask_0, *C_accum_40);
    _mm256_maskstore_ps(&C[4 * M + 8], packed_mask_1, *C_accum_41);
    _mm256_maskstore_ps(&C[5 * M], packed_mask_0, *C_accum_50);
    _mm256_maskstore_ps(&C[5 * M + 8], packed_mask_1, *C_accum_51);
}

void kernel_16x6_load_accum(const float* __restrict blockA_packed,
                            const float* __restrict blockB_packed,
                            float* __restrict C,
                            std::size_t mr,
                            std::size_t nr,
                            std::size_t kc,
                            std::size_t M)
{
    __m256 C_accum_00 = {};
    __m256 C_accum_01 = {};
    __m256 C_accum_10 = {};
    __m256 C_accum_11 = {};
    __m256 C_accum_20 = {};
    __m256 C_accum_21 = {};
    __m256 C_accum_30 = {};
    __m256 C_accum_31 = {};
    __m256 C_accum_40 = {};
    __m256 C_accum_41 = {};
    __m256 C_accum_50 = {};
    __m256 C_accum_51 = {};

    __m256 b_packFloat8 = {};
    __m256 a0_packFloat8 = {};
    __m256 a1_packFloat8 = {};
    __m256i packed_mask_0 = {};
    __m256i packed_mask_1 = {};

    if(mr != 16)
    {
        build_masks(&packed_mask_0, &packed_mask_1, mr);
        switch(nr)
        {
            case 1:
                maskload_accum_00(C, &C_accum_00, &C_accum_01, packed_mask_0, packed_mask_1, M);
                fma_loop_00(blockA_packed,
                            blockB_packed,
                            &C_accum_00,
                            &C_accum_01,
                            &a0_packFloat8,
                            &a1_packFloat8,
                            &b_packFloat8,
                            kc);
                maskstore_accum_00(C, &C_accum_00, &C_accum_01, packed_mask_0, packed_mask_1, M);
                break;
            case 2:
                maskload_accum_01(C,
                                  &C_accum_00,
                                  &C_accum_01,
                                  &C_accum_10,
                                  &C_accum_11,
                                  packed_mask_0,
                                  packed_mask_1,
                                  M);
                fma_loop_01(blockA_packed,
                            blockB_packed,
                            &C_accum_00,
                            &C_accum_01,
                            &C_accum_10,
                            &C_accum_11,
                            &a0_packFloat8,
                            &a1_packFloat8,
                            &b_packFloat8,
                            kc);
                maskstore_accum_01(C,
                                   &C_accum_00,
                                   &C_accum_01,
                                   &C_accum_10,
                                   &C_accum_11,
                                   packed_mask_0,
                                   packed_mask_1,
                                   M);
                break;
            case 3:
                maskload_accum_02(C,
                                  &C_accum_00,
                                  &C_accum_01,
                                  &C_accum_10,
                                  &C_accum_11,
                                  &C_accum_20,
                                  &C_accum_21,
                                  packed_mask_0,
                                  packed_mask_1,
                                  M);
                fma_loop_02(blockA_packed,
                            blockB_packed,
                            &C_accum_00,
                            &C_accum_01,
                            &C_accum_10,
                            &C_accum_11,
                            &C_accum_20,
                            &C_accum_21,
                            &a0_packFloat8,
                            &a1_packFloat8,
                            &b_packFloat8,
                            kc);
                maskstore_accum_02(C,
                                   &C_accum_00,
                                   &C_accum_01,
                                   &C_accum_10,
                                   &C_accum_11,
                                   &C_accum_20,
                                   &C_accum_21,
                                   packed_mask_0,
                                   packed_mask_1,
                                   M);
                break;
            case 4:
                maskload_accum_03(C,
                                  &C_accum_00,
                                  &C_accum_01,
                                  &C_accum_10,
                                  &C_accum_11,
                                  &C_accum_20,
                                  &C_accum_21,
                                  &C_accum_30,
                                  &C_accum_31,
                                  packed_mask_0,
                                  packed_mask_1,
                                  M);
                fma_loop_03(blockA_packed,
                            blockB_packed,
                            &C_accum_00,
                            &C_accum_01,
                            &C_accum_10,
                            &C_accum_11,
                            &C_accum_20,
                            &C_accum_21,
                            &C_accum_30,
                            &C_accum_31,
                            &a0_packFloat8,
                            &a1_packFloat8,
                            &b_packFloat8,
                            kc);
                maskstore_accum_03(C,
                                   &C_accum_00,
                                   &C_accum_01,
                                   &C_accum_10,
                                   &C_accum_11,
                                   &C_accum_20,
                                   &C_accum_21,
                                   &C_accum_30,
                                   &C_accum_31,
                                   packed_mask_0,
                                   packed_mask_1,
                                   M);
                break;
            case 5:
                maskload_accum_04(C,
                                  &C_accum_00,
                                  &C_accum_01,
                                  &C_accum_10,
                                  &C_accum_11,
                                  &C_accum_20,
                                  &C_accum_21,
                                  &C_accum_30,
                                  &C_accum_31,
                                  &C_accum_40,
                                  &C_accum_41,
                                  packed_mask_0,
                                  packed_mask_1,
                                  M);
                fma_loop_04(blockA_packed,
                            blockB_packed,
                            &C_accum_00,
                            &C_accum_01,
                            &C_accum_10,
                            &C_accum_11,
                            &C_accum_20,
                            &C_accum_21,
                            &C_accum_30,
                            &C_accum_31,
                            &C_accum_40,
                            &C_accum_41,
                            &a0_packFloat8,
                            &a1_packFloat8,
                            &b_packFloat8,
                            kc);
                maskstore_accum_04(C,
                                   &C_accum_00,
                                   &C_accum_01,
                                   &C_accum_10,
                                   &C_accum_11,
                                   &C_accum_20,
                                   &C_accum_21,
                                   &C_accum_30,
                                   &C_accum_31,
                                   &C_accum_40,
                                   &C_accum_41,
                                   packed_mask_0,
                                   packed_mask_1,
                                   M);
                break;
            case 6:
                maskload_accum_05(C,
                                  &C_accum_00,
                                  &C_accum_01,
                                  &C_accum_10,
                                  &C_accum_11,
                                  &C_accum_20,
                                  &C_accum_21,
                                  &C_accum_30,
                                  &C_accum_31,
                                  &C_accum_40,
                                  &C_accum_41,
                                  &C_accum_50,
                                  &C_accum_51,
                                  packed_mask_0,
                                  packed_mask_1,
                                  M);
                fma_loop_05(blockA_packed,
                            blockB_packed,
                            &C_accum_00,
                            &C_accum_01,
                            &C_accum_10,
                            &C_accum_11,
                            &C_accum_20,
                            &C_accum_21,
                            &C_accum_30,
                            &C_accum_31,
                            &C_accum_40,
                            &C_accum_41,
                            &C_accum_50,
                            &C_accum_51,
                            &a0_packFloat8,
                            &a1_packFloat8,
                            &b_packFloat8,
                            kc);
                maskstore_accum_05(C,
                                   &C_accum_00,
                                   &C_accum_01,
                                   &C_accum_10,
                                   &C_accum_11,
                                   &C_accum_20,
                                   &C_accum_21,
                                   &C_accum_30,
                                   &C_accum_31,
                                   &C_accum_40,
                                   &C_accum_41,
                                   &C_accum_50,
                                   &C_accum_51,
                                   packed_mask_0,
                                   packed_mask_1,
                                   M);
                break;
        }
    }
    else
    {
        switch(nr)
        {
            case 1:
                load_accum_00(C, &C_accum_00, &C_accum_01, M);
                fma_loop_00(blockA_packed,
                            blockB_packed,
                            &C_accum_00,
                            &C_accum_01,
                            &a0_packFloat8,
                            &a1_packFloat8,
                            &b_packFloat8,
                            kc);
                store_accum_00(C, &C_accum_00, &C_accum_01, M);
                break;
            case 2:
                load_accum_01(C, &C_accum_00, &C_accum_01, &C_accum_10, &C_accum_11, M);
                fma_loop_01(blockA_packed,
                            blockB_packed,
                            &C_accum_00,
                            &C_accum_01,
                            &C_accum_10,
                            &C_accum_11,
                            &a0_packFloat8,
                            &a1_packFloat8,
                            &b_packFloat8,
                            kc);
                store_accum_01(C, &C_accum_00, &C_accum_01, &C_accum_10, &C_accum_11, M);
                break;
            case 3:
                load_accum_02(C,
                              &C_accum_00,
                              &C_accum_01,
                              &C_accum_10,
                              &C_accum_11,
                              &C_accum_20,
                              &C_accum_21,
                              M);
                fma_loop_02(blockA_packed,
                            blockB_packed,
                            &C_accum_00,
                            &C_accum_01,
                            &C_accum_10,
                            &C_accum_11,
                            &C_accum_20,
                            &C_accum_21,
                            &a0_packFloat8,
                            &a1_packFloat8,
                            &b_packFloat8,
                            kc);
                store_accum_02(C,
                               &C_accum_00,
                               &C_accum_01,
                               &C_accum_10,
                               &C_accum_11,
                               &C_accum_20,
                               &C_accum_21,
                               M);
                break;
            case 4:
                load_accum_03(C,
                              &C_accum_00,
                              &C_accum_01,
                              &C_accum_10,
                              &C_accum_11,
                              &C_accum_20,
                              &C_accum_21,
                              &C_accum_30,
                              &C_accum_31,
                              M);
                fma_loop_03(blockA_packed,
                            blockB_packed,
                            &C_accum_00,
                            &C_accum_01,
                            &C_accum_10,
                            &C_accum_11,
                            &C_accum_20,
                            &C_accum_21,
                            &C_accum_30,
                            &C_accum_31,
                            &a0_packFloat8,
                            &a1_packFloat8,
                            &b_packFloat8,
                            kc);
                store_accum_03(C,
                               &C_accum_00,
                               &C_accum_01,
                               &C_accum_10,
                               &C_accum_11,
                               &C_accum_20,
                               &C_accum_21,
                               &C_accum_30,
                               &C_accum_31,
                               M);
                break;
            case 5:
                load_accum_04(C,
                              &C_accum_00,
                              &C_accum_01,
                              &C_accum_10,
                              &C_accum_11,
                              &C_accum_20,
                              &C_accum_21,
                              &C_accum_30,
                              &C_accum_31,
                              &C_accum_40,
                              &C_accum_41,
                              M);
                fma_loop_04(blockA_packed,
                            blockB_packed,
                            &C_accum_00,
                            &C_accum_01,
                            &C_accum_10,
                            &C_accum_11,
                            &C_accum_20,
                            &C_accum_21,
                            &C_accum_30,
                            &C_accum_31,
                            &C_accum_40,
                            &C_accum_41,
                            &a0_packFloat8,
                            &a1_packFloat8,
                            &b_packFloat8,
                            kc);
                store_accum_04(C,
                               &C_accum_00,
                               &C_accum_01,
                               &C_accum_10,
                               &C_accum_11,
                               &C_accum_20,
                               &C_accum_21,
                               &C_accum_30,
                               &C_accum_31,
                               &C_accum_40,
                               &C_accum_41,
                               M);

                break;
            case 6:
                load_accum_05(C,
                              &C_accum_00,
                              &C_accum_01,
                              &C_accum_10,
                              &C_accum_11,
                              &C_accum_20,
                              &C_accum_21,
                              &C_accum_30,
                              &C_accum_31,
                              &C_accum_40,
                              &C_accum_41,
                              &C_accum_50,
                              &C_accum_51,
                              M);
                fma_loop_05(blockA_packed,
                            blockB_packed,
                            &C_accum_00,
                            &C_accum_01,
                            &C_accum_10,
                            &C_accum_11,
                            &C_accum_20,
                            &C_accum_21,
                            &C_accum_30,
                            &C_accum_31,
                            &C_accum_40,
                            &C_accum_41,
                            &C_accum_50,
                            &C_accum_51,
                            &a0_packFloat8,
                            &a1_packFloat8,
                            &b_packFloat8,
                            kc);
                store_accum_05(C,
                               &C_accum_00,
                               &C_accum_01,
                               &C_accum_10,
                               &C_accum_11,
                               &C_accum_20,
                               &C_accum_21,
                               &C_accum_30,
                               &C_accum_31,
                               &C_accum_40,
                               &C_accum_41,
                               &C_accum_50,
                               &C_accum_51,
                               M);
                break;
        }
    }
}

void kernel_16x6_zero_init_accum(const float* __restrict blockA_packed,
                                 const float* __restrict blockB_packed,
                                 float* __restrict C,
                                 std::size_t mr,
                                 std::size_t nr,
                                 std::size_t kc,
                                 std::size_t M)
{
    __m256 C_accum_00 = {};
    __m256 C_accum_01 = {};
    __m256 C_accum_10 = {};
    __m256 C_accum_11 = {};
    __m256 C_accum_20 = {};
    __m256 C_accum_21 = {};
    __m256 C_accum_30 = {};
    __m256 C_accum_31 = {};
    __m256 C_accum_40 = {};
    __m256 C_accum_41 = {};
    __m256 C_accum_50 = {};
    __m256 C_accum_51 = {};

    __m256 b_packFloat8 = {};
    __m256 a0_packFloat8 = {};
    __m256 a1_packFloat8 = {};
    __m256i packed_mask_0 = {};
    __m256i packed_mask_1 = {};

    if(mr != 16)
    {
        build_masks(&packed_mask_0, &packed_mask_1, mr);
        switch(nr)
        {
            case 1:
                fma_loop_00(blockA_packed,
                            blockB_packed,
                            &C_accum_00,
                            &C_accum_01,
                            &a0_packFloat8,
                            &a1_packFloat8,
                            &b_packFloat8,
                            kc);
                maskstore_accum_00(C, &C_accum_00, &C_accum_01, packed_mask_0, packed_mask_1, M);
                break;
            case 2:
                fma_loop_01(blockA_packed,
                            blockB_packed,
                            &C_accum_00,
                            &C_accum_01,
                            &C_accum_10,
                            &C_accum_11,
                            &a0_packFloat8,
                            &a1_packFloat8,
                            &b_packFloat8,
                            kc);
                maskstore_accum_01(C,
                                   &C_accum_00,
                                   &C_accum_01,
                                   &C_accum_10,
                                   &C_accum_11,
                                   packed_mask_0,
                                   packed_mask_1,
                                   M);
                break;
            case 3:
                fma_loop_02(blockA_packed,
                            blockB_packed,
                            &C_accum_00,
                            &C_accum_01,
                            &C_accum_10,
                            &C_accum_11,
                            &C_accum_20,
                            &C_accum_21,
                            &a0_packFloat8,
                            &a1_packFloat8,
                            &b_packFloat8,
                            kc);
                maskstore_accum_02(C,
                                   &C_accum_00,
                                   &C_accum_01,
                                   &C_accum_10,
                                   &C_accum_11,
                                   &C_accum_20,
                                   &C_accum_21,
                                   packed_mask_0,
                                   packed_mask_1,
                                   M);
                break;
            case 4:
                fma_loop_03(blockA_packed,
                            blockB_packed,
                            &C_accum_00,
                            &C_accum_01,
                            &C_accum_10,
                            &C_accum_11,
                            &C_accum_20,
                            &C_accum_21,
                            &C_accum_30,
                            &C_accum_31,
                            &a0_packFloat8,
                            &a1_packFloat8,
                            &b_packFloat8,
                            kc);
                maskstore_accum_03(C,
                                   &C_accum_00,
                                   &C_accum_01,
                                   &C_accum_10,
                                   &C_accum_11,
                                   &C_accum_20,
                                   &C_accum_21,
                                   &C_accum_30,
                                   &C_accum_31,
                                   packed_mask_0,
                                   packed_mask_1,
                                   M);
                break;
            case 5:
                fma_loop_04(blockA_packed,
                            blockB_packed,
                            &C_accum_00,
                            &C_accum_01,
                            &C_accum_10,
                            &C_accum_11,
                            &C_accum_20,
                            &C_accum_21,
                            &C_accum_30,
                            &C_accum_31,
                            &C_accum_40,
                            &C_accum_41,
                            &a0_packFloat8,
                            &a1_packFloat8,
                            &b_packFloat8,
                            kc);
                maskstore_accum_04(C,
                                   &C_accum_00,
                                   &C_accum_01,
                                   &C_accum_10,
                                   &C_accum_11,
                                   &C_accum_20,
                                   &C_accum_21,
                                   &C_accum_30,
                                   &C_accum_31,
                                   &C_accum_40,
                                   &C_accum_41,
                                   packed_mask_0,
                                   packed_mask_1,
                                   M);
                break;
            case 6:
                fma_loop_05(blockA_packed,
                            blockB_packed,
                            &C_accum_00,
                            &C_accum_01,
                            &C_accum_10,
                            &C_accum_11,
                            &C_accum_20,
                            &C_accum_21,
                            &C_accum_30,
                            &C_accum_31,
                            &C_accum_40,
                            &C_accum_41,
                            &C_accum_50,
                            &C_accum_51,
                            &a0_packFloat8,
                            &a1_packFloat8,
                            &b_packFloat8,
                            kc);
                maskstore_accum_05(C,
                                   &C_accum_00,
                                   &C_accum_01,
                                   &C_accum_10,
                                   &C_accum_11,
                                   &C_accum_20,
                                   &C_accum_21,
                                   &C_accum_30,
                                   &C_accum_31,
                                   &C_accum_40,
                                   &C_accum_41,
                                   &C_accum_50,
                                   &C_accum_51,
                                   packed_mask_0,
                                   packed_mask_1,
                                   M);
                break;
        }
    }
    else
    {
        switch(nr)
        {
            case 1:
                fma_loop_00(blockA_packed,
                            blockB_packed,
                            &C_accum_00,
                            &C_accum_01,
                            &a0_packFloat8,
                            &a1_packFloat8,
                            &b_packFloat8,
                            kc);
                store_accum_00(C, &C_accum_00, &C_accum_01, M);
                break;
            case 2:
                fma_loop_01(blockA_packed,
                            blockB_packed,
                            &C_accum_00,
                            &C_accum_01,
                            &C_accum_10,
                            &C_accum_11,
                            &a0_packFloat8,
                            &a1_packFloat8,
                            &b_packFloat8,
                            kc);
                store_accum_01(C, &C_accum_00, &C_accum_01, &C_accum_10, &C_accum_11, M);
                break;
            case 3:
                fma_loop_02(blockA_packed,
                            blockB_packed,
                            &C_accum_00,
                            &C_accum_01,
                            &C_accum_10,
                            &C_accum_11,
                            &C_accum_20,
                            &C_accum_21,
                            &a0_packFloat8,
                            &a1_packFloat8,
                            &b_packFloat8,
                            kc);
                store_accum_02(C,
                               &C_accum_00,
                               &C_accum_01,
                               &C_accum_10,
                               &C_accum_11,
                               &C_accum_20,
                               &C_accum_21,
                               M);
                break;
            case 4:
                fma_loop_03(blockA_packed,
                            blockB_packed,
                            &C_accum_00,
                            &C_accum_01,
                            &C_accum_10,
                            &C_accum_11,
                            &C_accum_20,
                            &C_accum_21,
                            &C_accum_30,
                            &C_accum_31,
                            &a0_packFloat8,
                            &a1_packFloat8,
                            &b_packFloat8,
                            kc);
                store_accum_03(C,
                               &C_accum_00,
                               &C_accum_01,
                               &C_accum_10,
                               &C_accum_11,
                               &C_accum_20,
                               &C_accum_21,
                               &C_accum_30,
                               &C_accum_31,
                               M);
                break;
            case 5:
                fma_loop_04(blockA_packed,
                            blockB_packed,
                            &C_accum_00,
                            &C_accum_01,
                            &C_accum_10,
                            &C_accum_11,
                            &C_accum_20,
                            &C_accum_21,
                            &C_accum_30,
                            &C_accum_31,
                            &C_accum_40,
                            &C_accum_41,
                            &a0_packFloat8,
                            &a1_packFloat8,
                            &b_packFloat8,
                            kc);
                store_accum_04(C,
                               &C_accum_00,
                               &C_accum_01,
                               &C_accum_10,
                               &C_accum_11,
                               &C_accum_20,
                               &C_accum_21,
                               &C_accum_30,
                               &C_accum_31,
                               &C_accum_40,
                               &C_accum_41,
                               M);

                break;
            case 6:
                fma_loop_05(blockA_packed,
                            blockB_packed,
                            &C_accum_00,
                            &C_accum_01,
                            &C_accum_10,
                            &C_accum_11,
                            &C_accum_20,
                            &C_accum_21,
                            &C_accum_30,
                            &C_accum_31,
                            &C_accum_40,
                            &C_accum_41,
                            &C_accum_50,
                            &C_accum_51,
                            &a0_packFloat8,
                            &a1_packFloat8,
                            &b_packFloat8,
                            kc);
                store_accum_05(C,
                               &C_accum_00,
                               &C_accum_01,
                               &C_accum_10,
                               &C_accum_11,
                               &C_accum_20,
                               &C_accum_21,
                               &C_accum_30,
                               &C_accum_31,
                               &C_accum_40,
                               &C_accum_41,
                               &C_accum_50,
                               &C_accum_51,
                               M);
                break;
        }
    }
}

#define MULTITHREAD 1

void pack_panelB(const float* __restrict B,
                 float* __restrict blockB_packed,
                 std::size_t nr,
                 std::size_t kc,
                 std::size_t K) noexcept
{
    for(std::size_t p = 0; p < kc; p++)
    {
        for(std::size_t j = 0; j < nr; j++)
        {
            *blockB_packed++ = B[j * K + p];
        }
        for(std::size_t j = nr; j < 6; j++)
        {
            *blockB_packed++ = 0;
        }
    }
}

void pack_blockB(const float* __restrict B,
                 float* __restrict blockB_packed,
                 std::size_t nc,
                 std::size_t kc,
                 std::size_t K) noexcept
{
#if MULTITHREAD
    ThreadPoolWaiter waiter;
#endif /* MULTITHREAD */

    for(std::size_t j = 0; j < nc; j += 6)
    {
#if MULTITHREAD
        global_threadpool().add_work(
            [=]() -> void
            {
#endif /* MULTITHREAD */
                std::size_t nr = std::min(std::size_t(6), nc - j);
                pack_panelB(&B[j * K], &blockB_packed[j * kc], nr, kc, K);
#if MULTITHREAD
            }, &waiter);
#endif /* MULTITHREAD */
    }

#if MULTITHREAD
    waiter.wait();
#endif /* MULTITHREAD */
}

void pack_panelA(const float* __restrict A,
                 float* __restrict blockA_packed,
                 std::size_t mr,
                 std::size_t kc,
                 std::size_t M) noexcept
{
    for(std::size_t p = 0; p < kc; p++)
    {
        for(std::size_t i = 0; i < mr; i++)
        {
            *blockA_packed++ = A[p * M + i];
        }
        for(std::size_t i = mr; i < 16; i++)
        {
            *blockA_packed++ = 0;
        }
    }
}

void pack_blockA(const float* __restrict A,
                 float* __restrict blockA_packed,
                 std::size_t mc,
                 std::size_t kc,
                 std::size_t M) noexcept
{
#if MULTITHREAD
    ThreadPoolWaiter waiter;
#endif /* MULTITHREAD */

    for(std::size_t i = 0; i < mc; i += 16)
    {
#if MULTITHREAD
        global_threadpool().add_work(
            [=]() -> void
            {
#endif /* MULTITHREAD */
                std::size_t mr = std::min(std::size_t(16), mc - i);
                pack_panelA(&A[i], &blockA_packed[i * kc], mr, kc, M);
#if MULTITHREAD
            }, &waiter);
#endif /* MULTITHREAD */
    }

#if MULTITHREAD
    waiter.wait();
#endif /* MULTITHREAD */

}

void matmat_mulf_avx2_kernel(const float* __restrict A,
                             const float* __restrict B,
                             float* __restrict C,
                             std::size_t M,
                             std::size_t K,
                             std::size_t N) noexcept
{
    const std::size_t nthreads = std::min(std::size_t(16),
                                          static_cast<std::size_t>(get_num_procs() * 0.75));
    global_threadpool().set_max_active_workers(nthreads);

    const std::size_t KC = 256;
    const std::size_t MC = 16 * std::max(std::size_t(1), 42 / nthreads) * nthreads;
    const std::size_t NC = (6 * (800 / nthreads)) * nthreads;

    float* blockA_packed = mem_aligned_alloc<float>(MC * KC * sizeof(float), 32);
    float* blockB_packed = mem_aligned_alloc<float>(NC * KC * sizeof(float), 32);

    for(std::size_t j = 0; j < N; j += NC)
    {
        std::size_t nc = std::min(NC, N - j);
        std::size_t kc = std::min(KC, K);

        pack_blockB(&B[j * K], blockB_packed, nc, kc, K);

        for(std::size_t i = 0; i < M; i += MC)
        {
            std::size_t mc = std::min(MC, M - i);

            pack_blockA(&A[i], blockA_packed, mc, kc, M);

#if MULTITHREAD
            ThreadPoolWaiter waiter;
#endif /* MULTITHREAD */
 
            for(std::size_t jr = 0; jr < nc; jr += 6)
            {
#if MULTITHREAD
                global_threadpool().add_work(
                    [=]() -> void
                    {
#endif /* MULTITHREAD */
                        std::size_t global_j = j + jr;
                        std::size_t nr = std::min(std::size_t(6), N - global_j);

                        for(std::size_t ir = 0; ir < mc; ir += 16)
                        {
                            std::size_t global_i = i + ir;
                            std::size_t mr = std::min(std::size_t(16), M - global_i);

                            kernel_16x6_zero_init_accum(&blockA_packed[ir * kc],
                                                        &blockB_packed[jr * kc],
                                                        &C[global_j * M + global_i],
                                                        mr,
                                                        nr,
                                                        kc,
                                                        M);
                        }
#if MULTITHREAD
                    }, &waiter);
#endif /* MULTITHREAD */
            }

#if MULTITHREAD
            waiter.wait();
#endif /* MULTITHREAD */
        }

        for(std::size_t p = kc; p < K; p += KC)
        {
            kc = std::min(KC, K - p);

            pack_blockB(&B[j * K + p], blockB_packed, nc, kc, K);

            for(std::size_t i = 0; i < M; i += MC)
            {
                std::size_t mc = std::min(MC, M - i);

                pack_blockA(&A[p * M + i], blockA_packed, mc, kc, M);

#if MULTITHREAD
                ThreadPoolWaiter waiter;
#endif /* MULTITHREAD */

                for(std::size_t jr = 0; jr < nc; jr += 6)
                {
#if MULTITHREAD
                    global_threadpool().add_work(
                        [=]() -> void
                        {
#endif /* MULTITHREAD */
                            std::size_t global_j = j + jr;
                            std::size_t nr = std::min(std::size_t(6), N - global_j);

                            for(std::size_t ir = 0; ir < mc; ir += 16)
                            {
                                std::size_t global_i = i + ir;
                                std::size_t mr = std::min(std::size_t(16), M - global_i);

                                kernel_16x6_load_accum(&blockA_packed[ir * kc],
                                                       &blockB_packed[jr * kc],
                                                       &C[global_j * M + global_i],
                                                       mr,
                                                       nr,
                                                       kc,
                                                       M);
                            }
#if MULTITHREAD
                        }, &waiter);
#endif /* MULTITHREAD */
                }

#if MULTITHREAD
                waiter.wait();
#endif /* MULTITHREAD */
            }
        }
    }

    mem_aligned_free(blockA_packed);
    mem_aligned_free(blockB_packed);
}

/* Dispatcher */

void detail::matmat_mulf(const float* __restrict A,
                         const float* __restrict B,
                         float* __restrict C,
                         std::size_t M,
                         std::size_t K,
                         std::size_t N) noexcept
{
    switch(simd_get_vectorization_mode())
    {
        case VectorizationMode_Scalar:
        case VectorizationMode_SSE:
        case VectorizationMode_AVX:
            matmat_mulf_scalar_kernel(A, B, C, M, K, N);
            break;

        case VectorizationMode_AVX2:
            matmat_mulf_avx2_kernel(A, B, C, M, K, N);
            break;

        default:
            matmat_mulf_scalar_kernel(A, B, C, M, K, N);
            break;
    }
}

/********************************/
/* Mat-Mat mul for double */
/********************************/

/* Scalar version */

void matmat_muld_scalar_kernel(const double* __restrict A,
                               const double* __restrict B,
                               double* __restrict C,
                               std::size_t M,
                               std::size_t K,
                               std::size_t N) noexcept
{
    for(std::size_t i = 0; i < M; i++)
    {
        for(std::size_t j = 0; j < K; j++)
        {
            double sum = 0.0;

            for(std::size_t k = 0; k < N; k++)
            {
                sum += A[i + k * M] * B[k + j * K];
            }

            C[i * j * M] = sum;
        }
    }
}

/* Dispatcher */

void detail::matmat_muld(const double* __restrict A,
                         const double* __restrict B,
                         double* __restrict C,
                         std::size_t M,
                         std::size_t K,
                         std::size_t N) noexcept
{
    switch(simd_get_vectorization_mode())
    {
        case VectorizationMode_Scalar:
        case VectorizationMode_SSE:
        case VectorizationMode_AVX:
        case VectorizationMode_AVX2:

        default:
            matmat_muld_scalar_kernel(A, B, C, M, K, N);
            break;
    }
}

/********************************/
/* Mat-Mat mul for int */
/********************************/

/* Scalar version */

void matmat_muli_scalar_kernel(const std::int32_t* __restrict A,
                               const std::int32_t* __restrict B,
                               std::int32_t* __restrict C,
                               std::size_t M,
                               std::size_t K,
                               std::size_t N) noexcept
{
    for(std::size_t i = 0; i < M; i++)
    {
        for(std::size_t j = 0; j < K; j++)
        {
            std::int32_t sum = 0;

            for(std::size_t k = 0; k < N; k++)
            {
                /* Should we care about overflow ? */
                sum += A[i + k * M] * B[k + j * K];
            }

            C[i * j * M] = sum;
        }
    }
}

/* Dispatcher */

void detail::matmat_muli(const std::int32_t* __restrict A,
                         const std::int32_t* __restrict B,
                         std::int32_t* __restrict C,
                         std::size_t M,
                         std::size_t K,
                         std::size_t N) noexcept
{
    switch(simd_get_vectorization_mode())
    {
        case VectorizationMode_Scalar:
        case VectorizationMode_SSE:
        case VectorizationMode_AVX:
        case VectorizationMode_AVX2:

        default:
            matmat_muli_scalar_kernel(A, B, C, M, K, N);
            break;
    }
}

STDROMANO_NAMESPACE_END