// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - Present Romain Augier
// All rights reserved.

#include "stdromano/linalg/transform.hpp"
#include "stdromano/logger.hpp"

#include "test.hpp"

using namespace stdromano;

TEST_CASE(TestTransform44DefaultConstructor)
{
    Transform44F tr = Transform44F::ident();
    Transform44F ident = Transform44F::ident();
    ASSERT(tr.equal_with_abs_error(ident, 1e-6f));
}

TEST_CASE(TestTransform44ElementConstructor)
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

TEST_CASE(TestTransform44CopyConstructor)
{
    Transform44F src(1.0f, 2.0f, 3.0f, 4.0f,
                     5.0f, 6.0f, 7.0f, 8.0f,
                     9.0f, 10.0f, 11.0f, 12.0f,
                     13.0f, 14.0f, 15.0f, 16.0f);
    Transform44F dst(src);
    ASSERT(dst.equal_with_abs_error(src, 1e-6f));
}

TEST_CASE(TestTransform44Zero)
{
    Transform44F tr = Transform44F::zero();
    for (std::size_t i = 0; i < 16; ++i) {
        ASSERT_EQUAL(tr.data()[i], 0.0f);
    }
}

TEST_CASE(TestTransform44Identity)
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

TEST_CASE(TestTransform44FromTranslation)
{
    Vec3F t(1.0f, 2.0f, 3.0f);
    Transform44F tr = Transform44F::from_translation(t);
    Vec3F extracted;
    tr.decomp_translation(&extracted);
    ASSERT(maths::equal_with_abs_error(extracted.x, t.x, 1e-6f));
    ASSERT(maths::equal_with_abs_error(extracted.y, t.y, 1e-6f));
    ASSERT(maths::equal_with_abs_error(extracted.z, t.z, 1e-6f));
}

TEST_CASE(TestTransform44FromScale)
{
    Vec3F s(2.0f, 3.0f, 4.0f);
    Transform44F tr = Transform44F::from_scale(s);
    Vec3F extracted;
    tr.decomp_scale(&extracted);
    ASSERT(maths::equal_with_abs_error(extracted.x, s.x, 1e-6f));
    ASSERT(maths::equal_with_abs_error(extracted.y, s.y, 1e-6f));
    ASSERT(maths::equal_with_abs_error(extracted.z, s.z, 1e-6f));
}

TEST_CASE(TestTransform44FromRotX)
{
    float angle = maths::constants<float>::pi / 2.0f; // 90 degrees
    Transform44F tr = Transform44F::from_rotx(angle);
    Vec3F test_point(0.0f, 1.0f, 0.0f);
    Vec3F transformed = tr.transform_point(test_point);
    ASSERT(maths::equal_with_abs_error(transformed.x, 0.0f, 1e-6f));
    ASSERT(maths::equal_with_abs_error(transformed.y, 0.0f, 1e-6f));
    ASSERT(maths::equal_with_abs_error(transformed.z, 1.0f, 1e-6f));
}

TEST_CASE(TestTransform44FromRotY)
{
    float angle = maths::constants<float>::pi / 2.0f; // 90 degrees
    Transform44F tr = Transform44F::from_roty(angle);
    Vec3F test_point(1.0f, 0.0f, 0.0f);
    Vec3F transformed = tr.transform_point(test_point);
    ASSERT(maths::equal_with_abs_error(transformed.x, 0.0f, 1e-6f));
    ASSERT(maths::equal_with_abs_error(transformed.y, 0.0f, 1e-6f));
    ASSERT(maths::equal_with_abs_error(transformed.z, -1.0f, 1e-6f));
}

TEST_CASE(TestTransform44FromRotZ)
{
    float angle = maths::constants<float>::pi / 2.0f; // 90 degrees
    Transform44F tr = Transform44F::from_rotz(angle);
    Vec3F test_point(1.0f, 0.0f, 0.0f);
    Vec3F transformed = tr.transform_point(test_point);
    ASSERT(maths::equal_with_abs_error(transformed.x, 0.0f, 1e-6f));
    ASSERT(maths::equal_with_abs_error(transformed.y, 1.0f, 1e-6f));
    ASSERT(maths::equal_with_abs_error(transformed.z, 0.0f, 1e-6f));
}

TEST_CASE(TestTransform44FromAxisAngle)
{
    Vec3F axis(0.0f, 1.0f, 0.0f);
    float angle = maths::constants<float>::pi / 2.0f; // 90 degrees
    Transform44F tr = Transform44F::from_axis_angle(axis, angle);
    Vec3F test_point(1.0f, 0.0f, 0.0f);
    Vec3F transformed = tr.transform_point(test_point);
    ASSERT(maths::equal_with_abs_error(transformed.x, 0.0f, 1e-6f));
    ASSERT(maths::equal_with_abs_error(transformed.y, 0.0f, 1e-6f));
    ASSERT(maths::equal_with_abs_error(transformed.z, -1.0f, 1e-6f));
}

TEST_CASE(TestTransform44FromTRS)
{
    Vec3F t(1.0f, 2.0f, 3.0f);
    Vec3F r(maths::constants<float>::pi / 4.0f, 0.0f, 0.0f); // 45 degrees around X
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

TEST_CASE(TestTransform44FromXYZT)
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

TEST_CASE(TestTransform44FromLookAt)
{
    Vec3F eye(0.0f, 0.0f, 5.0f);
    Vec3F target(0.0f, 0.0f, 0.0f);
    Vec3F up(0.0f, 1.0f, 0.0f);
    Transform44F tr = Transform44F::from_lookat(eye, target, up);
    Vec3F z_axis(0.0f, 0.0f, -1.0f); // Normalized direction from eye to target
    Vec3F x_axis, y_axis, z_extracted, t_extracted;
    tr.decomp_xyzt(&x_axis, &y_axis, &z_extracted, &t_extracted);
    ASSERT(maths::equal_with_abs_error(z_extracted.x, z_axis.x, 1e-6f));
    ASSERT(maths::equal_with_abs_error(z_extracted.y, z_axis.y, 1e-6f));
    ASSERT(maths::equal_with_abs_error(z_extracted.z, z_axis.z, 1e-6f));
    ASSERT(maths::equal_with_abs_error(t_extracted.x, eye.x, 1e-6f));
    ASSERT(maths::equal_with_abs_error(t_extracted.y, eye.y, 1e-6f));
    ASSERT(maths::equal_with_abs_error(t_extracted.z, eye.z, 1e-6f));
}

TEST_CASE(TestTransform44MatrixMultiplication)
{
    Transform44F tr1 = Transform44F::from_translation(Vec3F(1.0f, 2.0f, 3.0f));
    Transform44F tr2 = Transform44F::from_scale(Vec3F(2.0f, 2.0f, 2.0f));
    Transform44F result = tr1 * tr2;
    Vec3F point(1.0f, 1.0f, 1.0f);
    Vec3F transformed = result.transform_point(point);
    // Expected: Scale first (2,2,2), then translate (1,2,3) => (2*1+1, 2*1+2, 2*1+3) = (3,4,5)
    ASSERT(maths::equal_with_abs_error(transformed.x, 3.0f, 1e-6f));
    ASSERT(maths::equal_with_abs_error(transformed.y, 4.0f, 1e-6f));
    ASSERT(maths::equal_with_abs_error(transformed.z, 5.0f, 1e-6f));
}

TEST_CASE(TestTransform44Transpose)
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

TEST_CASE(TestTransform44Trace)
{
    Transform44F tr = Transform44F::ident();
    float trace = tr.trace();
    ASSERT_EQUAL(trace, 4.0f); // 1+1+1+1
}

TEST_CASE(TestTransform44TransformPoint)
{
    Transform44F tr = Transform44F::from_translation(Vec3F(1.0f, 2.0f, 3.0f));
    Vec3F point(1.0f, 1.0f, 1.0f);
    Vec3F transformed = tr.transform_point(point);
    ASSERT(maths::equal_with_abs_error(transformed.x, 2.0f, 1e-6f));
    ASSERT(maths::equal_with_abs_error(transformed.y, 3.0f, 1e-6f));
    ASSERT(maths::equal_with_abs_error(transformed.z, 4.0f, 1e-6f));
}

TEST_CASE(TestTransform44TransformDir)
{
    Transform44F tr = Transform44F::from_scale(Vec3F(2.0f, 3.0f, 4.0f));
    Vec3F dir(1.0f, 1.0f, 1.0f);
    Vec3F transformed = tr.transform_dir(dir);
    ASSERT(maths::equal_with_abs_error(transformed.x, 2.0f, 1e-6f));
    ASSERT(maths::equal_with_abs_error(transformed.y, 3.0f, 1e-6f));
    ASSERT(maths::equal_with_abs_error(transformed.z, 4.0f, 1e-6f));
}

TEST_CASE(TestTransform44DecompTaitBryan)
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

int main()
{
    stdromano::set_log_level(stdromano::LogLevel::Debug);
    TestRunner runner;

    runner.add_test("Default Constructor", TestTransform44DefaultConstructor);
    runner.add_test("Element Constructor", TestTransform44ElementConstructor);
    runner.add_test("Copy Constructor", TestTransform44CopyConstructor);
    runner.add_test("Zero", TestTransform44Zero);
    runner.add_test("Identity", TestTransform44Identity);
    runner.add_test("From Translation", TestTransform44FromTranslation);
    runner.add_test("From Scale", TestTransform44FromScale);
    runner.add_test("From RotX", TestTransform44FromRotX);
    runner.add_test("From RotY", TestTransform44FromRotY);
    runner.add_test("From RotZ", TestTransform44FromRotZ);
    runner.add_test("From Axis Angle", TestTransform44FromAxisAngle);
    runner.add_test("From TRS", TestTransform44FromTRS);
    runner.add_test("From XYZT", TestTransform44FromXYZT);
    runner.add_test("From LookAt", TestTransform44FromLookAt);
    runner.add_test("Matrix Multiplication", TestTransform44MatrixMultiplication);
    runner.add_test("Transpose", TestTransform44Transpose);
    runner.add_test("Trace", TestTransform44Trace);
    runner.add_test("Transform Point", TestTransform44TransformPoint);
    runner.add_test("Transform Dir", TestTransform44TransformDir);
    runner.add_test("Decomp Tait-Bryan", TestTransform44DecompTaitBryan);

    runner.run_all();

    return 0;
}