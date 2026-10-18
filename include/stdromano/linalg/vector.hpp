// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - Present Romain Augier
// All rights reserved.

#pragma once

#if !defined(__STDROMANO_LINALG_VECTOR)
#define __STDROMANO_LINALG_VECTOR

#include "stdromano/linalg/traits.hpp"
#include "stdromano/maths.hpp"

#include <tuple>

#if defined(STDROMANO_CUDA)
#include <vector_functions.h>
#include <vector_types.h>
#endif /* defined(STDROMANO_CUDA) */

STDROMANO_NAMESPACE_BEGIN

/*
    We use structs because it is convenient to be able to call members
    instead of using getters/setters that would overcomplicate the code.

    Vector2 and Vector4 have the alignment of the CUDA vector types (float2 is 8 bytes aligned,
    float4 16), so device loads and stores are vectorized and CUDA buffers can be reinterpreted.
    Floating-point division by a scalar multiplies by the reciprocal.
*/

template<typename T>
constexpr std::size_t vector2_alignment_v = 2 * sizeof(T);

template<typename T>
constexpr std::size_t vector4_alignment_v = 4 * sizeof(T) > 16 ? 16 : 4 * sizeof(T);

/* The CUDA vector type with the same component type and count, void when there is none */
template<typename T, std::size_t N>
struct cuda_vector { using type = void; };

#if defined(STDROMANO_CUDA)
template<> struct cuda_vector<float, 2> { using type = float2; };
template<> struct cuda_vector<float, 3> { using type = float3; };
template<> struct cuda_vector<float, 4> { using type = float4; };
template<> struct cuda_vector<double, 2> { using type = double2; };
template<> struct cuda_vector<std::int32_t, 2> { using type = int2; };
template<> struct cuda_vector<std::int32_t, 3> { using type = int3; };
template<> struct cuda_vector<std::int32_t, 4> { using type = int4; };
template<> struct cuda_vector<std::uint32_t, 2> { using type = uint2; };
template<> struct cuda_vector<std::uint32_t, 3> { using type = uint3; };
template<> struct cuda_vector<std::uint32_t, 4> { using type = uint4; };
#endif /* defined(STDROMANO_CUDA) */

template<typename T, std::size_t N>
using cuda_vector_t = typename cuda_vector<T, N>::type;

template<typename V, typename T, std::size_t N>
constexpr bool is_cuda_vector_v = std::is_same_v<V, cuda_vector_t<T, N>>;

/********************************/
/* Vector2 */
/********************************/

template<typename T>
struct alignas(vector2_alignment_v<T>) Vector2
{
    static_assert(std::is_floating_point_v<T> || std::is_integral_v<T>,
                  "T needs to be floating-point or integral type");

    T x, y;

    STDROMANO_HOST_DEVICE constexpr Vector2() noexcept : x(make_zero_v<T>), y(make_zero_v<T>) {}
    STDROMANO_HOST_DEVICE constexpr Vector2(const T t) noexcept : x(t), y(t) {}
    STDROMANO_HOST_DEVICE constexpr Vector2(const T x, const T y) noexcept : x(x), y(y) {}

    template<typename S>
    STDROMANO_HOST_DEVICE constexpr Vector2(const Vector2<S>& v) noexcept : x(T(v.x)), y(T(v.y)) {}

    /* Any other type with the same members, explicitly (float4 to Vec3F, Vec4F to Vec2F...) */
    template<typename V, std::enable_if_t<has_xy_v<V> && !is_cuda_vector_v<V, T, 2>, int> = 0>
    STDROMANO_HOST_DEVICE constexpr explicit Vector2(const V& v) noexcept : x(T(v.x)), y(T(v.y)) {}

    /* The CUDA vector type of the same component type and count converts implicitly, both ways */
    template<typename V, std::enable_if_t<is_cuda_vector_v<V, T, 2>, int> = 0>
    STDROMANO_HOST_DEVICE constexpr Vector2(const V& v) noexcept : x(v.x), y(v.y) {}

    template<typename V, std::enable_if_t<is_cuda_vector_v<V, T, 2>, int> = 0>
    STDROMANO_HOST_DEVICE constexpr operator V() const noexcept { return V{this->x, this->y}; }

    STDROMANO_HOST_DEVICE constexpr Vector2 operator-() const noexcept { return Vector2(-this->x, -this->y); }
    STDROMANO_HOST_DEVICE constexpr Vector2 operator~() const noexcept { return Vector2(~this->x, ~this->y); }

    STDROMANO_HOST_DEVICE constexpr const T& operator[](const std::size_t i) const noexcept { return (&this->x)[i]; }
    STDROMANO_HOST_DEVICE constexpr T& operator[](const std::size_t i) noexcept { return (&this->x)[i]; }

    STDROMANO_HOST_DEVICE constexpr Vector2 operator+(const Vector2& other) const noexcept
    {
        return Vector2(this->x + other.x, this->y + other.y);
    }

    STDROMANO_HOST_DEVICE constexpr Vector2 operator-(const Vector2& other) const noexcept
    {
        return Vector2(this->x - other.x, this->y - other.y);
    }

    STDROMANO_HOST_DEVICE constexpr Vector2 operator*(const Vector2& other) const noexcept
    {
        return Vector2(this->x * other.x, this->y * other.y);
    }

    STDROMANO_HOST_DEVICE constexpr Vector2 operator/(const Vector2& other) const noexcept
    {
        return Vector2(this->x / other.x, this->y / other.y);
    }

    STDROMANO_HOST_DEVICE constexpr Vector2 operator+(const T other) const noexcept
    {
        return Vector2(this->x + other, this->y + other);
    }

    STDROMANO_HOST_DEVICE constexpr Vector2 operator-(const T other) const noexcept
    {
        return Vector2(this->x - other, this->y - other);
    }

    STDROMANO_HOST_DEVICE constexpr Vector2 operator*(const T other) const noexcept
    {
        return Vector2(this->x * other, this->y * other);
    }

    STDROMANO_HOST_DEVICE constexpr Vector2 operator/(const T other) const noexcept
    {
        if constexpr(std::is_floating_point_v<T>)
        {
            const T inv = maths::rcp_safe(other);
            return Vector2(this->x * inv, this->y * inv);
        }
        else
        {
            return Vector2(this->x / other, this->y / other);
        }
    }

    STDROMANO_HOST_DEVICE constexpr Vector2 operator&(const Vector2& other) const noexcept
    {
        return Vector2(this->x & other.x, this->y & other.y);
    }

    STDROMANO_HOST_DEVICE constexpr Vector2 operator|(const Vector2& other) const noexcept
    {
        return Vector2(this->x | other.x, this->y | other.y);
    }

    STDROMANO_HOST_DEVICE constexpr Vector2 operator^(const Vector2& other) const noexcept
    {
        return Vector2(this->x ^ other.x, this->y ^ other.y);
    }

    STDROMANO_HOST_DEVICE constexpr Vector2 operator&(const T other) const noexcept
    {
        return Vector2(this->x & other, this->y & other);
    }

    STDROMANO_HOST_DEVICE constexpr Vector2 operator|(const T other) const noexcept
    {
        return Vector2(this->x | other, this->y | other);
    }

    STDROMANO_HOST_DEVICE constexpr Vector2 operator^(const T other) const noexcept
    {
        return Vector2(this->x ^ other, this->y ^ other);
    }

    STDROMANO_HOST_DEVICE constexpr Vector2 operator<<(const unsigned int shift) const noexcept
    {
        return Vector2(this->x << shift, this->y << shift);
    }

    STDROMANO_HOST_DEVICE constexpr Vector2 operator>>(const unsigned int shift) const noexcept
    {
        return Vector2(this->x >> shift, this->y >> shift);
    }

    STDROMANO_HOST_DEVICE constexpr Vector2& operator+=(const Vector2& other) noexcept { return *this = *this + other; }
    STDROMANO_HOST_DEVICE constexpr Vector2& operator-=(const Vector2& other) noexcept { return *this = *this - other; }
    STDROMANO_HOST_DEVICE constexpr Vector2& operator*=(const Vector2& other) noexcept { return *this = *this * other; }
    STDROMANO_HOST_DEVICE constexpr Vector2& operator/=(const Vector2& other) noexcept { return *this = *this / other; }
    STDROMANO_HOST_DEVICE constexpr Vector2& operator+=(const T other) noexcept { return *this = *this + other; }
    STDROMANO_HOST_DEVICE constexpr Vector2& operator-=(const T other) noexcept { return *this = *this - other; }
    STDROMANO_HOST_DEVICE constexpr Vector2& operator*=(const T other) noexcept { return *this = *this * other; }
    STDROMANO_HOST_DEVICE constexpr Vector2& operator/=(const T other) noexcept { return *this = *this / other; }
    STDROMANO_HOST_DEVICE constexpr Vector2& operator&=(const Vector2& other) noexcept { return *this = *this & other; }
    STDROMANO_HOST_DEVICE constexpr Vector2& operator|=(const Vector2& other) noexcept { return *this = *this | other; }
    STDROMANO_HOST_DEVICE constexpr Vector2& operator^=(const Vector2& other) noexcept { return *this = *this ^ other; }
    STDROMANO_HOST_DEVICE constexpr Vector2& operator<<=(const unsigned int shift) noexcept { return *this = *this << shift; }
    STDROMANO_HOST_DEVICE constexpr Vector2& operator>>=(const unsigned int shift) noexcept { return *this = *this >> shift; }

    STDROMANO_HOST_DEVICE constexpr bool operator==(const Vector2& other) const noexcept
    {
        return this->x == other.x && this->y == other.y;
    }

    STDROMANO_HOST_DEVICE constexpr bool operator!=(const Vector2& other) const noexcept
    {
        return !this->operator==(other);
    }

    STDROMANO_HOST_DEVICE bool equal_with_abs_error(const Vector2& other, const T err) const noexcept
    {
        return maths::equal_with_abs_error(this->x, other.x, err) &&
               maths::equal_with_abs_error(this->y, other.y, err);
    }
};

template<typename T>
STDROMANO_HOST_DEVICE constexpr STDROMANO_FORCE_INLINE Vector2<T> operator*(const T t, const Vector2<T>& v) noexcept
{
    return v * t;
}

template<typename T>
STDROMANO_HOST_DEVICE constexpr STDROMANO_FORCE_INLINE Vector2<T> operator/(const T t, const Vector2<T>& v) noexcept
{
    return Vector2<T>(t / v.x, t / v.y);
}

template<typename T>
STDROMANO_HOST_DEVICE constexpr STDROMANO_FORCE_INLINE T dot(const Vector2<T>& lhs, const Vector2<T>& rhs) noexcept
{
    return lhs.x * rhs.x + lhs.y * rhs.y;
}

template<typename T>
STDROMANO_HOST_DEVICE constexpr STDROMANO_FORCE_INLINE T length2(const Vector2<T>& v) noexcept
{
    return dot(v, v);
}

template<typename T>
STDROMANO_HOST_DEVICE STDROMANO_FORCE_INLINE T length(const Vector2<T>& v) noexcept
{
    return maths::sqrt(dot(v, v));
}

template<typename T>
STDROMANO_HOST_DEVICE STDROMANO_FORCE_INLINE T dist2(const Vector2<T>& a, const Vector2<T>& b) noexcept
{
    return length2(a - b);
}

template<typename T>
STDROMANO_HOST_DEVICE STDROMANO_FORCE_INLINE T dist(const Vector2<T>& a, const Vector2<T>& b) noexcept
{
    return length(a - b);
}

/* Uses the rsqrt approximation, see normalize_safe for the exact one */
template<typename T>
STDROMANO_HOST_DEVICE STDROMANO_FORCE_INLINE Vector2<T> normalize(const Vector2<T>& v) noexcept
{
    return v * maths::rsqrt(dot(v, v));
}

template<typename T>
STDROMANO_HOST_DEVICE STDROMANO_FORCE_INLINE Vector2<T> normalize_safe(const Vector2<T>& v) noexcept
{
    return v * maths::rcp_safe(length(v));
}

template<typename T>
STDROMANO_HOST_DEVICE STDROMANO_FORCE_INLINE Vector2<T> abs(const Vector2<T>& v) noexcept
{
    return Vector2<T>(maths::abs(v.x), maths::abs(v.y));
}

template<typename T>
STDROMANO_HOST_DEVICE STDROMANO_FORCE_INLINE Vector2<T> min(const Vector2<T>& a, const Vector2<T>& b) noexcept
{
    return Vector2<T>(maths::min(a.x, b.x), maths::min(a.y, b.y));
}

template<typename T>
STDROMANO_HOST_DEVICE STDROMANO_FORCE_INLINE Vector2<T> max(const Vector2<T>& a, const Vector2<T>& b) noexcept
{
    return Vector2<T>(maths::max(a.x, b.x), maths::max(a.y, b.y));
}

template<typename T>
STDROMANO_HOST_DEVICE STDROMANO_FORCE_INLINE T min_component(const Vector2<T>& v) noexcept
{
    return maths::min(v.x, v.y);
}

template<typename T>
STDROMANO_HOST_DEVICE STDROMANO_FORCE_INLINE T max_component(const Vector2<T>& v) noexcept
{
    return maths::max(v.x, v.y);
}

template<typename T>
STDROMANO_HOST_DEVICE STDROMANO_FORCE_INLINE Vector2<T> clamp(const Vector2<T>& v,
                                                              const T lower = make_zero_v<T>,
                                                              const T upper = make_one_v<T>) noexcept
{
    return Vector2<T>(maths::clamp(v.x, lower, upper), maths::clamp(v.y, lower, upper));
}

template<typename T>
STDROMANO_HOST_DEVICE STDROMANO_FORCE_INLINE Vector2<T> lerp(const Vector2<T>& a, const Vector2<T>& b, const T t) noexcept
{
    return Vector2<T>(maths::lerp(a.x, b.x, t), maths::lerp(a.y, b.y, t));
}

template<typename T>
STDROMANO_HOST_DEVICE STDROMANO_FORCE_INLINE Vector2<T> lerp(const Vector2<T>& a,
                                                             const Vector2<T>& b,
                                                             const Vector2<T>& t) noexcept
{
    return Vector2<T>(maths::lerp(a.x, b.x, t.x), maths::lerp(a.y, b.y, t.y));
}

template<typename T>
STDROMANO_HOST_DEVICE STDROMANO_FORCE_INLINE Vector2<T> rcp(const Vector2<T>& v) noexcept
{
    return Vector2<T>(maths::rcp(v.x), maths::rcp(v.y));
}

template<typename T>
STDROMANO_HOST_DEVICE STDROMANO_FORCE_INLINE Vector2<T> rcp_safe(const Vector2<T>& v) noexcept
{
    return Vector2<T>(maths::rcp_safe(v.x), maths::rcp_safe(v.y));
}

template<typename T>
STDROMANO_HOST_DEVICE STDROMANO_FORCE_INLINE Vector2<T> floor(const Vector2<T>& v) noexcept
{
    return Vector2<T>(maths::floor(v.x), maths::floor(v.y));
}

template<typename T>
STDROMANO_HOST_DEVICE STDROMANO_FORCE_INLINE Vector2<T> ceil(const Vector2<T>& v) noexcept
{
    return Vector2<T>(maths::ceil(v.x), maths::ceil(v.y));
}

template<typename T>
STDROMANO_HOST_DEVICE STDROMANO_FORCE_INLINE Vector2<T> pow(const Vector2<T>& v, const T p) noexcept
{
    return Vector2<T>(maths::pow(v.x, p), maths::pow(v.y, p));
}

template<typename T>
STDROMANO_HOST_DEVICE STDROMANO_FORCE_INLINE Vector2<T> sign(const Vector2<T>& v) noexcept
{
    return Vector2<T>(maths::sign(v.x), maths::sign(v.y));
}

template<typename T>
STDROMANO_HOST_DEVICE STDROMANO_FORCE_INLINE bool isnan(const Vector2<T>& v) noexcept
{
    return maths::isnan(v.x) || maths::isnan(v.y);
}

template<typename T>
STDROMANO_HOST_DEVICE STDROMANO_FORCE_INLINE bool isinf(const Vector2<T>& v) noexcept
{
    return maths::isinf(v.x) || maths::isinf(v.y);
}

template<typename T>
STDROMANO_HOST_DEVICE STDROMANO_FORCE_INLINE bool isfinite(const Vector2<T>& v) noexcept
{
    return maths::isfinite(v.x) && maths::isfinite(v.y);
}

/* Reflects v about the plane of normal n (n must be normalized) */
template<typename T>
STDROMANO_HOST_DEVICE STDROMANO_FORCE_INLINE Vector2<T> reflect(const Vector2<T>& v, const Vector2<T>& n) noexcept
{
    return v - n * (T(2) * dot(v, n));
}

/* Rotates v by 90 degrees counter-clockwise */
template<typename T>
STDROMANO_HOST_DEVICE constexpr STDROMANO_FORCE_INLINE Vector2<T> perpendicular(const Vector2<T>& v) noexcept
{
    return Vector2<T>(-v.y, v.x);
}

/* Unsigned angle in radians, in [0, pi]. atan2 stays accurate near 0 and pi, where acos does not */
template<typename T>
STDROMANO_HOST_DEVICE STDROMANO_FORCE_INLINE T angle(const Vector2<T>& a, const Vector2<T>& b) noexcept
{
    return maths::atan2(maths::abs(a.x * b.y - a.y * b.x), dot(a, b));
}

using Vec2F = Vector2<float>;
using Vec2D = Vector2<double>;
using Vec2I = Vector2<std::int32_t>;
using Vec2U = Vector2<std::uint32_t>;

/********************************/
/* Vector3 */
/********************************/

template<typename T>
struct Vector3
{
    static_assert(std::is_floating_point_v<T> || std::is_integral_v<T>,
                  "T needs to be floating-point or integral type");

    T x, y, z;

    STDROMANO_HOST_DEVICE constexpr Vector3() noexcept : x(make_zero_v<T>), y(make_zero_v<T>), z(make_zero_v<T>) {}
    STDROMANO_HOST_DEVICE constexpr Vector3(const T t) noexcept : x(t), y(t), z(t) {}
    STDROMANO_HOST_DEVICE constexpr Vector3(const T x, const T y, const T z) noexcept : x(x), y(y), z(z) {}
    STDROMANO_HOST_DEVICE constexpr Vector3(const Vector2<T>& xy, const T z) noexcept : x(xy.x), y(xy.y), z(z) {}

    template<typename S>
    STDROMANO_HOST_DEVICE constexpr Vector3(const Vector3<S>& v) noexcept : x(T(v.x)), y(T(v.y)), z(T(v.z)) {}

    /* Any other type with the same members, explicitly (float4 to Vec3F, Vec4F to Vec2F...) */
    template<typename V, std::enable_if_t<has_xyz_v<V> && !is_cuda_vector_v<V, T, 3>, int> = 0>
    STDROMANO_HOST_DEVICE constexpr explicit Vector3(const V& v) noexcept : x(T(v.x)), y(T(v.y)), z(T(v.z)) {}

    /* The CUDA vector type of the same component type and count converts implicitly, both ways */
    template<typename V, std::enable_if_t<is_cuda_vector_v<V, T, 3>, int> = 0>
    STDROMANO_HOST_DEVICE constexpr Vector3(const V& v) noexcept : x(v.x), y(v.y), z(v.z) {}

    template<typename V, std::enable_if_t<is_cuda_vector_v<V, T, 3>, int> = 0>
    STDROMANO_HOST_DEVICE constexpr operator V() const noexcept { return V{this->x, this->y, this->z}; }

    STDROMANO_HOST_DEVICE constexpr Vector2<T> xy() const noexcept { return Vector2<T>(this->x, this->y); }

    STDROMANO_HOST_DEVICE constexpr Vector3 operator-() const noexcept { return Vector3(-this->x, -this->y, -this->z); }
    STDROMANO_HOST_DEVICE constexpr Vector3 operator~() const noexcept { return Vector3(~this->x, ~this->y, ~this->z); }

    STDROMANO_HOST_DEVICE constexpr const T& operator[](const std::size_t i) const noexcept { return (&this->x)[i]; }
    STDROMANO_HOST_DEVICE constexpr T& operator[](const std::size_t i) noexcept { return (&this->x)[i]; }

    STDROMANO_HOST_DEVICE constexpr Vector3 operator+(const Vector3& other) const noexcept
    {
        return Vector3(this->x + other.x, this->y + other.y, this->z + other.z);
    }

    STDROMANO_HOST_DEVICE constexpr Vector3 operator-(const Vector3& other) const noexcept
    {
        return Vector3(this->x - other.x, this->y - other.y, this->z - other.z);
    }

    STDROMANO_HOST_DEVICE constexpr Vector3 operator*(const Vector3& other) const noexcept
    {
        return Vector3(this->x * other.x, this->y * other.y, this->z * other.z);
    }

    STDROMANO_HOST_DEVICE constexpr Vector3 operator/(const Vector3& other) const noexcept
    {
        return Vector3(this->x / other.x, this->y / other.y, this->z / other.z);
    }

    STDROMANO_HOST_DEVICE constexpr Vector3 operator+(const T other) const noexcept
    {
        return Vector3(this->x + other, this->y + other, this->z + other);
    }

    STDROMANO_HOST_DEVICE constexpr Vector3 operator-(const T other) const noexcept
    {
        return Vector3(this->x - other, this->y - other, this->z - other);
    }

    STDROMANO_HOST_DEVICE constexpr Vector3 operator*(const T other) const noexcept
    {
        return Vector3(this->x * other, this->y * other, this->z * other);
    }

    STDROMANO_HOST_DEVICE constexpr Vector3 operator/(const T other) const noexcept
    {
        if constexpr(std::is_floating_point_v<T>)
        {
            const T inv = maths::rcp_safe(other);
            return Vector3(this->x * inv, this->y * inv, this->z * inv);
        }
        else
        {
            return Vector3(this->x / other, this->y / other, this->z / other);
        }
    }

    STDROMANO_HOST_DEVICE constexpr Vector3 operator&(const Vector3& other) const noexcept
    {
        return Vector3(this->x & other.x, this->y & other.y, this->z & other.z);
    }

    STDROMANO_HOST_DEVICE constexpr Vector3 operator|(const Vector3& other) const noexcept
    {
        return Vector3(this->x | other.x, this->y | other.y, this->z | other.z);
    }

    STDROMANO_HOST_DEVICE constexpr Vector3 operator^(const Vector3& other) const noexcept
    {
        return Vector3(this->x ^ other.x, this->y ^ other.y, this->z ^ other.z);
    }

    STDROMANO_HOST_DEVICE constexpr Vector3 operator&(const T other) const noexcept
    {
        return Vector3(this->x & other, this->y & other, this->z & other);
    }

    STDROMANO_HOST_DEVICE constexpr Vector3 operator|(const T other) const noexcept
    {
        return Vector3(this->x | other, this->y | other, this->z | other);
    }

    STDROMANO_HOST_DEVICE constexpr Vector3 operator^(const T other) const noexcept
    {
        return Vector3(this->x ^ other, this->y ^ other, this->z ^ other);
    }

    STDROMANO_HOST_DEVICE constexpr Vector3 operator<<(const unsigned int shift) const noexcept
    {
        return Vector3(this->x << shift, this->y << shift, this->z << shift);
    }

    STDROMANO_HOST_DEVICE constexpr Vector3 operator>>(const unsigned int shift) const noexcept
    {
        return Vector3(this->x >> shift, this->y >> shift, this->z >> shift);
    }

    STDROMANO_HOST_DEVICE constexpr Vector3& operator+=(const Vector3& other) noexcept { return *this = *this + other; }
    STDROMANO_HOST_DEVICE constexpr Vector3& operator-=(const Vector3& other) noexcept { return *this = *this - other; }
    STDROMANO_HOST_DEVICE constexpr Vector3& operator*=(const Vector3& other) noexcept { return *this = *this * other; }
    STDROMANO_HOST_DEVICE constexpr Vector3& operator/=(const Vector3& other) noexcept { return *this = *this / other; }
    STDROMANO_HOST_DEVICE constexpr Vector3& operator+=(const T other) noexcept { return *this = *this + other; }
    STDROMANO_HOST_DEVICE constexpr Vector3& operator-=(const T other) noexcept { return *this = *this - other; }
    STDROMANO_HOST_DEVICE constexpr Vector3& operator*=(const T other) noexcept { return *this = *this * other; }
    STDROMANO_HOST_DEVICE constexpr Vector3& operator/=(const T other) noexcept { return *this = *this / other; }
    STDROMANO_HOST_DEVICE constexpr Vector3& operator&=(const Vector3& other) noexcept { return *this = *this & other; }
    STDROMANO_HOST_DEVICE constexpr Vector3& operator|=(const Vector3& other) noexcept { return *this = *this | other; }
    STDROMANO_HOST_DEVICE constexpr Vector3& operator^=(const Vector3& other) noexcept { return *this = *this ^ other; }
    STDROMANO_HOST_DEVICE constexpr Vector3& operator<<=(const unsigned int shift) noexcept { return *this = *this << shift; }
    STDROMANO_HOST_DEVICE constexpr Vector3& operator>>=(const unsigned int shift) noexcept { return *this = *this >> shift; }

    STDROMANO_HOST_DEVICE constexpr bool operator==(const Vector3& other) const noexcept
    {
        return this->x == other.x && this->y == other.y && this->z == other.z;
    }

    STDROMANO_HOST_DEVICE constexpr bool operator!=(const Vector3& other) const noexcept
    {
        return !this->operator==(other);
    }

    STDROMANO_HOST_DEVICE bool equal_with_abs_error(const Vector3& other, const T err) const noexcept
    {
        return maths::equal_with_abs_error(this->x, other.x, err) &&
               maths::equal_with_abs_error(this->y, other.y, err) &&
               maths::equal_with_abs_error(this->z, other.z, err);
    }
};

template<typename T>
STDROMANO_HOST_DEVICE constexpr STDROMANO_FORCE_INLINE Vector3<T> operator*(const T t, const Vector3<T>& v) noexcept
{
    return v * t;
}

template<typename T>
STDROMANO_HOST_DEVICE constexpr STDROMANO_FORCE_INLINE Vector3<T> operator/(const T t, const Vector3<T>& v) noexcept
{
    return Vector3<T>(t / v.x, t / v.y, t / v.z);
}

template<typename T>
STDROMANO_HOST_DEVICE constexpr STDROMANO_FORCE_INLINE T dot(const Vector3<T>& lhs, const Vector3<T>& rhs) noexcept
{
    return lhs.x * rhs.x + lhs.y * rhs.y + lhs.z * rhs.z;
}

template<typename T>
STDROMANO_HOST_DEVICE constexpr STDROMANO_FORCE_INLINE T length2(const Vector3<T>& v) noexcept
{
    return dot(v, v);
}

template<typename T>
STDROMANO_HOST_DEVICE STDROMANO_FORCE_INLINE T length(const Vector3<T>& v) noexcept
{
    return maths::sqrt(dot(v, v));
}

template<typename T>
STDROMANO_HOST_DEVICE STDROMANO_FORCE_INLINE T dist2(const Vector3<T>& a, const Vector3<T>& b) noexcept
{
    return length2(a - b);
}

template<typename T>
STDROMANO_HOST_DEVICE STDROMANO_FORCE_INLINE T dist(const Vector3<T>& a, const Vector3<T>& b) noexcept
{
    return length(a - b);
}

template<typename T>
STDROMANO_HOST_DEVICE STDROMANO_FORCE_INLINE Vector3<T> normalize(const Vector3<T>& v) noexcept
{
    return v * maths::rsqrt(dot(v, v));
}

template<typename T>
STDROMANO_HOST_DEVICE STDROMANO_FORCE_INLINE Vector3<T> normalize_safe(const Vector3<T>& v) noexcept
{
    return v * maths::rcp_safe(length(v));
}

template<typename T>
STDROMANO_HOST_DEVICE constexpr STDROMANO_FORCE_INLINE Vector3<T> cross(const Vector3<T>& lhs, const Vector3<T>& rhs) noexcept
{
    return Vector3<T>(lhs.y * rhs.z - lhs.z * rhs.y,
                      lhs.z * rhs.x - lhs.x * rhs.z,
                      lhs.x * rhs.y - lhs.y * rhs.x);
}

template<typename T>
STDROMANO_HOST_DEVICE STDROMANO_FORCE_INLINE Vector3<T> abs(const Vector3<T>& v) noexcept
{
    return Vector3<T>(maths::abs(v.x), maths::abs(v.y), maths::abs(v.z));
}

template<typename T>
STDROMANO_HOST_DEVICE STDROMANO_FORCE_INLINE Vector3<T> min(const Vector3<T>& a, const Vector3<T>& b) noexcept
{
    return Vector3<T>(maths::min(a.x, b.x), maths::min(a.y, b.y), maths::min(a.z, b.z));
}

template<typename T>
STDROMANO_HOST_DEVICE STDROMANO_FORCE_INLINE Vector3<T> max(const Vector3<T>& a, const Vector3<T>& b) noexcept
{
    return Vector3<T>(maths::max(a.x, b.x), maths::max(a.y, b.y), maths::max(a.z, b.z));
}

template<typename T>
STDROMANO_HOST_DEVICE STDROMANO_FORCE_INLINE T min_component(const Vector3<T>& v) noexcept
{
    return maths::min(v.x, maths::min(v.y, v.z));
}

template<typename T>
STDROMANO_HOST_DEVICE STDROMANO_FORCE_INLINE T max_component(const Vector3<T>& v) noexcept
{
    return maths::max(v.x, maths::max(v.y, v.z));
}

template<typename T>
STDROMANO_HOST_DEVICE STDROMANO_FORCE_INLINE Vector3<T> clamp(const Vector3<T>& v,
                                                              const T lower = make_zero_v<T>,
                                                              const T upper = make_one_v<T>) noexcept
{
    return Vector3<T>(maths::clamp(v.x, lower, upper),
                      maths::clamp(v.y, lower, upper),
                      maths::clamp(v.z, lower, upper));
}

template<typename T>
STDROMANO_HOST_DEVICE STDROMANO_FORCE_INLINE Vector3<T> lerp(const Vector3<T>& a, const Vector3<T>& b, const T t) noexcept
{
    return Vector3<T>(maths::lerp(a.x, b.x, t), maths::lerp(a.y, b.y, t), maths::lerp(a.z, b.z, t));
}

template<typename T>
STDROMANO_HOST_DEVICE STDROMANO_FORCE_INLINE Vector3<T> lerp(const Vector3<T>& a,
                                                             const Vector3<T>& b,
                                                             const Vector3<T>& t) noexcept
{
    return Vector3<T>(maths::lerp(a.x, b.x, t.x), maths::lerp(a.y, b.y, t.y), maths::lerp(a.z, b.z, t.z));
}

template<typename T>
STDROMANO_HOST_DEVICE STDROMANO_FORCE_INLINE Vector3<T> rcp(const Vector3<T>& v) noexcept
{
    return Vector3<T>(maths::rcp(v.x), maths::rcp(v.y), maths::rcp(v.z));
}

template<typename T>
STDROMANO_HOST_DEVICE STDROMANO_FORCE_INLINE Vector3<T> rcp_safe(const Vector3<T>& v) noexcept
{
    return Vector3<T>(maths::rcp_safe(v.x), maths::rcp_safe(v.y), maths::rcp_safe(v.z));
}

template<typename T>
STDROMANO_HOST_DEVICE STDROMANO_FORCE_INLINE Vector3<T> floor(const Vector3<T>& v) noexcept
{
    return Vector3<T>(maths::floor(v.x), maths::floor(v.y), maths::floor(v.z));
}

template<typename T>
STDROMANO_HOST_DEVICE STDROMANO_FORCE_INLINE Vector3<T> ceil(const Vector3<T>& v) noexcept
{
    return Vector3<T>(maths::ceil(v.x), maths::ceil(v.y), maths::ceil(v.z));
}

template<typename T>
STDROMANO_HOST_DEVICE STDROMANO_FORCE_INLINE Vector3<T> pow(const Vector3<T>& v, const T p) noexcept
{
    return Vector3<T>(maths::pow(v.x, p), maths::pow(v.y, p), maths::pow(v.z, p));
}

template<typename T>
STDROMANO_HOST_DEVICE STDROMANO_FORCE_INLINE Vector3<T> sign(const Vector3<T>& v) noexcept
{
    return Vector3<T>(maths::sign(v.x), maths::sign(v.y), maths::sign(v.z));
}

template<typename T>
STDROMANO_HOST_DEVICE STDROMANO_FORCE_INLINE bool isnan(const Vector3<T>& v) noexcept
{
    return maths::isnan(v.x) || maths::isnan(v.y) || maths::isnan(v.z);
}

template<typename T>
STDROMANO_HOST_DEVICE STDROMANO_FORCE_INLINE bool isinf(const Vector3<T>& v) noexcept
{
    return maths::isinf(v.x) || maths::isinf(v.y) || maths::isinf(v.z);
}

template<typename T>
STDROMANO_HOST_DEVICE STDROMANO_FORCE_INLINE bool isfinite(const Vector3<T>& v) noexcept
{
    return maths::isfinite(v.x) && maths::isfinite(v.y) && maths::isfinite(v.z);
}

template<typename T>
STDROMANO_HOST_DEVICE STDROMANO_FORCE_INLINE Vector3<T> reflect(const Vector3<T>& v, const Vector3<T>& n) noexcept
{
    return v - n * (T(2) * dot(v, n));
}

template<typename T>
STDROMANO_HOST_DEVICE STDROMANO_FORCE_INLINE T angle(const Vector3<T>& a, const Vector3<T>& b) noexcept
{
    return maths::atan2(length(cross(a, b)), dot(a, b));
}

using Vec3F = Vector3<float>;
using Vec3D = Vector3<double>;
using Vec3I = Vector3<std::int32_t>;
using Vec3U = Vector3<std::uint32_t>;

/********************************/
/* Vector4 */
/********************************/

template<typename T>
struct alignas(vector4_alignment_v<T>) Vector4
{
    static_assert(std::is_floating_point_v<T> || std::is_integral_v<T>,
                  "T needs to be floating-point or integral type");

    T x, y, z, w;

    STDROMANO_HOST_DEVICE constexpr Vector4() noexcept : x(make_zero_v<T>), y(make_zero_v<T>), z(make_zero_v<T>), w(make_zero_v<T>) {}
    STDROMANO_HOST_DEVICE constexpr Vector4(const T t) noexcept : x(t), y(t), z(t), w(t) {}
    STDROMANO_HOST_DEVICE constexpr Vector4(const T x, const T y, const T z, const T w) noexcept : x(x), y(y), z(z), w(w) {}
    STDROMANO_HOST_DEVICE constexpr Vector4(const Vector3<T>& xyz, const T w) noexcept : x(xyz.x), y(xyz.y), z(xyz.z), w(w) {}

    template<typename S>
    STDROMANO_HOST_DEVICE constexpr Vector4(const Vector4<S>& v) noexcept : x(T(v.x)), y(T(v.y)), z(T(v.z)), w(T(v.w)) {}

    /* Any other type with the same members, explicitly (float4 to Vec3F, Vec4F to Vec2F...) */
    template<typename V, std::enable_if_t<has_xyzw_v<V> && !is_cuda_vector_v<V, T, 4>, int> = 0>
    STDROMANO_HOST_DEVICE constexpr explicit Vector4(const V& v) noexcept : x(T(v.x)), y(T(v.y)), z(T(v.z)), w(T(v.w)) {}

    /* The CUDA vector type of the same component type and count converts implicitly, both ways */
    template<typename V, std::enable_if_t<is_cuda_vector_v<V, T, 4>, int> = 0>
    STDROMANO_HOST_DEVICE constexpr Vector4(const V& v) noexcept : x(v.x), y(v.y), z(v.z), w(v.w) {}

    template<typename V, std::enable_if_t<is_cuda_vector_v<V, T, 4>, int> = 0>
    STDROMANO_HOST_DEVICE constexpr operator V() const noexcept { return V{this->x, this->y, this->z, this->w}; }

    STDROMANO_HOST_DEVICE constexpr Vector2<T> xy() const noexcept { return Vector2<T>(this->x, this->y); }
    STDROMANO_HOST_DEVICE constexpr Vector3<T> xyz() const noexcept { return Vector3<T>(this->x, this->y, this->z); }

    STDROMANO_HOST_DEVICE constexpr Vector4 operator-() const noexcept { return Vector4(-this->x, -this->y, -this->z, -this->w); }
    STDROMANO_HOST_DEVICE constexpr Vector4 operator~() const noexcept { return Vector4(~this->x, ~this->y, ~this->z, ~this->w); }

    STDROMANO_HOST_DEVICE constexpr const T& operator[](const std::size_t i) const noexcept { return (&this->x)[i]; }
    STDROMANO_HOST_DEVICE constexpr T& operator[](const std::size_t i) noexcept { return (&this->x)[i]; }

    STDROMANO_HOST_DEVICE constexpr Vector4 operator+(const Vector4& other) const noexcept
    {
        return Vector4(this->x + other.x, this->y + other.y, this->z + other.z, this->w + other.w);
    }

    STDROMANO_HOST_DEVICE constexpr Vector4 operator-(const Vector4& other) const noexcept
    {
        return Vector4(this->x - other.x, this->y - other.y, this->z - other.z, this->w - other.w);
    }

    STDROMANO_HOST_DEVICE constexpr Vector4 operator*(const Vector4& other) const noexcept
    {
        return Vector4(this->x * other.x, this->y * other.y, this->z * other.z, this->w * other.w);
    }

    STDROMANO_HOST_DEVICE constexpr Vector4 operator/(const Vector4& other) const noexcept
    {
        return Vector4(this->x / other.x, this->y / other.y, this->z / other.z, this->w / other.w);
    }

    STDROMANO_HOST_DEVICE constexpr Vector4 operator+(const T other) const noexcept
    {
        return Vector4(this->x + other, this->y + other, this->z + other, this->w + other);
    }

    STDROMANO_HOST_DEVICE constexpr Vector4 operator-(const T other) const noexcept
    {
        return Vector4(this->x - other, this->y - other, this->z - other, this->w - other);
    }

    STDROMANO_HOST_DEVICE constexpr Vector4 operator*(const T other) const noexcept
    {
        return Vector4(this->x * other, this->y * other, this->z * other, this->w * other);
    }

    STDROMANO_HOST_DEVICE constexpr Vector4 operator/(const T other) const noexcept
    {
        if constexpr(std::is_floating_point_v<T>)
        {
            const T inv = maths::rcp_safe(other);
            return Vector4(this->x * inv, this->y * inv, this->z * inv, this->w * inv);
        }
        else
        {
            return Vector4(this->x / other, this->y / other, this->z / other, this->w / other);
        }
    }

    STDROMANO_HOST_DEVICE constexpr Vector4 operator&(const Vector4& other) const noexcept
    {
        return Vector4(this->x & other.x, this->y & other.y, this->z & other.z, this->w & other.w);
    }

    STDROMANO_HOST_DEVICE constexpr Vector4 operator|(const Vector4& other) const noexcept
    {
        return Vector4(this->x | other.x, this->y | other.y, this->z | other.z, this->w | other.w);
    }

    STDROMANO_HOST_DEVICE constexpr Vector4 operator^(const Vector4& other) const noexcept
    {
        return Vector4(this->x ^ other.x, this->y ^ other.y, this->z ^ other.z, this->w ^ other.w);
    }

    STDROMANO_HOST_DEVICE constexpr Vector4 operator&(const T other) const noexcept
    {
        return Vector4(this->x & other, this->y & other, this->z & other, this->w & other);
    }

    STDROMANO_HOST_DEVICE constexpr Vector4 operator|(const T other) const noexcept
    {
        return Vector4(this->x | other, this->y | other, this->z | other, this->w | other);
    }

    STDROMANO_HOST_DEVICE constexpr Vector4 operator^(const T other) const noexcept
    {
        return Vector4(this->x ^ other, this->y ^ other, this->z ^ other, this->w ^ other);
    }

    STDROMANO_HOST_DEVICE constexpr Vector4 operator<<(const unsigned int shift) const noexcept
    {
        return Vector4(this->x << shift, this->y << shift, this->z << shift, this->w << shift);
    }

    STDROMANO_HOST_DEVICE constexpr Vector4 operator>>(const unsigned int shift) const noexcept
    {
        return Vector4(this->x >> shift, this->y >> shift, this->z >> shift, this->w >> shift);
    }

    STDROMANO_HOST_DEVICE constexpr Vector4& operator+=(const Vector4& other) noexcept { return *this = *this + other; }
    STDROMANO_HOST_DEVICE constexpr Vector4& operator-=(const Vector4& other) noexcept { return *this = *this - other; }
    STDROMANO_HOST_DEVICE constexpr Vector4& operator*=(const Vector4& other) noexcept { return *this = *this * other; }
    STDROMANO_HOST_DEVICE constexpr Vector4& operator/=(const Vector4& other) noexcept { return *this = *this / other; }
    STDROMANO_HOST_DEVICE constexpr Vector4& operator+=(const T other) noexcept { return *this = *this + other; }
    STDROMANO_HOST_DEVICE constexpr Vector4& operator-=(const T other) noexcept { return *this = *this - other; }
    STDROMANO_HOST_DEVICE constexpr Vector4& operator*=(const T other) noexcept { return *this = *this * other; }
    STDROMANO_HOST_DEVICE constexpr Vector4& operator/=(const T other) noexcept { return *this = *this / other; }
    STDROMANO_HOST_DEVICE constexpr Vector4& operator&=(const Vector4& other) noexcept { return *this = *this & other; }
    STDROMANO_HOST_DEVICE constexpr Vector4& operator|=(const Vector4& other) noexcept { return *this = *this | other; }
    STDROMANO_HOST_DEVICE constexpr Vector4& operator^=(const Vector4& other) noexcept { return *this = *this ^ other; }
    STDROMANO_HOST_DEVICE constexpr Vector4& operator<<=(const unsigned int shift) noexcept { return *this = *this << shift; }
    STDROMANO_HOST_DEVICE constexpr Vector4& operator>>=(const unsigned int shift) noexcept { return *this = *this >> shift; }

    STDROMANO_HOST_DEVICE constexpr bool operator==(const Vector4& other) const noexcept
    {
        return this->x == other.x && this->y == other.y && this->z == other.z && this->w == other.w;
    }

    STDROMANO_HOST_DEVICE constexpr bool operator!=(const Vector4& other) const noexcept
    {
        return !this->operator==(other);
    }

    STDROMANO_HOST_DEVICE bool equal_with_abs_error(const Vector4& other, const T err) const noexcept
    {
        return maths::equal_with_abs_error(this->x, other.x, err) &&
               maths::equal_with_abs_error(this->y, other.y, err) &&
               maths::equal_with_abs_error(this->z, other.z, err) &&
               maths::equal_with_abs_error(this->w, other.w, err);
    }

    constexpr std::tuple<Vector3<T>, T> as_axis_angle() const noexcept
    {
        return std::make_tuple(Vector3<T>(this->x, this->y, this->z), this->w);
    }
};

template<typename T>
STDROMANO_HOST_DEVICE constexpr STDROMANO_FORCE_INLINE Vector4<T> operator*(const T t, const Vector4<T>& v) noexcept
{
    return v * t;
}

template<typename T>
STDROMANO_HOST_DEVICE constexpr STDROMANO_FORCE_INLINE Vector4<T> operator/(const T t, const Vector4<T>& v) noexcept
{
    return Vector4<T>(t / v.x, t / v.y, t / v.z, t / v.w);
}

template<typename T>
STDROMANO_HOST_DEVICE constexpr STDROMANO_FORCE_INLINE T dot(const Vector4<T>& lhs, const Vector4<T>& rhs) noexcept
{
    return lhs.x * rhs.x + lhs.y * rhs.y + lhs.z * rhs.z + lhs.w * rhs.w;
}

template<typename T>
STDROMANO_HOST_DEVICE constexpr STDROMANO_FORCE_INLINE T length2(const Vector4<T>& v) noexcept
{
    return dot(v, v);
}

template<typename T>
STDROMANO_HOST_DEVICE STDROMANO_FORCE_INLINE T length(const Vector4<T>& v) noexcept
{
    return maths::sqrt(dot(v, v));
}

template<typename T>
STDROMANO_HOST_DEVICE STDROMANO_FORCE_INLINE T dist2(const Vector4<T>& a, const Vector4<T>& b) noexcept
{
    return length2(a - b);
}

template<typename T>
STDROMANO_HOST_DEVICE STDROMANO_FORCE_INLINE T dist(const Vector4<T>& a, const Vector4<T>& b) noexcept
{
    return length(a - b);
}

template<typename T>
STDROMANO_HOST_DEVICE STDROMANO_FORCE_INLINE Vector4<T> normalize(const Vector4<T>& v) noexcept
{
    return v * maths::rsqrt(dot(v, v));
}

template<typename T>
STDROMANO_HOST_DEVICE STDROMANO_FORCE_INLINE Vector4<T> normalize_safe(const Vector4<T>& v) noexcept
{
    return v * maths::rcp_safe(length(v));
}

template<typename T>
STDROMANO_HOST_DEVICE STDROMANO_FORCE_INLINE Vector4<T> abs(const Vector4<T>& v) noexcept
{
    return Vector4<T>(maths::abs(v.x), maths::abs(v.y), maths::abs(v.z), maths::abs(v.w));
}

template<typename T>
STDROMANO_HOST_DEVICE STDROMANO_FORCE_INLINE Vector4<T> min(const Vector4<T>& a, const Vector4<T>& b) noexcept
{
    return Vector4<T>(maths::min(a.x, b.x), maths::min(a.y, b.y), maths::min(a.z, b.z), maths::min(a.w, b.w));
}

template<typename T>
STDROMANO_HOST_DEVICE STDROMANO_FORCE_INLINE Vector4<T> max(const Vector4<T>& a, const Vector4<T>& b) noexcept
{
    return Vector4<T>(maths::max(a.x, b.x), maths::max(a.y, b.y), maths::max(a.z, b.z), maths::max(a.w, b.w));
}

template<typename T>
STDROMANO_HOST_DEVICE STDROMANO_FORCE_INLINE T min_component(const Vector4<T>& v) noexcept
{
    return maths::min(maths::min(v.x, v.y), maths::min(v.z, v.w));
}

template<typename T>
STDROMANO_HOST_DEVICE STDROMANO_FORCE_INLINE T max_component(const Vector4<T>& v) noexcept
{
    return maths::max(maths::max(v.x, v.y), maths::max(v.z, v.w));
}

template<typename T>
STDROMANO_HOST_DEVICE STDROMANO_FORCE_INLINE Vector4<T> clamp(const Vector4<T>& v,
                                                              const T lower = make_zero_v<T>,
                                                              const T upper = make_one_v<T>) noexcept
{
    return Vector4<T>(maths::clamp(v.x, lower, upper),
                      maths::clamp(v.y, lower, upper),
                      maths::clamp(v.z, lower, upper),
                      maths::clamp(v.w, lower, upper));
}

template<typename T>
STDROMANO_HOST_DEVICE STDROMANO_FORCE_INLINE Vector4<T> lerp(const Vector4<T>& a, const Vector4<T>& b, const T t) noexcept
{
    return Vector4<T>(maths::lerp(a.x, b.x, t),
                      maths::lerp(a.y, b.y, t),
                      maths::lerp(a.z, b.z, t),
                      maths::lerp(a.w, b.w, t));
}

template<typename T>
STDROMANO_HOST_DEVICE STDROMANO_FORCE_INLINE Vector4<T> lerp(const Vector4<T>& a,
                                                             const Vector4<T>& b,
                                                             const Vector4<T>& t) noexcept
{
    return Vector4<T>(maths::lerp(a.x, b.x, t.x),
                      maths::lerp(a.y, b.y, t.y),
                      maths::lerp(a.z, b.z, t.z),
                      maths::lerp(a.w, b.w, t.w));
}

template<typename T>
STDROMANO_HOST_DEVICE STDROMANO_FORCE_INLINE Vector4<T> rcp(const Vector4<T>& v) noexcept
{
    return Vector4<T>(maths::rcp(v.x), maths::rcp(v.y), maths::rcp(v.z), maths::rcp(v.w));
}

template<typename T>
STDROMANO_HOST_DEVICE STDROMANO_FORCE_INLINE Vector4<T> rcp_safe(const Vector4<T>& v) noexcept
{
    return Vector4<T>(maths::rcp_safe(v.x), maths::rcp_safe(v.y), maths::rcp_safe(v.z), maths::rcp_safe(v.w));
}

template<typename T>
STDROMANO_HOST_DEVICE STDROMANO_FORCE_INLINE Vector4<T> floor(const Vector4<T>& v) noexcept
{
    return Vector4<T>(maths::floor(v.x), maths::floor(v.y), maths::floor(v.z), maths::floor(v.w));
}

template<typename T>
STDROMANO_HOST_DEVICE STDROMANO_FORCE_INLINE Vector4<T> ceil(const Vector4<T>& v) noexcept
{
    return Vector4<T>(maths::ceil(v.x), maths::ceil(v.y), maths::ceil(v.z), maths::ceil(v.w));
}

template<typename T>
STDROMANO_HOST_DEVICE STDROMANO_FORCE_INLINE Vector4<T> pow(const Vector4<T>& v, const T p) noexcept
{
    return Vector4<T>(maths::pow(v.x, p), maths::pow(v.y, p), maths::pow(v.z, p), maths::pow(v.w, p));
}

template<typename T>
STDROMANO_HOST_DEVICE STDROMANO_FORCE_INLINE Vector4<T> sign(const Vector4<T>& v) noexcept
{
    return Vector4<T>(maths::sign(v.x), maths::sign(v.y), maths::sign(v.z), maths::sign(v.w));
}

template<typename T>
STDROMANO_HOST_DEVICE STDROMANO_FORCE_INLINE bool isnan(const Vector4<T>& v) noexcept
{
    return maths::isnan(v.x) || maths::isnan(v.y) || maths::isnan(v.z) || maths::isnan(v.w);
}

template<typename T>
STDROMANO_HOST_DEVICE STDROMANO_FORCE_INLINE bool isinf(const Vector4<T>& v) noexcept
{
    return maths::isinf(v.x) || maths::isinf(v.y) || maths::isinf(v.z) || maths::isinf(v.w);
}

template<typename T>
STDROMANO_HOST_DEVICE STDROMANO_FORCE_INLINE bool isfinite(const Vector4<T>& v) noexcept
{
    return maths::isfinite(v.x) && maths::isfinite(v.y) && maths::isfinite(v.z) && maths::isfinite(v.w);
}

using Vec4F = Vector4<float>;
using Vec4D = Vector4<double>;
using Vec4I = Vector4<std::int32_t>;
using Vec4U = Vector4<std::uint32_t>;

/********************************/
/* CUDA interop                 */
/********************************/

#if defined(STDROMANO_CUDA)
/* Same layout as the CUDA vector types, so device buffers can be reinterpreted both ways */
static_assert(sizeof(Vec2F) == sizeof(float2) && alignof(Vec2F) == alignof(float2), "Vec2F/float2 layout mismatch");
static_assert(sizeof(Vec3F) == sizeof(float3) && alignof(Vec3F) == alignof(float3), "Vec3F/float3 layout mismatch");
static_assert(sizeof(Vec4F) == sizeof(float4) && alignof(Vec4F) == alignof(float4), "Vec4F/float4 layout mismatch");
static_assert(sizeof(Vec2I) == sizeof(int2) && alignof(Vec2I) == alignof(int2), "Vec2I/int2 layout mismatch");
static_assert(sizeof(Vec3I) == sizeof(int3) && alignof(Vec3I) == alignof(int3), "Vec3I/int3 layout mismatch");
static_assert(sizeof(Vec4I) == sizeof(int4) && alignof(Vec4I) == alignof(int4), "Vec4I/int4 layout mismatch");
static_assert(sizeof(Vec2U) == sizeof(uint2) && alignof(Vec2U) == alignof(uint2), "Vec2U/uint2 layout mismatch");
static_assert(sizeof(Vec3U) == sizeof(uint3) && alignof(Vec3U) == alignof(uint3), "Vec3U/uint3 layout mismatch");
static_assert(sizeof(Vec4U) == sizeof(uint4) && alignof(Vec4U) == alignof(uint4), "Vec4U/uint4 layout mismatch");
static_assert(sizeof(Vec2D) == sizeof(double2) && alignof(Vec2D) == alignof(double2), "Vec2D/double2 layout mismatch");

STDROMANO_HOST_DEVICE STDROMANO_FORCE_INLINE float2 to_cuda(const Vec2F& v) noexcept { return make_float2(v.x, v.y); }
STDROMANO_HOST_DEVICE STDROMANO_FORCE_INLINE float3 to_cuda(const Vec3F& v) noexcept { return make_float3(v.x, v.y, v.z); }
STDROMANO_HOST_DEVICE STDROMANO_FORCE_INLINE float4 to_cuda(const Vec4F& v) noexcept { return make_float4(v.x, v.y, v.z, v.w); }
STDROMANO_HOST_DEVICE STDROMANO_FORCE_INLINE int2 to_cuda(const Vec2I& v) noexcept { return make_int2(v.x, v.y); }
STDROMANO_HOST_DEVICE STDROMANO_FORCE_INLINE int3 to_cuda(const Vec3I& v) noexcept { return make_int3(v.x, v.y, v.z); }
STDROMANO_HOST_DEVICE STDROMANO_FORCE_INLINE int4 to_cuda(const Vec4I& v) noexcept { return make_int4(v.x, v.y, v.z, v.w); }
STDROMANO_HOST_DEVICE STDROMANO_FORCE_INLINE uint2 to_cuda(const Vec2U& v) noexcept { return make_uint2(v.x, v.y); }
STDROMANO_HOST_DEVICE STDROMANO_FORCE_INLINE uint3 to_cuda(const Vec3U& v) noexcept { return make_uint3(v.x, v.y, v.z); }
STDROMANO_HOST_DEVICE STDROMANO_FORCE_INLINE uint4 to_cuda(const Vec4U& v) noexcept { return make_uint4(v.x, v.y, v.z, v.w); }
STDROMANO_HOST_DEVICE STDROMANO_FORCE_INLINE double2 to_cuda(const Vec2D& v) noexcept { return make_double2(v.x, v.y); }
#endif /* defined(STDROMANO_CUDA) */

STDROMANO_NAMESPACE_END

#endif /* !defined(__STDROMANO_LINALG_VECTOR) */
