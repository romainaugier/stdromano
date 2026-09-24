// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - Present Romain Augier
// All rights reserved.

#pragma once

#if !defined(__STDROMANO_LINALG_TRANSFORM)
#define __STDROMANO_LINALG_TRANSFORM

#include "stdromano/linalg/vector.hpp"
#include "stdromano/simd.hpp"
#include "stdromano/expected.hpp"

STDROMANO_NAMESPACE_BEGIN

/*
    Data in those matrices is stored in row-major layout
*/

/********************************/
/* Transform33 */
/********************************/

enum TransformOrder2D_ : std::uint32_t
{
    TransformOrder2D_SRT,
    TransformOrder2D_STR,
    TransformOrder2D_RST,
    TransformOrder2D_RTS,
    TransformOrder2D_TSR,
    TransformOrder2D_TRS,
};

template<typename T>
class Transform33
{
public:
    template<typename S>
    friend class Transform33;

    template<typename S>
    friend class Transform44;

private:
    T _data[9];

public:
    constexpr explicit Transform33() noexcept {}

    constexpr Transform33(T m00, T m01, T m02,
                          T m10, T m11, T m12,
                          T m20, T m21, T m22) noexcept
    {
        this->_data[0] = m00;
        this->_data[1] = m01;
        this->_data[2] = m02;
        this->_data[3] = m10;
        this->_data[4] = m11;
        this->_data[5] = m12;
        this->_data[6] = m20;
        this->_data[7] = m21;
        this->_data[8] = m22;
    }

    template<typename S>
    constexpr Transform33(const Transform33<S>& m) noexcept
    {
        for(std::size_t i = 0; i < 9; ++i)
            this->_data[i] = T(m._data[i]);
    }

    constexpr static Transform33<T> zero() noexcept
    {
        Transform33<T> tr;

        for(std::size_t i = 0; i < 9; i++)
            tr._data[i] = make_zero_v<T>;

        return tr;
    }

    constexpr static Transform33<T> ident() noexcept
    {
        return Transform33<T>(make_one_v<T>, make_zero_v<T>, make_zero_v<T>,
                              make_zero_v<T>, make_one_v<T>, make_zero_v<T>,
                              make_zero_v<T>, make_zero_v<T>, make_one_v<T>);
    }

    constexpr static Transform33<T> from_translation(const Vector2<T>& t) noexcept
    {
        Transform33<T> tr = Transform33<T>::ident();

        tr._data[2] = t.x;
        tr._data[5] = t.y;

        return tr;
    }

    constexpr static Transform33<T> from_scale(const Vector2<T>& s) noexcept
    {
        Transform33<T> tr = Transform33<T>::ident();

        tr._data[0] = s.x;
        tr._data[4] = s.y;

        return tr;
    }

    constexpr static Transform33<T> from_uniform_scale(const T s) noexcept
    {
        Transform33<T> tr = Transform33<T>::ident();

        tr._data[0] = s;
        tr._data[4] = s;

        return tr;
    }

    /*
        R(θ) = [ cosθ  sinθ  0
                -sinθ  cosθ  0
                  0      0   1 ]

        (formula is transposed as storage is row-major)

        Rotation is clockwise
    */
    constexpr static Transform33<T> from_rotation(const T angle, const bool radians = true) noexcept
    {
        Transform33<T> tr = Transform33<T>::ident();

        T theta = radians ? angle : maths::deg2rad(angle);

        T c, s;
        maths::sincos(theta, &s, &c);

        tr._data[0] = c;
        tr._data[1] = s;
        tr._data[3] = -s;
        tr._data[4] = c;

        return tr;
    }

    /*
        Builds a transform from translation, rotation angle and scale given the transform order.
        Rotation is in radians by default
    */
    constexpr static Transform33<T> from_trs(const Vector2<T>& translation,
                                             const T rotation,
                                             const Vector2<T>& scale,
                                             const std::uint32_t transform_order = TransformOrder2D_TRS,
                                             const bool radians = true) noexcept
    {
        const Transform33<T> Tr = Transform33<T>::from_translation(translation);
        const Transform33<T> R = Transform33<T>::from_rotation(rotation, radians);
        const Transform33<T> S = Transform33<T>::from_scale(scale);

        switch(transform_order)
        {
            case TransformOrder2D_SRT:
                return S * R * Tr;
            case TransformOrder2D_STR:
                return S * Tr * R;
            case TransformOrder2D_RST:
                return R * S * Tr;
            case TransformOrder2D_RTS:
                return R * Tr * S;
            case TransformOrder2D_TSR:
                return Tr * S * R;
            case TransformOrder2D_TRS:
                return Tr * R * S;
            default:
                STDROMANO_ASSERT(0, "Invalid transform order");
                return Tr * R * S;
        }
    }

    /*
        Builds a transform from x and y axes and translation
    */
    constexpr static Transform33<T> from_xyt(const Vector2<T>& x,
                                             const Vector2<T>& y,
                                             const Vector2<T>& t) noexcept
    {
        return Transform33<T>(x.x, x.y, t.x,
                              y.x, y.y, t.y,
                              make_zero_v<T>, make_zero_v<T>, make_one_v<T>);
    }

    /*
        Builds a 2D look-at transform. The x-axis points from eye towards target,
        the y-axis is perpendicular (rotated 90 degrees CCW)
    */
    constexpr static Transform33<T> from_lookat(const Vector2<T>& eye,
                                                const Vector2<T>& target) noexcept
    {
        const Vector2<T> forward = normalize(target - eye);
        const Vector2<T> right(-forward.y, forward.x);

        return Transform33<T>(forward.x, forward.y, -dot(forward, eye),
                              right.x, right.y, -dot(right, eye),
                              make_zero_v<T>, make_zero_v<T>, make_one_v<T>);
    }

    constexpr T& operator()(const std::size_t row, const std::size_t col) noexcept
    {
        STDROMANO_ASSERT(row < 3 && col < 3, "Out of bounds access");

        return this->_data[row * 3 + col];
    }

    constexpr const T& operator()(const std::size_t row, const std::size_t col) const noexcept
    {
        STDROMANO_ASSERT(row < 3 && col < 3, "Out of bounds access");

        return this->_data[row * 3 + col];
    }

    constexpr T& at(const std::size_t row, const std::size_t col) noexcept
    {
        STDROMANO_ASSERT(row < 3 && col < 3, "Out of bounds access");

        return this->_data[row * 3 + col];
    }

    constexpr const T& at(const std::size_t row, const std::size_t col) const noexcept
    {
        STDROMANO_ASSERT(row < 3 && col < 3, "Out of bounds access");

        return this->_data[row * 3 + col];
    }

    constexpr T* data() noexcept
    {
        return this->_data;
    }

    constexpr const T* data() const noexcept
    {
        return this->_data;
    }

    constexpr Transform33<T> operator*(const Transform33<T>& other) const noexcept
    {
        Transform33<T> res = Transform33<T>::zero();

        for(std::size_t i = 0; i < 3; ++i)
            for(std::size_t j = 0; j < 3; ++j)
                for(std::size_t k = 0; k < 3; ++k)
                    res._data[i * 3 + j] = maths::fma(this->_data[i * 3 + k], other._data[k * 3 + j], res._data[i * 3 + j]);

        return res;
    }

    constexpr Transform33<T> transpose() const noexcept
    {
        return Transform33<T>(this->_data[0], this->_data[3], this->_data[6],
                              this->_data[1], this->_data[4], this->_data[7],
                              this->_data[2], this->_data[5], this->_data[8]);
    }

    constexpr T trace() const noexcept
    {
        return this->_data[0] + this->_data[4] + this->_data[8];
    }

    /*
        Computes the determinant of the 3x3 matrix
    */
    constexpr T determinant() const noexcept
    {
        const T a = this->_data[0];
        const T b = this->_data[1];
        const T c = this->_data[2];

        return a * (this->_data[4] * this->_data[8] - this->_data[5] * this->_data[7]) -
               b * (this->_data[3] * this->_data[8] - this->_data[5] * this->_data[6]) +
               c * (this->_data[3] * this->_data[7] - this->_data[4] * this->_data[6]);
    }

    /*
        Computes the inverse using the adjugate method.
        Assumes the matrix is invertible (determinant != 0)
    */
    constexpr Expected<Transform33<T>> inverse() const noexcept
    {
        T det = this->determinant();

        if(!(maths::abs(det) > maths::constants<T>::large_epsilon))
            return Error("Matrix is singular");

        T inv_det = maths::rcp(det);

        Transform33<T> res;

        res._data[0] =  (this->_data[4] * this->_data[8] - this->_data[5] * this->_data[7]) * inv_det; // C00
        res._data[1] = -(this->_data[1] * this->_data[8] - this->_data[2] * this->_data[7]) * inv_det; // C10 (transposed)
        res._data[2] =  (this->_data[1] * this->_data[5] - this->_data[2] * this->_data[4]) * inv_det; // C20 (transposed)

        res._data[3] = -(this->_data[3] * this->_data[8] - this->_data[5] * this->_data[6]) * inv_det; // C01 (transposed)
        res._data[4] =  (this->_data[0] * this->_data[8] - this->_data[2] * this->_data[6]) * inv_det; // C11
        res._data[5] = -(this->_data[0] * this->_data[5] - this->_data[2] * this->_data[3]) * inv_det; // C21 (transposed)

        res._data[6] =  (this->_data[3] * this->_data[7] - this->_data[4] * this->_data[6]) * inv_det; // C02 (transposed)
        res._data[7] = -(this->_data[0] * this->_data[7] - this->_data[1] * this->_data[6]) * inv_det; // C12 (transposed)
        res._data[8] =  (this->_data[0] * this->_data[4] - this->_data[1] * this->_data[3]) * inv_det; // C22

        return res;
    }

    /*
        Transforms a 2D point (applies rotation, scale and translation)
    */
    constexpr Vector2<T> transform_point(const Vector2<T>& point) const noexcept
    {
        Vector2<T> res;

        res.x = point.x * this->_data[0] + point.y * this->_data[1] + this->_data[2];
        res.y = point.x * this->_data[3] + point.y * this->_data[4] + this->_data[5];

        return res;
    }

    /*
        Transforms a 2D direction (applies rotation and scale only, no translation)
    */
    constexpr Vector2<T> transform_dir(const Vector2<T>& dir) const noexcept
    {
        Vector2<T> res;

        res.x = dir.x * this->_data[0] + dir.y * this->_data[1];
        res.y = dir.x * this->_data[3] + dir.y * this->_data[4];

        return res;
    }

    // Compute the svd from eigenvalues and eigenvectors of A^T * A
    constexpr void svd(Transform33<T>* S, Transform33<T>* V, Transform33<T>* U) const noexcept
    {
        // A^T * A = | a  b |
        //           | b  c |
        const T a = maths::sqr(this->_data[0]) + maths::sqr(this->_data[3]);
        const T b = this->_data[0] * this->_data[1] + this->_data[3] * this->_data[4];
        const T c = maths::sqr(this->_data[1]) + maths::sqr(this->_data[4]);

        const T q = maths::sqrt(maths::sqr(a - c) + T(4) * maths::sqr(b));

        // Eigenvalues
        const T lambda0 = ((a + c) + q) / T(2);
        const T lambda1 = ((a + c) - q) / T(2);

        const T sigma0 = maths::sqrt(lambda0);
        const T sigma1 = maths::sqrt(lambda1);

        *S = Transform33<T>::ident();
        S->operator()(0, 0) = sigma0;
        S->operator()(1, 1) = sigma1;

        // V (eigenvectors of A^T A)
        if(maths::abs(b) > maths::constants<T>::large_epsilon)
        {
            *V = Transform33<T>::ident();

            T vx = b;
            T vy = lambda0 - a;

            T inv_len = make_one_v<T> / maths::sqrt(maths::sqr(vx) + maths::sqr(vy));
            vx *= inv_len;
            vy *= inv_len;

            V->operator()(0, 0) = vx;
            V->operator()(0, 1) = vy;
            V->operator()(1, 0) = -vy;
            V->operator()(1, 1) = vx;
        }
        else if(a < c)
        {
            *V = Transform33<T>::zero();
            V->operator()(0, 1) = make_one_v<T>;
            V->operator()(1, 0) = make_one_v<T>;
        }
        else
        {
            *V = Transform33<T>::ident();
        }

        // U = A * V * sigma.inv;
        *U = Transform33<T>::ident();

        T inv_sigma0 = sigma0 > maths::constants<T>::large_epsilon ? make_one_v<T> / sigma0 : make_zero_v<T>;
        T inv_sigma1 = sigma1 > maths::constants<T>::large_epsilon ? make_one_v<T> / sigma1 : make_zero_v<T>;

        U->operator()(0, 0) = (this->_data[0] * V->operator()(0, 0) + this->_data[1] * V->operator()(1, 0)) * inv_sigma0;
        U->operator()(1, 0) = (this->_data[3] * V->operator()(0, 0) + this->_data[4] * V->operator()(1, 0)) * inv_sigma0;

        U->operator()(0, 1) = (this->_data[0] * V->operator()(0, 1) + this->_data[1] * V->operator()(1, 1)) * inv_sigma1;
        U->operator()(1, 1) = (this->_data[3] * V->operator()(0, 1) + this->_data[4] * V->operator()(1, 1)) * inv_sigma1;

        // Normalize first column
        T len = maths::sqrt(maths::sqr(U->operator()(0,0)) + maths::sqr(U->operator()(1,0)));

        if(len > maths::constants<T>::large_epsilon)
        {
            U->operator()(0,0) /= len;
            U->operator()(1,0) /= len;
        }

        // Make second column perpendicular
        U->operator()(0,1) = -U->operator()(1,0);
        U->operator()(1,1) =  U->operator()(0,0);
    }

    /*
        Extracts the translation from the matrix
    */
    constexpr void decomp_translation(Vector2<T>* t) const noexcept
    {
        STDROMANO_ASSERT(t != nullptr, "t is nullptr");

        t->x = this->_data[2];
        t->y = this->_data[5];
    }

    /*
        Extracts the scale from the matrix (row magnitudes of the upper-left 2x2)
    */
    void decomp_scale(Vector2<T>* s) const noexcept
    {
        STDROMANO_ASSERT(s != nullptr, "s is nullptr");

        const T a = this->_data[0];
        const T b = this->_data[1];
        const T c = this->_data[3];
        const T d = this->_data[4];

        const T eps = maths::constants<T>::large_epsilon;

        const T x = a + d;
        const T y = c - b;

        T scale = maths::sqrt(maths::sqr(x) + maths::sqr(y));

        T r00, r01, r10, r11;

        if(scale > eps)
        {
            r00 =  x / scale;
            r01 = -y / scale;
            r10 =  y / scale;
            r11 =  x / scale;
        }
        else
        {
            r00 = 1;
            r01 = 0;
            r10 = 0;
            r11 = 1;
        }

        s->x = r00 * a + r10 * c;
        s->y = r01 * b + r11 * d;
    }

    /*
        Extracts the rotation angle from the matrix. Angle will be in radians by default
        Uses polar decomposition
    */
    void decomp_rotation(T* angle, const bool radians = true) const noexcept
    {
        STDROMANO_ASSERT(angle != nullptr, "angle is nullptr");

        const T a = this->_data[0];
        const T b = this->_data[1];
        const T c = this->_data[3];
        const T d = this->_data[4];

        const T eps = maths::constants<T>::large_epsilon;

        const T x = a + d;
        const T y = c - b;

        T scale = maths::sqrt(maths::sqr(x) + maths::sqr(y));

        T r00, r01;

        if(scale > eps)
        {
            r00 = x / scale;
            r01 = -y / scale;
        }
        else
        {
            r00 = make_one_v<T>;
            r01 = make_zero_v<T>;
        }

        *angle = maths::atan2(r01, r00);

        const T dot = a * r00 + b * r01;

        if(dot < 0)
            *angle = -*angle;

        if(!radians)
            *angle = maths::rad2deg(*angle);
    }

    /*
        Extracts the axes and translation from the matrix, normalizing by scale. If one
        component is not wanted, pass nullptr
    */
    void decomp_xyt(Vector2<T>* x, Vector2<T>* y, Vector2<T>* t) const noexcept
    {
        Vector2<T> scale;
        this->decomp_scale(&scale);

        if(x != nullptr)
        {
            x->x = this->_data[0];
            x->y = this->_data[1];

            if(scale.x > maths::constants<T>::large_epsilon)
            {
                x->x /= scale.x;
                x->y /= scale.x;
            }
        }

        if(y != nullptr)
        {
            y->x = this->_data[3];
            y->y = this->_data[4];

            if(scale.y > maths::constants<T>::large_epsilon)
            {
                y->x /= scale.y;
                y->y /= scale.y;
            }
        }

        if(t != nullptr)
            this->decomp_translation(t);
    }

    /*
        Extracts translation, rotation and scale. Angle will be in radians by default.
        If one component is not wanted, pass nullptr
    */
    void decomp_trs(Vector2<T>* t,
                    T* r,
                    Vector2<T>* s,
                    const bool radians = true) const noexcept
    {
        if(t != nullptr)
            this->decomp_translation(t);

        if(r != nullptr)
            this->decomp_rotation(r, radians);

        if(s != nullptr)
            this->decomp_scale(s);
    }

    constexpr bool equal_with_abs_error(const Transform33<T>& other,
                                        const T err = maths::constants<T>::large_epsilon) const noexcept
    {
        for(std::size_t i = 0; i < 9; ++i)
            if(!maths::equal_with_abs_error(this->_data[i], other._data[i], err))
                return false;

        return true;
    }
};

using Transform33F = Transform33<float>;
using Transform33D = Transform33<double>;

/********************************/
/* Transform44 */
/********************************/

enum TransformOrder_ : std::uint32_t
{
    TransformOrder_SRT,
    TransformOrder_STR,
    TransformOrder_RST,
    TransformOrder_RTS,
    TransformOrder_TSR,
    TransformOrder_TRS,
};

enum RotationOrder_ : std::uint32_t
{
    RotationOrder_XYZ,
    RotationOrder_XZY,
    RotationOrder_YXZ,
    RotationOrder_YZX,
    RotationOrder_ZXY,
    RotationOrder_ZYX,
};

template<typename T>
class Transform44
{
public:
    template<typename S>
    friend class Transform44;

private:
    /* Column-major storage, transforms column vectors: p' = M * p */
    T _data[16];

    static constexpr std::size_t index(const std::size_t row, const std::size_t col) noexcept
    {
        return col * 4 + row;
    }

public:
    constexpr explicit Transform44() noexcept {}

    /* Arguments are given row by row, in the order the matrix is written on paper */
    constexpr Transform44(T m00, T m01, T m02, T m03,
                          T m10, T m11, T m12, T m13,
                          T m20, T m21, T m22, T m23,
                          T m30, T m31, T m32, T m33) noexcept
    {
        this->_data[0] = m00;
        this->_data[1] = m10;
        this->_data[2] = m20;
        this->_data[3] = m30;
        this->_data[4] = m01;
        this->_data[5] = m11;
        this->_data[6] = m21;
        this->_data[7] = m31;
        this->_data[8] = m02;
        this->_data[9] = m12;
        this->_data[10] = m22;
        this->_data[11] = m32;
        this->_data[12] = m03;
        this->_data[13] = m13;
        this->_data[14] = m23;
        this->_data[15] = m33;
    }

    template<typename S>
    constexpr Transform44(const Transform44<S>& m) noexcept
    {
        for(std::size_t i = 0; i < 16; ++i)
            this->_data[i] = T(m._data[i]);
    }

    constexpr static Transform44<T> zero() noexcept
    {
        Transform44<T> tr;

        for(std::size_t i = 0; i < 16; i++)
            tr._data[i] = make_zero_v<T>;

        return tr;
    }

    constexpr static Transform44<T> ident() noexcept
    {
        Transform44<T> tr = Transform44<T>::zero();

        for(std::size_t i = 0; i < 4; i++)
            tr._data[index(i, i)] = make_one_v<T>;

        return tr;
    }

    constexpr static Transform44<T> from_translation(const Vector3<T>& t) noexcept
    {
        Transform44<T> tr = Transform44<T>::ident();

        tr(0, 3) = t.x;
        tr(1, 3) = t.y;
        tr(2, 3) = t.z;

        return tr;
    }

    constexpr static Transform44<T> from_scale(const Vector3<T>& s) noexcept
    {
        Transform44<T> tr = Transform44<T>::ident();

        tr(0, 0) = s.x;
        tr(1, 1) = s.y;
        tr(2, 2) = s.z;

        return tr;
    }

    constexpr static Transform44<T> from_rotx(const T rx, const bool radians = true) noexcept
    {
        Transform44<T> tr = Transform44<T>::ident();

        const T theta = radians ? rx : maths::deg2rad(rx);

        T c, s;
        maths::sincos(theta, &s, &c);

        tr(1, 1) = c;
        tr(1, 2) = -s;
        tr(2, 1) = s;
        tr(2, 2) = c;

        return tr;
    }

    constexpr static Transform44<T> from_roty(const T ry, const bool radians = true) noexcept
    {
        Transform44<T> tr = Transform44<T>::ident();

        const T theta = radians ? ry : maths::deg2rad(ry);

        T c, s;
        maths::sincos(theta, &s, &c);

        tr(0, 0) = c;
        tr(0, 2) = s;
        tr(2, 0) = -s;
        tr(2, 2) = c;

        return tr;
    }

    constexpr static Transform44<T> from_rotz(const T rz, const bool radians = true) noexcept
    {
        Transform44<T> tr = Transform44<T>::ident();

        const T theta = radians ? rz : maths::deg2rad(rz);

        T c, s;
        maths::sincos(theta, &s, &c);

        tr(0, 0) = c;
        tr(0, 1) = -s;
        tr(1, 0) = s;
        tr(1, 1) = c;

        return tr;
    }

    /* The axis is expected to be normalized */
    constexpr static Transform44<T> from_axis_angle(const Vector3<T>& axis,
                                                    const T angle,
                                                    const bool radians = true) noexcept
    {
        Transform44<T> tr = Transform44<T>::ident();

        const T theta = radians ? angle : maths::deg2rad(angle);

        T c, s;
        maths::sincos(theta, &s, &c);

        const T t = make_one_v<T> - c;

        tr(0, 0) = axis.x * axis.x * t + c;
        tr(0, 1) = axis.x * axis.y * t - axis.z * s;
        tr(0, 2) = axis.x * axis.z * t + axis.y * s;

        tr(1, 0) = axis.x * axis.y * t + axis.z * s;
        tr(1, 1) = axis.y * axis.y * t + c;
        tr(1, 2) = axis.y * axis.z * t - axis.x * s;

        tr(2, 0) = axis.x * axis.z * t - axis.y * s;
        tr(2, 1) = axis.y * axis.z * t + axis.x * s;
        tr(2, 2) = axis.z * axis.z * t + c;

        return tr;
    }

    /* The rotation order is the order in which the rotations are applied, XYZ is Rz * Ry * Rx */
    constexpr static Transform44<T> from_rotation(const Vector3<T>& rotation,
                                                  const std::uint32_t rotation_order = RotationOrder_XYZ,
                                                  const bool radians = true) noexcept
    {
        const Transform44<T> Rx = Transform44<T>::from_rotx(rotation.x, radians);
        const Transform44<T> Ry = Transform44<T>::from_roty(rotation.y, radians);
        const Transform44<T> Rz = Transform44<T>::from_rotz(rotation.z, radians);

        switch(rotation_order)
        {
            case RotationOrder_XYZ:
                return Rz * Ry * Rx;
            case RotationOrder_XZY:
                return Ry * Rz * Rx;
            case RotationOrder_YXZ:
                return Rz * Rx * Ry;
            case RotationOrder_YZX:
                return Rx * Rz * Ry;
            case RotationOrder_ZXY:
                return Ry * Rx * Rz;
            case RotationOrder_ZYX:
                return Rx * Ry * Rz;
            default:
                STDROMANO_ASSERT(0, "Invalid rotation order");
                return Rz * Ry * Rx;
        }
    }

    /* The transform order is the product order, TRS is T * R * S (scale is applied first) */
    constexpr static Transform44<T> from_trs(const Vector3<T>& translation,
                                             const Vector3<T>& rotation,
                                             const Vector3<T>& scale,
                                             const std::uint32_t transform_order = TransformOrder_TRS,
                                             const std::uint32_t rotation_order = RotationOrder_XYZ,
                                             const bool radians = true) noexcept
    {
        const Transform44<T> Tr = Transform44<T>::from_translation(translation);
        const Transform44<T> S = Transform44<T>::from_scale(scale);
        const Transform44<T> R = Transform44<T>::from_rotation(rotation, rotation_order, radians);

        switch(transform_order)
        {
            case TransformOrder_SRT:
                return S * R * Tr;
            case TransformOrder_STR:
                return S * Tr * R;
            case TransformOrder_RST:
                return R * S * Tr;
            case TransformOrder_RTS:
                return R * Tr * S;
            case TransformOrder_TSR:
                return Tr * S * R;
            case TransformOrder_TRS:
                return Tr * R * S;
            default:
                STDROMANO_ASSERT(0, "Invalid transform order");
                return Tr * R * S;
        }
    }

    /* The axis are the first three columns and the translation the last one */
    constexpr static Transform44<T> from_xyzt(const Vector3<T>& x,
                                              const Vector3<T>& y,
                                              const Vector3<T>& z,
                                              const Vector3<T>& t) noexcept
    {
        return Transform44<T>(x.x, y.x, z.x, t.x,
                              x.y, y.y, z.y, t.y,
                              x.z, y.z, z.z, t.z,
                              make_zero_v<T>, make_zero_v<T>, make_zero_v<T>, make_one_v<T>);
    }

    /* Places an object at eye with its +z axis pointing to target, invert it to get a view transform */
    constexpr static Transform44<T> from_lookat(const Vector3<T>& eye,
                                                const Vector3<T>& target,
                                                const Vector3<T>& up = Vector3<T>(make_zero_v<T>,
                                                                                  make_one_v<T>,
                                                                                  make_zero_v<T>)) noexcept
    {
        const Vector3<T> z = normalize(target - eye);
        const Vector3<T> x = normalize(cross(up, z));
        const Vector3<T> y = cross(z, x);

        return Transform44<T>::from_xyzt(x, y, z, eye);
    }

    constexpr T& operator()(const std::size_t row, const std::size_t col) noexcept
    {
        STDROMANO_ASSERT(row < 4 && col < 4, "Out of bounds access");

        return this->_data[index(row, col)];
    }

    constexpr const T& operator()(const std::size_t row, const std::size_t col) const noexcept
    {
        STDROMANO_ASSERT(row < 4 && col < 4, "Out of bounds access");

        return this->_data[index(row, col)];
    }

    constexpr T& at(const std::size_t row, const std::size_t col) noexcept
    {
        return (*this)(row, col);
    }

    constexpr const T& at(const std::size_t row, const std::size_t col) const noexcept
    {
        return (*this)(row, col);
    }

    constexpr T* data() noexcept
    {
        return this->_data;
    }

    constexpr const T* data() const noexcept
    {
        return this->_data;
    }

    constexpr Transform44<T> operator*(const Transform44<T>& other) const noexcept
    {
        Transform44<T> res = Transform44<T>::zero();

        for(std::size_t col = 0; col < 4; ++col)
            for(std::size_t k = 0; k < 4; ++k)
                for(std::size_t row = 0; row < 4; ++row)
                    res._data[index(row, col)] = maths::fma(this->_data[index(row, k)],
                                                            other._data[index(k, col)],
                                                            res._data[index(row, col)]);

        return res;
    }

    constexpr Transform44<T> transpose() const noexcept
    {
        Transform44<T> res;

        for(std::size_t col = 0; col < 4; ++col)
            for(std::size_t row = 0; row < 4; ++row)
                res._data[index(col, row)] = this->_data[index(row, col)];

        return res;
    }

    constexpr T trace() const noexcept
    {
        return this->_data[0] + this->_data[5] + this->_data[10] + this->_data[15];
    }

    constexpr Vector3<T> transform_point(const Vector3<T>& point) const noexcept
    {
        const Transform44<T>& m = *this;

        return Vector3<T>(m(0, 0) * point.x + m(0, 1) * point.y + m(0, 2) * point.z + m(0, 3),
                          m(1, 0) * point.x + m(1, 1) * point.y + m(1, 2) * point.z + m(1, 3),
                          m(2, 0) * point.x + m(2, 1) * point.y + m(2, 2) * point.z + m(2, 3));
    }

    constexpr Vector3<T> transform_dir(const Vector3<T>& dir) const noexcept
    {
        const Transform44<T>& m = *this;

        return Vector3<T>(m(0, 0) * dir.x + m(0, 1) * dir.y + m(0, 2) * dir.z,
                          m(1, 0) * dir.x + m(1, 1) * dir.y + m(1, 2) * dir.z,
                          m(2, 0) * dir.x + m(2, 1) * dir.y + m(2, 2) * dir.z);
    }

    constexpr void decomp_translation(Vector3<T>* t) const noexcept
    {
        STDROMANO_ASSERT(t != nullptr, "t is nullptr");

        t->x = (*this)(0, 3);
        t->y = (*this)(1, 3);
        t->z = (*this)(2, 3);
    }

    /* Valid for T * R * S transforms, the scale is the length of each axis column */
    void decomp_scale(Vector3<T>* s) const noexcept
    {
        STDROMANO_ASSERT(s != nullptr, "s is nullptr");

        const Transform44<T>& m = *this;

        s->x = maths::sqrt(maths::sqr(m(0, 0)) + maths::sqr(m(1, 0)) + maths::sqr(m(2, 0)));
        s->y = maths::sqrt(maths::sqr(m(0, 1)) + maths::sqr(m(1, 1)) + maths::sqr(m(2, 1)));
        s->z = maths::sqrt(maths::sqr(m(0, 2)) + maths::sqr(m(1, 2)) + maths::sqr(m(2, 2)));
    }

    void decomp_xyzt(Vector3<T>* x, Vector3<T>* y, Vector3<T>* z, Vector3<T>* t) const noexcept
    {
        Vector3<T> scale;
        this->decomp_scale(&scale);

        const T scales[3] = {scale.x, scale.y, scale.z};
        Vector3<T>* axes[3] = {x, y, z};

        for(std::size_t col = 0; col < 3; ++col)
        {
            if(axes[col] == nullptr)
                continue;

            const T inv_scale = scales[col] > maths::constants<T>::large_epsilon ? maths::rcp(scales[col])
                                                                                   : make_one_v<T>;

            axes[col]->x = (*this)(0, col) * inv_scale;
            axes[col]->y = (*this)(1, col) * inv_scale;
            axes[col]->z = (*this)(2, col) * inv_scale;
        }

        if(t != nullptr)
            this->decomp_translation(t);
    }

    /* Returns (x, y, z, w) of the rotation, the matrix is expected to have no scale */
    Vector4<T> to_quaternion() const noexcept
    {
        const Transform44<T>& m = *this;

        const T trace = m(0, 0) + m(1, 1) + m(2, 2);

        Vector4<T> q;

        if(trace > make_zero_v<T>)
        {
            const T s = maths::sqrt(trace + make_one_v<T>) * T(2);
            q.w = T(0.25) * s;
            q.x = (m(2, 1) - m(1, 2)) / s;
            q.y = (m(0, 2) - m(2, 0)) / s;
            q.z = (m(1, 0) - m(0, 1)) / s;
        }
        else if(m(0, 0) > m(1, 1) && m(0, 0) > m(2, 2))
        {
            const T s = maths::sqrt(make_one_v<T> + m(0, 0) - m(1, 1) - m(2, 2)) * T(2);
            q.w = (m(2, 1) - m(1, 2)) / s;
            q.x = T(0.25) * s;
            q.y = (m(0, 1) + m(1, 0)) / s;
            q.z = (m(0, 2) + m(2, 0)) / s;
        }
        else if(m(1, 1) > m(2, 2))
        {
            const T s = maths::sqrt(make_one_v<T> + m(1, 1) - m(0, 0) - m(2, 2)) * T(2);
            q.w = (m(0, 2) - m(2, 0)) / s;
            q.x = (m(0, 1) + m(1, 0)) / s;
            q.y = T(0.25) * s;
            q.z = (m(1, 2) + m(2, 1)) / s;
        }
        else
        {
            const T s = maths::sqrt(make_one_v<T> + m(2, 2) - m(0, 0) - m(1, 1)) * T(2);
            q.w = (m(1, 0) - m(0, 1)) / s;
            q.x = (m(0, 2) + m(2, 0)) / s;
            q.y = (m(1, 2) + m(2, 1)) / s;
            q.z = T(0.25) * s;
        }

        return q;
    }

    /* Inverse of from_rotation for the same rotation order, angles are in radians by default */
    void decomp_tait_bryan(Vector3<T>* angles,
                           const bool radians = true,
                           std::uint32_t rotation_order = RotationOrder_XYZ) const noexcept
    {
        STDROMANO_ASSERT(angles != nullptr, "angles is nullptr");

        Vector3<T> s;
        this->decomp_scale(&s);

        const T scales[3] = {s.x, s.y, s.z};

        T r[3][3];

        for(std::size_t col = 0; col < 3; ++col)
        {
            const T inv_scale = scales[col] > maths::constants<T>::large_epsilon ? maths::rcp(scales[col])
                                                                                   : make_zero_v<T>;

            for(std::size_t row = 0; row < 3; ++row)
                r[row][col] = (*this)(row, col) * inv_scale;
        }

        std::size_t i, j, k;

        switch(rotation_order)
        {
            case RotationOrder_XZY: i = 0; j = 2; k = 1; break;
            case RotationOrder_YXZ: i = 1; j = 0; k = 2; break;
            case RotationOrder_YZX: i = 1; j = 2; k = 0; break;
            case RotationOrder_ZXY: i = 2; j = 0; k = 1; break;
            case RotationOrder_ZYX: i = 2; j = 1; k = 0; break;
            default:
                STDROMANO_ASSERT(rotation_order == RotationOrder_XYZ, "Invalid rotation order");
                i = 0; j = 1; k = 2;
                break;
        }

        const T parity = ((j + 3 - i) % 3) == 1 ? make_one_v<T> : -make_one_v<T>;

        const T sin_middle = maths::clamp(-parity * r[k][i], -make_one_v<T>, make_one_v<T>);

        T result[3];
        result[i] = maths::atan2(parity * r[k][j], r[k][k]);
        result[j] = maths::asin(sin_middle);
        result[k] = maths::atan2(parity * r[j][i], r[i][i]);

        angles->x = radians ? result[0] : maths::rad2deg(result[0]);
        angles->y = radians ? result[1] : maths::rad2deg(result[1]);
        angles->z = radians ? result[2] : maths::rad2deg(result[2]);
    }

    constexpr void decomp_trs(Vector3<T>* t,
                              Vector3<T>* r,
                              Vector3<T>* s,
                              const bool radians = true,
                              const std::uint32_t rotation_order = RotationOrder_XYZ) const noexcept
    {
        if(t != nullptr)
            this->decomp_translation(t);

        if(r != nullptr)
            this->decomp_tait_bryan(r, radians, rotation_order);

        if(s != nullptr)
            this->decomp_scale(s);
    }

    constexpr bool equal_with_abs_error(const Transform44<T>& other,
                                        const T err = maths::constants<T>::large_epsilon) const noexcept
    {
        for(std::size_t i = 0; i < 16; ++i)
            if(!maths::equal_with_abs_error(this->_data[i], other._data[i], err))
                return false;

        return true;
    }
};

using Transform44F = Transform44<float>;
using Transform44D = Transform44<double>;

STDROMANO_NAMESPACE_END

#endif /* !defined(__STDROMANO_LINALG_TRANSFORM) */
