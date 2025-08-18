// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - Present Romain Augier
// All rights reserved.

#pragma once

#if !defined(__STDROMANO_MATHS)
#define __STDROMANO_MATHS

#include "stdromano/stdromano.hpp"

#include <immintrin.h>
#include <limits>
#include <cmath>

#if defined(STDROMANO_GCC) || defined(STDROMANO_CLANG)
#include <math.h>
#endif

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
    static constexpr float two_over_pi = 0.636619772367581343076;
    static constexpr float inv_pi = one_over_pi;
    static constexpr float inv_two_pi = 0.15915494309189533577;
    static constexpr float inv_four_pi = 0.07957747154594766788;

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

#if defined(STDROMANO_WIN)
/******************************************/
template<typename T>
STDROMANO_FORCE_INLINE bool isinf(const T x) noexcept;

template<>
STDROMANO_FORCE_INLINE bool isinf(const float x) noexcept { return _finite(x) == 0; }

template<>
STDROMANO_FORCE_INLINE bool isinf(const double x) noexcept { return _finite(x) == 0; }

/******************************************/
template<typename T>
STDROMANO_FORCE_INLINE bool isnan(const T x) noexcept;

template<>
STDROMANO_FORCE_INLINE bool isnan(const float x) noexcept { return _isnan(x) != 0; }

template<>
STDROMANO_FORCE_INLINE bool isnan(const double x) noexcept { return _isnan(x) != 0; }

/******************************************/
template<typename T>
STDROMANO_FORCE_INLINE bool isfinite(const T x) noexcept;

template<>
STDROMANO_FORCE_INLINE bool isfinite(const float x) noexcept { return _finite(x) != 0; }

template<>
STDROMANO_FORCE_INLINE bool isfinite(const double x) noexcept { return _finite(x) != 0; }
#else
/******************************************/
template<typename T>
STDROMANO_FORCE_INLINE bool isinf(const T x) noexcept;

template<>
STDROMANO_FORCE_INLINE bool isinf(const float x) noexcept { return __builtin_isinf(x) == 0; }

template<>
STDROMANO_FORCE_INLINE bool isinf(const double x) noexcept { return __builtin_isinf(x) == 0; }

/******************************************/
template<typename T>
STDROMANO_FORCE_INLINE bool isnan(const T x) noexcept;

template<>
STDROMANO_FORCE_INLINE bool isnan(const float x) noexcept { return __builtin_isnan(x) != 0; }

template<>
STDROMANO_FORCE_INLINE bool isnan(const double x) noexcept { return __builtin_isnan(x) != 0; }

/******************************************/
template<typename T>
STDROMANO_FORCE_INLINE bool isfinite(const T x) noexcept;

template<>
STDROMANO_FORCE_INLINE bool isfinite(const float x) noexcept { return __builtin_isfinite(x) != 0; }

template<>
STDROMANO_FORCE_INLINE bool isfinite(const double x) noexcept { return __builtin_isfinite(x) != 0; }
#endif /* defined(STDROMANO_WIN) */

/******************************************/
template<typename T>
STDROMANO_FORCE_INLINE T sqr(const T x) noexcept { return x * x; }

/******************************************/
template<typename T>
STDROMANO_FORCE_INLINE T rcp(const T x) noexcept;

template<>
STDROMANO_FORCE_INLINE float rcp(const float x) noexcept
{
    const __m128 a = _mm_set_ss(x);
    const __m128 r = _mm_rcp_ss(a);

#if defined(__AVX2__)
    return _mm_cvtss_f32(_mm_mul_ss(r, _mm_fnmadd_ss(r, a, _mm_set_ss(2.0f))));
#else
    return _mm_cvtss_f32(_mm_mul_ss(r, _mm_sub_ss(_mm_set_ss(2.0f), _mm_mul_ss(r, a))));
#endif /* defined(__AVX2__) */
}

template<>
STDROMANO_FORCE_INLINE double rcp(const double x) noexcept
{
    return 1.0 / x;
}

/******************************************/
template<typename T>
STDROMANO_FORCE_INLINE T rcp_safe(T a) noexcept { return constants<T>::one / a; }

/******************************************/
template<typename T>
STDROMANO_FORCE_INLINE T min(T a, T b) noexcept { return a < b ? a : b; }

/******************************************/
template<typename T>
STDROMANO_FORCE_INLINE T max(T a, T b) noexcept { return a > b ? a : b; }

/******************************************/
template<typename T>
STDROMANO_FORCE_INLINE T fit(T s, T a1, T a2, T b1, T b2) noexcept
{
    return b1 + ((s - a1) * (b2 - b1)) / (a2 - a1);
}

/******************************************/
template<typename T>
STDROMANO_FORCE_INLINE T fit01(T x, T a, T b) noexcept
{
    return x * (b - a) + a;
}

/******************************************/
template<typename T>
STDROMANO_FORCE_INLINE T lerp(T a, T b, T t) noexcept
{
    return (constants<T>::one - t) * a + t * b;
}

/******************************************/
template<typename T>
STDROMANO_FORCE_INLINE T clamp(T x,
                               T lower = constants<T>::zero,
                               T upper = constants<T>::one) noexcept
{
    return max(lower, min(x, upper));
}

/******************************************/
template<typename T>
STDROMANO_FORCE_INLINE T clampz(T x, 
                                T upper = constants<T>::one) noexcept
{
    return max(constants<T>::zero, min(x, upper));
}

/******************************************/
template<typename T>
STDROMANO_FORCE_INLINE T deg2rad(T deg) noexcept
{
    return deg * constants<T>::pi / static_cast<T>(180);
}

/******************************************/
template<typename T>
STDROMANO_FORCE_INLINE T rad2deg(T rad) noexcept
{
    return rad * static_cast<T>(180) / constants<T>::pi;
}

/******************************************/
template<typename T>
STDROMANO_FORCE_INLINE T abs(T x) noexcept;

template<>
STDROMANO_FORCE_INLINE float abs(float x) noexcept { return ::fabsf(x); }

template<>
STDROMANO_FORCE_INLINE double abs(double x) noexcept { return ::fabs(x); }

/******************************************/
template<typename T>
STDROMANO_FORCE_INLINE T exp(T x) noexcept;

template<>
STDROMANO_FORCE_INLINE float exp(float x) noexcept { return ::expf(x); }

template<>
STDROMANO_FORCE_INLINE double exp(double x) noexcept { return ::exp(x); }

/******************************************/
template<typename T>
STDROMANO_FORCE_INLINE T sqrt(T x) noexcept;

template<>
STDROMANO_FORCE_INLINE float sqrt(float x) noexcept { return ::sqrtf(x); }

template<>
STDROMANO_FORCE_INLINE double sqrt(double x) noexcept { return ::sqrt(x); }

/******************************************/
template<typename T>
STDROMANO_FORCE_INLINE T rsqrt(T x) noexcept;

template<>
STDROMANO_FORCE_INLINE float rsqrt(float x) noexcept
{
    const __m128 a = _mm_set_ss(x);
    __m128 r = _mm_rsqrt_ss(a);
    r = _mm_add_ss(_mm_mul_ss(_mm_set_ss(1.5f), r),
                   _mm_mul_ss(_mm_mul_ss(_mm_mul_ss(a, _mm_set_ss(-0.5f)), r), _mm_mul_ss(r, r)));
    return _mm_cvtss_f32(r);
}

template<>
STDROMANO_FORCE_INLINE double rsqrt(double x) noexcept
{
    return 1.0 / sqrt(x);
}

/******************************************/
template<typename T>
STDROMANO_FORCE_INLINE T fmod(T x, T y) noexcept;

template<>
STDROMANO_FORCE_INLINE float fmod(float x, float y) noexcept { return ::fmodf(x, y); }

template<>
STDROMANO_FORCE_INLINE double fmod(double x, double y) noexcept { return ::fmod(x, y); }

/******************************************/
template<typename T>
STDROMANO_FORCE_INLINE T log(T x) noexcept;

template<>
STDROMANO_FORCE_INLINE float log(float x) noexcept { return ::logf(x); }

template<>
STDROMANO_FORCE_INLINE double log(double x) noexcept { return ::log(x); }

/******************************************/
template<typename T>
STDROMANO_FORCE_INLINE T log2(T x) noexcept;

template<>
STDROMANO_FORCE_INLINE float log2(float x) noexcept { return ::log2f(x); }

template<>
STDROMANO_FORCE_INLINE double log2(double x) noexcept { return ::log2(x); }

/******************************************/
template<typename T>
STDROMANO_FORCE_INLINE T log10(T x) noexcept;

template<>
STDROMANO_FORCE_INLINE float log10(float x) noexcept { return ::log10f(x); }

template<>
STDROMANO_FORCE_INLINE double log10(double x) noexcept { return ::log10(x); }

/******************************************/
template<typename T>
STDROMANO_FORCE_INLINE T pow(T x, T y) noexcept;

template<>
STDROMANO_FORCE_INLINE float pow(float x, float y) noexcept { return ::powf(x, y); }

template<>
STDROMANO_FORCE_INLINE double pow(double x, double y) noexcept { return ::pow(x, y); }

/******************************************/
template<typename T>
STDROMANO_FORCE_INLINE T floor(T x) noexcept;

template<>
STDROMANO_FORCE_INLINE float floor(float x) noexcept { return ::floorf(x); }

template<>
STDROMANO_FORCE_INLINE double floor(double x) noexcept { return ::floor(x); }

/******************************************/
template<typename T>
STDROMANO_FORCE_INLINE T ceil(T x) noexcept;

template<>
STDROMANO_FORCE_INLINE float ceil(float x) noexcept { return ::ceilf(x); }

template<>
STDROMANO_FORCE_INLINE double ceil(double x) noexcept { return ::ceil(x); }

/******************************************/
template<typename T>
STDROMANO_FORCE_INLINE T frac(T x) noexcept { return x - floor(x); }

/******************************************/
template<typename T>
STDROMANO_FORCE_INLINE T rint(T x) noexcept;

template<>
STDROMANO_FORCE_INLINE float rint(float x) noexcept { return ::rintf(x); }

template<>
STDROMANO_FORCE_INLINE double rint(double x) noexcept { return ::rint(x); }

/******************************************/
template<typename T>
STDROMANO_FORCE_INLINE T acos(T x) noexcept;

template<>
STDROMANO_FORCE_INLINE float acos(float x) noexcept { return ::acosf(x); }

template<>
STDROMANO_FORCE_INLINE double acos(double x) noexcept { return ::acos(x); }

/******************************************/
template<typename T>
STDROMANO_FORCE_INLINE T asin(T x) noexcept;

template<>
STDROMANO_FORCE_INLINE float asin(float x) noexcept { return ::asinf(x); }

template<>
STDROMANO_FORCE_INLINE double asin(double x) noexcept { return ::asin(x); }

/******************************************/
template<typename T>
STDROMANO_FORCE_INLINE T atan(T x) noexcept;

template<>
STDROMANO_FORCE_INLINE float atan(float x) noexcept { return ::atanf(x); }

template<>
STDROMANO_FORCE_INLINE double atan(double x) noexcept { return ::atan(x); }

/******************************************/
template<typename T>
STDROMANO_FORCE_INLINE T atan2(T y, T x) noexcept;

template<>
STDROMANO_FORCE_INLINE float atan2(float y, float x) noexcept { return ::atan2f(y, x); }

template<>
STDROMANO_FORCE_INLINE double atan2(double y, double x) noexcept { return ::atan2(y, x); }

/******************************************/
template<typename T>
STDROMANO_FORCE_INLINE T cos(T x) noexcept;

template<>
STDROMANO_FORCE_INLINE float cos(float x) noexcept { return ::cosf(x); }

template<>
STDROMANO_FORCE_INLINE double cos(double x) noexcept { return ::cos(x); }

/******************************************/
template<typename T>
STDROMANO_FORCE_INLINE T sin(T x) noexcept;

template<>
STDROMANO_FORCE_INLINE float sin(float x) noexcept { return ::sinf(x); }

template<>
STDROMANO_FORCE_INLINE double sin(double x) noexcept { return ::sin(x); }

/******************************************/
template<typename T>
STDROMANO_FORCE_INLINE T tan(T x) noexcept;

template<>
STDROMANO_FORCE_INLINE float tan(float x) noexcept { return ::tanf(x); }

template<>
STDROMANO_FORCE_INLINE double tan(double x) noexcept { return ::tan(x); }

/******************************************/
template<typename T>
STDROMANO_FORCE_INLINE T cosh(T x) noexcept;

template<>
STDROMANO_FORCE_INLINE float cosh(float x) noexcept { return ::coshf(x); }

template<>
STDROMANO_FORCE_INLINE double cosh(double x) noexcept { return ::cosh(x); }

/******************************************/
template<typename T>
STDROMANO_FORCE_INLINE T sinh(T x) noexcept;

template<>
STDROMANO_FORCE_INLINE float sinh(float x) noexcept { return ::sinhf(x); }

template<>
STDROMANO_FORCE_INLINE double sinh(double x) noexcept { return ::sinh(x); }

/******************************************/
template<typename T>
STDROMANO_FORCE_INLINE T tanh(T x) noexcept;

template<>
STDROMANO_FORCE_INLINE float tanh(float x) noexcept { return ::tanhf(x); }

template<>
STDROMANO_FORCE_INLINE double tanh(double x) noexcept { return ::tanh(x); }

/******************************************/
template<typename T>
STDROMANO_FORCE_INLINE void sincos(T theta, T* s, T* c) noexcept;

template<>
STDROMANO_FORCE_INLINE void sincos(float theta, float* s, float* c) noexcept
{
#if defined(STDROMANO_GCC)
    __builtin_sincosf(theta, sin, cos);
#else
    *s = sin(theta);
    *c = cos(theta);
#endif /* defined(STDROMANO_GCC) */
}

template<>
STDROMANO_FORCE_INLINE void sincos(double theta, double* s, double* c) noexcept
{
#if defined(STDROMANO_GCC)
    __builtin_sincos(theta, sin, cos);
#else
    *s = sin(theta);
    *c = cos(theta);
#endif /* defined(STDROMANO_GCC) */
}

/******************************************/
template<typename T>
STDROMANO_FORCE_INLINE T fma(T a, T b, T c) noexcept;

template<>
STDROMANO_FORCE_INLINE float fma(float a, float b, float c) noexcept
{
#if defined(__AVX2__)
    return _mm_cvtss_f32(_mm_fmadd_ss(_mm_set_ss(a), _mm_set_ss(b), _mm_set_ss(c)));
#else
    return a * b + c;
#endif /* defined(__AVX2__) */
}

template<>
STDROMANO_FORCE_INLINE double fma(double a, double b, double c) noexcept
{
#if defined(__AVX2__)
    return _mm_cvtsd_f64(_mm_fmadd_sd(_mm_set_sd(a), _mm_set_sd(b), _mm_set_sd(c)));
#else
    return a * b + c;
#endif /* defined(__AVX2__) */
}

/******************************************/
template<typename T>
STDROMANO_FORCE_INLINE T fms(T a, T b, T c) noexcept;

template<>
STDROMANO_FORCE_INLINE float fms(float a, float b, float c) noexcept
{
#if defined(__AVX2__)
    return _mm_cvtss_f32(_mm_fmsub_ss(_mm_set_ss(a), _mm_set_ss(b), _mm_set_ss(c)));
#else
    return a * b - c;
#endif /* defined(__AVX2__) */
}

template<>
STDROMANO_FORCE_INLINE double fms(double a, double b, double c) noexcept
{
#if defined(__AVX2__)
    return _mm_cvtsd_f64(_mm_fmsub_sd(_mm_set_sd(a), _mm_set_sd(b), _mm_set_sd(c)));
#else
    return a * b - c;
#endif /* defined(__AVX2__) */
}

/******************************************/
template<typename T>
STDROMANO_FORCE_INLINE T nfma(T a, T b, T c) noexcept;

template<>
STDROMANO_FORCE_INLINE float nfma(float a, float b, float c) noexcept
{
#if defined(__AVX2__)
    return _mm_cvtss_f32(_mm_fnmadd_ss(_mm_set_ss(a), _mm_set_ss(b), _mm_set_ss(c)));
#else
    return -a * b + c;
#endif /* defined(__AVX2__) */
}

template<>
STDROMANO_FORCE_INLINE double nfma(double a, double b, double c) noexcept
{
#if defined(__AVX2__)
    return _mm_cvtsd_f64(_mm_fnmadd_sd(_mm_set_sd(a), _mm_set_sd(b), _mm_set_sd(c)));
#else
    return -a * b + c;
#endif /* defined(__AVX2__) */
}

/******************************************/
template<typename T>
STDROMANO_FORCE_INLINE T nfms(T a, T b, T c) noexcept;

template<>
STDROMANO_FORCE_INLINE float nfms(float a, float b, float c) noexcept
{
#if defined(__AVX2__)
    return _mm_cvtss_f32(_mm_fnmsub_ss(_mm_set_ss(a), _mm_set_ss(b), _mm_set_ss(c)));
#else
    return -a * b - c;
#endif /* defined(__AVX2__) */
}

template<>
STDROMANO_FORCE_INLINE double nfms(double a, double b, double c) noexcept
{
#if defined(__AVX2__)
    return _mm_cvtsd_f64(_mm_fnmsub_sd(_mm_set_sd(a), _mm_set_sd(b), _mm_set_sd(c)));
#else
    return -a * b - c;
#endif /* defined(__AVX2__) */
}

MATHS_NAMESPACE_END
STDROMANO_NAMESPACE_END

#endif /* !defined(__STDROMANO_MATHS) */
