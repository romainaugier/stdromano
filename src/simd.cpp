// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - Present Romain Augier
// All rights reserved.

#include "stdromano/simd.hpp"
#include "stdromano/cpu.hpp"

#include <cstdlib>
#include <cstring>

STDROMANO_NAMESPACE_BEGIN

static uint32_t _g_max_vectorization_mode = VectorizationMode_Scalar;
static uint32_t _g_vectorization_mode = VectorizationMode_Scalar;

void simd_check_vectorization() noexcept
{
    int regs[4];

    cpuid(regs, 1);

    const bool has_sse = (regs[3] & (1 << 25)) != 0;
    const bool has_avx = (regs[2] & (1 << 28)) != 0;

    bool has_avx2 = false;

    if(has_avx)
    {
        cpuidex(regs, 7, 0);
        has_avx2 = (regs[1] & (1 << 5)) != 0;
    }

    if(has_avx2)
    {
        _g_max_vectorization_mode = VectorizationMode_AVX2;
    }
    else if(has_avx)
    {
        _g_max_vectorization_mode = VectorizationMode_AVX;
    }
    else if(has_sse)
    {
        _g_max_vectorization_mode = VectorizationMode_SSE;
    }

    _g_vectorization_mode = _g_max_vectorization_mode;

    if(std::getenv("STDROMANO_VECTORIZATION") != nullptr)
    {
        const char* env_val = getenv("STDROMANO_VECTORIZATION");

        if(std::strcmp(env_val, "SCALAR") == 0)
        {
            return;
        }
        else if(std::strcmp(env_val, "SSE") == 0 && has_sse)
        {
            _g_vectorization_mode = VectorizationMode_SSE;
        }
        else if(std::strcmp(env_val, "AVX") == 0 && has_avx)
        {
            _g_vectorization_mode = VectorizationMode_AVX;
        }
        else if(std::strcmp(env_val, "AVX2") == 0 && has_avx2)
        {
            _g_vectorization_mode = VectorizationMode_AVX2;
        }
    }
}

bool simd_has_sse() noexcept
{
    return _g_max_vectorization_mode >= 1;
}

bool simd_has_avx() noexcept
{
    return _g_max_vectorization_mode >= 2;
}

bool simd_has_avx2() noexcept
{
    return _g_max_vectorization_mode >= 3;
}

VectorizationMode simd_get_vectorization_mode() noexcept
{
    return static_cast<VectorizationMode>(_g_vectorization_mode);
}

const char* simd_get_vectorization_mode_as_string() noexcept
{
    switch(_g_vectorization_mode)
    {
        case VectorizationMode_Scalar:
            return "Scalar";
        case VectorizationMode_SSE:
            return "SSE";
        case VectorizationMode_AVX:
            return "AVX";
        case VectorizationMode_AVX2:
            return "AVX2";
        default:
            /* Should never happen */
            return "Unknown";
    }
}

bool simd_force_vectorization_mode(const VectorizationMode mode) noexcept
{
    if(mode > _g_max_vectorization_mode)
    {
        return false;
    }

    _g_vectorization_mode = mode;

    return true;
}

STDROMANO_NAMESPACE_END