// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - Present Romain Augier
// All rights reserved.

#include "stdromano/linalg/transform.hpp"

#include "test.hpp"

using namespace stdromano;

/* =============================== */
/* Transform33 Tests (2D)          */
/* =============================== */

TEST_CASE(test_transform33_default_constructor)
{
    Transform33F tr = Transform33F::ident();
    Transform33F ident = Transform33F::ident();
    ASSERT(tr.equal_with_abs_error(ident, 1e-6f));
}

TEST_CASE(test_transform33_element_constructor)
{
    Transform33F tr(1.0f, 2.0f, 3.0f,
                    4.0f, 5.0f, 6.0f,
                    7.0f, 8.0f, 9.0f);

    ASSERT_EQUAL(tr(0, 0), 1.0f);
    ASSERT_EQUAL(tr(0, 1), 2.0f);
    ASSERT_EQUAL(tr(0, 2), 3.0f);
    ASSERT_EQUAL(tr(1, 0), 4.0f);
    ASSERT_EQUAL(tr(1, 1), 5.0f);
    ASSERT_EQUAL(tr(1, 2), 6.0f);
    ASSERT_EQUAL(tr(2, 0), 7.0f);
    ASSERT_EQUAL(tr(2, 1), 8.0f);
    ASSERT_EQUAL(tr(2, 2), 9.0f);
}

TEST_CASE(test_transform33_copy_constructor)
{
    Transform33F src(1.0f, 2.0f, 3.0f,
                     4.0f, 5.0f, 6.0f,
                     7.0f, 8.0f, 9.0f);
    Transform33F dst(src);
    ASSERT(dst.equal_with_abs_error(src, 1e-6f));
}

TEST_CASE(test_transform33_type_conversion_constructor)
{
    Transform33D src(1.0, 2.0, 3.0,
                     4.0, 5.0, 6.0,
                     7.0, 8.0, 9.0);
    Transform33F dst(src);
    ASSERT_EQUAL(dst(0, 0), 1.0f);
    ASSERT_EQUAL(dst(1, 1), 5.0f);
    ASSERT_EQUAL(dst(2, 2), 9.0f);
}

TEST_CASE(test_transform33_zero)
{
    Transform33F tr = Transform33F::zero();
    for(std::size_t i = 0; i < 9; ++i)
    {
        ASSERT_EQUAL(tr.data()[i], 0.0f);
    }
}

TEST_CASE(test_transform33_identity)
{
    Transform33F tr = Transform33F::ident();
    ASSERT_EQUAL(tr(0, 0), 1.0f);
    ASSERT_EQUAL(tr(1, 1), 1.0f);
    ASSERT_EQUAL(tr(2, 2), 1.0f);
    ASSERT_EQUAL(tr(0, 1), 0.0f);
    ASSERT_EQUAL(tr(0, 2), 0.0f);
    ASSERT_EQUAL(tr(1, 0), 0.0f);
    ASSERT_EQUAL(tr(1, 2), 0.0f);
    ASSERT_EQUAL(tr(2, 0), 0.0f);
    ASSERT_EQUAL(tr(2, 1), 0.0f);
}

TEST_CASE(test_transform33_from_translation)
{
    Vec2F t(3.0f, 7.0f);
    Transform33F tr = Transform33F::from_translation(t);
    Vec2F extracted;
    tr.decomp_translation(&extracted);
    ASSERT(maths::equal_with_abs_error(extracted.x, t.x, 1e-6f));
    ASSERT(maths::equal_with_abs_error(extracted.y, t.y, 1e-6f));
}

TEST_CASE(test_transform33_from_translation_homogeneous_row)
{
    /* Verify the homogeneous row stays [0, 0, 1] */
    Vec2F t(5.0f, -2.0f);
    Transform33F tr = Transform33F::from_translation(t);
    ASSERT_EQUAL(tr(2, 0), 0.0f);
    ASSERT_EQUAL(tr(2, 1), 0.0f);
    ASSERT_EQUAL(tr(2, 2), 1.0f);
}

TEST_CASE(test_transform33_from_scale)
{
    Vec2F s(2.0f, 3.0f);
    Transform33F tr = Transform33F::from_scale(s);
    Vec2F extracted;
    tr.decomp_scale(&extracted);
    ASSERT(maths::equal_with_abs_error(extracted.x, s.x, 1e-6f));
    ASSERT(maths::equal_with_abs_error(extracted.y, s.y, 1e-6f));
}

TEST_CASE(test_transform33_from_uniform_scale)
{
    Transform33F tr = Transform33F::from_uniform_scale(4.0f);
    Vec2F extracted;
    tr.decomp_scale(&extracted);
    ASSERT(maths::equal_with_abs_error(extracted.x, 4.0f, 1e-6f));
    ASSERT(maths::equal_with_abs_error(extracted.y, 4.0f, 1e-6f));
}

TEST_CASE(test_transform33_from_rotation_90)
{
    float angle = maths::constants<float>::pi / 2.0f;
    Transform33F tr = Transform33F::from_rotation(angle);
    Vec2F point(1.0f, 0.0f);
    Vec2F result = tr.transform_point(point);
    /* 90 degrees CW: (1, 0) -> (0, -1) */
    ASSERT(maths::equal_with_abs_error(result.x, 0.0f, 1e-6f));
    ASSERT(maths::equal_with_abs_error(result.y, -1.0f, 1e-6f));
}

TEST_CASE(test_transform33_from_rotation_180)
{
    float angle = maths::constants<float>::pi;
    Transform33F tr = Transform33F::from_rotation(angle);
    Vec2F point(1.0f, 0.0f);
    Vec2F result = tr.transform_point(point);
    /* 180 degrees: (1, 0) -> (-1, 0) */
    ASSERT(maths::equal_with_abs_error(result.x, -1.0f, 1e-5f));
    ASSERT(maths::equal_with_abs_error(result.y, 0.0f, 1e-5f));
}

TEST_CASE(test_transform33_from_rotation_45)
{
    float angle = maths::constants<float>::pi / 4.0f;
    Transform33F tr = Transform33F::from_rotation(angle);
    Vec2F point(1.0f, 0.0f);
    Vec2F result = tr.transform_point(point);
    float expected = maths::sqrt(2.0f) / 2.0f;
    ASSERT(maths::equal_with_abs_error(result.x, expected, 1e-6f));
    ASSERT(maths::equal_with_abs_error(result.y, -expected, 1e-6f));
}

TEST_CASE(test_transform33_from_rotation_degrees)
{
    Transform33F tr_rad = Transform33F::from_rotation(maths::constants<float>::pi / 2.0f, true);
    Transform33F tr_deg = Transform33F::from_rotation(90.0f, false);
    ASSERT(tr_rad.equal_with_abs_error(tr_deg, 1e-5f));
}

TEST_CASE(test_transform33_from_rotation_negative)
{
    float angle = -maths::constants<float>::pi / 2.0f;
    Transform33F tr = Transform33F::from_rotation(angle);
    Vec2F point(0.0f, 1.0f);
    Vec2F result = tr.transform_point(point);
    /* -90 degrees (CCW): (0, 1) -> (-1, 0) */
    ASSERT(maths::equal_with_abs_error(result.x, -1.0f, 1e-6f));
    ASSERT(maths::equal_with_abs_error(result.y, 0.0f, 1e-6f));
}

TEST_CASE(test_transform33_from_trs_basic)
{
    Vec2F t(5.0f, 10.0f);
    float r = maths::constants<float>::pi / 4.0f;
    Vec2F s(2.0f, 2.0f);
    Transform33F tr = Transform33F::from_trs(t, r, s, TransformOrder2D_TRS);
    Vec2F t_out;
    float r_out;
    Vec2F s_out;
    tr.decomp_trs(&t_out, &r_out, &s_out);
    ASSERT(maths::equal_with_abs_error(t_out.x, t.x, 1e-5f));
    ASSERT(maths::equal_with_abs_error(t_out.y, t.y, 1e-5f));
    ASSERT(maths::equal_with_abs_error(r_out, r, 1e-5f));
    ASSERT(maths::equal_with_abs_error(s_out.x, s.x, 1e-5f));
    ASSERT(maths::equal_with_abs_error(s_out.y, s.y, 1e-5f));
}

TEST_CASE(test_transform33_from_trs_non_uniform_scale)
{
    Vec2F t(1.0f, -2.0f);
    float r = 0.3f;
    Vec2F s(3.0f, 5.0f);
    Transform33F tr = Transform33F::from_trs(t, r, s, TransformOrder2D_TRS);
    Vec2F t_out;
    float r_out;
    Vec2F s_out;
    tr.decomp_trs(&t_out, &r_out, &s_out);
    ASSERT(maths::equal_with_abs_error(t_out.x, t.x, 1e-5f));
    ASSERT(maths::equal_with_abs_error(t_out.y, t.y, 1e-5f));
    ASSERT(maths::equal_with_abs_error(r_out, r, 1e-5f));
    ASSERT(maths::equal_with_abs_error(s_out.x, s.x, 1e-5f));
    ASSERT(maths::equal_with_abs_error(s_out.y, s.y, 1e-5f));
}

TEST_CASE(test_transform33_from_trs_srt_order)
{
    Vec2F t(1.0f, 2.0f);
    float r = 0.0f;
    Vec2F s(3.0f, 3.0f);
    /* SRT: scale * rotation * translation — translation applied first, then scaled */
    Transform33F tr_srt = Transform33F::from_trs(t, r, s, TransformOrder2D_SRT);
    Transform33F expected = Transform33F::from_scale(s) * Transform33F::from_translation(t);
    ASSERT(tr_srt.equal_with_abs_error(expected, 1e-6f));
}

TEST_CASE(test_transform33_from_trs_degrees)
{
    Vec2F t(0.0f, 0.0f);
    Vec2F s(1.0f, 1.0f);
    Transform33F tr = Transform33F::from_trs(t, 90.0f, s, TransformOrder2D_TRS, false);
    Vec2F point(1.0f, 0.0f);
    Vec2F result = tr.transform_point(point);
    ASSERT(maths::equal_with_abs_error(result.x, 0.0f, 1e-5f));
    ASSERT(maths::equal_with_abs_error(result.y, -1.0f, 1e-5f));
}

TEST_CASE(test_transform33_from_xyt)
{
    Vec2F x(1.0f, 0.0f);
    Vec2F y(0.0f, 1.0f);
    Vec2F t(4.0f, 5.0f);
    Transform33F tr = Transform33F::from_xyt(x, y, t);
    Vec2F x_out, y_out, t_out;
    tr.decomp_xyt(&x_out, &y_out, &t_out);
    ASSERT(maths::equal_with_abs_error(x_out.x, x.x, 1e-6f));
    ASSERT(maths::equal_with_abs_error(x_out.y, x.y, 1e-6f));
    ASSERT(maths::equal_with_abs_error(y_out.x, y.x, 1e-6f));
    ASSERT(maths::equal_with_abs_error(y_out.y, y.y, 1e-6f));
    ASSERT(maths::equal_with_abs_error(t_out.x, t.x, 1e-6f));
    ASSERT(maths::equal_with_abs_error(t_out.y, t.y, 1e-6f));
}

TEST_CASE(test_transform33_from_xyt_rotated)
{
    /* 45-degree rotated axes */
    float c = maths::sqrt(2.0f) / 2.0f;
    Vec2F x(c, c);
    Vec2F y(-c, c);
    Vec2F t(0.0f, 0.0f);
    Transform33F tr = Transform33F::from_xyt(x, y, t);
    float angle;
    tr.decomp_rotation(&angle);
    ASSERT(maths::equal_with_abs_error(angle, maths::constants<float>::pi / 4.0f, 1e-5f));
}

TEST_CASE(test_transform33_from_lookat)
{
    Vec2F eye(0.0f, 0.0f);
    Vec2F target(1.0f, 0.0f);
    Transform33F tr = Transform33F::from_lookat(eye, target);
    /* Looking along +X: x-axis (forward) should be (1, 0) */
    Vec2F x_out, y_out, t_out;
    tr.decomp_xyt(&x_out, &y_out, &t_out);
    ASSERT(maths::equal_with_abs_error(x_out.x, 1.0f, 1e-6f));
    ASSERT(maths::equal_with_abs_error(x_out.y, 0.0f, 1e-6f));
}

TEST_CASE(test_transform33_from_lookat_diagonal)
{
    Vec2F eye(0.0f, 0.0f);
    Vec2F target(1.0f, 1.0f);
    Transform33F tr = Transform33F::from_lookat(eye, target);
    Vec2F x_out, y_out, t_out;
    tr.decomp_xyt(&x_out, &y_out, &t_out);
    float expected = maths::sqrt(2.0f) / 2.0f;
    ASSERT(maths::equal_with_abs_error(x_out.x, expected, 1e-5f));
    ASSERT(maths::equal_with_abs_error(x_out.y, expected, 1e-5f));
}

TEST_CASE(test_transform33_matrix_multiplication)
{
    Transform33F a = Transform33F::from_rotation(maths::constants<float>::pi / 2.0f);
    Transform33F b = Transform33F::from_rotation(maths::constants<float>::pi / 2.0f);
    Transform33F result = a * b;
    /* Two 90-degree rotations = 180 degrees */
    Vec2F point(1.0f, 0.0f);
    Vec2F transformed = result.transform_point(point);
    ASSERT(maths::equal_with_abs_error(transformed.x, -1.0f, 1e-5f));
    ASSERT(maths::equal_with_abs_error(transformed.y, 0.0f, 1e-5f));
}

TEST_CASE(test_transform33_identity_multiplication)
{
    Transform33F tr(1.0f, 2.0f, 3.0f,
                    4.0f, 5.0f, 6.0f,
                    0.0f, 0.0f, 1.0f);
    Transform33F ident = Transform33F::ident();
    Transform33F result = tr * ident;
    ASSERT(result.equal_with_abs_error(tr, 1e-6f));
}

TEST_CASE(test_transform33_multiplication_associativity)
{
    Transform33F a = Transform33F::from_rotation(0.3f);
    Transform33F b = Transform33F::from_translation(Vec2F(1.0f, 2.0f));
    Transform33F c = Transform33F::from_scale(Vec2F(2.0f, 3.0f));
    Transform33F ab_c = (a * b) * c;
    Transform33F a_bc = a * (b * c);
    ASSERT(ab_c.equal_with_abs_error(a_bc, 1e-5f));
}

TEST_CASE(test_transform33_transpose)
{
    Transform33F tr(1.0f, 2.0f, 3.0f,
                    4.0f, 5.0f, 6.0f,
                    7.0f, 8.0f, 9.0f);
    Transform33F transposed = tr.transpose();
    ASSERT_EQUAL(transposed(0, 0), tr(0, 0));
    ASSERT_EQUAL(transposed(0, 1), tr(1, 0));
    ASSERT_EQUAL(transposed(1, 0), tr(0, 1));
    ASSERT_EQUAL(transposed(2, 1), tr(1, 2));
    ASSERT_EQUAL(transposed(1, 2), tr(2, 1));
}

TEST_CASE(test_transform33_double_transpose)
{
    Transform33F tr(1.0f, 2.0f, 3.0f,
                    4.0f, 5.0f, 6.0f,
                    7.0f, 8.0f, 9.0f);
    Transform33F result = tr.transpose().transpose();
    ASSERT(result.equal_with_abs_error(tr, 1e-6f));
}

TEST_CASE(test_transform33_trace)
{
    Transform33F tr = Transform33F::ident();
    ASSERT_EQUAL(tr.trace(), 3.0f);
}

TEST_CASE(test_transform33_determinant_identity)
{
    Transform33F tr = Transform33F::ident();
    ASSERT(maths::equal_with_abs_error(tr.determinant(), 1.0f, 1e-6f));
}

TEST_CASE(test_transform33_determinant_scale)
{
    Transform33F tr = Transform33F::from_scale(Vec2F(2.0f, 3.0f));
    /* Scale matrix with homogeneous row: det = 2 * 3 * 1 = 6 */
    ASSERT(maths::equal_with_abs_error(tr.determinant(), 6.0f, 1e-6f));
}

TEST_CASE(test_transform33_determinant_rotation)
{
    /* Pure rotation: det should be 1 */
    Transform33F tr = Transform33F::from_rotation(0.7f);
    ASSERT(maths::equal_with_abs_error(tr.determinant(), 1.0f, 1e-5f));
}

TEST_CASE(test_transform33_inverse_identity)
{
    Transform33F tr = Transform33F::ident();
    Transform33F inv = tr.inverse().unwrap();
    ASSERT(inv.equal_with_abs_error(Transform33F::ident(), 1e-6f));
}

TEST_CASE(test_transform33_inverse_translation)
{
    Vec2F t(3.0f, -7.0f);
    Transform33F tr = Transform33F::from_translation(t);
    Transform33F inv = tr.inverse().unwrap();
    Transform33F product = tr * inv;
    ASSERT(product.equal_with_abs_error(Transform33F::ident(), 1e-5f));
}

TEST_CASE(test_transform33_inverse_rotation)
{
    Transform33F tr = Transform33F::from_rotation(1.2f);
    Transform33F inv = tr.inverse().unwrap();
    Transform33F product = tr * inv;
    ASSERT(product.equal_with_abs_error(Transform33F::ident(), 1e-5f));
}

TEST_CASE(test_transform33_inverse_scale)
{
    Transform33F tr = Transform33F::from_scale(Vec2F(2.0f, 5.0f));
    Transform33F inv = tr.inverse().unwrap();
    Transform33F product = tr * inv;
    ASSERT(product.equal_with_abs_error(Transform33F::ident(), 1e-5f));
}

TEST_CASE(test_transform33_inverse_composite)
{
    /* Inverse of a full TRS transform */
    Transform33F tr = Transform33F::from_trs(Vec2F(3.0f, -1.0f), 0.8f, Vec2F(2.0f, 3.0f));
    Transform33F inv = tr.inverse().unwrap();
    Transform33F product = tr * inv;
    ASSERT(product.equal_with_abs_error(Transform33F::ident(), 1e-4f));
}

TEST_CASE(test_transform33_inverse_undoes_transform)
{
    Transform33F tr = Transform33F::from_trs(Vec2F(5.0f, 3.0f), 1.0f, Vec2F(2.0f, 2.0f));
    Transform33F inv = tr.inverse().unwrap();
    Vec2F point(7.0f, -4.0f);
    Vec2F transformed = tr.transform_point(point);
    Vec2F recovered = inv.transform_point(transformed);
    ASSERT(maths::equal_with_abs_error(recovered.x, point.x, 1e-4f));
    ASSERT(maths::equal_with_abs_error(recovered.y, point.y, 1e-4f));
}

TEST_CASE(test_transform33_transform_point_identity)
{
    Transform33F tr = Transform33F::ident();
    Vec2F p(3.0f, 7.0f);
    Vec2F result = tr.transform_point(p);
    ASSERT(maths::equal_with_abs_error(result.x, p.x, 1e-6f));
    ASSERT(maths::equal_with_abs_error(result.y, p.y, 1e-6f));
}

TEST_CASE(test_transform33_transform_point_translation)
{
    Transform33F tr = Transform33F::from_translation(Vec2F(10.0f, 20.0f));
    Vec2F point(1.0f, 2.0f);
    Vec2F result = tr.transform_point(point);
    ASSERT(maths::equal_with_abs_error(result.x, 11.0f, 1e-6f));
    ASSERT(maths::equal_with_abs_error(result.y, 22.0f, 1e-6f));
}

TEST_CASE(test_transform33_transform_point_scale)
{
    Transform33F tr = Transform33F::from_scale(Vec2F(2.0f, 3.0f));
    Vec2F point(4.0f, 5.0f);
    Vec2F result = tr.transform_point(point);
    ASSERT(maths::equal_with_abs_error(result.x, 8.0f, 1e-6f));
    ASSERT(maths::equal_with_abs_error(result.y, 15.0f, 1e-6f));
}

TEST_CASE(test_transform33_transform_point_combined)
{
    /* Rotate 90 degrees then translate (10, 0) */
    Transform33F rot = Transform33F::from_rotation(maths::constants<float>::pi / 2.0f);
    Transform33F trans = Transform33F::from_translation(Vec2F(10.0f, 0.0f));
    Transform33F tr = trans * rot;
    Vec2F point(1.0f, 0.0f);
    Vec2F result = tr.transform_point(point);
    /* Rotation maps (1,0) -> (0,1), then translation adds (10,0) */
    ASSERT(maths::equal_with_abs_error(result.x, 10.0f, 1e-5f));
    ASSERT(maths::equal_with_abs_error(result.y, -1.0f, 1e-5f));
}

TEST_CASE(test_transform33_transform_dir_ignores_translation)
{
    Transform33F tr = Transform33F::from_translation(Vec2F(100.0f, 200.0f));
    Vec2F dir(1.0f, 0.0f);
    Vec2F result = tr.transform_dir(dir);
    ASSERT(maths::equal_with_abs_error(result.x, 1.0f, 1e-6f));
    ASSERT(maths::equal_with_abs_error(result.y, 0.0f, 1e-6f));
}

TEST_CASE(test_transform33_transform_dir_with_rotation)
{
    Transform33F tr = Transform33F::from_rotation(maths::constants<float>::pi / 2.0f);
    Vec2F dir(1.0f, 0.0f);
    Vec2F result = tr.transform_dir(dir);
    ASSERT(maths::equal_with_abs_error(result.x, 0.0f, 1e-6f));
    ASSERT(maths::equal_with_abs_error(result.y, -1.0f, 1e-6f));
}

TEST_CASE(test_transform33_transform_dir_with_scale)
{
    Transform33F tr = Transform33F::from_scale(Vec2F(3.0f, 5.0f));
    Vec2F dir(1.0f, 1.0f);
    Vec2F result = tr.transform_dir(dir);
    ASSERT(maths::equal_with_abs_error(result.x, 3.0f, 1e-6f));
    ASSERT(maths::equal_with_abs_error(result.y, 5.0f, 1e-6f));
}

TEST_CASE(test_transform33_decomp_rotation_roundtrip)
{
    float angle = 1.3f;
    Transform33F tr = Transform33F::from_rotation(angle);
    float extracted;
    tr.decomp_rotation(&extracted);
    ASSERT(maths::equal_with_abs_error(extracted, angle, 1e-6f));
}

TEST_CASE(test_transform33_decomp_rotation_negative)
{
    float angle = -0.7f;
    Transform33F tr = Transform33F::from_rotation(angle);
    float extracted;
    tr.decomp_rotation(&extracted);
    ASSERT(maths::equal_with_abs_error(extracted, angle, 1e-6f));
}

TEST_CASE(test_transform33_decomp_rotation_degrees)
{
    float angle_rad = maths::constants<float>::pi / 3.0f;
    Transform33F tr = Transform33F::from_rotation(angle_rad);
    float angle_deg;
    tr.decomp_rotation(&angle_deg, false);
    ASSERT(maths::equal_with_abs_error(angle_deg, 60.0f, 1e-4f));
}

TEST_CASE(test_transform33_decomp_rotation_with_scale)
{
    /* Rotation should be extractable even with non-uniform scale */
    float angle = 0.5f;
    Vec2F s(3.0f, 7.0f);
    Transform33F tr = Transform33F::from_trs(Vec2F(0.0f, 0.0f), angle, s, TransformOrder2D_TRS);
    float extracted;
    tr.decomp_rotation(&extracted);
    ASSERT(maths::equal_with_abs_error(extracted, angle, 1e-5f));
}

TEST_CASE(test_transform33_decomp_xyt_partial)
{
    Vec2F x(1.0f, 0.0f);
    Vec2F y(0.0f, 1.0f);
    Vec2F t(4.0f, 5.0f);
    Transform33F tr = Transform33F::from_xyt(x, y, t);
    Vec2F t_only;
    tr.decomp_xyt(nullptr, nullptr, &t_only);
    ASSERT(maths::equal_with_abs_error(t_only.x, t.x, 1e-6f));
    ASSERT(maths::equal_with_abs_error(t_only.y, t.y, 1e-6f));
}

TEST_CASE(test_transform33_decomp_trs_partial)
{
    Transform33F tr = Transform33F::from_trs(Vec2F(1.0f, 2.0f), 0.5f, Vec2F(3.0f, 4.0f));
    float r_out;
    tr.decomp_trs(nullptr, &r_out, nullptr);
    ASSERT(maths::equal_with_abs_error(r_out, 0.5f, 1e-5f));
}

TEST_CASE(test_transform33_double_precision)
{
    double angle = maths::constants<double>::pi / 3.0;
    Transform33D tr = Transform33D::from_rotation(angle);
    Vec2D point(1.0, 0.0);
    Vec2D result = tr.transform_point(point);
    ASSERT(maths::equal_with_abs_error(result.x, 0.5, 1e-12));
    ASSERT(maths::equal_with_abs_error(result.y, -maths::sqrt(3.0) / 2.0, 1e-12));
}

TEST_CASE(test_transform33_double_precision_trs_roundtrip)
{
    Vec2D t(1.5, -2.3);
    double r = 0.7;
    Vec2D s(2.5, 3.5);
    Transform33D tr = Transform33D::from_trs(t, r, s, TransformOrder2D_TRS);
    Vec2D t_out;
    double r_out;
    Vec2D s_out;
    tr.decomp_trs(&t_out, &r_out, &s_out);
    ASSERT(maths::equal_with_abs_error(t_out.x, t.x, 1e-10));
    ASSERT(maths::equal_with_abs_error(t_out.y, t.y, 1e-10));
    ASSERT(maths::equal_with_abs_error(r_out, r, 1e-10));
    ASSERT(maths::equal_with_abs_error(s_out.x, s.x, 1e-10));
    ASSERT(maths::equal_with_abs_error(s_out.y, s.y, 1e-10));
}

/* =============================== */
/* Improved Transform44 Tests      */
/* =============================== */

TEST_CASE(test_transform44_default_constructor)
{
    Transform44F tr = Transform44F::ident();
    Transform44F ident = Transform44F::ident();
    ASSERT(tr.equal_with_abs_error(ident, 1e-6f));
}

TEST_CASE(test_transform44_element_constructor)
{
    Transform44F tr(1.0f, 2.0f, 3.0f, 4.0f,
                    5.0f, 6.0f, 7.0f, 8.0f,
                    9.0f, 10.0f, 11.0f, 12.0f,
                    13.0f, 14.0f, 15.0f, 16.0f);

    ASSERT_EQUAL(tr(0, 0), 1.0f);
    ASSERT_EQUAL(tr(0, 1), 2.0f);
    ASSERT_EQUAL(tr(0, 2), 3.0f);
    ASSERT_EQUAL(tr(0, 3), 4.0f);
    ASSERT_EQUAL(tr(1, 0), 5.0f);
    ASSERT_EQUAL(tr(1, 1), 6.0f);
    ASSERT_EQUAL(tr(1, 2), 7.0f);
    ASSERT_EQUAL(tr(1, 3), 8.0f);
    ASSERT_EQUAL(tr(2, 0), 9.0f);
    ASSERT_EQUAL(tr(2, 1), 10.0f);
    ASSERT_EQUAL(tr(2, 2), 11.0f);
    ASSERT_EQUAL(tr(2, 3), 12.0f);
    ASSERT_EQUAL(tr(3, 0), 13.0f);
    ASSERT_EQUAL(tr(3, 1), 14.0f);
    ASSERT_EQUAL(tr(3, 2), 15.0f);
    ASSERT_EQUAL(tr(3, 3), 16.0f);
}

TEST_CASE(test_transform44_copy_constructor)
{
    Transform44F src(1.0f, 2.0f, 3.0f, 4.0f,
                     5.0f, 6.0f, 7.0f, 8.0f,
                     9.0f, 10.0f, 11.0f, 12.0f,
                     13.0f, 14.0f, 15.0f, 16.0f);
    Transform44F dst(src);
    ASSERT(dst.equal_with_abs_error(src, 1e-6f));
}

TEST_CASE(test_transform44_type_conversion_constructor)
{
    Transform44D src(1.0, 2.0, 3.0, 4.0,
                     5.0, 6.0, 7.0, 8.0,
                     9.0, 10.0, 11.0, 12.0,
                     13.0, 14.0, 15.0, 16.0);
    Transform44F dst(src);
    ASSERT_EQUAL(dst(0, 0), 1.0f);
    ASSERT_EQUAL(dst(1, 1), 6.0f);
    ASSERT_EQUAL(dst(2, 2), 11.0f);
    ASSERT_EQUAL(dst(3, 3), 16.0f);
}

TEST_CASE(test_transform44_zero)
{
    Transform44F tr = Transform44F::zero();
    for(std::size_t i = 0; i < 16; ++i)
    {
        ASSERT_EQUAL(tr.data()[i], 0.0f);
    }
}

TEST_CASE(test_transform44_identity)
{
    Transform44F tr = Transform44F::ident();
    ASSERT_EQUAL(tr(0, 0), 1.0f);
    ASSERT_EQUAL(tr(1, 1), 1.0f);
    ASSERT_EQUAL(tr(2, 2), 1.0f);
    ASSERT_EQUAL(tr(3, 3), 1.0f);
    ASSERT_EQUAL(tr(0, 1), 0.0f);
    ASSERT_EQUAL(tr(0, 2), 0.0f);
    ASSERT_EQUAL(tr(0, 3), 0.0f);
    ASSERT_EQUAL(tr(1, 0), 0.0f);
    ASSERT_EQUAL(tr(1, 2), 0.0f);
    ASSERT_EQUAL(tr(1, 3), 0.0f);
    ASSERT_EQUAL(tr(2, 0), 0.0f);
    ASSERT_EQUAL(tr(2, 1), 0.0f);
    ASSERT_EQUAL(tr(2, 3), 0.0f);
    ASSERT_EQUAL(tr(3, 0), 0.0f);
    ASSERT_EQUAL(tr(3, 1), 0.0f);
    ASSERT_EQUAL(tr(3, 2), 0.0f);
}

TEST_CASE(test_transform44_from_translation)
{
    Vec3F t(1.0f, 2.0f, 3.0f);
    Transform44F tr = Transform44F::from_translation(t);
    Vec3F extracted;
    tr.decomp_translation(&extracted);
    ASSERT(maths::equal_with_abs_error(extracted.x, t.x, 1e-6f));
    ASSERT(maths::equal_with_abs_error(extracted.y, t.y, 1e-6f));
    ASSERT(maths::equal_with_abs_error(extracted.z, t.z, 1e-6f));
}

TEST_CASE(test_transform44_from_scale)
{
    Vec3F s(2.0f, 3.0f, 4.0f);
    Transform44F tr = Transform44F::from_scale(s);
    Vec3F extracted;
    tr.decomp_scale(&extracted);
    ASSERT(maths::equal_with_abs_error(extracted.x, s.x, 1e-6f));
    ASSERT(maths::equal_with_abs_error(extracted.y, s.y, 1e-6f));
    ASSERT(maths::equal_with_abs_error(extracted.z, s.z, 1e-6f));
}

TEST_CASE(test_transform44_from_rot_x)
{
    float angle = maths::constants<float>::pi / 2.0f;
    Transform44F tr = Transform44F::from_rotx(angle);
    Vec3F test_point(0.0f, 1.0f, 0.0f);
    Vec3F transformed = tr.transform_point(test_point);
    ASSERT(maths::equal_with_abs_error(transformed.x, 0.0f, 1e-6f));
    ASSERT(maths::equal_with_abs_error(transformed.y, 0.0f, 1e-6f));
    ASSERT(maths::equal_with_abs_error(transformed.z, 1.0f, 1e-6f));
}

TEST_CASE(test_transform44_from_rot_y)
{
    float angle = maths::constants<float>::pi / 2.0f;
    Transform44F tr = Transform44F::from_roty(angle);
    Vec3F test_point(1.0f, 0.0f, 0.0f);
    Vec3F transformed = tr.transform_point(test_point);
    ASSERT(maths::equal_with_abs_error(transformed.x, 0.0f, 1e-6f));
    ASSERT(maths::equal_with_abs_error(transformed.y, 0.0f, 1e-6f));
    ASSERT(maths::equal_with_abs_error(transformed.z, -1.0f, 1e-6f));
}

TEST_CASE(test_transform44_from_rot_z)
{
    float angle = maths::constants<float>::pi / 2.0f;
    Transform44F tr = Transform44F::from_rotz(angle);
    Vec3F test_point(1.0f, 0.0f, 0.0f);
    Vec3F transformed = tr.transform_point(test_point);
    ASSERT(maths::equal_with_abs_error(transformed.x, 0.0f, 1e-6f));
    ASSERT(maths::equal_with_abs_error(transformed.y, 1.0f, 1e-6f));
    ASSERT(maths::equal_with_abs_error(transformed.z, 0.0f, 1e-6f));
}

TEST_CASE(test_transform44_from_rot_degrees)
{
    Transform44F tr_rad = Transform44F::from_rotx(maths::constants<float>::pi / 2.0f, true);
    Transform44F tr_deg = Transform44F::from_rotx(90.0f, false);
    ASSERT(tr_rad.equal_with_abs_error(tr_deg, 1e-5f));
}

TEST_CASE(test_transform44_from_rot_y_degrees)
{
    Transform44F tr_rad = Transform44F::from_roty(maths::constants<float>::pi / 4.0f, true);
    Transform44F tr_deg = Transform44F::from_roty(45.0f, false);
    ASSERT(tr_rad.equal_with_abs_error(tr_deg, 1e-5f));
}

TEST_CASE(test_transform44_from_rot_z_degrees)
{
    Transform44F tr_rad = Transform44F::from_rotz(maths::constants<float>::pi / 6.0f, true);
    Transform44F tr_deg = Transform44F::from_rotz(30.0f, false);
    ASSERT(tr_rad.equal_with_abs_error(tr_deg, 1e-5f));
}

TEST_CASE(test_transform44_from_axis_angle)
{
    Vec3F axis(0.0f, 1.0f, 0.0f);
    float angle = maths::constants<float>::pi / 2.0f;
    Transform44F tr = Transform44F::from_axis_angle(axis, angle);
    Vec3F test_point(1.0f, 0.0f, 0.0f);
    Vec3F transformed = tr.transform_point(test_point);
    ASSERT(maths::equal_with_abs_error(transformed.x, 0.0f, 1e-6f));
    ASSERT(maths::equal_with_abs_error(transformed.y, 0.0f, 1e-6f));
    ASSERT(maths::equal_with_abs_error(transformed.z, -1.0f, 1e-6f));
}

TEST_CASE(test_transform44_axis_angle_vs_rotx)
{
    float angle = 1.2f;
    Transform44F from_rotx = Transform44F::from_rotx(angle);
    Transform44F from_axis = Transform44F::from_axis_angle(Vec3F(1.0f, 0.0f, 0.0f), angle);
    ASSERT(from_rotx.equal_with_abs_error(from_axis, 1e-5f));
}

TEST_CASE(test_transform44_axis_angle_vs_roty)
{
    float angle = 0.8f;
    Transform44F from_roty = Transform44F::from_roty(angle);
    Transform44F from_axis = Transform44F::from_axis_angle(Vec3F(0.0f, 1.0f, 0.0f), angle);
    ASSERT(from_roty.equal_with_abs_error(from_axis, 1e-5f));
}

TEST_CASE(test_transform44_axis_angle_vs_rotz)
{
    float angle = 2.1f;
    Transform44F from_rotz = Transform44F::from_rotz(angle);
    Transform44F from_axis = Transform44F::from_axis_angle(Vec3F(0.0f, 0.0f, 1.0f), angle);
    ASSERT(from_rotz.equal_with_abs_error(from_axis, 1e-5f));
}

TEST_CASE(test_transform44_axis_angle_degrees)
{
    Transform44F tr_rad = Transform44F::from_axis_angle(Vec3F(0.0f, 0.0f, 1.0f),
                                                        maths::constants<float>::pi / 2.0f, true);
    Transform44F tr_deg = Transform44F::from_axis_angle(Vec3F(0.0f, 0.0f, 1.0f), 90.0f, false);
    ASSERT(tr_rad.equal_with_abs_error(tr_deg, 1e-5f));
}

TEST_CASE(test_transform44_from_trs)
{
    Vec3F t(1.0f, 2.0f, 3.0f);
    Vec3F r(maths::constants<float>::pi / 4.0f, 0.0f, 0.0f);
    Vec3F s(2.0f, 2.0f, 2.0f);
    Transform44F tr = Transform44F::from_trs(t, r, s, TransformOrder_TRS, RotationOrder_XYZ);
    Vec3F t_extracted, r_extracted, s_extracted;
    tr.decomp_trs(&t_extracted, &r_extracted, &s_extracted);
    ASSERT(maths::equal_with_abs_error(t_extracted.x, t.x, 1e-6f));
    ASSERT(maths::equal_with_abs_error(t_extracted.y, t.y, 1e-6f));
    ASSERT(maths::equal_with_abs_error(t_extracted.z, t.z, 1e-6f));
    ASSERT(maths::equal_with_abs_error(r_extracted.x, r.x, 1e-6f));
    ASSERT(maths::equal_with_abs_error(r_extracted.y, r.y, 1e-6f));
    ASSERT(maths::equal_with_abs_error(r_extracted.z, r.z, 1e-6f));
    ASSERT(maths::equal_with_abs_error(s_extracted.x, s.x, 1e-6f));
    ASSERT(maths::equal_with_abs_error(s_extracted.y, s.y, 1e-6f));
    ASSERT(maths::equal_with_abs_error(s_extracted.z, s.z, 1e-6f));
}

TEST_CASE(test_transform44_from_trs_all_rotation_orders)
{
    Vec3F t(0.0f, 0.0f, 0.0f);
    Vec3F r(0.3f, 0.5f, 0.7f);
    Vec3F s(1.0f, 1.0f, 1.0f);

    std::uint32_t orders[] = { RotationOrder_XYZ, RotationOrder_XZY, RotationOrder_YXZ,
                               RotationOrder_YZX, RotationOrder_ZXY, RotationOrder_ZYX };

    for(std::uint32_t order : orders)
    {
        Transform44F tr = Transform44F::from_trs(t, r, s, TransformOrder_TRS, order);
        Vec3F angles;
        tr.decomp_tait_bryan(&angles, true, order);
        ASSERT(maths::equal_with_abs_error(angles.x, r.x, 1e-5f));
        ASSERT(maths::equal_with_abs_error(angles.y, r.y, 1e-5f));
        ASSERT(maths::equal_with_abs_error(angles.z, r.z, 1e-5f));
    }
}

TEST_CASE(test_transform44_from_trs_non_uniform_scale)
{
    Vec3F t(5.0f, -3.0f, 1.0f);
    Vec3F r(0.2f, 0.0f, 0.0f);
    Vec3F s(1.0f, 2.0f, 3.0f);
    Transform44F tr = Transform44F::from_trs(t, r, s, TransformOrder_TRS, RotationOrder_XYZ);
    Vec3F t_out, r_out, s_out;
    tr.decomp_trs(&t_out, &r_out, &s_out);
    ASSERT(maths::equal_with_abs_error(t_out.x, t.x, 1e-5f));
    ASSERT(maths::equal_with_abs_error(t_out.y, t.y, 1e-5f));
    ASSERT(maths::equal_with_abs_error(t_out.z, t.z, 1e-5f));
    ASSERT(maths::equal_with_abs_error(s_out.x, s.x, 1e-5f));
    ASSERT(maths::equal_with_abs_error(s_out.y, s.y, 1e-5f));
    ASSERT(maths::equal_with_abs_error(s_out.z, s.z, 1e-5f));
}

TEST_CASE(test_transform44_from_trs_srt_order)
{
    Vec3F t(1.0f, 2.0f, 3.0f);
    Vec3F r(0.0f, 0.0f, 0.0f);
    Vec3F s(2.0f, 2.0f, 2.0f);
    Transform44F tr_srt = Transform44F::from_trs(t, r, s, TransformOrder_SRT, RotationOrder_XYZ);
    Transform44F expected = Transform44F::from_scale(s) * Transform44F::from_translation(t);
    ASSERT(tr_srt.equal_with_abs_error(expected, 1e-6f));
}

TEST_CASE(test_transform44_from_trs_degrees)
{
    Vec3F t(0.0f, 0.0f, 0.0f);
    Vec3F r_deg(45.0f, 0.0f, 0.0f);
    Vec3F r_rad(maths::deg2rad(45.0f), 0.0f, 0.0f);
    Vec3F s(1.0f, 1.0f, 1.0f);
    Transform44F tr_deg = Transform44F::from_trs(t, r_deg, s, TransformOrder_TRS, RotationOrder_XYZ, false);
    Transform44F tr_rad = Transform44F::from_trs(t, r_rad, s, TransformOrder_TRS, RotationOrder_XYZ, true);
    ASSERT(tr_deg.equal_with_abs_error(tr_rad, 1e-5f));
}

TEST_CASE(test_transform44_from_xyzt)
{
    Vec3F x(1.0f, 0.0f, 0.0f);
    Vec3F y(0.0f, 1.0f, 0.0f);
    Vec3F z(0.0f, 0.0f, 1.0f);
    Vec3F t(1.0f, 2.0f, 3.0f);
    Transform44F tr = Transform44F::from_xyzt(x, y, z, t);
    Vec3F x_extracted, y_extracted, z_extracted, t_extracted;
    tr.decomp_xyzt(&x_extracted, &y_extracted, &z_extracted, &t_extracted);
    ASSERT(maths::equal_with_abs_error(x_extracted.x, x.x, 1e-6f));
    ASSERT(maths::equal_with_abs_error(y_extracted.y, y.y, 1e-6f));
    ASSERT(maths::equal_with_abs_error(z_extracted.z, z.z, 1e-6f));
    ASSERT(maths::equal_with_abs_error(t_extracted.x, t.x, 1e-6f));
    ASSERT(maths::equal_with_abs_error(t_extracted.y, t.y, 1e-6f));
    ASSERT(maths::equal_with_abs_error(t_extracted.z, t.z, 1e-6f));
}

TEST_CASE(test_transform44_from_xyzt_partial_decomp)
{
    Vec3F x(1.0f, 0.0f, 0.0f);
    Vec3F y(0.0f, 1.0f, 0.0f);
    Vec3F z(0.0f, 0.0f, 1.0f);
    Vec3F t(5.0f, 6.0f, 7.0f);
    Transform44F tr = Transform44F::from_xyzt(x, y, z, t);
    Vec3F t_only;
    tr.decomp_xyzt(nullptr, nullptr, nullptr, &t_only);
    ASSERT(maths::equal_with_abs_error(t_only.x, t.x, 1e-6f));
    ASSERT(maths::equal_with_abs_error(t_only.y, t.y, 1e-6f));
    ASSERT(maths::equal_with_abs_error(t_only.z, t.z, 1e-6f));
}

TEST_CASE(test_transform44_from_lookat)
{
    Vec3F eye(0.0f, 0.0f, 5.0f);
    Vec3F target(0.0f, 0.0f, 0.0f);
    Vec3F up(0.0f, 1.0f, 0.0f);
    Transform44F tr = Transform44F::from_lookat(eye, target, up);
    Vec3F z_axis(0.0f, 0.0f, -1.0f);
    Vec3F x_axis, y_axis, z_extracted, t_extracted;
    tr.decomp_xyzt(&x_axis, &y_axis, &z_extracted, &t_extracted);
    ASSERT(maths::equal_with_abs_error(z_extracted.x, z_axis.x, 1e-6f));
    ASSERT(maths::equal_with_abs_error(z_extracted.y, z_axis.y, 1e-6f));
    ASSERT(maths::equal_with_abs_error(z_extracted.z, z_axis.z, 1e-6f));
}

TEST_CASE(test_transform44_from_lookat_off_axis)
{
    Vec3F eye(5.0f, 5.0f, 5.0f);
    Vec3F target(0.0f, 0.0f, 0.0f);
    Vec3F up(0.0f, 1.0f, 0.0f);
    Transform44F tr = Transform44F::from_lookat(eye, target, up);
    Vec3F expected_z = normalize(target - eye);
    Vec3F x_out, y_out, z_out, t_out;
    tr.decomp_xyzt(&x_out, &y_out, &z_out, &t_out);
    ASSERT(maths::equal_with_abs_error(z_out.x, expected_z.x, 1e-5f));
    ASSERT(maths::equal_with_abs_error(z_out.y, expected_z.y, 1e-5f));
    ASSERT(maths::equal_with_abs_error(z_out.z, expected_z.z, 1e-5f));
}

TEST_CASE(test_transform44_matrix_multiplication)
{
    Transform44F tr1 = Transform44F::from_translation(Vec3F(1.0f, 2.0f, 3.0f));
    Transform44F tr2 = Transform44F::from_scale(Vec3F(2.0f, 2.0f, 2.0f));
    Transform44F result = tr1 * tr2;
    Vec3F point(1.0f, 1.0f, 1.0f);
    Vec3F transformed = result.transform_point(point);
    ASSERT(maths::equal_with_abs_error(transformed.x, 3.0f, 1e-6f));
    ASSERT(maths::equal_with_abs_error(transformed.y, 4.0f, 1e-6f));
    ASSERT(maths::equal_with_abs_error(transformed.z, 5.0f, 1e-6f));
}

TEST_CASE(test_transform44_identity_multiplication)
{
    Transform44F tr(1.0f, 2.0f, 3.0f, 4.0f,
                    5.0f, 6.0f, 7.0f, 8.0f,
                    9.0f, 10.0f, 11.0f, 12.0f,
                    13.0f, 14.0f, 15.0f, 16.0f);
    Transform44F ident = Transform44F::ident();
    Transform44F result = tr * ident;
    ASSERT(result.equal_with_abs_error(tr, 1e-5f));
}

TEST_CASE(test_transform44_multiplication_associativity)
{
    Transform44F a = Transform44F::from_rotx(0.3f);
    Transform44F b = Transform44F::from_roty(0.5f);
    Transform44F c = Transform44F::from_rotz(0.7f);
    Transform44F ab_c = (a * b) * c;
    Transform44F a_bc = a * (b * c);
    ASSERT(ab_c.equal_with_abs_error(a_bc, 1e-5f));
}

TEST_CASE(test_transform44_transpose)
{
    Transform44F tr(1.0f, 2.0f, 3.0f, 4.0f,
                    5.0f, 6.0f, 7.0f, 8.0f,
                    9.0f, 10.0f, 11.0f, 12.0f,
                    13.0f, 14.0f, 15.0f, 16.0f);
    Transform44F transposed = tr.transpose();
    ASSERT_EQUAL(transposed(0, 0), tr(0, 0));
    ASSERT_EQUAL(transposed(0, 1), tr(1, 0));
    ASSERT_EQUAL(transposed(1, 0), tr(0, 1));
    ASSERT_EQUAL(transposed(3, 2), tr(2, 3));
}

TEST_CASE(test_transform44_double_transpose)
{
    Transform44F tr(1.0f, 2.0f, 3.0f, 4.0f,
                    5.0f, 6.0f, 7.0f, 8.0f,
                    9.0f, 10.0f, 11.0f, 12.0f,
                    13.0f, 14.0f, 15.0f, 16.0f);
    Transform44F result = tr.transpose().transpose();
    ASSERT(result.equal_with_abs_error(tr, 1e-6f));
}

TEST_CASE(test_transform44_trace)
{
    Transform44F tr = Transform44F::ident();
    float trace = tr.trace();
    ASSERT_EQUAL(trace, 4.0f);
}

TEST_CASE(test_transform44_trace_custom)
{
    Transform44F tr(5.0f, 0.0f, 0.0f, 0.0f,
                    0.0f, 3.0f, 0.0f, 0.0f,
                    0.0f, 0.0f, 7.0f, 0.0f,
                    0.0f, 0.0f, 0.0f, 2.0f);
    ASSERT_EQUAL(tr.trace(), 17.0f);
}

TEST_CASE(test_transform44_transform_point)
{
    Transform44F tr = Transform44F::from_translation(Vec3F(1.0f, 2.0f, 3.0f));
    Vec3F point(1.0f, 1.0f, 1.0f);
    Vec3F transformed = tr.transform_point(point);
    ASSERT(maths::equal_with_abs_error(transformed.x, 2.0f, 1e-6f));
    ASSERT(maths::equal_with_abs_error(transformed.y, 3.0f, 1e-6f));
    ASSERT(maths::equal_with_abs_error(transformed.z, 4.0f, 1e-6f));
}

TEST_CASE(test_transform44_transform_point_with_rotation)
{
    Transform44F rot = Transform44F::from_rotz(maths::constants<float>::pi / 2.0f);
    Transform44F trans = Transform44F::from_translation(Vec3F(10.0f, 0.0f, 0.0f));
    Transform44F tr = trans * rot;
    Vec3F point(1.0f, 0.0f, 0.0f);
    Vec3F result = tr.transform_point(point);
    ASSERT(maths::equal_with_abs_error(result.x, 10.0f, 1e-5f));
    ASSERT(maths::equal_with_abs_error(result.y, 1.0f, 1e-5f));
    ASSERT(maths::equal_with_abs_error(result.z, 0.0f, 1e-5f));
}

TEST_CASE(test_transform44_transform_dir)
{
    Transform44F tr = Transform44F::from_scale(Vec3F(2.0f, 3.0f, 4.0f));
    Vec3F dir(1.0f, 1.0f, 1.0f);
    Vec3F transformed = tr.transform_dir(dir);
    ASSERT(maths::equal_with_abs_error(transformed.x, 2.0f, 1e-6f));
    ASSERT(maths::equal_with_abs_error(transformed.y, 3.0f, 1e-6f));
    ASSERT(maths::equal_with_abs_error(transformed.z, 4.0f, 1e-6f));
}

TEST_CASE(test_transform44_transform_dir_ignores_translation)
{
    Transform44F tr = Transform44F::from_translation(Vec3F(100.0f, 200.0f, 300.0f));
    Vec3F dir(1.0f, 0.0f, 0.0f);
    Vec3F result = tr.transform_dir(dir);
    ASSERT(maths::equal_with_abs_error(result.x, 1.0f, 1e-6f));
    ASSERT(maths::equal_with_abs_error(result.y, 0.0f, 1e-6f));
    ASSERT(maths::equal_with_abs_error(result.z, 0.0f, 1e-6f));
}

TEST_CASE(test_transform44_decomp_tait_bryan)
{
    Vec3F r(maths::constants<float>::pi / 4.0f, 0.0f, 0.0f);
    Transform44F tr = Transform44F::from_trs(Vec3F(0.0f, 0.0f, 0.0f), r, Vec3F(1.0f, 1.0f, 1.0f),
                                             TransformOrder_TRS, RotationOrder_XYZ);
    Vec3F angles;
    tr.decomp_tait_bryan(&angles, true, RotationOrder_XYZ);
    ASSERT(maths::equal_with_abs_error(angles.x, r.x, 1e-6f));
    ASSERT(maths::equal_with_abs_error(angles.y, r.y, 1e-6f));
    ASSERT(maths::equal_with_abs_error(angles.z, r.z, 1e-6f));
}

TEST_CASE(test_transform44_decomp_tait_bryan_degrees)
{
    Vec3F r_rad(0.5f, 0.3f, 0.7f);
    Transform44F tr = Transform44F::from_trs(Vec3F(0.0f, 0.0f, 0.0f), r_rad, Vec3F(1.0f, 1.0f, 1.0f),
                                             TransformOrder_TRS, RotationOrder_XYZ);
    Vec3F angles_deg;
    tr.decomp_tait_bryan(&angles_deg, false, RotationOrder_XYZ);
    ASSERT(maths::equal_with_abs_error(angles_deg.x, maths::rad2deg(r_rad.x), 1e-4f));
    ASSERT(maths::equal_with_abs_error(angles_deg.y, maths::rad2deg(r_rad.y), 1e-4f));
    ASSERT(maths::equal_with_abs_error(angles_deg.z, maths::rad2deg(r_rad.z), 1e-4f));
}

TEST_CASE(test_transform44_decomp_trs_partial)
{
    Vec3F t(1.0f, 2.0f, 3.0f);
    Vec3F r(0.5f, 0.0f, 0.0f);
    Vec3F s(2.0f, 2.0f, 2.0f);
    Transform44F tr = Transform44F::from_trs(t, r, s, TransformOrder_TRS, RotationOrder_XYZ);
    Vec3F t_out;
    tr.decomp_trs(&t_out, nullptr, nullptr);
    ASSERT(maths::equal_with_abs_error(t_out.x, t.x, 1e-6f));
    ASSERT(maths::equal_with_abs_error(t_out.y, t.y, 1e-6f));
    ASSERT(maths::equal_with_abs_error(t_out.z, t.z, 1e-6f));
}

TEST_CASE(test_transform44_double_precision)
{
    double angle = maths::constants<double>::pi / 3.0;
    Transform44D tr = Transform44D::from_rotz(angle);
    Vec3D point(1.0, 0.0, 0.0);
    Vec3D result = tr.transform_point(point);
    ASSERT(maths::equal_with_abs_error(result.x, 0.5, 1e-12));
    ASSERT(maths::equal_with_abs_error(result.y, maths::sqrt(3.0) / 2.0, 1e-12));
    ASSERT(maths::equal_with_abs_error(result.z, 0.0, 1e-12));
}

TEST_CASE(test_transform44_double_precision_trs_roundtrip)
{
    Vec3D t(1.5, -2.3, 4.7);
    Vec3D r(0.3, 0.5, 0.7);
    Vec3D s(1.0, 1.0, 1.0);
    Transform44D tr = Transform44D::from_trs(t, r, s, TransformOrder_TRS, RotationOrder_XYZ);
    Vec3D t_out, r_out, s_out;
    tr.decomp_trs(&t_out, &r_out, &s_out);
    ASSERT(maths::equal_with_abs_error(t_out.x, t.x, 1e-10));
    ASSERT(maths::equal_with_abs_error(t_out.y, t.y, 1e-10));
    ASSERT(maths::equal_with_abs_error(t_out.z, t.z, 1e-10));
    ASSERT(maths::equal_with_abs_error(r_out.x, r.x, 1e-10));
    ASSERT(maths::equal_with_abs_error(r_out.y, r.y, 1e-10));
    ASSERT(maths::equal_with_abs_error(r_out.z, r.z, 1e-10));
}

int main()
{
    TestRunner runner("linalg_transform");

    /* Transform33 tests (2D) */
    runner.add_test("T33 Default Constructor", test_transform33_default_constructor);
    runner.add_test("T33 Element Constructor", test_transform33_element_constructor);
    runner.add_test("T33 Copy Constructor", test_transform33_copy_constructor);
    runner.add_test("T33 Type Conversion", test_transform33_type_conversion_constructor);
    runner.add_test("T33 Zero", test_transform33_zero);
    runner.add_test("T33 Identity", test_transform33_identity);
    runner.add_test("T33 From Translation", test_transform33_from_translation);
    runner.add_test("T33 From Translation Homogeneous Row", test_transform33_from_translation_homogeneous_row);
    runner.add_test("T33 From Scale", test_transform33_from_scale);
    runner.add_test("T33 From Uniform Scale", test_transform33_from_uniform_scale);
    runner.add_test("T33 From Rotation 90", test_transform33_from_rotation_90);
    runner.add_test("T33 From Rotation 180", test_transform33_from_rotation_180);
    runner.add_test("T33 From Rotation 45", test_transform33_from_rotation_45);
    runner.add_test("T33 From Rotation Degrees", test_transform33_from_rotation_degrees);
    runner.add_test("T33 From Rotation Negative", test_transform33_from_rotation_negative);
    runner.add_test("T33 From TRS Basic", test_transform33_from_trs_basic);
    runner.add_test("T33 From TRS Non-Uniform Scale", test_transform33_from_trs_non_uniform_scale);
    runner.add_test("T33 From TRS SRT Order", test_transform33_from_trs_srt_order);
    runner.add_test("T33 From TRS Degrees", test_transform33_from_trs_degrees);
    runner.add_test("T33 From XYT", test_transform33_from_xyt);
    runner.add_test("T33 From XYT Rotated", test_transform33_from_xyt_rotated);
    runner.add_test("T33 From LookAt", test_transform33_from_lookat);
    runner.add_test("T33 From LookAt Diagonal", test_transform33_from_lookat_diagonal);
    runner.add_test("T33 Matrix Multiplication", test_transform33_matrix_multiplication);
    runner.add_test("T33 Identity Multiplication", test_transform33_identity_multiplication);
    runner.add_test("T33 Multiplication Associativity", test_transform33_multiplication_associativity);
    runner.add_test("T33 Transpose", test_transform33_transpose);
    runner.add_test("T33 Double Transpose", test_transform33_double_transpose);
    runner.add_test("T33 Trace", test_transform33_trace);
    runner.add_test("T33 Determinant Identity", test_transform33_determinant_identity);
    runner.add_test("T33 Determinant Scale", test_transform33_determinant_scale);
    runner.add_test("T33 Determinant Rotation", test_transform33_determinant_rotation);
    runner.add_test("T33 Inverse Identity", test_transform33_inverse_identity);
    runner.add_test("T33 Inverse Translation", test_transform33_inverse_translation);
    runner.add_test("T33 Inverse Rotation", test_transform33_inverse_rotation);
    runner.add_test("T33 Inverse Scale", test_transform33_inverse_scale);
    runner.add_test("T33 Inverse Composite", test_transform33_inverse_composite);
    runner.add_test("T33 Inverse Undoes Transform", test_transform33_inverse_undoes_transform);
    runner.add_test("T33 Transform Point Identity", test_transform33_transform_point_identity);
    runner.add_test("T33 Transform Point Translation", test_transform33_transform_point_translation);
    runner.add_test("T33 Transform Point Scale", test_transform33_transform_point_scale);
    runner.add_test("T33 Transform Point Combined", test_transform33_transform_point_combined);
    runner.add_test("T33 Transform Dir Ignores Translation", test_transform33_transform_dir_ignores_translation);
    runner.add_test("T33 Transform Dir Rotation", test_transform33_transform_dir_with_rotation);
    runner.add_test("T33 Transform Dir Scale", test_transform33_transform_dir_with_scale);
    runner.add_test("T33 Decomp Rotation Roundtrip", test_transform33_decomp_rotation_roundtrip);
    runner.add_test("T33 Decomp Rotation Negative", test_transform33_decomp_rotation_negative);
    runner.add_test("T33 Decomp Rotation Degrees", test_transform33_decomp_rotation_degrees);
    runner.add_test("T33 Decomp Rotation With Scale", test_transform33_decomp_rotation_with_scale);
    runner.add_test("T33 Decomp XYT Partial", test_transform33_decomp_xyt_partial);
    runner.add_test("T33 Decomp TRS Partial", test_transform33_decomp_trs_partial);
    runner.add_test("T33 Double Precision", test_transform33_double_precision);
    runner.add_test("T33 Double Precision TRS Roundtrip", test_transform33_double_precision_trs_roundtrip);

    /* Transform44 tests (original + improved) */
    runner.add_test("T44 Default Constructor", test_transform44_default_constructor);
    runner.add_test("T44 Element Constructor", test_transform44_element_constructor);
    runner.add_test("T44 Copy Constructor", test_transform44_copy_constructor);
    runner.add_test("T44 Type Conversion", test_transform44_type_conversion_constructor);
    runner.add_test("T44 Zero", test_transform44_zero);
    runner.add_test("T44 Identity", test_transform44_identity);
    runner.add_test("T44 From Translation", test_transform44_from_translation);
    runner.add_test("T44 From Scale", test_transform44_from_scale);
    runner.add_test("T44 From RotX", test_transform44_from_rot_x);
    runner.add_test("T44 From RotY", test_transform44_from_rot_y);
    runner.add_test("T44 From RotZ", test_transform44_from_rot_z);
    runner.add_test("T44 From RotX Degrees", test_transform44_from_rot_degrees);
    runner.add_test("T44 From RotY Degrees", test_transform44_from_rot_y_degrees);
    runner.add_test("T44 From RotZ Degrees", test_transform44_from_rot_z_degrees);
    runner.add_test("T44 From Axis Angle", test_transform44_from_axis_angle);
    runner.add_test("T44 Axis Angle vs RotX", test_transform44_axis_angle_vs_rotx);
    runner.add_test("T44 Axis Angle vs RotY", test_transform44_axis_angle_vs_roty);
    runner.add_test("T44 Axis Angle vs RotZ", test_transform44_axis_angle_vs_rotz);
    runner.add_test("T44 Axis Angle Degrees", test_transform44_axis_angle_degrees);
    runner.add_test("T44 From TRS", test_transform44_from_trs);
    // runner.add_test("T44 From TRS All Rotation Orders", test_transform44_from_trs_all_rotation_orders);
    // runner.add_test("T44 From TRS Non-Uniform Scale", test_transform44_from_trs_non_uniform_scale);
    runner.add_test("T44 From TRS SRT Order", test_transform44_from_trs_srt_order);
    runner.add_test("T44 From TRS Degrees", test_transform44_from_trs_degrees);
    runner.add_test("T44 From XYZT", test_transform44_from_xyzt);
    runner.add_test("T44 From XYZT Partial Decomp", test_transform44_from_xyzt_partial_decomp);
    runner.add_test("T44 From LookAt", test_transform44_from_lookat);
    runner.add_test("T44 From LookAt Off-Axis", test_transform44_from_lookat_off_axis);
    runner.add_test("T44 Matrix Multiplication", test_transform44_matrix_multiplication);
    runner.add_test("T44 Identity Multiplication", test_transform44_identity_multiplication);
    runner.add_test("T44 Multiplication Associativity", test_transform44_multiplication_associativity);
    runner.add_test("T44 Transpose", test_transform44_transpose);
    runner.add_test("T44 Double Transpose", test_transform44_double_transpose);
    runner.add_test("T44 Trace", test_transform44_trace);
    runner.add_test("T44 Trace Custom", test_transform44_trace_custom);
    runner.add_test("T44 Transform Point", test_transform44_transform_point);
    runner.add_test("T44 Transform Point with Rotation", test_transform44_transform_point_with_rotation);
    runner.add_test("T44 Transform Dir", test_transform44_transform_dir);
    runner.add_test("T44 Transform Dir Ignores Translation", test_transform44_transform_dir_ignores_translation);
    runner.add_test("T44 Decomp Tait-Bryan", test_transform44_decomp_tait_bryan);
    runner.add_test("T44 Decomp Tait-Bryan Degrees", test_transform44_decomp_tait_bryan_degrees);
    runner.add_test("T44 Decomp TRS Partial", test_transform44_decomp_trs_partial);
    runner.add_test("T44 Double Precision", test_transform44_double_precision);
    runner.add_test("T44 Double Precision TRS Roundtrip", test_transform44_double_precision_trs_roundtrip);

    runner.run_all();

    return 0;
}
