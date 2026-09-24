// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - Present Romain Augier
// All rights reserved.

#include "stdromano/linalg/transform.hpp"

#include "fixtures.hpp"

#include <algorithm>
#include <cmath>

using namespace stdromano;

STDROMANO_TEST_CASE(transform33_default_constructor)
{
    Transform33F tr = Transform33F::ident();
    Transform33F ident = Transform33F::ident();
    STDROMANO_CHECK(tr.equal_with_abs_error(ident, 1e-6f));
}

STDROMANO_TEST_CASE(transform33_element_constructor)
{
    Transform33F tr(1.0f, 2.0f, 3.0f,
                    4.0f, 5.0f, 6.0f,
                    7.0f, 8.0f, 9.0f);

    STDROMANO_CHECK_EQ(tr(0, 0), 1.0f);
    STDROMANO_CHECK_EQ(tr(0, 1), 2.0f);
    STDROMANO_CHECK_EQ(tr(0, 2), 3.0f);
    STDROMANO_CHECK_EQ(tr(1, 0), 4.0f);
    STDROMANO_CHECK_EQ(tr(1, 1), 5.0f);
    STDROMANO_CHECK_EQ(tr(1, 2), 6.0f);
    STDROMANO_CHECK_EQ(tr(2, 0), 7.0f);
    STDROMANO_CHECK_EQ(tr(2, 1), 8.0f);
    STDROMANO_CHECK_EQ(tr(2, 2), 9.0f);
}

STDROMANO_TEST_CASE(transform33_copy_constructor)
{
    Transform33F src(1.0f, 2.0f, 3.0f,
                     4.0f, 5.0f, 6.0f,
                     7.0f, 8.0f, 9.0f);
    Transform33F dst(src);
    STDROMANO_CHECK(dst.equal_with_abs_error(src, 1e-6f));
}

STDROMANO_TEST_CASE(transform33_type_conversion_constructor)
{
    Transform33D src(1.0, 2.0, 3.0,
                     4.0, 5.0, 6.0,
                     7.0, 8.0, 9.0);
    Transform33F dst(src);
    STDROMANO_CHECK_EQ(dst(0, 0), 1.0f);
    STDROMANO_CHECK_EQ(dst(1, 1), 5.0f);
    STDROMANO_CHECK_EQ(dst(2, 2), 9.0f);
}

STDROMANO_TEST_CASE(transform33_zero)
{
    Transform33F tr = Transform33F::zero();
    for(std::size_t i = 0; i < 9; ++i)
    {
        STDROMANO_CHECK_EQ(tr.data()[i], 0.0f);
    }
}

STDROMANO_TEST_CASE(transform33_identity)
{
    Transform33F tr = Transform33F::ident();
    STDROMANO_CHECK_EQ(tr(0, 0), 1.0f);
    STDROMANO_CHECK_EQ(tr(1, 1), 1.0f);
    STDROMANO_CHECK_EQ(tr(2, 2), 1.0f);
    STDROMANO_CHECK_EQ(tr(0, 1), 0.0f);
    STDROMANO_CHECK_EQ(tr(0, 2), 0.0f);
    STDROMANO_CHECK_EQ(tr(1, 0), 0.0f);
    STDROMANO_CHECK_EQ(tr(1, 2), 0.0f);
    STDROMANO_CHECK_EQ(tr(2, 0), 0.0f);
    STDROMANO_CHECK_EQ(tr(2, 1), 0.0f);
}

STDROMANO_TEST_CASE(transform33_from_translation)
{
    Vec2F t(3.0f, 7.0f);
    Transform33F tr = Transform33F::from_translation(t);
    Vec2F extracted;
    tr.decomp_translation(&extracted);
    STDROMANO_CHECK(maths::equal_with_abs_error(extracted.x, t.x, 1e-6f));
    STDROMANO_CHECK(maths::equal_with_abs_error(extracted.y, t.y, 1e-6f));
}

STDROMANO_TEST_CASE(transform33_from_translation_homogeneous_row)
{
    Vec2F t(5.0f, -2.0f);
    Transform33F tr = Transform33F::from_translation(t);
    STDROMANO_CHECK_EQ(tr(2, 0), 0.0f);
    STDROMANO_CHECK_EQ(tr(2, 1), 0.0f);
    STDROMANO_CHECK_EQ(tr(2, 2), 1.0f);
}

STDROMANO_TEST_CASE(transform33_from_scale)
{
    Vec2F s(2.0f, 3.0f);
    Transform33F tr = Transform33F::from_scale(s);
    Vec2F extracted;
    tr.decomp_scale(&extracted);
    STDROMANO_CHECK(maths::equal_with_abs_error(extracted.x, s.x, 1e-6f));
    STDROMANO_CHECK(maths::equal_with_abs_error(extracted.y, s.y, 1e-6f));
}

STDROMANO_TEST_CASE(transform33_from_uniform_scale)
{
    Transform33F tr = Transform33F::from_uniform_scale(4.0f);
    Vec2F extracted;
    tr.decomp_scale(&extracted);
    STDROMANO_CHECK(maths::equal_with_abs_error(extracted.x, 4.0f, 1e-6f));
    STDROMANO_CHECK(maths::equal_with_abs_error(extracted.y, 4.0f, 1e-6f));
}

STDROMANO_TEST_CASE(transform33_from_rotation_90)
{
    float angle = maths::constants<float>::pi / 2.0f;
    Transform33F tr = Transform33F::from_rotation(angle);
    Vec2F point(1.0f, 0.0f);
    Vec2F result = tr.transform_point(point);
    STDROMANO_CHECK(maths::equal_with_abs_error(result.x, 0.0f, 1e-6f));
    STDROMANO_CHECK(maths::equal_with_abs_error(result.y, -1.0f, 1e-6f));
}

STDROMANO_TEST_CASE(transform33_from_rotation_180)
{
    float angle = maths::constants<float>::pi;
    Transform33F tr = Transform33F::from_rotation(angle);
    Vec2F point(1.0f, 0.0f);
    Vec2F result = tr.transform_point(point);
    STDROMANO_CHECK(maths::equal_with_abs_error(result.x, -1.0f, 1e-5f));
    STDROMANO_CHECK(maths::equal_with_abs_error(result.y, 0.0f, 1e-5f));
}

STDROMANO_TEST_CASE(transform33_from_rotation_45)
{
    float angle = maths::constants<float>::pi / 4.0f;
    Transform33F tr = Transform33F::from_rotation(angle);
    Vec2F point(1.0f, 0.0f);
    Vec2F result = tr.transform_point(point);
    float expected = maths::sqrt(2.0f) / 2.0f;
    STDROMANO_CHECK(maths::equal_with_abs_error(result.x, expected, 1e-6f));
    STDROMANO_CHECK(maths::equal_with_abs_error(result.y, -expected, 1e-6f));
}

STDROMANO_TEST_CASE(transform33_from_rotation_degrees)
{
    Transform33F tr_rad = Transform33F::from_rotation(maths::constants<float>::pi / 2.0f, true);
    Transform33F tr_deg = Transform33F::from_rotation(90.0f, false);
    STDROMANO_CHECK(tr_rad.equal_with_abs_error(tr_deg, 1e-5f));
}

STDROMANO_TEST_CASE(transform33_from_rotation_negative)
{
    float angle = -maths::constants<float>::pi / 2.0f;
    Transform33F tr = Transform33F::from_rotation(angle);
    Vec2F point(0.0f, 1.0f);
    Vec2F result = tr.transform_point(point);
    STDROMANO_CHECK(maths::equal_with_abs_error(result.x, -1.0f, 1e-6f));
    STDROMANO_CHECK(maths::equal_with_abs_error(result.y, 0.0f, 1e-6f));
}

STDROMANO_TEST_CASE(transform33_from_trs_basic)
{
    Vec2F t(5.0f, 10.0f);
    float r = maths::constants<float>::pi / 4.0f;
    Vec2F s(2.0f, 2.0f);
    Transform33F tr = Transform33F::from_trs(t, r, s, TransformOrder2D_TRS);
    Vec2F t_out;
    float r_out;
    Vec2F s_out;
    tr.decomp_trs(&t_out, &r_out, &s_out);
    STDROMANO_CHECK(maths::equal_with_abs_error(t_out.x, t.x, 1e-5f));
    STDROMANO_CHECK(maths::equal_with_abs_error(t_out.y, t.y, 1e-5f));
    STDROMANO_CHECK(maths::equal_with_abs_error(r_out, r, 1e-5f));
    STDROMANO_CHECK(maths::equal_with_abs_error(s_out.x, s.x, 1e-5f));
    STDROMANO_CHECK(maths::equal_with_abs_error(s_out.y, s.y, 1e-5f));
}

STDROMANO_TEST_CASE(transform33_from_trs_non_uniform_scale)
{
    Vec2F t(1.0f, -2.0f);
    float r = 0.3f;
    Vec2F s(3.0f, 5.0f);
    Transform33F tr = Transform33F::from_trs(t, r, s, TransformOrder2D_TRS);
    Vec2F t_out;
    float r_out;
    Vec2F s_out;
    tr.decomp_trs(&t_out, &r_out, &s_out);
    STDROMANO_CHECK(maths::equal_with_abs_error(t_out.x, t.x, 1e-5f));
    STDROMANO_CHECK(maths::equal_with_abs_error(t_out.y, t.y, 1e-5f));
    STDROMANO_CHECK(maths::equal_with_abs_error(r_out, r, 1e-5f));
    STDROMANO_CHECK(maths::equal_with_abs_error(s_out.x, s.x, 1e-5f));
    STDROMANO_CHECK(maths::equal_with_abs_error(s_out.y, s.y, 1e-5f));
}

STDROMANO_TEST_CASE(transform33_from_trs_srt_order)
{
    Vec2F t(1.0f, 2.0f);
    float r = 0.0f;
    Vec2F s(3.0f, 3.0f);
    Transform33F tr_srt = Transform33F::from_trs(t, r, s, TransformOrder2D_SRT);
    Transform33F expected = Transform33F::from_scale(s) * Transform33F::from_translation(t);
    STDROMANO_CHECK(tr_srt.equal_with_abs_error(expected, 1e-6f));
}

STDROMANO_TEST_CASE(transform33_from_trs_degrees)
{
    Vec2F t(0.0f, 0.0f);
    Vec2F s(1.0f, 1.0f);
    Transform33F tr = Transform33F::from_trs(t, 90.0f, s, TransformOrder2D_TRS, false);
    Vec2F point(1.0f, 0.0f);
    Vec2F result = tr.transform_point(point);
    STDROMANO_CHECK(maths::equal_with_abs_error(result.x, 0.0f, 1e-5f));
    STDROMANO_CHECK(maths::equal_with_abs_error(result.y, -1.0f, 1e-5f));
}

STDROMANO_TEST_CASE(transform33_from_xyt)
{
    Vec2F x(1.0f, 0.0f);
    Vec2F y(0.0f, 1.0f);
    Vec2F t(4.0f, 5.0f);
    Transform33F tr = Transform33F::from_xyt(x, y, t);
    Vec2F x_out, y_out, t_out;
    tr.decomp_xyt(&x_out, &y_out, &t_out);
    STDROMANO_CHECK(maths::equal_with_abs_error(x_out.x, x.x, 1e-6f));
    STDROMANO_CHECK(maths::equal_with_abs_error(x_out.y, x.y, 1e-6f));
    STDROMANO_CHECK(maths::equal_with_abs_error(y_out.x, y.x, 1e-6f));
    STDROMANO_CHECK(maths::equal_with_abs_error(y_out.y, y.y, 1e-6f));
    STDROMANO_CHECK(maths::equal_with_abs_error(t_out.x, t.x, 1e-6f));
    STDROMANO_CHECK(maths::equal_with_abs_error(t_out.y, t.y, 1e-6f));
}

STDROMANO_TEST_CASE(transform33_from_xyt_rotated)
{
    float c = maths::sqrt(2.0f) / 2.0f;
    Vec2F x(c, c);
    Vec2F y(-c, c);
    Vec2F t(0.0f, 0.0f);
    Transform33F tr = Transform33F::from_xyt(x, y, t);
    float angle;
    tr.decomp_rotation(&angle);
    STDROMANO_CHECK(maths::equal_with_abs_error(angle, maths::constants<float>::pi / 4.0f, 1e-5f));
}

STDROMANO_TEST_CASE(transform33_from_lookat)
{
    Vec2F eye(0.0f, 0.0f);
    Vec2F target(1.0f, 0.0f);
    Transform33F tr = Transform33F::from_lookat(eye, target);
    Vec2F x_out, y_out, t_out;
    tr.decomp_xyt(&x_out, &y_out, &t_out);
    STDROMANO_CHECK(maths::equal_with_abs_error(x_out.x, 1.0f, 1e-6f));
    STDROMANO_CHECK(maths::equal_with_abs_error(x_out.y, 0.0f, 1e-6f));
}

STDROMANO_TEST_CASE(transform33_from_lookat_diagonal)
{
    Vec2F eye(0.0f, 0.0f);
    Vec2F target(1.0f, 1.0f);
    Transform33F tr = Transform33F::from_lookat(eye, target);
    Vec2F x_out, y_out, t_out;
    tr.decomp_xyt(&x_out, &y_out, &t_out);
    float expected = maths::sqrt(2.0f) / 2.0f;
    STDROMANO_CHECK(maths::equal_with_abs_error(x_out.x, expected, 1e-5f));
    STDROMANO_CHECK(maths::equal_with_abs_error(x_out.y, expected, 1e-5f));
}

STDROMANO_TEST_CASE(transform33_matrix_multiplication)
{
    Transform33F a = Transform33F::from_rotation(maths::constants<float>::pi / 2.0f);
    Transform33F b = Transform33F::from_rotation(maths::constants<float>::pi / 2.0f);
    Transform33F result = a * b;
    Vec2F point(1.0f, 0.0f);
    Vec2F transformed = result.transform_point(point);
    STDROMANO_CHECK(maths::equal_with_abs_error(transformed.x, -1.0f, 1e-5f));
    STDROMANO_CHECK(maths::equal_with_abs_error(transformed.y, 0.0f, 1e-5f));
}

STDROMANO_TEST_CASE(transform33_identity_multiplication)
{
    Transform33F tr(1.0f, 2.0f, 3.0f,
                    4.0f, 5.0f, 6.0f,
                    0.0f, 0.0f, 1.0f);
    Transform33F ident = Transform33F::ident();
    Transform33F result = tr * ident;
    STDROMANO_CHECK(result.equal_with_abs_error(tr, 1e-6f));
}

STDROMANO_TEST_CASE(transform33_multiplication_associativity)
{
    Transform33F a = Transform33F::from_rotation(0.3f);
    Transform33F b = Transform33F::from_translation(Vec2F(1.0f, 2.0f));
    Transform33F c = Transform33F::from_scale(Vec2F(2.0f, 3.0f));
    Transform33F ab_c = (a * b) * c;
    Transform33F a_bc = a * (b * c);
    STDROMANO_CHECK(ab_c.equal_with_abs_error(a_bc, 1e-5f));
}

STDROMANO_TEST_CASE(transform33_transpose)
{
    Transform33F tr(1.0f, 2.0f, 3.0f,
                    4.0f, 5.0f, 6.0f,
                    7.0f, 8.0f, 9.0f);
    Transform33F transposed = tr.transpose();
    STDROMANO_CHECK_EQ(transposed(0, 0), tr(0, 0));
    STDROMANO_CHECK_EQ(transposed(0, 1), tr(1, 0));
    STDROMANO_CHECK_EQ(transposed(1, 0), tr(0, 1));
    STDROMANO_CHECK_EQ(transposed(2, 1), tr(1, 2));
    STDROMANO_CHECK_EQ(transposed(1, 2), tr(2, 1));
}

STDROMANO_TEST_CASE(transform33_double_transpose)
{
    Transform33F tr(1.0f, 2.0f, 3.0f,
                    4.0f, 5.0f, 6.0f,
                    7.0f, 8.0f, 9.0f);
    Transform33F result = tr.transpose().transpose();
    STDROMANO_CHECK(result.equal_with_abs_error(tr, 1e-6f));
}

STDROMANO_TEST_CASE(transform33_trace)
{
    Transform33F tr = Transform33F::ident();
    STDROMANO_CHECK_EQ(tr.trace(), 3.0f);
}

STDROMANO_TEST_CASE(transform33_determinant_identity)
{
    Transform33F tr = Transform33F::ident();
    STDROMANO_CHECK(maths::equal_with_abs_error(tr.determinant(), 1.0f, 1e-6f));
}

STDROMANO_TEST_CASE(transform33_determinant_scale)
{
    Transform33F tr = Transform33F::from_scale(Vec2F(2.0f, 3.0f));
    STDROMANO_CHECK(maths::equal_with_abs_error(tr.determinant(), 6.0f, 1e-6f));
}

STDROMANO_TEST_CASE(transform33_determinant_rotation)
{
    Transform33F tr = Transform33F::from_rotation(0.7f);
    STDROMANO_CHECK(maths::equal_with_abs_error(tr.determinant(), 1.0f, 1e-5f));
}

STDROMANO_TEST_CASE(transform33_inverse_identity)
{
    Transform33F tr = Transform33F::ident();
    Transform33F inv = tr.inverse().unwrap();
    STDROMANO_CHECK(inv.equal_with_abs_error(Transform33F::ident(), 1e-6f));
}

STDROMANO_TEST_CASE(transform33_inverse_translation)
{
    Vec2F t(3.0f, -7.0f);
    Transform33F tr = Transform33F::from_translation(t);
    Transform33F inv = tr.inverse().unwrap();
    Transform33F product = tr * inv;
    STDROMANO_CHECK(product.equal_with_abs_error(Transform33F::ident(), 1e-5f));
}

STDROMANO_TEST_CASE(transform33_inverse_rotation)
{
    Transform33F tr = Transform33F::from_rotation(1.2f);
    Transform33F inv = tr.inverse().unwrap();
    Transform33F product = tr * inv;
    STDROMANO_CHECK(product.equal_with_abs_error(Transform33F::ident(), 1e-5f));
}

STDROMANO_TEST_CASE(transform33_inverse_scale)
{
    Transform33F tr = Transform33F::from_scale(Vec2F(2.0f, 5.0f));
    Transform33F inv = tr.inverse().unwrap();
    Transform33F product = tr * inv;
    STDROMANO_CHECK(product.equal_with_abs_error(Transform33F::ident(), 1e-5f));
}

STDROMANO_TEST_CASE(transform33_inverse_composite)
{
    Transform33F tr = Transform33F::from_trs(Vec2F(3.0f, -1.0f), 0.8f, Vec2F(2.0f, 3.0f));
    Transform33F inv = tr.inverse().unwrap();
    Transform33F product = tr * inv;
    STDROMANO_CHECK(product.equal_with_abs_error(Transform33F::ident(), 1e-4f));
}

STDROMANO_TEST_CASE(transform33_inverse_undoes_transform)
{
    Transform33F tr = Transform33F::from_trs(Vec2F(5.0f, 3.0f), 1.0f, Vec2F(2.0f, 2.0f));
    Transform33F inv = tr.inverse().unwrap();
    Vec2F point(7.0f, -4.0f);
    Vec2F transformed = tr.transform_point(point);
    Vec2F recovered = inv.transform_point(transformed);
    STDROMANO_CHECK(maths::equal_with_abs_error(recovered.x, point.x, 1e-4f));
    STDROMANO_CHECK(maths::equal_with_abs_error(recovered.y, point.y, 1e-4f));
}

STDROMANO_TEST_CASE(transform33_transform_point_identity)
{
    Transform33F tr = Transform33F::ident();
    Vec2F p(3.0f, 7.0f);
    Vec2F result = tr.transform_point(p);
    STDROMANO_CHECK(maths::equal_with_abs_error(result.x, p.x, 1e-6f));
    STDROMANO_CHECK(maths::equal_with_abs_error(result.y, p.y, 1e-6f));
}

STDROMANO_TEST_CASE(transform33_transform_point_translation)
{
    Transform33F tr = Transform33F::from_translation(Vec2F(10.0f, 20.0f));
    Vec2F point(1.0f, 2.0f);
    Vec2F result = tr.transform_point(point);
    STDROMANO_CHECK(maths::equal_with_abs_error(result.x, 11.0f, 1e-6f));
    STDROMANO_CHECK(maths::equal_with_abs_error(result.y, 22.0f, 1e-6f));
}

STDROMANO_TEST_CASE(transform33_transform_point_scale)
{
    Transform33F tr = Transform33F::from_scale(Vec2F(2.0f, 3.0f));
    Vec2F point(4.0f, 5.0f);
    Vec2F result = tr.transform_point(point);
    STDROMANO_CHECK(maths::equal_with_abs_error(result.x, 8.0f, 1e-6f));
    STDROMANO_CHECK(maths::equal_with_abs_error(result.y, 15.0f, 1e-6f));
}

STDROMANO_TEST_CASE(transform33_transform_point_combined)
{
    Transform33F rot = Transform33F::from_rotation(maths::constants<float>::pi / 2.0f);
    Transform33F trans = Transform33F::from_translation(Vec2F(10.0f, 0.0f));
    Transform33F tr = trans * rot;
    Vec2F point(1.0f, 0.0f);
    Vec2F result = tr.transform_point(point);
    STDROMANO_CHECK(maths::equal_with_abs_error(result.x, 10.0f, 1e-5f));
    STDROMANO_CHECK(maths::equal_with_abs_error(result.y, -1.0f, 1e-5f));
}

STDROMANO_TEST_CASE(transform33_transform_dir_ignores_translation)
{
    Transform33F tr = Transform33F::from_translation(Vec2F(100.0f, 200.0f));
    Vec2F dir(1.0f, 0.0f);
    Vec2F result = tr.transform_dir(dir);
    STDROMANO_CHECK(maths::equal_with_abs_error(result.x, 1.0f, 1e-6f));
    STDROMANO_CHECK(maths::equal_with_abs_error(result.y, 0.0f, 1e-6f));
}

STDROMANO_TEST_CASE(transform33_transform_dir_with_rotation)
{
    Transform33F tr = Transform33F::from_rotation(maths::constants<float>::pi / 2.0f);
    Vec2F dir(1.0f, 0.0f);
    Vec2F result = tr.transform_dir(dir);
    STDROMANO_CHECK(maths::equal_with_abs_error(result.x, 0.0f, 1e-6f));
    STDROMANO_CHECK(maths::equal_with_abs_error(result.y, -1.0f, 1e-6f));
}

STDROMANO_TEST_CASE(transform33_transform_dir_with_scale)
{
    Transform33F tr = Transform33F::from_scale(Vec2F(3.0f, 5.0f));
    Vec2F dir(1.0f, 1.0f);
    Vec2F result = tr.transform_dir(dir);
    STDROMANO_CHECK(maths::equal_with_abs_error(result.x, 3.0f, 1e-6f));
    STDROMANO_CHECK(maths::equal_with_abs_error(result.y, 5.0f, 1e-6f));
}

STDROMANO_TEST_CASE(transform33_decomp_rotation_roundtrip)
{
    float angle = 1.3f;
    Transform33F tr = Transform33F::from_rotation(angle);
    float extracted;
    tr.decomp_rotation(&extracted);
    STDROMANO_CHECK(maths::equal_with_abs_error(extracted, angle, 1e-6f));
}

STDROMANO_TEST_CASE(transform33_decomp_rotation_negative)
{
    float angle = -0.7f;
    Transform33F tr = Transform33F::from_rotation(angle);
    float extracted;
    tr.decomp_rotation(&extracted);
    STDROMANO_CHECK(maths::equal_with_abs_error(extracted, angle, 1e-6f));
}

STDROMANO_TEST_CASE(transform33_decomp_rotation_degrees)
{
    float angle_rad = maths::constants<float>::pi / 3.0f;
    Transform33F tr = Transform33F::from_rotation(angle_rad);
    float angle_deg;
    tr.decomp_rotation(&angle_deg, false);
    STDROMANO_CHECK(maths::equal_with_abs_error(angle_deg, 60.0f, 1e-4f));
}

STDROMANO_TEST_CASE(transform33_decomp_rotation_with_scale)
{
    float angle = 0.5f;
    Vec2F s(3.0f, 7.0f);
    Transform33F tr = Transform33F::from_trs(Vec2F(0.0f, 0.0f), angle, s, TransformOrder2D_TRS);
    float extracted;
    tr.decomp_rotation(&extracted);
    STDROMANO_CHECK(maths::equal_with_abs_error(extracted, angle, 1e-5f));
}

STDROMANO_TEST_CASE(transform33_decomp_xyt_partial)
{
    Vec2F x(1.0f, 0.0f);
    Vec2F y(0.0f, 1.0f);
    Vec2F t(4.0f, 5.0f);
    Transform33F tr = Transform33F::from_xyt(x, y, t);
    Vec2F t_only;
    tr.decomp_xyt(nullptr, nullptr, &t_only);
    STDROMANO_CHECK(maths::equal_with_abs_error(t_only.x, t.x, 1e-6f));
    STDROMANO_CHECK(maths::equal_with_abs_error(t_only.y, t.y, 1e-6f));
}

STDROMANO_TEST_CASE(transform33_decomp_trs_partial)
{
    Transform33F tr = Transform33F::from_trs(Vec2F(1.0f, 2.0f), 0.5f, Vec2F(3.0f, 4.0f));
    float r_out;
    tr.decomp_trs(nullptr, &r_out, nullptr);
    STDROMANO_CHECK(maths::equal_with_abs_error(r_out, 0.5f, 1e-5f));
}

STDROMANO_TEST_CASE(transform33_double_precision)
{
    double angle = maths::constants<double>::pi / 3.0;
    Transform33D tr = Transform33D::from_rotation(angle);
    Vec2D point(1.0, 0.0);
    Vec2D result = tr.transform_point(point);
    STDROMANO_CHECK(maths::equal_with_abs_error(result.x, 0.5, 1e-12));
    STDROMANO_CHECK(maths::equal_with_abs_error(result.y, -maths::sqrt(3.0) / 2.0, 1e-12));
}

STDROMANO_TEST_CASE(transform33_double_precision_trs_roundtrip)
{
    Vec2D t(1.5, -2.3);
    double r = 0.7;
    Vec2D s(2.5, 3.5);
    Transform33D tr = Transform33D::from_trs(t, r, s, TransformOrder2D_TRS);
    Vec2D t_out;
    double r_out;
    Vec2D s_out;
    tr.decomp_trs(&t_out, &r_out, &s_out);
    STDROMANO_CHECK(maths::equal_with_abs_error(t_out.x, t.x, 1e-10));
    STDROMANO_CHECK(maths::equal_with_abs_error(t_out.y, t.y, 1e-10));
    STDROMANO_CHECK(maths::equal_with_abs_error(r_out, r, 1e-10));
    STDROMANO_CHECK(maths::equal_with_abs_error(s_out.x, s.x, 1e-10));
    STDROMANO_CHECK(maths::equal_with_abs_error(s_out.y, s.y, 1e-10));
}

STDROMANO_TEST_CASE(transform44_default_constructor)
{
    Transform44F tr = Transform44F::ident();
    Transform44F ident = Transform44F::ident();
    STDROMANO_CHECK(tr.equal_with_abs_error(ident, 1e-6f));
}

STDROMANO_TEST_CASE(transform44_element_constructor)
{
    Transform44F tr(1.0f, 2.0f, 3.0f, 4.0f,
                    5.0f, 6.0f, 7.0f, 8.0f,
                    9.0f, 10.0f, 11.0f, 12.0f,
                    13.0f, 14.0f, 15.0f, 16.0f);

    STDROMANO_CHECK_EQ(tr(0, 0), 1.0f);
    STDROMANO_CHECK_EQ(tr(0, 1), 2.0f);
    STDROMANO_CHECK_EQ(tr(0, 2), 3.0f);
    STDROMANO_CHECK_EQ(tr(0, 3), 4.0f);
    STDROMANO_CHECK_EQ(tr(1, 0), 5.0f);
    STDROMANO_CHECK_EQ(tr(1, 1), 6.0f);
    STDROMANO_CHECK_EQ(tr(1, 2), 7.0f);
    STDROMANO_CHECK_EQ(tr(1, 3), 8.0f);
    STDROMANO_CHECK_EQ(tr(2, 0), 9.0f);
    STDROMANO_CHECK_EQ(tr(2, 1), 10.0f);
    STDROMANO_CHECK_EQ(tr(2, 2), 11.0f);
    STDROMANO_CHECK_EQ(tr(2, 3), 12.0f);
    STDROMANO_CHECK_EQ(tr(3, 0), 13.0f);
    STDROMANO_CHECK_EQ(tr(3, 1), 14.0f);
    STDROMANO_CHECK_EQ(tr(3, 2), 15.0f);
    STDROMANO_CHECK_EQ(tr(3, 3), 16.0f);
}

STDROMANO_TEST_CASE(transform44_copy_constructor)
{
    Transform44F src(1.0f, 2.0f, 3.0f, 4.0f,
                     5.0f, 6.0f, 7.0f, 8.0f,
                     9.0f, 10.0f, 11.0f, 12.0f,
                     13.0f, 14.0f, 15.0f, 16.0f);
    Transform44F dst(src);
    STDROMANO_CHECK(dst.equal_with_abs_error(src, 1e-6f));
}

STDROMANO_TEST_CASE(transform44_type_conversion_constructor)
{
    Transform44D src(1.0, 2.0, 3.0, 4.0,
                     5.0, 6.0, 7.0, 8.0,
                     9.0, 10.0, 11.0, 12.0,
                     13.0, 14.0, 15.0, 16.0);
    Transform44F dst(src);
    STDROMANO_CHECK_EQ(dst(0, 0), 1.0f);
    STDROMANO_CHECK_EQ(dst(1, 1), 6.0f);
    STDROMANO_CHECK_EQ(dst(2, 2), 11.0f);
    STDROMANO_CHECK_EQ(dst(3, 3), 16.0f);
}

STDROMANO_TEST_CASE(transform44_zero)
{
    Transform44F tr = Transform44F::zero();
    for(std::size_t i = 0; i < 16; ++i)
    {
        STDROMANO_CHECK_EQ(tr.data()[i], 0.0f);
    }
}

STDROMANO_TEST_CASE(transform44_identity)
{
    Transform44F tr = Transform44F::ident();
    STDROMANO_CHECK_EQ(tr(0, 0), 1.0f);
    STDROMANO_CHECK_EQ(tr(1, 1), 1.0f);
    STDROMANO_CHECK_EQ(tr(2, 2), 1.0f);
    STDROMANO_CHECK_EQ(tr(3, 3), 1.0f);
    STDROMANO_CHECK_EQ(tr(0, 1), 0.0f);
    STDROMANO_CHECK_EQ(tr(0, 2), 0.0f);
    STDROMANO_CHECK_EQ(tr(0, 3), 0.0f);
    STDROMANO_CHECK_EQ(tr(1, 0), 0.0f);
    STDROMANO_CHECK_EQ(tr(1, 2), 0.0f);
    STDROMANO_CHECK_EQ(tr(1, 3), 0.0f);
    STDROMANO_CHECK_EQ(tr(2, 0), 0.0f);
    STDROMANO_CHECK_EQ(tr(2, 1), 0.0f);
    STDROMANO_CHECK_EQ(tr(2, 3), 0.0f);
    STDROMANO_CHECK_EQ(tr(3, 0), 0.0f);
    STDROMANO_CHECK_EQ(tr(3, 1), 0.0f);
    STDROMANO_CHECK_EQ(tr(3, 2), 0.0f);
}

STDROMANO_TEST_CASE(transform44_from_translation)
{
    Vec3F t(1.0f, 2.0f, 3.0f);
    Transform44F tr = Transform44F::from_translation(t);
    Vec3F extracted;
    tr.decomp_translation(&extracted);
    STDROMANO_CHECK(maths::equal_with_abs_error(extracted.x, t.x, 1e-6f));
    STDROMANO_CHECK(maths::equal_with_abs_error(extracted.y, t.y, 1e-6f));
    STDROMANO_CHECK(maths::equal_with_abs_error(extracted.z, t.z, 1e-6f));
}

STDROMANO_TEST_CASE(transform44_from_scale)
{
    Vec3F s(2.0f, 3.0f, 4.0f);
    Transform44F tr = Transform44F::from_scale(s);
    Vec3F extracted;
    tr.decomp_scale(&extracted);
    STDROMANO_CHECK(maths::equal_with_abs_error(extracted.x, s.x, 1e-6f));
    STDROMANO_CHECK(maths::equal_with_abs_error(extracted.y, s.y, 1e-6f));
    STDROMANO_CHECK(maths::equal_with_abs_error(extracted.z, s.z, 1e-6f));
}

STDROMANO_TEST_CASE(transform44_from_rot_x)
{
    float angle = maths::constants<float>::pi / 2.0f;
    Transform44F tr = Transform44F::from_rotx(angle);
    Vec3F test_point(0.0f, 1.0f, 0.0f);
    Vec3F transformed = tr.transform_point(test_point);
    STDROMANO_CHECK(maths::equal_with_abs_error(transformed.x, 0.0f, 1e-6f));
    STDROMANO_CHECK(maths::equal_with_abs_error(transformed.y, 0.0f, 1e-6f));
    STDROMANO_CHECK(maths::equal_with_abs_error(transformed.z, 1.0f, 1e-6f));
}

STDROMANO_TEST_CASE(transform44_from_rot_y)
{
    float angle = maths::constants<float>::pi / 2.0f;
    Transform44F tr = Transform44F::from_roty(angle);
    Vec3F test_point(1.0f, 0.0f, 0.0f);
    Vec3F transformed = tr.transform_point(test_point);
    STDROMANO_CHECK(maths::equal_with_abs_error(transformed.x, 0.0f, 1e-6f));
    STDROMANO_CHECK(maths::equal_with_abs_error(transformed.y, 0.0f, 1e-6f));
    STDROMANO_CHECK(maths::equal_with_abs_error(transformed.z, -1.0f, 1e-6f));
}

STDROMANO_TEST_CASE(transform44_from_rot_z)
{
    float angle = maths::constants<float>::pi / 2.0f;
    Transform44F tr = Transform44F::from_rotz(angle);
    Vec3F test_point(1.0f, 0.0f, 0.0f);
    Vec3F transformed = tr.transform_point(test_point);
    STDROMANO_CHECK(maths::equal_with_abs_error(transformed.x, 0.0f, 1e-6f));
    STDROMANO_CHECK(maths::equal_with_abs_error(transformed.y, 1.0f, 1e-6f));
    STDROMANO_CHECK(maths::equal_with_abs_error(transformed.z, 0.0f, 1e-6f));
}

STDROMANO_TEST_CASE(transform44_from_rot_degrees)
{
    Transform44F tr_rad = Transform44F::from_rotx(maths::constants<float>::pi / 2.0f, true);
    Transform44F tr_deg = Transform44F::from_rotx(90.0f, false);
    STDROMANO_CHECK(tr_rad.equal_with_abs_error(tr_deg, 1e-5f));
}

STDROMANO_TEST_CASE(transform44_from_rot_y_degrees)
{
    Transform44F tr_rad = Transform44F::from_roty(maths::constants<float>::pi / 4.0f, true);
    Transform44F tr_deg = Transform44F::from_roty(45.0f, false);
    STDROMANO_CHECK(tr_rad.equal_with_abs_error(tr_deg, 1e-5f));
}

STDROMANO_TEST_CASE(transform44_from_rot_z_degrees)
{
    Transform44F tr_rad = Transform44F::from_rotz(maths::constants<float>::pi / 6.0f, true);
    Transform44F tr_deg = Transform44F::from_rotz(30.0f, false);
    STDROMANO_CHECK(tr_rad.equal_with_abs_error(tr_deg, 1e-5f));
}

STDROMANO_TEST_CASE(transform44_from_axis_angle)
{
    Vec3F axis(0.0f, 1.0f, 0.0f);
    float angle = maths::constants<float>::pi / 2.0f;
    Transform44F tr = Transform44F::from_axis_angle(axis, angle);
    Vec3F test_point(1.0f, 0.0f, 0.0f);
    Vec3F transformed = tr.transform_point(test_point);
    STDROMANO_CHECK(maths::equal_with_abs_error(transformed.x, 0.0f, 1e-6f));
    STDROMANO_CHECK(maths::equal_with_abs_error(transformed.y, 0.0f, 1e-6f));
    STDROMANO_CHECK(maths::equal_with_abs_error(transformed.z, -1.0f, 1e-6f));
}

STDROMANO_TEST_CASE(transform44_axis_angle_vs_rotx)
{
    float angle = 1.2f;
    Transform44F from_rotx = Transform44F::from_rotx(angle);
    Transform44F from_axis = Transform44F::from_axis_angle(Vec3F(1.0f, 0.0f, 0.0f), angle);
    STDROMANO_CHECK(from_rotx.equal_with_abs_error(from_axis, 1e-5f));
}

STDROMANO_TEST_CASE(transform44_axis_angle_vs_roty)
{
    float angle = 0.8f;
    Transform44F from_roty = Transform44F::from_roty(angle);
    Transform44F from_axis = Transform44F::from_axis_angle(Vec3F(0.0f, 1.0f, 0.0f), angle);
    STDROMANO_CHECK(from_roty.equal_with_abs_error(from_axis, 1e-5f));
}

STDROMANO_TEST_CASE(transform44_axis_angle_vs_rotz)
{
    float angle = 2.1f;
    Transform44F from_rotz = Transform44F::from_rotz(angle);
    Transform44F from_axis = Transform44F::from_axis_angle(Vec3F(0.0f, 0.0f, 1.0f), angle);
    STDROMANO_CHECK(from_rotz.equal_with_abs_error(from_axis, 1e-5f));
}

STDROMANO_TEST_CASE(transform44_axis_angle_degrees)
{
    Transform44F tr_rad = Transform44F::from_axis_angle(Vec3F(0.0f, 0.0f, 1.0f),
                                                        maths::constants<float>::pi / 2.0f, true);
    Transform44F tr_deg = Transform44F::from_axis_angle(Vec3F(0.0f, 0.0f, 1.0f), 90.0f, false);
    STDROMANO_CHECK(tr_rad.equal_with_abs_error(tr_deg, 1e-5f));
}

STDROMANO_TEST_CASE(transform44_from_trs)
{
    Vec3F t(1.0f, 2.0f, 3.0f);
    Vec3F r(maths::constants<float>::pi / 4.0f, 0.0f, 0.0f);
    Vec3F s(2.0f, 2.0f, 2.0f);
    Transform44F tr = Transform44F::from_trs(t, r, s, TransformOrder_TRS, RotationOrder_XYZ);
    Vec3F t_extracted, r_extracted, s_extracted;
    tr.decomp_trs(&t_extracted, &r_extracted, &s_extracted);
    STDROMANO_CHECK(maths::equal_with_abs_error(t_extracted.x, t.x, 1e-6f));
    STDROMANO_CHECK(maths::equal_with_abs_error(t_extracted.y, t.y, 1e-6f));
    STDROMANO_CHECK(maths::equal_with_abs_error(t_extracted.z, t.z, 1e-6f));
    STDROMANO_CHECK(maths::equal_with_abs_error(r_extracted.x, r.x, 1e-6f));
    STDROMANO_CHECK(maths::equal_with_abs_error(r_extracted.y, r.y, 1e-6f));
    STDROMANO_CHECK(maths::equal_with_abs_error(r_extracted.z, r.z, 1e-6f));
    STDROMANO_CHECK(maths::equal_with_abs_error(s_extracted.x, s.x, 1e-6f));
    STDROMANO_CHECK(maths::equal_with_abs_error(s_extracted.y, s.y, 1e-6f));
    STDROMANO_CHECK(maths::equal_with_abs_error(s_extracted.z, s.z, 1e-6f));
}

STDROMANO_TEST_CASE(transform44_from_trs_srt_order)
{
    Vec3F t(1.0f, 2.0f, 3.0f);
    Vec3F r(0.0f, 0.0f, 0.0f);
    Vec3F s(2.0f, 2.0f, 2.0f);
    Transform44F tr_srt = Transform44F::from_trs(t, r, s, TransformOrder_SRT, RotationOrder_XYZ);
    Transform44F expected = Transform44F::from_scale(s) * Transform44F::from_translation(t);
    STDROMANO_CHECK(tr_srt.equal_with_abs_error(expected, 1e-6f));
}

STDROMANO_TEST_CASE(transform44_from_trs_degrees)
{
    Vec3F t(0.0f, 0.0f, 0.0f);
    Vec3F r_deg(45.0f, 0.0f, 0.0f);
    Vec3F r_rad(maths::deg2rad(45.0f), 0.0f, 0.0f);
    Vec3F s(1.0f, 1.0f, 1.0f);
    Transform44F tr_deg = Transform44F::from_trs(t, r_deg, s, TransformOrder_TRS, RotationOrder_XYZ, false);
    Transform44F tr_rad = Transform44F::from_trs(t, r_rad, s, TransformOrder_TRS, RotationOrder_XYZ, true);
    STDROMANO_CHECK(tr_deg.equal_with_abs_error(tr_rad, 1e-5f));
}

STDROMANO_TEST_CASE(transform44_from_xyzt)
{
    Vec3F x(1.0f, 0.0f, 0.0f);
    Vec3F y(0.0f, 1.0f, 0.0f);
    Vec3F z(0.0f, 0.0f, 1.0f);
    Vec3F t(1.0f, 2.0f, 3.0f);
    Transform44F tr = Transform44F::from_xyzt(x, y, z, t);
    Vec3F x_extracted, y_extracted, z_extracted, t_extracted;
    tr.decomp_xyzt(&x_extracted, &y_extracted, &z_extracted, &t_extracted);
    STDROMANO_CHECK(maths::equal_with_abs_error(x_extracted.x, x.x, 1e-6f));
    STDROMANO_CHECK(maths::equal_with_abs_error(y_extracted.y, y.y, 1e-6f));
    STDROMANO_CHECK(maths::equal_with_abs_error(z_extracted.z, z.z, 1e-6f));
    STDROMANO_CHECK(maths::equal_with_abs_error(t_extracted.x, t.x, 1e-6f));
    STDROMANO_CHECK(maths::equal_with_abs_error(t_extracted.y, t.y, 1e-6f));
    STDROMANO_CHECK(maths::equal_with_abs_error(t_extracted.z, t.z, 1e-6f));
}

STDROMANO_TEST_CASE(transform44_from_xyzt_partial_decomp)
{
    Vec3F x(1.0f, 0.0f, 0.0f);
    Vec3F y(0.0f, 1.0f, 0.0f);
    Vec3F z(0.0f, 0.0f, 1.0f);
    Vec3F t(5.0f, 6.0f, 7.0f);
    Transform44F tr = Transform44F::from_xyzt(x, y, z, t);
    Vec3F t_only;
    tr.decomp_xyzt(nullptr, nullptr, nullptr, &t_only);
    STDROMANO_CHECK(maths::equal_with_abs_error(t_only.x, t.x, 1e-6f));
    STDROMANO_CHECK(maths::equal_with_abs_error(t_only.y, t.y, 1e-6f));
    STDROMANO_CHECK(maths::equal_with_abs_error(t_only.z, t.z, 1e-6f));
}

STDROMANO_TEST_CASE(transform44_from_lookat)
{
    Vec3F eye(0.0f, 0.0f, 5.0f);
    Vec3F target(0.0f, 0.0f, 0.0f);
    Vec3F up(0.0f, 1.0f, 0.0f);
    Transform44F tr = Transform44F::from_lookat(eye, target, up);
    Vec3F z_axis(0.0f, 0.0f, -1.0f);
    Vec3F x_axis, y_axis, z_extracted, t_extracted;
    tr.decomp_xyzt(&x_axis, &y_axis, &z_extracted, &t_extracted);
    STDROMANO_CHECK(maths::equal_with_abs_error(z_extracted.x, z_axis.x, 1e-6f));
    STDROMANO_CHECK(maths::equal_with_abs_error(z_extracted.y, z_axis.y, 1e-6f));
    STDROMANO_CHECK(maths::equal_with_abs_error(z_extracted.z, z_axis.z, 1e-6f));
}

STDROMANO_TEST_CASE(transform44_from_lookat_off_axis)
{
    Vec3F eye(5.0f, 5.0f, 5.0f);
    Vec3F target(0.0f, 0.0f, 0.0f);
    Vec3F up(0.0f, 1.0f, 0.0f);
    Transform44F tr = Transform44F::from_lookat(eye, target, up);
    Vec3F expected_z = normalize(target - eye);
    Vec3F x_out, y_out, z_out, t_out;
    tr.decomp_xyzt(&x_out, &y_out, &z_out, &t_out);
    STDROMANO_CHECK(maths::equal_with_abs_error(z_out.x, expected_z.x, 1e-5f));
    STDROMANO_CHECK(maths::equal_with_abs_error(z_out.y, expected_z.y, 1e-5f));
    STDROMANO_CHECK(maths::equal_with_abs_error(z_out.z, expected_z.z, 1e-5f));
}

STDROMANO_TEST_CASE(transform44_matrix_multiplication)
{
    Transform44F tr1 = Transform44F::from_translation(Vec3F(1.0f, 2.0f, 3.0f));
    Transform44F tr2 = Transform44F::from_scale(Vec3F(2.0f, 2.0f, 2.0f));
    Transform44F result = tr1 * tr2;
    Vec3F point(1.0f, 1.0f, 1.0f);
    Vec3F transformed = result.transform_point(point);
    STDROMANO_CHECK(maths::equal_with_abs_error(transformed.x, 3.0f, 1e-6f));
    STDROMANO_CHECK(maths::equal_with_abs_error(transformed.y, 4.0f, 1e-6f));
    STDROMANO_CHECK(maths::equal_with_abs_error(transformed.z, 5.0f, 1e-6f));
}

STDROMANO_TEST_CASE(transform44_identity_multiplication)
{
    Transform44F tr(1.0f, 2.0f, 3.0f, 4.0f,
                    5.0f, 6.0f, 7.0f, 8.0f,
                    9.0f, 10.0f, 11.0f, 12.0f,
                    13.0f, 14.0f, 15.0f, 16.0f);
    Transform44F ident = Transform44F::ident();
    Transform44F result = tr * ident;
    STDROMANO_CHECK(result.equal_with_abs_error(tr, 1e-5f));
}

STDROMANO_TEST_CASE(transform44_multiplication_associativity)
{
    Transform44F a = Transform44F::from_rotx(0.3f);
    Transform44F b = Transform44F::from_roty(0.5f);
    Transform44F c = Transform44F::from_rotz(0.7f);
    Transform44F ab_c = (a * b) * c;
    Transform44F a_bc = a * (b * c);
    STDROMANO_CHECK(ab_c.equal_with_abs_error(a_bc, 1e-5f));
}

STDROMANO_TEST_CASE(transform44_transpose)
{
    Transform44F tr(1.0f, 2.0f, 3.0f, 4.0f,
                    5.0f, 6.0f, 7.0f, 8.0f,
                    9.0f, 10.0f, 11.0f, 12.0f,
                    13.0f, 14.0f, 15.0f, 16.0f);
    Transform44F transposed = tr.transpose();
    STDROMANO_CHECK_EQ(transposed(0, 0), tr(0, 0));
    STDROMANO_CHECK_EQ(transposed(0, 1), tr(1, 0));
    STDROMANO_CHECK_EQ(transposed(1, 0), tr(0, 1));
    STDROMANO_CHECK_EQ(transposed(3, 2), tr(2, 3));
}

STDROMANO_TEST_CASE(transform44_double_transpose)
{
    Transform44F tr(1.0f, 2.0f, 3.0f, 4.0f,
                    5.0f, 6.0f, 7.0f, 8.0f,
                    9.0f, 10.0f, 11.0f, 12.0f,
                    13.0f, 14.0f, 15.0f, 16.0f);
    Transform44F result = tr.transpose().transpose();
    STDROMANO_CHECK(result.equal_with_abs_error(tr, 1e-6f));
}

STDROMANO_TEST_CASE(transform44_trace)
{
    Transform44F tr = Transform44F::ident();
    float trace = tr.trace();
    STDROMANO_CHECK_EQ(trace, 4.0f);
}

STDROMANO_TEST_CASE(transform44_trace_custom)
{
    Transform44F tr(5.0f, 0.0f, 0.0f, 0.0f,
                    0.0f, 3.0f, 0.0f, 0.0f,
                    0.0f, 0.0f, 7.0f, 0.0f,
                    0.0f, 0.0f, 0.0f, 2.0f);
    STDROMANO_CHECK_EQ(tr.trace(), 17.0f);
}

STDROMANO_TEST_CASE(transform44_transform_point)
{
    Transform44F tr = Transform44F::from_translation(Vec3F(1.0f, 2.0f, 3.0f));
    Vec3F point(1.0f, 1.0f, 1.0f);
    Vec3F transformed = tr.transform_point(point);
    STDROMANO_CHECK(maths::equal_with_abs_error(transformed.x, 2.0f, 1e-6f));
    STDROMANO_CHECK(maths::equal_with_abs_error(transformed.y, 3.0f, 1e-6f));
    STDROMANO_CHECK(maths::equal_with_abs_error(transformed.z, 4.0f, 1e-6f));
}

STDROMANO_TEST_CASE(transform44_transform_point_with_rotation)
{
    Transform44F rot = Transform44F::from_rotz(maths::constants<float>::pi / 2.0f);
    Transform44F trans = Transform44F::from_translation(Vec3F(10.0f, 0.0f, 0.0f));
    Transform44F tr = trans * rot;
    Vec3F point(1.0f, 0.0f, 0.0f);
    Vec3F result = tr.transform_point(point);
    STDROMANO_CHECK(maths::equal_with_abs_error(result.x, 10.0f, 1e-5f));
    STDROMANO_CHECK(maths::equal_with_abs_error(result.y, 1.0f, 1e-5f));
    STDROMANO_CHECK(maths::equal_with_abs_error(result.z, 0.0f, 1e-5f));
}

STDROMANO_TEST_CASE(transform44_transform_dir)
{
    Transform44F tr = Transform44F::from_scale(Vec3F(2.0f, 3.0f, 4.0f));
    Vec3F dir(1.0f, 1.0f, 1.0f);
    Vec3F transformed = tr.transform_dir(dir);
    STDROMANO_CHECK(maths::equal_with_abs_error(transformed.x, 2.0f, 1e-6f));
    STDROMANO_CHECK(maths::equal_with_abs_error(transformed.y, 3.0f, 1e-6f));
    STDROMANO_CHECK(maths::equal_with_abs_error(transformed.z, 4.0f, 1e-6f));
}

STDROMANO_TEST_CASE(transform44_transform_dir_ignores_translation)
{
    Transform44F tr = Transform44F::from_translation(Vec3F(100.0f, 200.0f, 300.0f));
    Vec3F dir(1.0f, 0.0f, 0.0f);
    Vec3F result = tr.transform_dir(dir);
    STDROMANO_CHECK(maths::equal_with_abs_error(result.x, 1.0f, 1e-6f));
    STDROMANO_CHECK(maths::equal_with_abs_error(result.y, 0.0f, 1e-6f));
    STDROMANO_CHECK(maths::equal_with_abs_error(result.z, 0.0f, 1e-6f));
}

STDROMANO_TEST_CASE(transform44_decomp_tait_bryan)
{
    Vec3F r(maths::constants<float>::pi / 4.0f, 0.0f, 0.0f);
    Transform44F tr = Transform44F::from_trs(Vec3F(0.0f, 0.0f, 0.0f), r, Vec3F(1.0f, 1.0f, 1.0f),
                                             TransformOrder_TRS, RotationOrder_XYZ);
    Vec3F angles;
    tr.decomp_tait_bryan(&angles, true, RotationOrder_XYZ);
    STDROMANO_CHECK(maths::equal_with_abs_error(angles.x, r.x, 1e-6f));
    STDROMANO_CHECK(maths::equal_with_abs_error(angles.y, r.y, 1e-6f));
    STDROMANO_CHECK(maths::equal_with_abs_error(angles.z, r.z, 1e-6f));
}

STDROMANO_TEST_CASE(transform44_decomp_tait_bryan_degrees)
{
    Vec3F r_rad(0.5f, 0.3f, 0.7f);
    Transform44F tr = Transform44F::from_trs(Vec3F(0.0f, 0.0f, 0.0f), r_rad, Vec3F(1.0f, 1.0f, 1.0f),
                                             TransformOrder_TRS, RotationOrder_XYZ);
    Vec3F angles_deg;
    tr.decomp_tait_bryan(&angles_deg, false, RotationOrder_XYZ);
    STDROMANO_CHECK(maths::equal_with_abs_error(angles_deg.x, maths::rad2deg(r_rad.x), 1e-4f));
    STDROMANO_CHECK(maths::equal_with_abs_error(angles_deg.y, maths::rad2deg(r_rad.y), 1e-4f));
    STDROMANO_CHECK(maths::equal_with_abs_error(angles_deg.z, maths::rad2deg(r_rad.z), 1e-4f));
}

STDROMANO_TEST_CASE(transform44_decomp_trs_partial)
{
    Vec3F t(1.0f, 2.0f, 3.0f);
    Vec3F r(0.5f, 0.0f, 0.0f);
    Vec3F s(2.0f, 2.0f, 2.0f);
    Transform44F tr = Transform44F::from_trs(t, r, s, TransformOrder_TRS, RotationOrder_XYZ);
    Vec3F t_out;
    tr.decomp_trs(&t_out, nullptr, nullptr);
    STDROMANO_CHECK(maths::equal_with_abs_error(t_out.x, t.x, 1e-6f));
    STDROMANO_CHECK(maths::equal_with_abs_error(t_out.y, t.y, 1e-6f));
    STDROMANO_CHECK(maths::equal_with_abs_error(t_out.z, t.z, 1e-6f));
}

STDROMANO_TEST_CASE(transform44_double_precision)
{
    double angle = maths::constants<double>::pi / 3.0;
    Transform44D tr = Transform44D::from_rotz(angle);
    Vec3D point(1.0, 0.0, 0.0);
    Vec3D result = tr.transform_point(point);
    STDROMANO_CHECK(maths::equal_with_abs_error(result.x, 0.5, 1e-12));
    STDROMANO_CHECK(maths::equal_with_abs_error(result.y, maths::sqrt(3.0) / 2.0, 1e-12));
    STDROMANO_CHECK(maths::equal_with_abs_error(result.z, 0.0, 1e-12));
}

STDROMANO_TEST_CASE(transform44_double_precision_trs_roundtrip)
{
    Vec3D t(1.5, -2.3, 4.7);
    Vec3D r(0.3, 0.5, 0.7);
    Vec3D s(1.0, 1.0, 1.0);
    Transform44D tr = Transform44D::from_trs(t, r, s, TransformOrder_TRS, RotationOrder_XYZ);
    Vec3D t_out, r_out, s_out;
    tr.decomp_trs(&t_out, &r_out, &s_out);
    STDROMANO_CHECK(maths::equal_with_abs_error(t_out.x, t.x, 1e-10));
    STDROMANO_CHECK(maths::equal_with_abs_error(t_out.y, t.y, 1e-10));
    STDROMANO_CHECK(maths::equal_with_abs_error(t_out.z, t.z, 1e-10));
    STDROMANO_CHECK(maths::equal_with_abs_error(r_out.x, r.x, 1e-10));
    STDROMANO_CHECK(maths::equal_with_abs_error(r_out.y, r.y, 1e-10));
    STDROMANO_CHECK(maths::equal_with_abs_error(r_out.z, r.z, 1e-10));
}

static Transform33D random_trs33(fuzz::Source& source)
{
    const Vec2D translation(source.finite(-100.0, 100.0), source.finite(-100.0, 100.0));
    const double rotation = source.finite(-6.3, 6.3);
    const Vec2D scale(source.finite(0.1, 10.0) * (source.boolean() ? 1.0 : -1.0), source.finite(0.1, 10.0));

    return Transform33D::from_trs(translation, rotation, scale);
}

STDROMANO_TEST_CASE(fuzz_transform33_algebra)
{
    const auto report = fuzz::run_property(fixtures::options("transform33_algebra", 1000), [](fuzz::Source& source) {
        const Transform33D a = random_trs33(source);
        const Transform33D b = random_trs33(source);

        const auto inverse = a.inverse();
        STDROMANO_FUZZ_CHECK(inverse.has_value());
        STDROMANO_FUZZ_CHECK((a * inverse.value()).equal_with_abs_error(Transform33D::ident(), 1e-6));
        STDROMANO_FUZZ_CHECK((inverse.value() * a).equal_with_abs_error(Transform33D::ident(), 1e-6));

        const double det_product = (a * b).determinant();
        const double product_det = a.determinant() * b.determinant();
        STDROMANO_FUZZ_CHECK(std::abs(det_product - product_det) <= 1e-9 * std::max(1.0, std::abs(product_det)));

        STDROMANO_FUZZ_CHECK(a.transpose().transpose().equal_with_abs_error(a, 1e-12));

        const Vec2D point(source.finite(-50.0, 50.0), source.finite(-50.0, 50.0));
        const Vec2D back = inverse.value().transform_point(a.transform_point(point));
        STDROMANO_FUZZ_CHECK(back.equal_with_abs_error(point, 1e-6));

        return true;
    });

    TESTS_REQUIRE_PROPERTY(report);
}

STDROMANO_TEST_CASE(fuzz_transform44_rotations_preserve_lengths)
{
    const auto report = fuzz::run_property(fixtures::options("transform44_rotations", 1000), [](fuzz::Source& source) {
        const Transform44D rotation = Transform44D::from_rotx(source.finite(-6.3, 6.3)) *
                                      Transform44D::from_roty(source.finite(-6.3, 6.3)) *
                                      Transform44D::from_rotz(source.finite(-6.3, 6.3));

        const Vec3D dir(source.finite(-10.0, 10.0), source.finite(-10.0, 10.0), source.finite(-10.0, 10.0));
        const Vec3D rotated = rotation.transform_dir(dir);

        STDROMANO_FUZZ_CHECK(std::abs(length(rotated) - length(dir)) <= 1e-9);
        STDROMANO_FUZZ_CHECK((rotation * rotation.transpose()).equal_with_abs_error(Transform44D::ident(), 1e-9));

        const Vec3D translation(source.finite(-10.0, 10.0), source.finite(-10.0, 10.0), source.finite(-10.0, 10.0));
        const Transform44D moved = Transform44D::from_translation(translation);

        STDROMANO_FUZZ_CHECK(moved.transform_point(dir).equal_with_abs_error(dir + translation, 1e-12));
        STDROMANO_FUZZ_CHECK(moved.transform_dir(dir).equal_with_abs_error(dir, 1e-12));

        return true;
    });

    TESTS_REQUIRE_PROPERTY(report);
}

STDROMANO_TEST_CASE(transform44_storage_is_column_major)
{
    const Transform44F tr(1.0f, 2.0f, 3.0f, 4.0f,
                          5.0f, 6.0f, 7.0f, 8.0f,
                          9.0f, 10.0f, 11.0f, 12.0f,
                          13.0f, 14.0f, 15.0f, 16.0f);

    STDROMANO_CHECK_EQ(tr(0, 1), 2.0f);
    STDROMANO_CHECK_EQ(tr(1, 0), 5.0f);
    STDROMANO_CHECK_EQ(tr.data()[1], 5.0f);
    STDROMANO_CHECK_EQ(tr.data()[4], 2.0f);

    const Transform44F translation = Transform44F::from_translation(Vec3F(1.0f, 2.0f, 3.0f));
    STDROMANO_CHECK_EQ(translation.data()[12], 1.0f);
    STDROMANO_CHECK_EQ(translation.data()[13], 2.0f);
    STDROMANO_CHECK_EQ(translation.data()[14], 3.0f);
}

STDROMANO_TEST_CASE(transform44_rotations_are_counter_clockwise)
{
    const float half_pi = maths::constants<float>::pi / 2.0f;

    STDROMANO_CHECK(Transform44F::from_rotz(half_pi).transform_dir(Vec3F(1.0f, 0.0f, 0.0f))
                        .equal_with_abs_error(Vec3F(0.0f, 1.0f, 0.0f), 1e-6f));
    STDROMANO_CHECK(Transform44F::from_rotx(half_pi).transform_dir(Vec3F(0.0f, 1.0f, 0.0f))
                        .equal_with_abs_error(Vec3F(0.0f, 0.0f, 1.0f), 1e-6f));
    STDROMANO_CHECK(Transform44F::from_roty(half_pi).transform_dir(Vec3F(0.0f, 0.0f, 1.0f))
                        .equal_with_abs_error(Vec3F(1.0f, 0.0f, 0.0f), 1e-6f));
    STDROMANO_CHECK(Transform44F::from_axis_angle(Vec3F(0.0f, 0.0f, 1.0f), half_pi)
                        .equal_with_abs_error(Transform44F::from_rotz(half_pi), 1e-6f));
}

STDROMANO_TEST_CASE(transform44_from_trs_all_rotation_orders)
{
    const Vec3F t(0.0f, 0.0f, 0.0f);
    const Vec3F r(0.3f, 0.5f, 0.7f);
    const Vec3F s(1.0f, 1.0f, 1.0f);

    for(const std::uint32_t order : {RotationOrder_XYZ, RotationOrder_XZY, RotationOrder_YXZ,
                                     RotationOrder_YZX, RotationOrder_ZXY, RotationOrder_ZYX})
    {
        const Transform44F tr = Transform44F::from_trs(t, r, s, TransformOrder_TRS, order);

        Vec3F angles;
        tr.decomp_tait_bryan(&angles, true, order);

        STDROMANO_CHECK_MSG(angles.equal_with_abs_error(r, 1e-5f), StringD::make_fmt("rotation order {}", order));
    }
}

STDROMANO_TEST_CASE(transform44_from_trs_non_uniform_scale)
{
    const Vec3F t(5.0f, -3.0f, 1.0f);
    const Vec3F r(0.2f, -0.4f, 1.1f);
    const Vec3F s(1.0f, 2.0f, 3.0f);

    const Transform44F tr = Transform44F::from_trs(t, r, s, TransformOrder_TRS, RotationOrder_XYZ);

    Vec3F t_out, r_out, s_out;
    tr.decomp_trs(&t_out, &r_out, &s_out);

    STDROMANO_CHECK(t_out.equal_with_abs_error(t, 1e-5f));
    STDROMANO_CHECK(r_out.equal_with_abs_error(r, 1e-5f));
    STDROMANO_CHECK(s_out.equal_with_abs_error(s, 1e-5f));

    const Vec3F p(0.5f, -1.0f, 2.0f);
    const Vec3F expected = Transform44F::from_translation(t).transform_point(
        Transform44F::from_rotation(r).transform_dir(Transform44F::from_scale(s).transform_dir(p)));

    STDROMANO_CHECK(tr.transform_point(p).equal_with_abs_error(expected, 1e-5f));
}

STDROMANO_TEST_CASE(transform44_lookat_places_the_object)
{
    const Vec3F eye(3.0f, 4.0f, -2.0f);
    const Vec3F target(-1.0f, 0.5f, 6.0f);

    const Transform44F tr = Transform44F::from_lookat(eye, target);

    STDROMANO_CHECK(tr.transform_point(Vec3F(0.0f)).equal_with_abs_error(eye, 1e-5f));

    const Vec3F forward = tr.transform_dir(Vec3F(0.0f, 0.0f, 1.0f));
    STDROMANO_CHECK(forward.equal_with_abs_error(normalize(target - eye), 1e-5f));
    STDROMANO_CHECK_NEAR(tr.transform_dir(Vec3F(1.0f, 0.0f, 0.0f)).y, 0.0f, 1e-6f);
}

STDROMANO_TEST_CASE(fuzz_transform44_composition_and_decomposition)
{
    const auto report = fuzz::run_property(fixtures::options("transform44_composition", 1000), [](fuzz::Source& source) {
        const std::uint32_t order = static_cast<std::uint32_t>(source.index(6));

        const Vec3D t(source.finite(-100.0, 100.0), source.finite(-100.0, 100.0), source.finite(-100.0, 100.0));
        const Vec3D r(source.finite(-3.1, 3.1), source.finite(-1.5, 1.5), source.finite(-3.1, 3.1));
        const Vec3D s(source.finite(0.1, 10.0), source.finite(0.1, 10.0), source.finite(0.1, 10.0));

        Vec3D angles(r);
        const std::size_t middle[6] = {1, 2, 0, 2, 0, 1};
        const std::size_t first[6] = {0, 0, 1, 1, 2, 2};
        const std::size_t last[6] = {2, 1, 2, 0, 1, 0};

        angles[first[order]] = r.x;
        angles[middle[order]] = r.y;
        angles[last[order]] = r.z;

        const Transform44D tr = Transform44D::from_trs(t, angles, s, TransformOrder_TRS, order);

        Vec3D t_out, r_out, s_out;
        tr.decomp_trs(&t_out, &r_out, &s_out, true, order);

        STDROMANO_FUZZ_CHECK(t_out.equal_with_abs_error(t, 1e-9));
        STDROMANO_FUZZ_CHECK(s_out.equal_with_abs_error(s, 1e-9));
        STDROMANO_FUZZ_CHECK(r_out.equal_with_abs_error(angles, 1e-7));

        const Transform44D a = Transform44D::from_rotation(Vec3D(source.finite(-3.0, 3.0), 0.0, 0.0));
        const Transform44D b = Transform44D::from_translation(t);
        const Vec3D p(source.finite(-10.0, 10.0), source.finite(-10.0, 10.0), source.finite(-10.0, 10.0));

        STDROMANO_FUZZ_CHECK((a * b).transform_point(p).equal_with_abs_error(a.transform_point(b.transform_point(p)), 1e-9));

        const Transform44D rotation = Transform44D::from_rotation(angles, order);
        const Vec4D q = rotation.to_quaternion();
        const Vec3D axis(q.x, q.y, q.z);
        const double sin_half = length(axis);

        if(sin_half > 1e-6)
        {
            const double angle = 2.0 * std::atan2(sin_half, q.w);
            const Transform44D rebuilt = Transform44D::from_axis_angle(axis / sin_half, angle);
            STDROMANO_FUZZ_CHECK(rebuilt.equal_with_abs_error(rotation, 1e-9));
        }

        return true;
    });

    TESTS_REQUIRE_PROPERTY(report);
}

STDROMANO_TEST_MAIN()
