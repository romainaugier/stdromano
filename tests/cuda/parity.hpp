// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - Present Romain Augier
// All rights reserved.

#pragma once

#if !defined(__STDROMANO_TESTS_CUDA_PARITY)
#define __STDROMANO_TESTS_CUDA_PARITY

#include "stdromano/linalg/random.hpp"
#include "stdromano/linalg/vector.hpp"
#include "stdromano/maths.hpp"

#include <cstddef>
#include <cstdint>

/*
    The same host/device function computes every value on the host and in a kernel, the test then
    compares both results with a tolerance in ulps per function. The tolerances are the maximum
    error of the CUDA math functions (CUDA C Programming Guide, Mathematical Functions) plus the
    error of the host libm, and 0 when both sides must be bit exact
*/

namespace parity {

struct Input
{
    float x; /* [-100, 100] */
    float p; /* [1e-3, 100] */
    float n; /* [-1, 1] */
    std::uint32_t u;
};

enum Function_ : std::uint32_t
{
    Function_Sqr,
    Function_Rcp,
    Function_RcpSafe,
    Function_Min,
    Function_Max,
    Function_Clamp,
    Function_Lerp,
    Function_Deg2Rad,
    Function_Abs,
    Function_Sign,
    Function_Sqrt,
    Function_Rsqrt,
    Function_Floor,
    Function_Ceil,
    Function_Frac,
    Function_Rint,
    Function_Fma,
    Function_Fms,
    Function_Nfma,
    Function_Nfms,
    Function_Exp,
    Function_Log,
    Function_Log2,
    Function_Pow,
    Function_Sin,
    Function_Cos,
    Function_SincosSin,
    Function_SincosCos,
    Function_Tan,
    Function_Acos,
    Function_Atan2,
    Function_IsNan,
    Function_IsFinite,
    Function_Dot3,
    Function_Length3,
    Function_Normalize3,
    Function_NormalizeSafe3,
    Function_Cross3,
    Function_Angle3,
    Function_Div3,
    Function_Pcg3d,
    Function_Pcg4dFloat,
    Function_WangHash,
    Function_Count,
};

/* A value passes when it is within max_ulps, or within abs_error for the cancellation-prone ones */
struct Tolerance
{
    const char* name;
    std::uint32_t max_ulps;
    float abs_error;
};

/* fma contraction may differ between the host and the device compilers on composite expressions */
static constexpr Tolerance tolerances[Function_Count] = {
    {"sqr", 0, 0.0f},
    {"rcp", 3, 0.0f},
    {"rcp_safe", 0, 0.0f},
    {"min", 0, 0.0f},
    {"max", 0, 0.0f},
    {"clamp", 0, 0.0f},
    {"lerp", 2, 1e-5f},
    {"deg2rad", 0, 0.0f},
    {"abs", 0, 0.0f},
    {"sign", 0, 0.0f},
    {"sqrt", 0, 0.0f},
    {"rsqrt", 4, 0.0f},
    {"floor", 0, 0.0f},
    {"ceil", 0, 0.0f},
    {"frac", 0, 0.0f},
    {"rint", 0, 0.0f},
    {"fma", 0, 0.0f},
    {"fms", 0, 0.0f},
    {"nfma", 0, 0.0f},
    {"nfms", 0, 0.0f},
    {"exp", 3, 0.0f},
    {"log", 2, 0.0f},
    {"log2", 2, 0.0f},
    {"pow", 5, 0.0f},
    {"sin", 3, 0.0f},
    {"cos", 3, 0.0f},
    {"sincos.sin", 3, 0.0f},
    {"sincos.cos", 3, 0.0f},
    {"tan", 5, 0.0f},
    {"acos", 3, 0.0f},
    {"atan2", 4, 0.0f},
    {"isnan", 0, 0.0f},
    {"isfinite", 0, 0.0f},
    {"dot3", 2, 1e-3f},
    {"length3", 2, 0.0f},
    {"normalize3", 6, 0.0f},
    {"normalize_safe3", 3, 0.0f},
    {"cross3", 2, 1e-3f},
    {"angle3", 6, 1e-6f},
    {"div3", 0, 0.0f},
    {"pcg3d", 0, 0.0f},
    {"pcg4d_float", 0, 0.0f},
    {"wang_hash", 0, 0.0f},
};

STDROMANO_HOST_DEVICE inline void evaluate(const Input& in, float* out) noexcept
{
    using namespace stdromano;

    out[Function_Sqr] = maths::sqr(in.x);
    out[Function_Rcp] = maths::rcp(in.p);
    out[Function_RcpSafe] = maths::rcp_safe(in.p);
    out[Function_Min] = maths::min(in.x, in.n);
    out[Function_Max] = maths::max(in.x, in.n);
    out[Function_Clamp] = maths::clamp(in.x, -2.0f, 3.0f);
    out[Function_Lerp] = maths::lerp(in.x, in.p, in.n);
    out[Function_Deg2Rad] = maths::deg2rad(in.x);
    out[Function_Abs] = maths::abs(in.x);
    out[Function_Sign] = maths::sign(in.x);
    out[Function_Sqrt] = maths::sqrt(in.p);
    out[Function_Rsqrt] = maths::rsqrt(in.p);
    out[Function_Floor] = maths::floor(in.x);
    out[Function_Ceil] = maths::ceil(in.x);
    out[Function_Frac] = maths::frac(in.x);
    out[Function_Rint] = maths::rint(in.x);
    out[Function_Fma] = maths::fma(in.x, in.p, in.n);
    out[Function_Fms] = maths::fms(in.x, in.p, in.n);
    out[Function_Nfma] = maths::nfma(in.x, in.p, in.n);
    out[Function_Nfms] = maths::nfms(in.x, in.p, in.n);
    out[Function_Exp] = maths::exp(in.n * 10.0f);
    out[Function_Log] = maths::log(in.p);
    out[Function_Log2] = maths::log2(in.p);
    out[Function_Pow] = maths::pow(in.p, in.n * 4.0f);
    out[Function_Sin] = maths::sin(in.x);
    out[Function_Cos] = maths::cos(in.x);
    maths::sincos(in.x, &out[Function_SincosSin], &out[Function_SincosCos]);
    out[Function_Tan] = maths::tan(in.n);
    out[Function_Acos] = maths::acos(in.n);
    out[Function_Atan2] = maths::atan2(in.x, in.n);
    out[Function_IsNan] = maths::isnan(maths::log(in.x)) ? 1.0f : 0.0f;
    out[Function_IsFinite] = maths::isfinite(in.x * 1e37f) ? 1.0f : 0.0f;

    const Vec3F a(in.x, in.p, in.n);
    const Vec3F b(in.n, in.x, in.p);

    out[Function_Dot3] = dot(a, b);
    out[Function_Length3] = length(a);
    out[Function_Normalize3] = normalize(a).y;
    out[Function_NormalizeSafe3] = normalize_safe(a).z;
    out[Function_Cross3] = cross(a, b).x;
    out[Function_Angle3] = angle(a, b);
    out[Function_Div3] = (a / in.p).x;

    const Vec3U h3 = pcg3d_hash(Vec3U(in.u, in.u ^ 0x9E3779B9U, in.u * 3U));
    out[Function_Pcg3d] = static_cast<float>(h3.x ^ h3.y ^ h3.z);
    out[Function_Pcg4dFloat] = pcg4d_hash_float(Vec4U(in.u, 1U, 2U, in.u >> 7U)).w;
    out[Function_WangHash] = static_cast<float>(wang_hash(in.u));
}

/* Returns false when no CUDA device is available, outputs are Function_Count floats per input */
bool evaluate_on_device(const Input* inputs, const std::size_t count, float* outputs) noexcept;

} // namespace parity

#endif /* !defined(__STDROMANO_TESTS_CUDA_PARITY) */
