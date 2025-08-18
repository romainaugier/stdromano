// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - Present Romain Augier
// All rights reserved.

#pragma once

#if !defined(__STDROMANO_LINALG_TRANSFORM)
#define __STDROMANO_LINALG_TRANSFORM

#include "stdromano/linalg/vector.hpp"
#include "stdromano/simd.hpp"

STDROMANO_NAMESPACE_BEGIN

/* 
    Data in those matrices is stored in row-major layout
*/

/********************************/
/* Transform33 */
/********************************/

/********************************/
/* Transform44 */
/********************************/

enum Transform44FTransformOrder_ : uint32_t
{
    Transform44TransformOrder_SRT,
    Transform44TransformOrder_STR,
    Transform44TransformOrder_RST,
    Transform44TransformOrder_RTS,
    Transform44TransformOrder_TSR,
    Transform44TransformOrder_TRS,
};

enum Transform44FRotationOrder_ : uint32_t
{
    Transform44RotationOrder_XYZ,
    Transform44RotationOrder_XZY,
    Transform44RotationOrder_YXZ,
    Transform44RotationOrder_YZX,
    Transform44RotationOrder_ZXY,
    Transform44RotationOrder_ZYX,
};

template<typename T>
class Transform44
{
private:
    T _data[16]; 

public:
    Transform44() { *this = Transform44::ident(); }

    Transform44(T m00, T m01, T m02, T m03,
                T m10, T m11, T m12, T m13,
                T m20, T m21, T m22, T m23,
                T m30, T m31, T m32, T m33)
    {
        this->_data[0] = m00;
        this->_data[1] = m01;
        this->_data[2] = m02;
        this->_data[3] = m03;
        this->_data[4] = m10;
        this->_data[5] = m11;
        this->_data[6] = m12;
        this->_data[7] = m13;
        this->_data[8] = m20;
        this->_data[9] = m21;
        this->_data[10] = m22;
        this->_data[11] = m23;
        this->_data[12] = m30;
        this->_data[13] = m31;
        this->_data[14] = m32;
        this->_data[15] = m33;
    }

    static Transform44<T> zero() noexcept
    {
        Transform44<T> tr;

        for(std::size_t i = 0; i < 16; i++)
        {
            tr._data[i] = make_zero_v<T>;
        }

        return tr;
    }

    static Transform44<T> ident() noexcept 
    {
        return Transform44<T>(make_one_v<T>, make_zero_v<T>, make_zero_v<T>, make_zero_v<T>,
                              make_zero_v<T>, make_one_v<T>, make_zero_v<T>, make_zero_v<T>,
                              make_zero_v<T>, make_zero_v<T>, make_one_v<T>, make_zero_v<T>,
                              make_zero_v<T>, make_zero_v<T>, make_zero_v<T>, make_one_v<T>);
    }

    static Transform44<T> from_translation(const Vector3<T>& t) noexcept
    {
        Transform44<T> tr = Transform44<T>::ident();

        tr._data[3] = t.x;
        tr._data[7] = t.y;
        tr._data[11] = t.z;

        return tr;
    }

    static Transform44<T> from_scale(const Vector3<T>& t) noexcept
    {
        Transform44<T> tr = Transform44<T>::ident();

        tr._data[0] = t.x;
        tr._data[5] = t.x;
        tr._data[10] = t.x;

        return tr;
    }
    
    /*
        Rx(θ) = [ 1  0     0
                  0 cosθ -sinθ
                  0 sinθ  cosθ  ]
    */
    static Transform44<T> from_rotx(const T rx, const bool radians = true) noexcept
    {
        Transform44<T> tr = Transform44<T>::ident();

        T theta = radians ? rx : maths::deg2rad(rx);

        T c, s;
        maths::sincos(theta, *s, *c);

        tr._data[6] = c;
        tr._data[7] = -s;
        tr._data[9] = s;
        tr._data[10] = c;

        return tr;
    }
    
    /*
        Ry(θ) = [ cosθ  0 sinθ 
                   0    1  0 
                  -sinθ 0 cosθ ]
    */
    static Transform44<T> from_roty(const T ry, const bool radians = true) noexcept
    {
        Transform44<T> tr = Transform44<T>::ident();

        T theta = radians ? ry : maths::deg2rad(ry);

        T c, s;
        maths::sincos(theta, *s, *c);

        tr._data[0] = c;
        tr._data[2] = s;
        tr._data[8] = -s;
        tr._data[10] = c;

        return tr;
    }
    
    /*
        Rz(θ) = [ cosθ -sinθ 0
                  sinθ  cosθ 0
                   0      0  1 ]
    */
    static Transform44<T> from_rotz(const T rz, const bool radians = true) noexcept
    {
        Transform44<T> tr = Transform44<T>::ident();

        T theta = radians ? rz : maths::deg2rad(rz);

        T c, s;
        maths::sincos(theta, *s, *c);

        tr._data[0] = c;
        tr._data[1] = -s;
        tr._data[4] = s;
        tr._data[5] = c;

        return tr;
    }

    /*
        Builds a transform from an axis and an angle given in radians or degrees
        R = [ ux²(1 - cosθ) + cosθ       ux uy(1 - cosθ) - uz sinθ  ux uz(1 - cosθ) + uy sinθ
              ux uy(1 - cosθ) + uz sinθ  uy²(1 - cosθ) + cosθ       uy uz(1 - cosθ) - ux sinθ
              ux uz(1 - cosθ) - uy sinθ  uy uz(1 - cosθ) + ux sinθ  uz²(1 - cosθ) + cosθ      ]
    */
    static Transform44<T> from_axis_angle(const Vector3<T>& axis, const T angle, const bool radians = true) noexcept
    {
        Transform44<T> tr = Transform44<T>::ident();

        T theta = radians ? angle : maths::deg2rad(angle); 

        T c, s;
        maths::sincos(theta, *s, *c);

        T one_minus_c = make_one_v<T>;

        tr._data[0] = axis.x * axis.x * one_minus_c + c;
        tr._data[1] = axis.x * axis.y * one_minus_c - axis.z * s;
        tr._data[2] = axis.x * axis.z * one_minus_c + axis.y * s;

        tr._data[4] = axis.x * axis.y * one_minus_c + axis.z * s;
        tr._data[5] = axis.y * axis.y * one_minus_c + c;
        tr._data[6] = axis.y * axis.z * one_minus_c - axis.x * s;

        tr._data[8] = axis.x * axis.z * one_minus_c - axis.y * s;
        tr._data[9] = axis.y * axis.z * one_minus_c + axis.x * s;
        tr._data[10] = axis.z * axis.z * one_minus_c + c;

        return tr;
    }

    /*
        Builds a transform from translation, rotation and scale given the transform order and 
        rotation order. Rotation is in radians by default 
    */
    static Transform44<T> from_trs(const Vector3<T>& translation,
                                   const Vector3<T>& rotation,
                                   const Vector3<T>& scale,
                                   const std::uint32_t transform_order = Transform44TransformOrder_TRS,
                                   const std::uint32_t rotation_order = Transform44RotationOrder_XYZ,
                                   const bool radians = true) noexcept
    {
        const Transform44<T> Tr = Transform44<T>::from_translation(translation);
        const Transform44<T> S = Transform44<T>::from_scale(scale);

        const Transform44<T> Rx = Transform44<T>::from_rotx(rotation.x);
        const Transform44<T> Ry = Transform44<T>::from_roty(rotation.y);
        const Transform44<T> Rz = Transform44<T>::from_rotz(rotation.z);

        Transform44<T> R;

        switch(rotation_order)
        {
            case Transform44RotationOrder_XYZ:
                R = Rz * Ry * Rx;
                break;
            case Transform44RotationOrder_XZY:
                R = Ry * Rz * Rx;
                break;
            case Transform44RotationOrder_YXZ:
                R = Rz * Rx * Ry;
                break;
            case Transform44RotationOrder_YZX:
                R = Rx * Rz * Ry;
                break;
            case Transform44RotationOrder_ZXY:
                R = Ry * Rx * Rz;
                break;
            case Transform44RotationOrder_ZYX:
                R = Rx * Ry * Rz;
                break;
            default:
                R = Rz * Ry * Rx;
        }

        switch(transform_order)
        {
            case Transform44TransformOrder_SRT:
                return S * R * Tr;
            case Transform44TransformOrder_STR:
                return S * Tr * R;
            case Transform44TransformOrder_RST:
                return R * S * Tr;
            case Transform44TransformOrder_RTS:
                return R * Tr * S;
            case Transform44TransformOrder_TSR:
                return Tr * S * R;
            case Transform44TransformOrder_TRS:
                return Tr * R * S;
            default:
                return Tr * R * S;
        }
    }

    /*
        Builds a transform from x, y and z axis and translation 
    */
    static Transform44<T> from_xyzt(const Vector3<T>& x,
                                    const Vector3<T>& y,
                                    const Vector3<T>& z,
                                    const Vector3<T>& t) noexcept
    {
        return Transform44<T>(x.x, x.y, x.z, t.x,
                              y.x, y.y, y.z, t.y,
                              z.x, z.y, z.z, t.z,
                              make_zero_v<T>, make_zero_v<T>, make_zero_v<T>, make_one_v<T>);
    }

    /* 
        Builds a transform from a point of view and a target 
    */
    static Transform44<T> from_lookat(const Vector3<T>& eye,
                                      const Vector3<T>& target,
                                      const Vector3<T>& up = Vector3<T>(make_zero_v<T>, make_one_v<T>, make_zero_v<T>)) noexcept
    {
        const Vector3<T> z = normalize(eye - target);
        const Vector3<T> x = normalize(cross(up, z));
        const Vector3<T> y = cross(z, x);

        return Transform44<T>(x.x, x.y, x.z, -dot(x, eye),
                              y.x, y.y, y.z, -dot(y, eye),
                              z.x, z.y, z.z, -dot(z, eye),
                              make_zero_v<T>, make_zero_v<T>, make_zero_v<T>, make_one_v<T>);
    }

    T& operator()(const std::size_t row, const std::size_t col) noexcept
    {
        STDROMANO_ASSERT(row < 4 && col < 4, "Out of bounds access");

        return this->_data[row * 4 + col];
    }

    const T& operator()(const std::size_t row, const std::size_t col) const noexcept
    {
        STDROMANO_ASSERT(row < 4 && col < 4, "Out of bounds access");

        return this->_data[row * 4 + col];
    }

    T& at(const std::size_t row, const std::size_t col) noexcept
    {
        STDROMANO_ASSERT(row < 4 && col < 4, "Out of bounds access");

        return this->_data[row * 4 + col];
    }

    const T& at(const std::size_t row, const std::size_t col) const noexcept
    {
        STDROMANO_ASSERT(row < 4 && col < 4, "Out of bounds access");

        return this->_data[row * 4 + col];
    }

    T* data() noexcept
    {
        return this->_data;
    }

    const T* data() const noexcept
    {
        return this->_data;
    }

    Transform44<T> operator*(const Transform44<T>& other) const noexcept
    {
        Transform44<T> res = Transform44<T>::zero();

        /* TODO: simd with sse instructions */
        switch(simd_get_vectorization_mode())
        {
            default:
            {
                for(std::size_t i = 0; i < 4; ++i)
                {
                    for(std::size_t j = 0; j < 4; ++j)
                    {
                        for(std::size_t k = 0; k < 4; ++k)
                        {
                            res._data[i * 4 + j] = maths::fma(this->_data[i * 4 + k],
                                                            other._data[k * 4 + j],
                                                            res._data[i * 4 + j]);
                        }
                    }
                }
            }
        }

        return res;
    }

    Transform44<T> transpose() const noexcept
    {
        return Transform44<T>(this->_data[0], this->_data[4], this->_data[8], this->_data[12],
                              this->_data[1], this->_data[5], this->_data[9], this->_data[13],
                              this->_data[2], this->_data[6], this->_data[10], this->_data[14],
                              this->_data[3], this->_data[7], this->_data[11], this->_data[15]);
    }

    T trace() const noexcept
    {
        return this->_data[0] + this->_data[5] + this->_data[10] + this->_dazta[15];
    }

    Vector3<T> transform_point(const Vector3<T>& point) const noexcept
    {
        Vector3<T> res;

        res.x = point.x * this->_data[0] + point.y * this->_data[1] + point.z * this->_data[2] + this->_data[3];
        res.y = point.x * this->_data[4] + point.y * this->_data[5] + point.z * this->_data[6] + this->_data[7];
        res.z = point.x * this->_data[8] + point.y * this->_data[9] + point.z * this->_data[10] + this->_data[11];

        return res;
    }

    Vector3<T> transform_dir(const Vector3<T>& dir) const noexcept
    {
        Vector3<T> res;

        res.x = dir.x * this->_data[0] + dir.y * this->_data[1] + dir.z * this->_data[2];
        res.y = dir.x * this->_data[4] + dir.y * this->_data[5] + dir.z * this->_data[6];
        res.z = dir.x * this->_data[8] + dir.y * this->_data[9] + dir.z * this->_data[10];

        return res;
    }

    /*
        Extracts the translation from the matrix
    */
    void decomp_translation(Vector3<T>* t) const noexcept;

    /*
        Extracts the scale from the matrix
    */
    void decomp_scale(Vector3<T>* s) const noexcept;

    /*
        Extracts the axis and translation from the matrix. If one component is not wanted, pass
        nullptr
    */
    void decomp_xyzt(Vector3<T>* x, Vector3<T>* y, Vector3<T>* z, Vector3<T>* t) const noexcept;

    /* 
        Angles will be in radians by default
    */
    void decomp_euler(Vector3<T>* angles, const bool radians = true) const noexcept;

    void decomp_trs(Vector3<T>* t, Vector3<T>* r, Vector3<T>* s) const noexcept;
};

using Transform44F = Transform44<float>;
using Transform44D = Transform44<double>;

STDROMANO_NAMESPACE_END

#endif /* !defined(__STDROMANO_LINALG_TRANSFORM) */