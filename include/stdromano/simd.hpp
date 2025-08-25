// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - Present Romain Augier
// All rights reserved.

#pragma once

#if !defined(__STDROMANO_SIMD)
#define __STDROMANO_SIMD

#include "stdromano/stdromano.hpp"

#include <limits>
#include <immintrin.h>

STDROMANO_NAMESPACE_BEGIN

enum VectorizationMode : uint32_t
{
    VectorizationMode_Scalar = 0,
    VectorizationMode_SSE,
    VectorizationMode_AVX,
    VectorizationMode_AVX2,
    VectorizationMode_Max,
};

void simd_check_vectorization() noexcept;

STDROMANO_API bool simd_has_sse() noexcept;

STDROMANO_API bool simd_has_avx() noexcept;

STDROMANO_API bool simd_has_avx2() noexcept;

STDROMANO_API bool simd_has_fma() noexcept;

STDROMANO_API bool simd_has_f16c() noexcept;

STDROMANO_API VectorizationMode simd_get_vectorization_mode() noexcept;

STDROMANO_API const char* simd_get_vectorization_mode_as_string() noexcept;

STDROMANO_API bool simd_force_vectorization_mode(std::uint32_t mode) noexcept;

/* SIMD helper functions */

STDROMANO_FORCE_INLINE __m128 _mm_abs_ps(const __m128& x) noexcept
{
    static const __m128 sign_mask = _mm_set1_ps(-0.0f);
    return _mm_andnot_ps(sign_mask, x);
}

STDROMANO_FORCE_INLINE __m256 _mm256_abs_ps(const __m256& x) noexcept
{
    static const __m256 sign_mask = _mm256_set1_ps(-0.0f);
    return _mm256_andnot_ps(sign_mask, x);
}

STDROMANO_FORCE_INLINE __m128 _mm_cvtepu32_ps(const __m128i& x) noexcept
{
    static const __m128i sign_mask = _mm_set1_epi32(0x80000000);
    static const __m128 imax = _mm_set1_ps(static_cast<float>(std::numeric_limits<std::int32_t>::max()));

    const __m128i lower = _mm_andnot_si128(sign_mask, x);
    const __m128 f = _mm_cvtepi32_ps(lower);
    const __m128i sign_bits = _mm_and_si128(x, sign_mask);
    const __m128 add = _mm_castsi128_ps(_mm_cmpeq_epi32(sign_bits, sign_mask));

    return _mm_add_ps(f, _mm_and_ps(imax, add));
}

STDROMANO_FORCE_INLINE __m256 _mm256_cvtepu32_ps(const __m256i& x) noexcept
{
    static const __m256i sign_mask = _mm256_set1_epi32(0x80000000);
    static const __m256 imax = _mm256_set1_ps(static_cast<float>(std::numeric_limits<std::int32_t>::max()));

    const __m256i lower = _mm256_andnot_si256(sign_mask, x);
    const __m256 f = _mm256_cvtepi32_ps(lower);
    const __m256i sign_bits = _mm256_and_si256(x, sign_mask);
    const __m256 add = _mm256_castsi256_ps(_mm256_cmpeq_epi32(sign_bits, sign_mask));

    return _mm256_add_ps(f, _mm256_and_ps(imax, add));
}

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

STDROMANO_FORCE_INLINE __m256i _mm256_cmplt_epi8(const __m256i a, const __m256i b) noexcept
{
    return _mm256_cmpgt_epi8(b, a);
}

STDROMANO_FORCE_INLINE __m256i _mm256_cmple_epi8(const __m256i a, const __m256i b) noexcept
{
    return _mm256_xor_si256(_mm256_cmpgt_epi8(a, b), _mm256_set1_epi8(-1));
}

STDROMANO_FORCE_INLINE __m256i _mm256_cmpge_epi8(const __m256i a, const __m256i b) noexcept
{
    return _mm256_xor_si256(_mm256_cmpgt_epi8(b, a), _mm256_set1_epi8(-1));
}

STDROMANO_NAMESPACE_END

#endif /* !defined(__STDROMANO_SIMD) */