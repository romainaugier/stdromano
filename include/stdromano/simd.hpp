// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - Present Romain Augier
// All rights reserved.

#pragma once

#if !defined(__STDROMANO_SIMD)
#define __STDROMANO_SIMD

#include "stdromano/stdromano.hpp"

#include <immintrin.h>

STDROMANO_NAMESPACE_BEGIN

enum VectorizationMode : uint32_t
{
    VectorizationMode_Scalar = 0,
    VectorizationMode_SSE = 1,
    VectorizationMode_AVX = 2,
    VectorizationMode_AVX2 = 3,
};

void simd_check_vectorization() noexcept;

STDROMANO_API bool simd_has_sse() noexcept;

STDROMANO_API bool simd_has_avx() noexcept;

STDROMANO_API bool simd_has_avx2() noexcept;

STDROMANO_API VectorizationMode simd_get_vectorization_mode() noexcept;

STDROMANO_API const char* simd_get_vectorization_mode_as_string() noexcept;

STDROMANO_API bool simd_force_vectorization_mode(const VectorizationMode mode) noexcept;

/* SIMD helper functions */

/* Horizontal sums (sum the entire vector to a single element) */
/* https://stackoverflow.com/questions/6996764/fastest-way-to-do-horizontal-sse-vector-sum-or-other-reduction
 */

STDROMANO_FORCE_INLINE float _mm_hsum_ps(const __m128 x) noexcept
{
    __m128 shuf = _mm_shuffle_ps(x, x, _MM_SHUFFLE(2, 3, 0, 1));
    __m128 sums = _mm_add_ps(x, shuf);
    shuf = _mm_movehl_ps(shuf, sums);
    sums = _mm_add_ss(sums, shuf);
    return _mm_cvtss_f32(sums);
}

STDROMANO_FORCE_INLINE float _mm256_hsum_ps(const __m256 x) noexcept
{
    const __m128 hi = _mm256_extractf128_ps(x, 1);
    const __m128 lo = _mm256_castps256_ps128(x);
    __m128 sum = _mm_add_ps(hi, lo);
    sum = _mm_hadd_ps(sum, sum);
    sum = _mm_hadd_ps(sum, sum);
    return _mm_cvtss_f32(sum);
}

STDROMANO_NAMESPACE_END

#endif /* !defined(__STDROMANO_SIMD) */