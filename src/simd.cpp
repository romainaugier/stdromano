// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - Present Romain Augier
// All rights reserved.

#include "stdromano/simd.hpp"
#include "stdromano/cpu.hpp"

#include <cstdlib>
#include <cstring>

STDROMANO_NAMESPACE_BEGIN

static std::uint32_t g_max_vectorization_mode = VectorizationMode_Scalar;
static std::uint32_t g_vectorization_mode = VectorizationMode_Scalar;

static bool g_has_fma = false;
static bool g_has_f16c = false;

static std::uint32_t simd_mode_from_string(const char* name) noexcept
{
    if(std::strcmp(name, "SCALAR") == 0)
        return VectorizationMode_Scalar;
    if(std::strcmp(name, "SSE") == 0)
        return VectorizationMode_SSE;
    if(std::strcmp(name, "AVX") == 0)
        return VectorizationMode_AVX;
    if(std::strcmp(name, "AVX2") == 0)
        return VectorizationMode_AVX2;
    if(std::strcmp(name, "NEON") == 0)
        return VectorizationMode_NEON;

    return VectorizationMode_Max;
}

/*
    cpu_check() fills the cpu features table and must have run before this function
    (both are called from lib_entry, in that order)
*/
void simd_check_vectorization() noexcept
{
#if defined(STDROMANO_INTEL)
    g_has_fma = cpu_has_feature(CPUFeature_FMA3);
    g_has_f16c = cpu_has_feature(CPUFeature_F16C);

    if(cpu_has_feature(CPUFeature_AVX2))
    {
        g_max_vectorization_mode = VectorizationMode_AVX2;
    }
    else if(cpu_has_feature(CPUFeature_AVX))
    {
        g_max_vectorization_mode = VectorizationMode_AVX;
    }
    else if(cpu_has_feature(CPUFeature_SSE))
    {
        g_max_vectorization_mode = VectorizationMode_SSE;
    }
#elif defined(STDROMANO_AARCH64)
    /* fmla and fp16 conversions are part of the mandatory armv8-a FP/AdvSIMD set */
    g_has_fma = cpu_has_feature(CPUFeature_AdvSIMD);
    g_has_f16c = cpu_has_feature(CPUFeature_AdvSIMD);

    if(cpu_has_feature(CPUFeature_NEON))
    {
        g_max_vectorization_mode = VectorizationMode_NEON;
    }
#endif /* defined(STDROMANO_INTEL) */

    g_vectorization_mode = g_max_vectorization_mode;

    const char* env_val = std::getenv("STDROMANO_VECTORIZATION");

    if(env_val != nullptr)
    {
        const std::uint32_t mode = simd_mode_from_string(env_val);

        if(mode != VectorizationMode_Max && simd_mode_is_available(mode))
        {
            g_vectorization_mode = mode;
        }
    }
}

bool simd_has_sse() noexcept
{
#if defined(STDROMANO_INTEL)
    return g_max_vectorization_mode >= VectorizationMode_SSE &&
           g_max_vectorization_mode <= VectorizationMode_AVX2;
#else
    return false;
#endif /* defined(STDROMANO_INTEL) */
}

bool simd_has_avx() noexcept
{
#if defined(STDROMANO_INTEL)
    return g_max_vectorization_mode >= VectorizationMode_AVX &&
           g_max_vectorization_mode <= VectorizationMode_AVX2;
#else
    return false;
#endif /* defined(STDROMANO_INTEL) */
}

bool simd_has_avx2() noexcept
{
#if defined(STDROMANO_INTEL)
    return g_max_vectorization_mode == VectorizationMode_AVX2;
#else
    return false;
#endif /* defined(STDROMANO_INTEL) */
}

bool simd_has_neon() noexcept
{
#if defined(STDROMANO_AARCH64)
    return g_max_vectorization_mode == VectorizationMode_NEON;
#else
    return false;
#endif /* defined(STDROMANO_AARCH64) */
}

bool simd_has_fma() noexcept
{
    return g_has_fma;
}

bool simd_has_f16c() noexcept
{
    return g_has_f16c;
}

bool simd_mode_is_available(std::uint32_t mode) noexcept
{
    if(mode == VectorizationMode_Scalar)
        return true;

#if defined(STDROMANO_INTEL)
    if(mode == VectorizationMode_NEON)
        return false;
#elif defined(STDROMANO_AARCH64)
    if(mode != VectorizationMode_NEON)
        return false;
#else
    return false;
#endif /* defined(STDROMANO_INTEL) */

    return mode <= g_max_vectorization_mode;
}

VectorizationMode simd_get_vectorization_mode() noexcept
{
    return static_cast<VectorizationMode>(g_vectorization_mode);
}

const char* simd_get_vectorization_mode_as_string() noexcept
{
    switch(g_vectorization_mode)
    {
        case VectorizationMode_Scalar:
            return "Scalar";
        case VectorizationMode_SSE:
            return "SSE";
        case VectorizationMode_AVX:
            return "AVX";
        case VectorizationMode_AVX2:
            return "AVX2";
        case VectorizationMode_NEON:
            return "NEON";
        default:
            /* Should never happen */
            return "Unknown";
    }
}

bool simd_force_vectorization_mode(std::uint32_t mode) noexcept
{
    if(!simd_mode_is_available(mode))
    {
        return false;
    }

    g_vectorization_mode = mode;

    return true;
}

STDROMANO_NAMESPACE_END
