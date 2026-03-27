// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - Present Romain Augier
// All rights reserved.

#include "stdromano/linalg/vector.hpp"

#include "test.hpp"

#include <cmath>

using namespace stdromano;

/********************************/
/* Vector2 Tests */
/********************************/

TEST_CASE(test_vector2_default_constructor)
{
    Vec2F v;
    ASSERT_EQUAL(0.0f, v.x);
    ASSERT_EQUAL(0.0f, v.y);
}

TEST_CASE(test_vector2_single_value_constructor)
{
    Vec2F v(5.0f);
    ASSERT_EQUAL(5.0f, v.x);
    ASSERT_EQUAL(5.0f, v.y);
}

TEST_CASE(test_vector2_two_value_constructor)
{
    Vec2F v(3.0f, 4.0f);
    ASSERT_EQUAL(3.0f, v.x);
    ASSERT_EQUAL(4.0f, v.y);
}

TEST_CASE(test_vector2_copy_constructor)
{
    Vec2I original(10, 20);
    Vec2F copy(original);
    ASSERT_EQUAL(10.0f, copy.x);
    ASSERT_EQUAL(20.0f, copy.y);
}

TEST_CASE(test_vector2_assignment)
{
    Vec2I src(7, 8);
    Vec2F dst;
    dst = src;
    ASSERT_EQUAL(7.0f, dst.x);
    ASSERT_EQUAL(8.0f, dst.y);
}

TEST_CASE(test_vector2_unary_minus)
{
    Vec2F v(3.0f, -4.0f);
    Vec2F negated = -v;
    ASSERT_EQUAL(-3.0f, negated.x);
    ASSERT_EQUAL(4.0f, negated.y);
}

TEST_CASE(test_vector2_index_operator)
{
    Vec2F v(1.0f, 2.0f);
    ASSERT_EQUAL(1.0f, v[0]);
    ASSERT_EQUAL(2.0f, v[1]);

    v[0] = 10.0f;
    v[1] = 20.0f;
    ASSERT_EQUAL(10.0f, v.x);
    ASSERT_EQUAL(20.0f, v.y);
}

TEST_CASE(test_vector2_vector_addition)
{
    Vec2F a(1.0f, 2.0f);
    Vec2F b(3.0f, 4.0f);
    Vec2F result = a + b;
    ASSERT_EQUAL(4.0f, result.x);
    ASSERT_EQUAL(6.0f, result.y);
}

TEST_CASE(test_vector2_vector_subtraction)
{
    Vec2F a(5.0f, 7.0f);
    Vec2F b(2.0f, 3.0f);
    Vec2F result = a - b;
    ASSERT_EQUAL(3.0f, result.x);
    ASSERT_EQUAL(4.0f, result.y);
}

TEST_CASE(test_vector2_vector_multiplication)
{
    Vec2F a(2.0f, 3.0f);
    Vec2F b(4.0f, 5.0f);
    Vec2F result = a * b;
    ASSERT_EQUAL(8.0f, result.x);
    ASSERT_EQUAL(15.0f, result.y);
}

TEST_CASE(test_vector2_vector_division)
{
    Vec2F a(12.0f, 15.0f);
    Vec2F b(3.0f, 5.0f);
    Vec2F result = a / b;
    ASSERT_EQUAL(4.0f, result.x);
    ASSERT_EQUAL(3.0f, result.y);
}

TEST_CASE(test_vector2_scalar_operations)
{
    Vec2F v(2.0f, 3.0f);

    Vec2F add_result = v + 5.0f;
    ASSERT_EQUAL(7.0f, add_result.x);
    ASSERT_EQUAL(8.0f, add_result.y);

    Vec2F sub_result = v - 1.0f;
    ASSERT_EQUAL(1.0f, sub_result.x);
    ASSERT_EQUAL(2.0f, sub_result.y);

    Vec2F mul_result = v * 2.0f;
    ASSERT_EQUAL(4.0f, mul_result.x);
    ASSERT_EQUAL(6.0f, mul_result.y);

    Vec2F div_result = v / 2.0f;
    ASSERT_EQUAL(1.0f, div_result.x);
    ASSERT_EQUAL(1.5f, div_result.y);
}

TEST_CASE(test_vector2_compound_assignment)
{
    Vec2F v(2.0f, 3.0f);
    Vec2F other(1.0f, 2.0f);

    v += other;
    ASSERT_EQUAL(3.0f, v.x);
    ASSERT_EQUAL(5.0f, v.y);

    v -= other;
    ASSERT_EQUAL(2.0f, v.x);
    ASSERT_EQUAL(3.0f, v.y);

    v *= other;
    ASSERT_EQUAL(2.0f, v.x);
    ASSERT_EQUAL(6.0f, v.y);

    v /= Vec2F(2.0f, 3.0f);
    ASSERT_EQUAL(1.0f, v.x);
    ASSERT_EQUAL(2.0f, v.y);
}

TEST_CASE(test_vector2_scalar_compound_assignment)
{
    Vec2F v(4.0f, 6.0f);

    v += 2.0f;
    ASSERT_EQUAL(6.0f, v.x);
    ASSERT_EQUAL(8.0f, v.y);

    v -= 2.0f;
    ASSERT_EQUAL(4.0f, v.x);
    ASSERT_EQUAL(6.0f, v.y);

    v *= 2.0f;
    ASSERT_EQUAL(8.0f, v.x);
    ASSERT_EQUAL(12.0f, v.y);

    v /= 4.0f;
    ASSERT_EQUAL(2.0f, v.x);
    ASSERT_EQUAL(3.0f, v.y);
}

TEST_CASE(test_vector2_equality)
{
    Vec2F a(1.0f, 2.0f);
    Vec2F b(1.0f, 2.0f);
    Vec2F c(1.0f, 3.0f);

    ASSERT_EQUAL(true, a == b);
    ASSERT_EQUAL(false, a == c);
    ASSERT_EQUAL(false, a != b);
    ASSERT_EQUAL(true, a != c);
}

TEST_CASE(test_vector2_equal_with_abs_error)
{
    Vec2F a(1.0f, 2.0f);
    Vec2F b(1.001f, 2.001f);

    ASSERT_EQUAL(true, a.equal_with_abs_error(b, 0.01f));
    ASSERT_EQUAL(false, a.equal_with_abs_error(b, 0.0001f));
}

TEST_CASE(test_vector2_dot)
{
    Vec2F a(3.0f, 4.0f);
    Vec2F b(2.0f, 1.0f);
    float result = dot(a, b);
    ASSERT_EQUAL(10.0f, result); // 3*2 + 4*1 = 10
}

TEST_CASE(test_vector2_length)
{
    Vec2F v(3.0f, 4.0f);
    float len = length(v);
    float expected = 5.0f; // sqrt(3*3 + 4*4) = sqrt(25) = 5
    ASSERT(std::abs(len - expected) < 0.001f);
}

TEST_CASE(test_vector2_length2)
{
    Vec2F v(3.0f, 4.0f);
    float len2 = length2(v);
    ASSERT_EQUAL(25.0f, len2); // 3*3 + 4*4 = 25
}

TEST_CASE(test_vector2_normalize)
{
    Vec2F v(3.0f, 4.0f);
    Vec2F normalized = normalize(v);
    float len = length(normalized);
    ASSERT(std::abs(len - 1.0f) < 0.001f);
    ASSERT(std::abs(normalized.x - 0.6f) < 0.001f);
    ASSERT(std::abs(normalized.y - 0.8f) < 0.001f);
}

/********************************/
/* Vector3 Tests */
/********************************/

TEST_CASE(test_vector3_default_constructor)
{
    Vec3F v;
    ASSERT_EQUAL(0.0f, v.x);
    ASSERT_EQUAL(0.0f, v.y);
    ASSERT_EQUAL(0.0f, v.z);
}

TEST_CASE(test_vector3_single_value_constructor)
{
    Vec3F v(7.0f);
    ASSERT_EQUAL(7.0f, v.x);
    ASSERT_EQUAL(7.0f, v.y);
    ASSERT_EQUAL(7.0f, v.z);
}

TEST_CASE(test_vector3_three_value_constructor)
{
    Vec3F v(1.0f, 2.0f, 3.0f);
    ASSERT_EQUAL(1.0f, v.x);
    ASSERT_EQUAL(2.0f, v.y);
    ASSERT_EQUAL(3.0f, v.z);
}

TEST_CASE(test_vector3_vector_addition)
{
    Vec3F a(1.0f, 2.0f, 3.0f);
    Vec3F b(4.0f, 5.0f, 6.0f);
    Vec3F result = a + b;
    ASSERT_EQUAL(5.0f, result.x);
    ASSERT_EQUAL(7.0f, result.y);
    ASSERT_EQUAL(9.0f, result.z);
}

TEST_CASE(test_vector3_vector_subtraction)
{
    Vec3F a(10.0f, 8.0f, 6.0f);
    Vec3F b(3.0f, 2.0f, 1.0f);
    Vec3F result = a - b;
    ASSERT_EQUAL(7.0f, result.x);
    ASSERT_EQUAL(6.0f, result.y);
    ASSERT_EQUAL(5.0f, result.z);
}

TEST_CASE(test_vector3_scalar_multiplication)
{
    Vec3F v(2.0f, 3.0f, 4.0f);
    Vec3F result = v * 3.0f;
    ASSERT_EQUAL(6.0f, result.x);
    ASSERT_EQUAL(9.0f, result.y);
    ASSERT_EQUAL(12.0f, result.z);
}

TEST_CASE(test_vector3_index_operator)
{
    Vec3F v(1.0f, 2.0f, 3.0f);
    ASSERT_EQUAL(1.0f, v[0]);
    ASSERT_EQUAL(2.0f, v[1]);
    ASSERT_EQUAL(3.0f, v[2]);

    v[0] = 10.0f;
    v[1] = 20.0f;
    v[2] = 30.0f;
    ASSERT_EQUAL(10.0f, v.x);
    ASSERT_EQUAL(20.0f, v.y);
    ASSERT_EQUAL(30.0f, v.z);
}

TEST_CASE(test_vector3_dot)
{
    Vec3F a(1.0f, 2.0f, 3.0f);
    Vec3F b(4.0f, 5.0f, 6.0f);
    float result = dot(a, b);
    ASSERT_EQUAL(32.0f, result); // 1*4 + 2*5 + 3*6 = 4 + 10 + 18 = 32
}

TEST_CASE(test_vector3_cross)
{
    Vec3F a(1.0f, 0.0f, 0.0f);
    Vec3F b(0.0f, 1.0f, 0.0f);
    Vec3F result = cross(a, b);
    ASSERT_EQUAL(0.0f, result.x);
    ASSERT_EQUAL(0.0f, result.y);
    ASSERT_EQUAL(1.0f, result.z);

    // Test with arbitrary vectors
    Vec3F c(2.0f, 3.0f, 4.0f);
    Vec3F d(5.0f, 6.0f, 7.0f);
    Vec3F cross_result = cross(c, d);
    // Expected: (3*7 - 4*6, 4*5 - 2*7, 2*6 - 3*5) = (21-24, 20-14, 12-15) = (-3, 6, -3)
    ASSERT_EQUAL(-3.0f, cross_result.x);
    ASSERT_EQUAL(6.0f, cross_result.y);
    ASSERT_EQUAL(-3.0f, cross_result.z);
}

TEST_CASE(test_vector3_length)
{
    Vec3F v(3.0f, 4.0f, 0.0f);
    float len = length(v);
    ASSERT(std::abs(len - 5.0f) < 0.001f);

    Vec3F unit_vector(1.0f, 0.0f, 0.0f);
    float unit_len = length(unit_vector);
    ASSERT(std::abs(unit_len - 1.0f) < 0.001f);
}

TEST_CASE(test_vector3_normalize)
{
    Vec3F v(3.0f, 4.0f, 0.0f);
    Vec3F normalized = normalize(v);
    float len = length(normalized);
    ASSERT(std::abs(len - 1.0f) < 0.001f);
    ASSERT(std::abs(normalized.x - 0.6f) < 0.001f);
    ASSERT(std::abs(normalized.y - 0.8f) < 0.001f);
    ASSERT(std::abs(normalized.z - 0.0f) < 0.001f);
}

TEST_CASE(test_vector3_compound_assignment)
{
    Vec3F v(2.0f, 4.0f, 6.0f);
    Vec3F other(1.0f, 2.0f, 3.0f);

    v += other;
    ASSERT_EQUAL(3.0f, v.x);
    ASSERT_EQUAL(6.0f, v.y);
    ASSERT_EQUAL(9.0f, v.z);

    v -= other;
    ASSERT_EQUAL(2.0f, v.x);
    ASSERT_EQUAL(4.0f, v.y);
    ASSERT_EQUAL(6.0f, v.z);

    v *= other;
    ASSERT_EQUAL(2.0f, v.x);
    ASSERT_EQUAL(8.0f, v.y);
    ASSERT_EQUAL(18.0f, v.z);
}

/********************************/
/* Vector4 Tests */
/********************************/

TEST_CASE(test_vector4_default_constructor)
{
    Vec4F v;
    ASSERT_EQUAL(0.0f, v.x);
    ASSERT_EQUAL(0.0f, v.y);
    ASSERT_EQUAL(0.0f, v.z);
    ASSERT_EQUAL(0.0f, v.w);
}

TEST_CASE(test_vector4_four_value_constructor)
{
    Vec4F v(1.0f, 2.0f, 3.0f, 4.0f);
    ASSERT_EQUAL(1.0f, v.x);
    ASSERT_EQUAL(2.0f, v.y);
    ASSERT_EQUAL(3.0f, v.z);
    ASSERT_EQUAL(4.0f, v.w);
}

TEST_CASE(test_vector4_index_operator)
{
    Vec4F v(1.0f, 2.0f, 3.0f, 4.0f);
    ASSERT_EQUAL(1.0f, v[0]);
    ASSERT_EQUAL(2.0f, v[1]);
    ASSERT_EQUAL(3.0f, v[2]);
    ASSERT_EQUAL(4.0f, v[3]);

    v[3] = 10.0f;
    ASSERT_EQUAL(10.0f, v.w);
}

TEST_CASE(test_vector4_dot)
{
    Vec4F a(1.0f, 2.0f, 3.0f, 4.0f);
    Vec4F b(2.0f, 3.0f, 4.0f, 5.0f);
    float result = dot(a, b);
    ASSERT_EQUAL(40.0f, result); // 1*2 + 2*3 + 3*4 + 4*5 = 2 + 6 + 12 + 20 = 40
}

TEST_CASE(test_vector4_length)
{
    Vec4F v(1.0f, 2.0f, 2.0f, 0.0f);
    float len = length(v);
    float expected = 3.0f; // sqrt(1 + 4 + 4 + 0) = sqrt(9) = 3
    ASSERT(std::abs(len - expected) < 0.001f);
}

TEST_CASE(test_vector4_axis_angle)
{
    Vec4F v(1.0f, 0.0f, 0.0f, 3.14159f);
    auto [axis, angle] = v.as_axis_angle();
    ASSERT_EQUAL(1.0f, axis.x);
    ASSERT_EQUAL(0.0f, axis.y);
    ASSERT_EQUAL(0.0f, axis.z);
    ASSERT(std::abs(angle - 3.14159f) < 0.001f);
}

TEST_CASE(test_vector4_equality)
{
    Vec4F a(1.0f, 2.0f, 3.0f, 4.0f);
    Vec4F b(1.0f, 2.0f, 3.0f, 4.0f);
    Vec4F c(1.0f, 2.0f, 3.0f, 5.0f);

    ASSERT_EQUAL(true, a == b);
    ASSERT_EQUAL(false, a == c);
    ASSERT_EQUAL(false, a != b);
    ASSERT_EQUAL(true, a != c);
}

/********************************/
/* Edge Cases and Type Tests */
/********************************/

TEST_CASE(test_integer_vectors)
{
    Vec2I a(5, 3);
    Vec2I b(2, 7);
    Vec2I result = a + b;
    ASSERT_EQUAL(7, result.x);
    ASSERT_EQUAL(10, result.y);

    Vec3I c(10, 15, 20);
    Vec3I d(2, 3, 4);
    Vec3I div_result = c / d;
    ASSERT_EQUAL(5, div_result.x);
    ASSERT_EQUAL(5, div_result.y);
    ASSERT_EQUAL(5, div_result.z);
}

TEST_CASE(test_double_vectors)
{
    Vec3D a(1.5, 2.5, 3.5);
    Vec3D b(0.5, 1.5, 2.5);
    Vec3D result = a - b;
    ASSERT_EQUAL(1.0, result.x);
    ASSERT_EQUAL(1.0, result.y);
    ASSERT_EQUAL(1.0, result.z);
}

TEST_CASE(test_zero_vectors)
{
    Vec2F zero_vec;
    Vec2F other(1.0f, 2.0f);

    Vec2F add_result = zero_vec + other;
    ASSERT_EQUAL(other.x, add_result.x);
    ASSERT_EQUAL(other.y, add_result.y);

    Vec2F sub_result = other - zero_vec;
    ASSERT_EQUAL(other.x, sub_result.x);
    ASSERT_EQUAL(other.y, sub_result.y);
}

/********************************/
/* Performance and Constexpr Tests */
/********************************/

TEST_CASE(test_constexpr_evaluation)
{
    constexpr Vec2F a(3.0f, 4.0f);
    constexpr Vec2F b(1.0f, 2.0f);
    constexpr Vec2F sum = a + b;
    constexpr float dot_product = dot(a, b);
    constexpr float len_squared = length2(a);

    ASSERT_EQUAL(4.0f, sum.x);
    ASSERT_EQUAL(6.0f, sum.y);
    ASSERT_EQUAL(11.0f, dot_product);
    ASSERT_EQUAL(25.0f, len_squared);
}

int main()
{
    TestRunner runner("linalg_vector");

    runner.add_test("Vector2 Default Constructor", test_vector2_default_constructor);
    runner.add_test("Vector2 Single Value Constructor", test_vector2_single_value_constructor);
    runner.add_test("Vector2 Two Value Constructor", test_vector2_two_value_constructor);
    runner.add_test("Vector2 Copy Constructor", test_vector2_copy_constructor);
    runner.add_test("Vector2 Assignment", test_vector2_assignment);
    runner.add_test("Vector2 Unary Minus", test_vector2_unary_minus);
    runner.add_test("Vector2 Index Operator", test_vector2_index_operator);
    runner.add_test("Vector2 Vector Addition", test_vector2_vector_addition);
    runner.add_test("Vector2 Vector Subtraction", test_vector2_vector_subtraction);
    runner.add_test("Vector2 Vector Multiplication", test_vector2_vector_multiplication);
    runner.add_test("Vector2 Vector Division", test_vector2_vector_division);
    runner.add_test("Vector2 Scalar Operations", test_vector2_scalar_operations);
    runner.add_test("Vector2 Compound Assignment", test_vector2_compound_assignment);
    runner.add_test("Vector2 Scalar Compound Assignment", test_vector2_scalar_compound_assignment);
    runner.add_test("Vector2 Equality", test_vector2_equality);
    runner.add_test("Vector2 Equal With Abs Error", test_vector2_equal_with_abs_error);
    runner.add_test("Vector2 Dot Product", test_vector2_dot);
    runner.add_test("Vector2 Length", test_vector2_length);
    runner.add_test("Vector2 Length Squared", test_vector2_length2);
    runner.add_test("Vector2 Normalize", test_vector2_normalize);

    runner.add_test("Vector3 Default Constructor", test_vector3_default_constructor);
    runner.add_test("Vector3 Single Value Constructor", test_vector3_single_value_constructor);
    runner.add_test("Vector3 Three Value Constructor", test_vector3_three_value_constructor);
    runner.add_test("Vector3 Vector Addition", test_vector3_vector_addition);
    runner.add_test("Vector3 Vector Subtraction", test_vector3_vector_subtraction);
    runner.add_test("Vector3 Scalar Multiplication", test_vector3_scalar_multiplication);
    runner.add_test("Vector3 Index Operator", test_vector3_index_operator);
    runner.add_test("Vector3 Dot Product", test_vector3_dot);
    runner.add_test("Vector3 Cross Product", test_vector3_cross);
    runner.add_test("Vector3 Length", test_vector3_length);
    runner.add_test("Vector3 Normalize", test_vector3_normalize);
    runner.add_test("Vector3 Compound Assignment", test_vector3_compound_assignment);

    runner.add_test("Vector4 Default Constructor", test_vector4_default_constructor);
    runner.add_test("Vector4 Four Value Constructor", test_vector4_four_value_constructor);
    runner.add_test("Vector4 Index Operator", test_vector4_index_operator);
    runner.add_test("Vector4 Dot Product", test_vector4_dot);
    runner.add_test("Vector4 Length", test_vector4_length);
    runner.add_test("Vector4 As Axis Angle", test_vector4_axis_angle);
    runner.add_test("Vector4 Equality", test_vector4_equality);

    runner.add_test("Integer Vectors", test_integer_vectors);
    runner.add_test("Double Vectors", test_double_vectors);
    runner.add_test("Zero Vectors", test_zero_vectors);
    runner.add_test("Constexpr Evaluation", test_constexpr_evaluation);

    runner.run_all();

    return 0;
}
