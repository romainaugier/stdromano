// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - Present Romain Augier
// All rights reserved.

#pragma once

#if !defined(__STDROMANO_LINALG_VECTOR)
#define __STDROMANO_LINALG_VECTOR

#include "stdromano/linalg/traits.hpp"
#include "stdromano/maths.hpp"

#include <tuple>

STDROMANO_NAMESPACE_BEGIN

/* 
    We use structs because it is convenient to be able to call members 
    instead of using getters/setters that would overcomplicate the code 
*/

/********************************/
/* Vector2 */
/********************************/

template<typename T>
struct Vector2
{
    static_assert(std::is_floating_point_v<T> || std::is_integral_v<T>,
                  "T needs to be floating-point or integral type");

    T x, y;

    Vector2() : x(make_zero_v<T>), y(make_zero_v<T>) {}
    Vector2(T t) : x(t), y(t) {}
    Vector2(T x, T y) : x(x), y(y) {}

    Vector2 operator-() const noexcept { return Vector2(-this->x, -this->y); }

    const T& operator[](std::size_t i) const noexcept { return &(this->x)[i]; }
    T& operator[](std::size_t i) noexcept { return &(this->x)[i]; }

    Vector2 operator+(const Vector2& other) const noexcept
    {
        return Vector2(this->x + other.x, this->y + other.y);
    }

    Vector2 operator-(const Vector2& other) const noexcept
    {
        return Vector2(this->x - other.x, this->y - other.y);
    }

    Vector2 operator*(const Vector2& other) const noexcept
    {
        return Vector2(this->x * other.x, this->y * other.y);
    }

    Vector2 operator/(const Vector2& other) const noexcept
    {
        return Vector2(this->x / other.x, this->y / other.y);
    }

    Vector2 operator+(const T& other) const noexcept
    {
        return Vector2(this->x + other, this->y + other);
    }

    Vector2 operator-(const T& other) const noexcept
    {
        return Vector2(this->x - other, this->y - other);
    }

    Vector2 operator*(const T& other) const noexcept
    {
        return Vector2(this->x * other, this->y * other);
    }

    Vector2 operator/(const T& other) const noexcept
    {
        return Vector2(this->x / other, this->y / other);
    }

    Vector2& operator+=(const Vector2& other) noexcept
    {
        this->x += other.x;
        this->y += other.y;
        return *this;
    }

    Vector2& operator-=(const Vector2& other) noexcept
    {
        this->x -= other.x;
        this->y -= other.y;
        return *this;
    }

    Vector2& operator*=(const Vector2& other) noexcept
    {
        this->x *= other.x;
        this->y *= other.y;
        return *this;
    }

    Vector2& operator/=(const Vector2& other) noexcept
    {
        this->x /= other.x;
        this->y /= other.y;
        return *this;
    }

    Vector2& operator+=(const T other) noexcept
    {
        this->x += other;
        this->y += other;
        return *this;
    }

    Vector2& operator-=(const T other) noexcept
    {
        this->x -= other;
        this->y -= other;
        return *this;
    }

    Vector2& operator*=(const T other) noexcept
    {
        this->x *= other;
        this->y *= other;
        return *this;
    }

    Vector2& operator/=(const T other) noexcept
    {
        this->x /= other;
        this->y /= other;
        return *this;
    }
};

template<typename T>
T operator*(const T t, const Vector2<T>& v) noexcept
{
    return v * t;
}

template<typename T>
T operator/(const T t, const Vector2<T>& v) noexcept
{
    return Vector2<T>(t / v.x, t / v.y);
}

template<typename T>
STDROMANO_FORCE_INLINE T dot(const Vector2<T>& lhs, const Vector2<T>& rhs) noexcept
{
    return lhs.x * rhs.x + lhs.y * rhs.y;
}

template<typename T>
STDROMANO_FORCE_INLINE T length(const Vector2<T>& v) noexcept
{
    return ::sqrt(dot(v, v));
}

template<typename T>
STDROMANO_FORCE_INLINE T length2(const Vector2<T>& v) noexcept
{
    return dot(v, v);
}

template<typename T>
STDROMANO_FORCE_INLINE Vector2<T> normalize(const Vector2<T>& v) noexcept
{
    T t = make_one_v<T> / length(v);
    return Vector2<T>(v.x * t, v.y * t);
}

template<typename T>
STDROMANO_FORCE_INLINE std::enable_if<std::is_same_v<T, float>, Vector2<float>> normalize(const Vector2<float>& v) noexcept
{
    float t = maths::rsqrt(dot(v, v));
    return Vector2<float>(v.x * t, v.y * t);
}

using Vec2F = Vector2<float>;
using Vec2D = Vector2<double>;
using Vec2I = Vector2<std::int32_t>;

/********************************/
/* Vector3 */
/********************************/

template<typename T>
struct Vector3
{
    static_assert(std::is_floating_point_v<T> || std::is_integral_v<T>,
                  "T needs to be floating-point or integral type");

    T x, y, z;

    Vector3() : x(make_zero_v<T>), y(make_zero_v<T>), z(make_zero_v<T>) {}
    Vector3(T t) : x(t), y(t), z(t) {}
    Vector3(T x, T y, T z) : x(x), y(y), z(z) {}

    Vector3 operator-() const noexcept { return Vector3(-this->x, -this->y, -this->z); }

    const T& operator[](std::size_t i) const noexcept { return &(this->x)[i]; }
    T& operator[](std::size_t i) noexcept { return &(this->x)[i]; }

    Vector3 operator+(const Vector3& other) const noexcept
    {
        return Vector3(this->x + other.x, this->y + other.y, this->z + other.z);
    }

    Vector3 operator-(const Vector3& other) const noexcept
    {
        return Vector3(this->x - other.x, this->y - other.y, this->z - other.z);
    }

    Vector3 operator*(const Vector3& other) const noexcept
    {
        return Vector3(this->x * other.x, this->y * other.y, this->z * other.z);
    }

    Vector3 operator/(const Vector3& other) const noexcept
    {
        return Vector3(this->x / other.x, this->y / other.y, this->z / other.z);
    }

    Vector3 operator+(const T& other) const noexcept
    {
        return Vector3(this->x + other, this->y + other, this->z + other);
    }

    Vector3 operator-(const T& other) const noexcept
    {
        return Vector3(this->x - other, this->y - other, this->z - other);
    }

    Vector3 operator*(const T& other) const noexcept
    {
        return Vector3(this->x * other, this->y * other, this->z * other);
    }

    Vector3 operator/(const T& other) const noexcept
    {
        return Vector3(this->x / other, this->y / other, this->z / other);
    }

    Vector3& operator+=(const Vector3& other) noexcept
    {
        this->x += other.x;
        this->y += other.y;
        this->z += other.z;
        return *this;
    }

    Vector3& operator-=(const Vector3& other) noexcept
    {
        this->x -= other.x;
        this->y -= other.y;
        this->z -= other.z;
        return *this;
    }

    Vector3& operator*=(const Vector3& other) noexcept
    {
        this->x *= other.x;
        this->y *= other.y;
        this->z *= other.z;
        return *this;
    }

    Vector3& operator/=(const Vector3& other) noexcept
    {
        this->x /= other.x;
        this->y /= other.y;
        this->z /= other.z;
        return *this;
    }

    Vector3& operator+=(const T other) noexcept
    {
        this->x += other;
        this->y += other;
        this->z += other;
        return *this;
    }

    Vector3& operator-=(const T other) noexcept
    {
        this->x -= other;
        this->y -= other;
        this->z -= other;
        return *this;
    }

    Vector3& operator*=(const T other) noexcept
    {
        this->x *= other;
        this->y *= other;
        this->z *= other;
        return *this;
    }

    Vector3& operator/=(const T other) noexcept
    {
        this->x /= other;
        this->y /= other;
        this->z /= other;
        return *this;
    }
};

template<typename T>
T operator*(const T t, const Vector3<T>& v) noexcept
{
    return v * t;
}

template<typename T>
T operator/(const T t, const Vector3<T>& v) noexcept
{
    return Vector3<T>(t / v.x, t / v.y, t / v.z);
}

template<typename T>
STDROMANO_FORCE_INLINE T dot(const Vector3<T>& lhs, const Vector3<T>& rhs) noexcept
{
    return lhs.x * rhs.x + lhs.y * rhs.y + lhs.z * rhs.z;
}

template<typename T>
STDROMANO_FORCE_INLINE T length(const Vector3<T>& v) noexcept
{
    return ::sqrt(dot(v, v));
}

template<typename T>
STDROMANO_FORCE_INLINE T length2(const Vector3<T>& v) noexcept
{
    return dot(v, v);
}

template<typename T>
STDROMANO_FORCE_INLINE Vector3<T> normalize(const Vector3<T>& v) noexcept
{
    T t = make_one_v<T> / length(v);
    return Vector3<T>(v.x * t, v.y * t, v.z * t);
}

template<typename T>
STDROMANO_FORCE_INLINE std::enable_if<std::is_same_v<T, float>, Vector3<float>> normalize(const Vector3<float>& v) noexcept
{
    float t = maths::rsqrt(dot(v, v));
    return Vector3<float>(v.x * t, v.y * t, v.z * t);
}

template<typename T>
STDROMANO_FORCE_INLINE Vector3<T> cross(const Vector3<T>& lhs, const Vector3<T>& rhs) noexcept
{
    return Vector3<T>(lhs.y * rhs.z - lhs.z * rhs.y,
                      lhs.z * rhs.x - lhs.x * rhs.z,
                      lhs.x * rhs.y - lhs.y * rhs.x);
}

using Vec3F = Vector3<float>;
using Vec3D = Vector3<double>;
using Vec3I = Vector3<std::int32_t>;

/********************************/
/* Vector4 */
/********************************/

template<typename T>
struct Vector4
{
    static_assert(std::is_floating_point_v<T> || std::is_integral_v<T>,
                  "T needs to be floating-point or integral type");

    T x, y, z, w;

    Vector4() : x(make_zero_v<T>), y(make_zero_v<T>), z(make_zero_v<T>), w(make_zero_v<T>) {}
    Vector4(T t) : x(t), y(t), z(t), w(w) {}
    Vector4(T x, T y, T z, T w) : x(x), y(y), z(z), w(w) {}

    Vector4 operator-() const noexcept { return Vector4(-this->x, -this->y, -this->z, -this->w); }

    const T& operator[](std::size_t i) const noexcept { return &(this->x)[i]; }
    T& operator[](std::size_t i) noexcept { return &(this->x)[i]; }

    Vector4 operator+(const Vector4& other) const noexcept
    {
        return Vector4(this->x + other.x, this->y + other.y, this->z + other.z, this->w + other.w);
    }

    Vector4 operator-(const Vector4& other) const noexcept
    {
        return Vector4(this->x - other.x, this->y - other.y, this->z - other.z, this->w + other.w);
    }

    Vector4 operator*(const Vector4& other) const noexcept
    {
        return Vector4(this->x * other.x, this->y * other.y, this->z * other.z, this->w * other.w);
    }

    Vector4 operator/(const Vector4& other) const noexcept
    {
        return Vector4(this->x / other.x, this->y / other.y, this->z / other.z, this->w * other.w);
    }

    Vector4 operator+(const T& other) const noexcept
    {
        return Vector4(this->x + other, this->y + other, this->z + other, this->w + other);
    }

    Vector4 operator-(const T& other) const noexcept
    {
        return Vector4(this->x - other, this->y - other, this->z - other, this->w - other);
    }

    Vector4 operator*(const T& other) const noexcept
    {
        return Vector4(this->x * other, this->y * other, this->z * other, this->w * other);
    }

    Vector4 operator/(const T& other) const noexcept
    {
        return Vector4(this->x / other, this->y / other, this->z / other, this->w / other);
    }

    Vector4& operator+=(const Vector4& other) noexcept
    {
        this->x += other.x;
        this->y += other.y;
        this->z += other.z;
        this->w += other.w;
        return *this;
    }

    Vector4& operator-=(const Vector4& other) noexcept
    {
        this->x -= other.x;
        this->y -= other.y;
        this->z -= other.z;
        this->w -= other.w;
        return *this;
    }

    Vector4& operator*=(const Vector4& other) noexcept
    {
        this->x *= other.x;
        this->y *= other.y;
        this->z *= other.z;
        this->w *= other.w;
        return *this;
    }

    Vector4& operator/=(const Vector4& other) noexcept
    {
        this->x /= other.x;
        this->y /= other.y;
        this->z /= other.z;
        this->w /= other.w;
        return *this;
    }

    Vector4& operator+=(const T other) noexcept
    {
        this->x += other;
        this->y += other;
        this->z += other;
        this->w += other;
        return *this;
    }

    Vector4& operator-=(const T other) noexcept
    {
        this->x -= other;
        this->y -= other;
        this->z -= other;
        this->w -= other;
        return *this;
    }

    Vector4& operator*=(const T other) noexcept
    {
        this->x *= other;
        this->y *= other;
        this->z *= other;
        this->w *= other;
        return *this;
    }

    Vector4& operator/=(const T other) noexcept
    {
        this->x /= other;
        this->y /= other;
        this->z /= other;
        this->w /= other;
        return *this;
    }

    std::tuple<Vector3<T>, T> as_axis_angle() const noexcept
    {
        return std::make_tuple(Vector3<T>(this->x, this->y, this->z), this->w);
    }
};

template<typename T>
T operator*(const T t, const Vector4<T>& v) noexcept
{
    return v * t;
}

template<typename T>
T operator/(const T t, const Vector4<T>& v) noexcept
{
    return Vector4(t / v.x, t / v.y, t / v.z, t / v.w);
}

template<typename T>
STDROMANO_FORCE_INLINE T dot(const Vector4<T>& lhs, const Vector4<T>& rhs) noexcept
{
    return lhs.x * rhs.x + lhs.y * rhs.y + lhs.z * rhs.z + lhs.w * rhs.w;
}

template<typename T>
STDROMANO_FORCE_INLINE T length(const Vector4<T>& v) noexcept
{
    return ::sqrt(dot(v, v));
}

template<typename T>
STDROMANO_FORCE_INLINE T length2(const Vector4<T>& v) noexcept
{
    return dot(v, v);
}

template<typename T>
STDROMANO_FORCE_INLINE Vector4<T> normalize(const Vector4<T>& v) noexcept
{
    T t = make_one_v<T> / length(v);
    return Vector4<T>(v.x * t, v.y * t, v.z * t, v.w * t);
}

template<typename T>
STDROMANO_FORCE_INLINE std::enable_if<std::is_same_v<T, float>, Vector4<float>> normalize(const Vector4<float>& v) noexcept
{
    float t = maths::rsqrt(dot(v, v));
    return Vector4<float>(v.x * t, v.y * t, v.z * t, v.w * t);
}

using Vec4F = Vector4<float>;
using Vec4D = Vector4<double>;
using Vec4I = Vector4<std::int32_t>;

STDROMANO_NAMESPACE_END

#endif /* !defined(__STDROMANO_LINALG_VECTOR) */