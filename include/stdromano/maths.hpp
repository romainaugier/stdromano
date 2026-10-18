// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - Present Romain Augier
// All rights reserved.

#pragma once

#if !defined(__STDROMANO_MATHS)
#define __STDROMANO_MATHS

#include "stdromano/stdromano.hpp"

#if !defined(STDROMANO_CUDA_DEVICE)
#if defined(STDROMANO_X86_64)
#include <immintrin.h>
#elif defined(STDROMANO_AARCH64)
#include <arm_neon.h>
#endif /* defined(STDROMANO_X86_64) */
#endif /* !defined(STDROMANO_CUDA_DEVICE) */

#include <limits>
#include <cmath>
#include <type_traits>

#if defined(STDROMANO_GCC) || defined(STDROMANO_CLANG)
#include <math.h>
#endif

/*
    __AVX2__ was used as a proxy for the fma3 intrinsics, but the right macro is __FMA__
    (gcc and clang define it with -mfma, msvc only defines __AVX2__)
*/
#if defined(STDROMANO_X86_64) && (defined(__FMA__) || defined(__AVX2__))
#define STDROMANO_HAS_FMA_INTRINSICS
#endif /* defined(STDROMANO_X86_64) && (defined(__FMA__) || defined(__AVX2__)) */

/* msvc has no __has_builtin, and "defined(__has_builtin) && __has_builtin(x)" does not preprocess */
#if defined(__has_builtin)
#if __has_builtin(__builtin_sincosf) && __has_builtin(__builtin_sincos)
#define STDROMANO_HAS_BUILTIN_SINCOS
#endif /* __has_builtin(__builtin_sincosf) && __has_builtin(__builtin_sincos) */
#endif /* defined(__has_builtin) */

/*
    Every function is usable from host and device code. On the device, functions mapping to libm
    (sqrt, exp, sin...) call the CUDA math library and follow the compilation flags (with
    --use_fast_math, sin becomes __sinf, sqrt becomes sqrt.approx...), while the functions that
    are approximations by definition (rcp, rsqrt) and the fused ones (fma...) always use the
    hardware instructions. rcp_safe is correctly rounded on both sides, whatever the flags
*/

#define MATHS_NAMESPACE_BEGIN namespace maths {
#define MATHS_NAMESPACE_END }

STDROMANO_NAMESPACE_BEGIN
MATHS_NAMESPACE_BEGIN

template<typename T>
struct constants;

template<>
struct constants<float>
{
    static constexpr float pi = 3.14159265358979323846f;
    static constexpr float two_pi = 2.0f * 3.14159265358979323846f;
    static constexpr float pi_over_two = 1.57079632679489661923f;
    static constexpr float pi_over_four = 0.785398163397448309616f;
    static constexpr float one_over_pi = 0.318309886183790671538f;
    static constexpr float two_over_pi = 0.636619772367581343076f;
    static constexpr float inv_pi = one_over_pi;
    static constexpr float inv_two_pi = 0.15915494309189533577f;
    static constexpr float inv_four_pi = 0.07957747154594766788f;

    static constexpr float deg_to_rad = 0.0174532925199432957692f;
    static constexpr float rad_to_deg = 57.2957795130823208768f;

    static constexpr float inf = std::numeric_limits<float>::infinity();
    static constexpr float neginf = -std::numeric_limits<float>::infinity();

    static constexpr float sqrt2 = 1.41421356237309504880f;
    static constexpr float one_over_sqrt2 = 0.707106781186547524401f;

    static constexpr float e = 2.71828182845904523536f;

    static constexpr float log2e = 1.44269504088896340736f;
    static constexpr float log10e = 0.434294481903251827651f;
    static constexpr float ln2 = 0.693147180559945309417f;
    static constexpr float ln10 = 2.30258509299404568402f;

    static constexpr float zero = 0.0f;
    static constexpr float one = 1.0f;
    static constexpr float min_float = std::numeric_limits<float>::lowest();
    static constexpr float max_float = std::numeric_limits<float>::max();
    static constexpr float very_far = max_float;

    static constexpr float large_epsilon = 0.001f;
    static constexpr float epsilon = std::numeric_limits<float>::epsilon();
};

template<>
struct constants<double>
{
    static constexpr double pi = 3.14159265358979323846;
    static constexpr double two_pi = 2.0 * 3.14159265358979323846;
    static constexpr double pi_over_two = 1.57079632679489661923;
    static constexpr double pi_over_four = 0.785398163397448309616;
    static constexpr double one_over_pi = 0.318309886183790671538;
    static constexpr double two_over_pi = 0.636619772367581343076;
    static constexpr double inv_pi = one_over_pi;
    static constexpr double inv_two_pi = 0.15915494309189533577;
    static constexpr double inv_four_pi = 0.07957747154594766788;

    static constexpr double deg_to_rad = 0.0174532925199432957692;
    static constexpr double rad_to_deg = 57.2957795130823208768;

    static constexpr double inf = std::numeric_limits<double>::infinity();
    static constexpr double neginf = -std::numeric_limits<double>::infinity();

    static constexpr double sqrt2 = 1.41421356237309504880;
    static constexpr double one_over_sqrt2 = 0.707106781186547524401;

    static constexpr double e = 2.71828182845904523536;

    static constexpr double log2e = 1.44269504088896340736;
    static constexpr double log10e = 0.434294481903251827651;
    static constexpr double ln2 = 0.693147180559945309417;
    static constexpr double ln10 = 2.30258509299404568402;

    static constexpr double zero = 0.0;
    static constexpr double one = 1.0;
    static constexpr double min_double = std::numeric_limits<double>::lowest();
    static constexpr double max_double = std::numeric_limits<double>::max();
    static constexpr double very_far = max_double;

    static constexpr double large_epsilon = 0.00001;
    static constexpr double epsilon = std::numeric_limits<double>::epsilon();
};

/******************************************/
template<typename T>
STDROMANO_HOST_DEVICE STDROMANO_FORCE_INLINE bool isinf(const T x) noexcept;

template<>
STDROMANO_HOST_DEVICE STDROMANO_FORCE_INLINE bool isinf(const float x) noexcept
{
#if defined(STDROMANO_CUDA_DEVICE)
    return (__float_as_uint(x) & 0x7FFFFFFFU) == 0x7F800000U;
#elif defined(STDROMANO_WIN)
    return _finite(x) == 0 && _isnan(x) == 0;
#else
    return __builtin_isinf(x) != 0;
#endif /* defined(STDROMANO_CUDA_DEVICE) */
}

template<>
STDROMANO_HOST_DEVICE STDROMANO_FORCE_INLINE bool isinf(const double x) noexcept
{
#if defined(STDROMANO_CUDA_DEVICE)
    return (static_cast<unsigned long long>(__double_as_longlong(x)) & 0x7FFFFFFFFFFFFFFFULL) ==
           0x7FF0000000000000ULL;
#elif defined(STDROMANO_WIN)
    return _finite(x) == 0 && _isnan(x) == 0;
#else
    return __builtin_isinf(x) != 0;
#endif /* defined(STDROMANO_CUDA_DEVICE) */
}

/******************************************/
template<typename T>
STDROMANO_HOST_DEVICE STDROMANO_FORCE_INLINE bool isnan(const T x) noexcept;

template<>
STDROMANO_HOST_DEVICE STDROMANO_FORCE_INLINE bool isnan(const float x) noexcept
{
#if defined(STDROMANO_CUDA_DEVICE)
    return (__float_as_uint(x) & 0x7FFFFFFFU) > 0x7F800000U;
#elif defined(STDROMANO_WIN)
    return _isnan(x) != 0;
#else
    return __builtin_isnan(x) != 0;
#endif /* defined(STDROMANO_CUDA_DEVICE) */
}

template<>
STDROMANO_HOST_DEVICE STDROMANO_FORCE_INLINE bool isnan(const double x) noexcept
{
#if defined(STDROMANO_CUDA_DEVICE)
    return (static_cast<unsigned long long>(__double_as_longlong(x)) & 0x7FFFFFFFFFFFFFFFULL) >
           0x7FF0000000000000ULL;
#elif defined(STDROMANO_WIN)
    return _isnan(x) != 0;
#else
    return __builtin_isnan(x) != 0;
#endif /* defined(STDROMANO_CUDA_DEVICE) */
}

/******************************************/
template<typename T>
STDROMANO_HOST_DEVICE STDROMANO_FORCE_INLINE bool isfinite(const T x) noexcept;

template<>
STDROMANO_HOST_DEVICE STDROMANO_FORCE_INLINE bool isfinite(const float x) noexcept
{
#if defined(STDROMANO_CUDA_DEVICE)
    return (__float_as_uint(x) & 0x7F800000U) != 0x7F800000U;
#elif defined(STDROMANO_WIN)
    return _finite(x) != 0;
#else
    return __builtin_isfinite(x) != 0;
#endif /* defined(STDROMANO_CUDA_DEVICE) */
}

template<>
STDROMANO_HOST_DEVICE STDROMANO_FORCE_INLINE bool isfinite(const double x) noexcept
{
#if defined(STDROMANO_CUDA_DEVICE)
    return (static_cast<unsigned long long>(__double_as_longlong(x)) & 0x7FF0000000000000ULL) !=
           0x7FF0000000000000ULL;
#elif defined(STDROMANO_WIN)
    return _finite(x) != 0;
#else
    return __builtin_isfinite(x) != 0;
#endif /* defined(STDROMANO_CUDA_DEVICE) */
}

/******************************************/
template<typename T>
STDROMANO_HOST_DEVICE constexpr STDROMANO_FORCE_INLINE T sqr(const T x) noexcept { return x * x; }

/******************************************/
template<typename T>
STDROMANO_HOST_DEVICE STDROMANO_FORCE_INLINE T rcp(const T x) noexcept;

template<>
STDROMANO_HOST_DEVICE STDROMANO_FORCE_INLINE float rcp(const float x) noexcept
{
#if defined(STDROMANO_CUDA_DEVICE)
    return __fdividef(1.0f, x);
#elif defined(STDROMANO_X86_64)
    const __m128 a = _mm_set_ss(x);
    const __m128 r = _mm_rcp_ss(a);

#if defined(STDROMANO_HAS_FMA_INTRINSICS)
    return _mm_cvtss_f32(_mm_mul_ss(r, _mm_fnmadd_ss(r, a, _mm_set_ss(2.0f))));
#else
    return _mm_cvtss_f32(_mm_mul_ss(r, _mm_sub_ss(_mm_set_ss(2.0f), _mm_mul_ss(r, a))));
#endif /* defined(STDROMANO_HAS_FMA_INTRINSICS) */
#elif defined(STDROMANO_AARCH64)
    /* frecpe only gives ~8 bits, two Newton-Raphson steps (frecps) to match the x86 accuracy */
    float r = vrecpes_f32(x);
    r = r * vrecpss_f32(x, r);
    r = r * vrecpss_f32(x, r);
    return r;
#else
    return 1.0f / x;
#endif /* defined(STDROMANO_CUDA_DEVICE) */
}

template<>
STDROMANO_HOST_DEVICE STDROMANO_FORCE_INLINE double rcp(const double x) noexcept
{
#if defined(STDROMANO_CUDA_DEVICE)
    return __drcp_rn(x);
#else
    return 1.0 / x;
#endif /* defined(STDROMANO_CUDA_DEVICE) */
}

/******************************************/
template<typename T>
STDROMANO_HOST_DEVICE constexpr STDROMANO_FORCE_INLINE T rcp_safe(const T a) noexcept
{
#if defined(STDROMANO_CUDA_DEVICE)
    /* --use_fast_math turns 1.0f / a into an approximation, __frcp_rn keeps it exact */
    if constexpr(std::is_same_v<T, float>)
        return __frcp_rn(a);
    else if constexpr(std::is_same_v<T, double>)
        return __drcp_rn(a);
    else
        return constants<T>::one / a;
#else
    return constants<T>::one / a;
#endif /* defined(STDROMANO_CUDA_DEVICE) */
}

/******************************************/
template<typename T>
STDROMANO_HOST_DEVICE constexpr STDROMANO_FORCE_INLINE T min(const T a, const T b) noexcept
{
#if defined(STDROMANO_CUDA_DEVICE)
    /* a < b ? a : b is a setp and a selp, fminf is a single min instruction */
    if constexpr(std::is_same_v<T, float>)
        return ::fminf(a, b);
    else if constexpr(std::is_same_v<T, double>)
        return ::fmin(a, b);
    else
        return a < b ? a : b;
#else
    return a < b ? a : b;
#endif /* defined(STDROMANO_CUDA_DEVICE) */
}

/******************************************/
template<typename T>
STDROMANO_HOST_DEVICE constexpr STDROMANO_FORCE_INLINE T max(const T a, const T b) noexcept
{
#if defined(STDROMANO_CUDA_DEVICE)
    if constexpr(std::is_same_v<T, float>)
        return ::fmaxf(a, b);
    else if constexpr(std::is_same_v<T, double>)
        return ::fmax(a, b);
    else
        return a > b ? a : b;
#else
    return a > b ? a : b;
#endif /* defined(STDROMANO_CUDA_DEVICE) */
}

/******************************************/
template<typename T>
STDROMANO_HOST_DEVICE constexpr STDROMANO_FORCE_INLINE T fit(const T s,
                                                             const T a1,
                                                             const T a2,
                                                             const T b1,
                                                             const T b2) noexcept
{
    return b1 + ((s - a1) * (b2 - b1)) / (a2 - a1);
}

/******************************************/
template<typename T>
STDROMANO_HOST_DEVICE constexpr STDROMANO_FORCE_INLINE T fit01(const T x, const T a, const T b) noexcept
{
    return x * (b - a) + a;
}

/******************************************/
template<typename T>
STDROMANO_HOST_DEVICE constexpr STDROMANO_FORCE_INLINE T lerp(const T a, const T b, const T t) noexcept
{
    return (constants<T>::one - t) * a + t * b;
}

/******************************************/
template<typename T>
STDROMANO_HOST_DEVICE constexpr STDROMANO_FORCE_INLINE T clamp(const T x,
                                                               const T lower = constants<T>::zero,
                                                               const T upper = constants<T>::one) noexcept
{
    return max(lower, min(x, upper));
}

/******************************************/
template<typename T>
STDROMANO_HOST_DEVICE constexpr STDROMANO_FORCE_INLINE T clampz(const T x,
                                                                const T upper = constants<T>::one) noexcept
{
    return max(constants<T>::zero, min(x, upper));
}

/******************************************/
template<typename T>
STDROMANO_HOST_DEVICE constexpr STDROMANO_FORCE_INLINE T deg2rad(const T deg) noexcept
{
    return deg * constants<T>::deg_to_rad;
}

/******************************************/
template<typename T>
STDROMANO_HOST_DEVICE constexpr STDROMANO_FORCE_INLINE T rad2deg(const T rad) noexcept
{
    return rad * constants<T>::rad_to_deg;
}

/******************************************/
template<typename T>
STDROMANO_HOST_DEVICE STDROMANO_FORCE_INLINE T abs(const T x) noexcept;

template<>
STDROMANO_HOST_DEVICE STDROMANO_FORCE_INLINE float abs(const float x) noexcept { return ::fabsf(x); }

template<>
STDROMANO_HOST_DEVICE STDROMANO_FORCE_INLINE double abs(const double x) noexcept { return ::fabs(x); }

/******************************************/
template<typename T>
STDROMANO_HOST_DEVICE STDROMANO_FORCE_INLINE T copysign(const T mag, const T sgn) noexcept;

template<>
STDROMANO_HOST_DEVICE STDROMANO_FORCE_INLINE float copysign(const float mag, const float sgn) noexcept
{
    return ::copysignf(mag, sgn);
}

template<>
STDROMANO_HOST_DEVICE STDROMANO_FORCE_INLINE double copysign(const double mag, const double sgn) noexcept
{
    return ::copysign(mag, sgn);
}

/******************************************/
/* 1 or -1 following the sign bit (so -0 gives -1), branchless on both sides */
template<typename T>
STDROMANO_HOST_DEVICE STDROMANO_FORCE_INLINE T sign(const T x) noexcept
{
    return copysign(constants<T>::one, x);
}

/******************************************/
template<typename T>
STDROMANO_HOST_DEVICE STDROMANO_FORCE_INLINE T exp(const T x) noexcept;

template<>
STDROMANO_HOST_DEVICE STDROMANO_FORCE_INLINE float exp(const float x) noexcept { return ::expf(x); }

template<>
STDROMANO_HOST_DEVICE STDROMANO_FORCE_INLINE double exp(const double x) noexcept { return ::exp(x); }

/******************************************/
template<typename T>
STDROMANO_HOST_DEVICE STDROMANO_FORCE_INLINE T sqrt(const T x) noexcept;

template<>
STDROMANO_HOST_DEVICE STDROMANO_FORCE_INLINE float sqrt(const float x) noexcept { return ::sqrtf(x); }

template<>
STDROMANO_HOST_DEVICE STDROMANO_FORCE_INLINE double sqrt(const double x) noexcept { return ::sqrt(x); }

/******************************************/
template<typename T>
STDROMANO_HOST_DEVICE STDROMANO_FORCE_INLINE T rsqrt(const T x) noexcept;

template<>
STDROMANO_HOST_DEVICE STDROMANO_FORCE_INLINE float rsqrt(const float x) noexcept
{
#if defined(STDROMANO_CUDA_DEVICE)
    return ::rsqrtf(x);
#elif defined(STDROMANO_X86_64)
    const __m128 a = _mm_set_ss(x);
    __m128 r = _mm_rsqrt_ss(a);
    r = _mm_add_ss(_mm_mul_ss(_mm_set_ss(1.5f), r),
                   _mm_mul_ss(_mm_mul_ss(_mm_mul_ss(a, _mm_set_ss(-0.5f)), r), _mm_mul_ss(r, r)));
    return _mm_cvtss_f32(r);
#elif defined(STDROMANO_AARCH64)
    float r = vrsqrtes_f32(x);
    r = r * vrsqrtss_f32(x, r * r);
    r = r * vrsqrtss_f32(x, r * r);
    return r;
#else
    return 1.0f / ::sqrtf(x);
#endif /* defined(STDROMANO_CUDA_DEVICE) */
}

template<>
STDROMANO_HOST_DEVICE STDROMANO_FORCE_INLINE double rsqrt(const double x) noexcept
{
#if defined(STDROMANO_CUDA_DEVICE)
    return ::rsqrt(x);
#else
    return 1.0 / ::sqrt(x);
#endif /* defined(STDROMANO_CUDA_DEVICE) */
}

/******************************************/
template<typename T>
STDROMANO_HOST_DEVICE STDROMANO_FORCE_INLINE T fmod(const T x, const T y) noexcept;

template<>
STDROMANO_HOST_DEVICE STDROMANO_FORCE_INLINE float fmod(const float x, const float y) noexcept { return ::fmodf(x, y); }

template<>
STDROMANO_HOST_DEVICE STDROMANO_FORCE_INLINE double fmod(const double x, const double y) noexcept { return ::fmod(x, y); }

/******************************************/
template<typename T>
STDROMANO_HOST_DEVICE STDROMANO_FORCE_INLINE T log(const T x) noexcept;

template<>
STDROMANO_HOST_DEVICE STDROMANO_FORCE_INLINE float log(const float x) noexcept { return ::logf(x); }

template<>
STDROMANO_HOST_DEVICE STDROMANO_FORCE_INLINE double log(const double x) noexcept { return ::log(x); }

/******************************************/
template<typename T>
STDROMANO_HOST_DEVICE STDROMANO_FORCE_INLINE T log2(const T x) noexcept;

template<>
STDROMANO_HOST_DEVICE STDROMANO_FORCE_INLINE float log2(const float x) noexcept { return ::log2f(x); }

template<>
STDROMANO_HOST_DEVICE STDROMANO_FORCE_INLINE double log2(const double x) noexcept { return ::log2(x); }

/******************************************/
template<typename T>
STDROMANO_HOST_DEVICE STDROMANO_FORCE_INLINE T log10(const T x) noexcept;

template<>
STDROMANO_HOST_DEVICE STDROMANO_FORCE_INLINE float log10(const float x) noexcept { return ::log10f(x); }

template<>
STDROMANO_HOST_DEVICE STDROMANO_FORCE_INLINE double log10(const double x) noexcept { return ::log10(x); }

/******************************************/
template<typename T>
STDROMANO_HOST_DEVICE STDROMANO_FORCE_INLINE T pow(const T x, const T y) noexcept;

template<>
STDROMANO_HOST_DEVICE STDROMANO_FORCE_INLINE float pow(const float x, const float y) noexcept { return ::powf(x, y); }

template<>
STDROMANO_HOST_DEVICE STDROMANO_FORCE_INLINE double pow(const double x, const double y) noexcept { return ::pow(x, y); }

/******************************************/
template<typename T>
STDROMANO_HOST_DEVICE STDROMANO_FORCE_INLINE T floor(const T x) noexcept;

template<>
STDROMANO_HOST_DEVICE STDROMANO_FORCE_INLINE float floor(const float x) noexcept { return ::floorf(x); }

template<>
STDROMANO_HOST_DEVICE STDROMANO_FORCE_INLINE double floor(const double x) noexcept { return ::floor(x); }

/******************************************/
template<typename T>
STDROMANO_HOST_DEVICE STDROMANO_FORCE_INLINE T ceil(const T x) noexcept;

template<>
STDROMANO_HOST_DEVICE STDROMANO_FORCE_INLINE float ceil(const float x) noexcept { return ::ceilf(x); }

template<>
STDROMANO_HOST_DEVICE STDROMANO_FORCE_INLINE double ceil(const double x) noexcept { return ::ceil(x); }

/******************************************/
template<typename T>
STDROMANO_HOST_DEVICE STDROMANO_FORCE_INLINE T frac(const T x) noexcept { return x - floor(x); }

/******************************************/
template<typename T>
STDROMANO_HOST_DEVICE STDROMANO_FORCE_INLINE T rint(const T x) noexcept;

template<>
STDROMANO_HOST_DEVICE STDROMANO_FORCE_INLINE float rint(const float x) noexcept { return ::rintf(x); }

template<>
STDROMANO_HOST_DEVICE STDROMANO_FORCE_INLINE double rint(const double x) noexcept { return ::rint(x); }

/******************************************/
template<typename T>
STDROMANO_HOST_DEVICE STDROMANO_FORCE_INLINE T acos(const T x) noexcept;

template<>
STDROMANO_HOST_DEVICE STDROMANO_FORCE_INLINE float acos(const float x) noexcept { return ::acosf(x); }

template<>
STDROMANO_HOST_DEVICE STDROMANO_FORCE_INLINE double acos(const double x) noexcept { return ::acos(x); }

/******************************************/
template<typename T>
STDROMANO_HOST_DEVICE STDROMANO_FORCE_INLINE T asin(const T x) noexcept;

template<>
STDROMANO_HOST_DEVICE STDROMANO_FORCE_INLINE float asin(const float x) noexcept { return ::asinf(x); }

template<>
STDROMANO_HOST_DEVICE STDROMANO_FORCE_INLINE double asin(const double x) noexcept { return ::asin(x); }

/******************************************/
template<typename T>
STDROMANO_HOST_DEVICE STDROMANO_FORCE_INLINE T atan(const T x) noexcept;

template<>
STDROMANO_HOST_DEVICE STDROMANO_FORCE_INLINE float atan(const float x) noexcept { return ::atanf(x); }

template<>
STDROMANO_HOST_DEVICE STDROMANO_FORCE_INLINE double atan(const double x) noexcept { return ::atan(x); }

/******************************************/
template<typename T>
STDROMANO_HOST_DEVICE STDROMANO_FORCE_INLINE T atan2(const T y, const T x) noexcept;

template<>
STDROMANO_HOST_DEVICE STDROMANO_FORCE_INLINE float atan2(const float y, const float x) noexcept { return ::atan2f(y, x); }

template<>
STDROMANO_HOST_DEVICE STDROMANO_FORCE_INLINE double atan2(const double y, const double x) noexcept { return ::atan2(y, x); }

/******************************************/
template<typename T>
STDROMANO_HOST_DEVICE STDROMANO_FORCE_INLINE T cos(const T x) noexcept;

template<>
STDROMANO_HOST_DEVICE STDROMANO_FORCE_INLINE float cos(const float x) noexcept { return ::cosf(x); }

template<>
STDROMANO_HOST_DEVICE STDROMANO_FORCE_INLINE double cos(const double x) noexcept { return ::cos(x); }

/******************************************/
template<typename T>
STDROMANO_HOST_DEVICE STDROMANO_FORCE_INLINE T sin(const T x) noexcept;

template<>
STDROMANO_HOST_DEVICE STDROMANO_FORCE_INLINE float sin(const float x) noexcept { return ::sinf(x); }

template<>
STDROMANO_HOST_DEVICE STDROMANO_FORCE_INLINE double sin(const double x) noexcept { return ::sin(x); }

/******************************************/
template<typename T>
STDROMANO_HOST_DEVICE STDROMANO_FORCE_INLINE T tan(const T x) noexcept;

template<>
STDROMANO_HOST_DEVICE STDROMANO_FORCE_INLINE float tan(const float x) noexcept { return ::tanf(x); }

template<>
STDROMANO_HOST_DEVICE STDROMANO_FORCE_INLINE double tan(const double x) noexcept { return ::tan(x); }

/******************************************/
template<typename T>
STDROMANO_HOST_DEVICE STDROMANO_FORCE_INLINE T cosh(const T x) noexcept;

template<>
STDROMANO_HOST_DEVICE STDROMANO_FORCE_INLINE float cosh(const float x) noexcept { return ::coshf(x); }

template<>
STDROMANO_HOST_DEVICE STDROMANO_FORCE_INLINE double cosh(const double x) noexcept { return ::cosh(x); }

/******************************************/
template<typename T>
STDROMANO_HOST_DEVICE STDROMANO_FORCE_INLINE T sinh(const T x) noexcept;

template<>
STDROMANO_HOST_DEVICE STDROMANO_FORCE_INLINE float sinh(const float x) noexcept { return ::sinhf(x); }

template<>
STDROMANO_HOST_DEVICE STDROMANO_FORCE_INLINE double sinh(const double x) noexcept { return ::sinh(x); }

/******************************************/
template<typename T>
STDROMANO_HOST_DEVICE STDROMANO_FORCE_INLINE T tanh(const T x) noexcept;

template<>
STDROMANO_HOST_DEVICE STDROMANO_FORCE_INLINE float tanh(const float x) noexcept { return ::tanhf(x); }

template<>
STDROMANO_HOST_DEVICE STDROMANO_FORCE_INLINE double tanh(const double x) noexcept { return ::tanh(x); }

/******************************************/
template<typename T>
STDROMANO_HOST_DEVICE STDROMANO_FORCE_INLINE void sincos(const T theta, T* s, T* c) noexcept;

template<>
STDROMANO_HOST_DEVICE STDROMANO_FORCE_INLINE void sincos(const float theta, float* s, float* c) noexcept
{
#if defined(STDROMANO_CUDA_DEVICE)
    ::sincosf(theta, s, c);
#elif defined(STDROMANO_HAS_BUILTIN_SINCOS)
    __builtin_sincosf(theta, s, c);
#else
    *s = ::sinf(theta);
    *c = ::cosf(theta);
#endif /* defined(STDROMANO_CUDA_DEVICE) */
}

template<>
STDROMANO_HOST_DEVICE STDROMANO_FORCE_INLINE void sincos(const double theta, double* s, double* c) noexcept
{
#if defined(STDROMANO_CUDA_DEVICE)
    ::sincos(theta, s, c);
#elif defined(STDROMANO_HAS_BUILTIN_SINCOS)
    __builtin_sincos(theta, s, c);
#else
    *s = ::sin(theta);
    *c = ::cos(theta);
#endif /* defined(STDROMANO_CUDA_DEVICE) */
}

/******************************************/
/* a * b + c */
template<typename T>
STDROMANO_HOST_DEVICE STDROMANO_FORCE_INLINE T fma(const T a, const T b, const T c) noexcept;

template<>
STDROMANO_HOST_DEVICE STDROMANO_FORCE_INLINE float fma(const float a, const float b, const float c) noexcept
{
#if defined(STDROMANO_CUDA_DEVICE)
    return __fmaf_rn(a, b, c);
#elif defined(STDROMANO_HAS_FMA_INTRINSICS)
    return _mm_cvtss_f32(_mm_fmadd_ss(_mm_set_ss(a), _mm_set_ss(b), _mm_set_ss(c)));
#elif defined(STDROMANO_AARCH64)
    return std::fma(a, b, c);
#else
    return a * b + c;
#endif /* defined(STDROMANO_CUDA_DEVICE) */
}

template<>
STDROMANO_HOST_DEVICE STDROMANO_FORCE_INLINE double fma(const double a, const double b, const double c) noexcept
{
#if defined(STDROMANO_CUDA_DEVICE)
    return __fma_rn(a, b, c);
#elif defined(STDROMANO_HAS_FMA_INTRINSICS)
    return _mm_cvtsd_f64(_mm_fmadd_sd(_mm_set_sd(a), _mm_set_sd(b), _mm_set_sd(c)));
#elif defined(STDROMANO_AARCH64)
    return std::fma(a, b, c);
#else
    return a * b + c;
#endif /* defined(STDROMANO_CUDA_DEVICE) */
}

/******************************************/
/* a * b - c */
template<typename T>
STDROMANO_HOST_DEVICE STDROMANO_FORCE_INLINE T fms(const T a, const T b, const T c) noexcept;

template<>
STDROMANO_HOST_DEVICE STDROMANO_FORCE_INLINE float fms(const float a, const float b, const float c) noexcept
{
#if defined(STDROMANO_CUDA_DEVICE)
    return __fmaf_rn(a, b, -c);
#elif defined(STDROMANO_HAS_FMA_INTRINSICS)
    return _mm_cvtss_f32(_mm_fmsub_ss(_mm_set_ss(a), _mm_set_ss(b), _mm_set_ss(c)));
#elif defined(STDROMANO_AARCH64)
    return std::fma(a, b, -c);
#else
    return a * b - c;
#endif /* defined(STDROMANO_CUDA_DEVICE) */
}

template<>
STDROMANO_HOST_DEVICE STDROMANO_FORCE_INLINE double fms(const double a, const double b, const double c) noexcept
{
#if defined(STDROMANO_CUDA_DEVICE)
    return __fma_rn(a, b, -c);
#elif defined(STDROMANO_HAS_FMA_INTRINSICS)
    return _mm_cvtsd_f64(_mm_fmsub_sd(_mm_set_sd(a), _mm_set_sd(b), _mm_set_sd(c)));
#elif defined(STDROMANO_AARCH64)
    return std::fma(a, b, -c);
#else
    return a * b - c;
#endif /* defined(STDROMANO_CUDA_DEVICE) */
}

/******************************************/
/* -a * b + c */
template<typename T>
STDROMANO_HOST_DEVICE STDROMANO_FORCE_INLINE T nfma(const T a, const T b, const T c) noexcept;

template<>
STDROMANO_HOST_DEVICE STDROMANO_FORCE_INLINE float nfma(const float a, const float b, const float c) noexcept
{
#if defined(STDROMANO_CUDA_DEVICE)
    return __fmaf_rn(-a, b, c);
#elif defined(STDROMANO_HAS_FMA_INTRINSICS)
    return _mm_cvtss_f32(_mm_fnmadd_ss(_mm_set_ss(a), _mm_set_ss(b), _mm_set_ss(c)));
#elif defined(STDROMANO_AARCH64)
    return std::fma(-a, b, c);
#else
    return -a * b + c;
#endif /* defined(STDROMANO_CUDA_DEVICE) */
}

template<>
STDROMANO_HOST_DEVICE STDROMANO_FORCE_INLINE double nfma(const double a, const double b, const double c) noexcept
{
#if defined(STDROMANO_CUDA_DEVICE)
    return __fma_rn(-a, b, c);
#elif defined(STDROMANO_HAS_FMA_INTRINSICS)
    return _mm_cvtsd_f64(_mm_fnmadd_sd(_mm_set_sd(a), _mm_set_sd(b), _mm_set_sd(c)));
#elif defined(STDROMANO_AARCH64)
    return std::fma(-a, b, c);
#else
    return -a * b + c;
#endif /* defined(STDROMANO_CUDA_DEVICE) */
}

/******************************************/
/* -a * b - c */
template<typename T>
STDROMANO_HOST_DEVICE STDROMANO_FORCE_INLINE T nfms(const T a, const T b, const T c) noexcept;

template<>
STDROMANO_HOST_DEVICE STDROMANO_FORCE_INLINE float nfms(const float a, const float b, const float c) noexcept
{
#if defined(STDROMANO_CUDA_DEVICE)
    return __fmaf_rn(-a, b, -c);
#elif defined(STDROMANO_HAS_FMA_INTRINSICS)
    return _mm_cvtss_f32(_mm_fnmsub_ss(_mm_set_ss(a), _mm_set_ss(b), _mm_set_ss(c)));
#elif defined(STDROMANO_AARCH64)
    return -std::fma(a, b, c);
#else
    return -a * b - c;
#endif /* defined(STDROMANO_CUDA_DEVICE) */
}

template<>
STDROMANO_HOST_DEVICE STDROMANO_FORCE_INLINE double nfms(const double a, const double b, const double c) noexcept
{
#if defined(STDROMANO_CUDA_DEVICE)
    return __fma_rn(-a, b, -c);
#elif defined(STDROMANO_HAS_FMA_INTRINSICS)
    return _mm_cvtsd_f64(_mm_fnmsub_sd(_mm_set_sd(a), _mm_set_sd(b), _mm_set_sd(c)));
#elif defined(STDROMANO_AARCH64)
    return -std::fma(a, b, c);
#else
    return -a * b - c;
#endif /* defined(STDROMANO_CUDA_DEVICE) */
}

/******************************************/
template<typename T>
STDROMANO_HOST_DEVICE STDROMANO_FORCE_INLINE bool equal_with_abs_error(const T lhs, const T rhs, const T err) noexcept
{
    return abs(lhs - rhs) < err;
}

MATHS_NAMESPACE_END
STDROMANO_NAMESPACE_END

#endif /* !defined(__STDROMANO_MATHS) */
