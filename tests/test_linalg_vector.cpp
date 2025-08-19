// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - Present Romain Augier
// All rights reserved.

#include "stdromano/linalg/vector.hpp"
#include "stdromano/logger.hpp"

#include "test.hpp"

#include <cmath>

using namespace stdromano;

/********************************/
/* Vector2 Tests */
/********************************/

TEST_CASE(TestVector2DefaultConstructor)
{
    Vec2F v;
    ASSERT_EQUAL(0.0f, v.x);
    ASSERT_EQUAL(0.0f, v.y);
}

TEST_CASE(TestVector2SingleValueConstructor)
{
    Vec2F v(5.0f);
    ASSERT_EQUAL(5.0f, v.x);
    ASSERT_EQUAL(5.0f, v.y);
}

TEST_CASE(TestVector2TwoValueConstructor)
{
    Vec2F v(3.0f, 4.0f);
    ASSERT_EQUAL(3.0f, v.x);
    ASSERT_EQUAL(4.0f, v.y);
}

TEST_CASE(TestVector2CopyConstructor)
{
    Vec2I original(10, 20);
    Vec2F copy(original);
    ASSERT_EQUAL(10.0f, copy.x);
    ASSERT_EQUAL(20.0f, copy.y);
}

TEST_CASE(TestVector2Assignment)
{
    Vec2I src(7, 8);
    Vec2F dst;
    dst = src;
    ASSERT_EQUAL(7.0f, dst.x);
    ASSERT_EQUAL(8.0f, dst.y);
}

TEST_CASE(TestVector2UnaryMinus)
{
    Vec2F v(3.0f, -4.0f);
    Vec2F negated = -v;
    ASSERT_EQUAL(-3.0f, negated.x);
    ASSERT_EQUAL(4.0f, negated.y);
}

TEST_CASE(TestVector2IndexOperator)
{
    Vec2F v(1.0f, 2.0f);
    ASSERT_EQUAL(1.0f, v[0]);
    ASSERT_EQUAL(2.0f, v[1]);
    
    v[0] = 10.0f;
    v[1] = 20.0f;
    ASSERT_EQUAL(10.0f, v.x);
    ASSERT_EQUAL(20.0f, v.y);
}

TEST_CASE(TestVector2VectorAddition)
{
    Vec2F a(1.0f, 2.0f);
    Vec2F b(3.0f, 4.0f);
    Vec2F result = a + b;
    ASSERT_EQUAL(4.0f, result.x);
    ASSERT_EQUAL(6.0f, result.y);
}

TEST_CASE(TestVector2VectorSubtraction)
{
    Vec2F a(5.0f, 7.0f);
    Vec2F b(2.0f, 3.0f);
    Vec2F result = a - b;
    ASSERT_EQUAL(3.0f, result.x);
    ASSERT_EQUAL(4.0f, result.y);
}

TEST_CASE(TestVector2VectorMultiplication)
{
    Vec2F a(2.0f, 3.0f);
    Vec2F b(4.0f, 5.0f);
    Vec2F result = a * b;
    ASSERT_EQUAL(8.0f, result.x);
    ASSERT_EQUAL(15.0f, result.y);
}

TEST_CASE(TestVector2VectorDivision)
{
    Vec2F a(12.0f, 15.0f);
    Vec2F b(3.0f, 5.0f);
    Vec2F result = a / b;
    ASSERT_EQUAL(4.0f, result.x);
    ASSERT_EQUAL(3.0f, result.y);
}

TEST_CASE(TestVector2ScalarOperations)
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

TEST_CASE(TestVector2CompoundAssignment)
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

TEST_CASE(TestVector2ScalarCompoundAssignment)
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

TEST_CASE(TestVector2Equality)
{
    Vec2F a(1.0f, 2.0f);
    Vec2F b(1.0f, 2.0f);
    Vec2F c(1.0f, 3.0f);
    
    ASSERT_EQUAL(true, a == b);
    ASSERT_EQUAL(false, a == c);
    ASSERT_EQUAL(false, a != b);
    ASSERT_EQUAL(true, a != c);
}

TEST_CASE(TestVector2EqualWithAbsError)
{
    Vec2F a(1.0f, 2.0f);
    Vec2F b(1.001f, 2.001f);
    
    ASSERT_EQUAL(true, a.equal_with_abs_error(b, 0.01f));
    ASSERT_EQUAL(false, a.equal_with_abs_error(b, 0.0001f));
}

TEST_CASE(TestVector2Dot)
{
    Vec2F a(3.0f, 4.0f);
    Vec2F b(2.0f, 1.0f);
    float result = dot(a, b);
    ASSERT_EQUAL(10.0f, result); // 3*2 + 4*1 = 10
}

TEST_CASE(TestVector2Length)
{
    Vec2F v(3.0f, 4.0f);
    float len = length(v);
    float expected = 5.0f; // sqrt(3*3 + 4*4) = sqrt(25) = 5
    ASSERT(std::abs(len - expected) < 0.001f);
}

TEST_CASE(TestVector2Length2)
{
    Vec2F v(3.0f, 4.0f);
    float len2 = length2(v);
    ASSERT_EQUAL(25.0f, len2); // 3*3 + 4*4 = 25
}

TEST_CASE(TestVector2Normalize)
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

TEST_CASE(TestVector3DefaultConstructor)
{
    Vec3F v;
    ASSERT_EQUAL(0.0f, v.x);
    ASSERT_EQUAL(0.0f, v.y);
    ASSERT_EQUAL(0.0f, v.z);
}

TEST_CASE(TestVector3SingleValueConstructor)
{
    Vec3F v(7.0f);
    ASSERT_EQUAL(7.0f, v.x);
    ASSERT_EQUAL(7.0f, v.y);
    ASSERT_EQUAL(7.0f, v.z);
}

TEST_CASE(TestVector3ThreeValueConstructor)
{
    Vec3F v(1.0f, 2.0f, 3.0f);
    ASSERT_EQUAL(1.0f, v.x);
    ASSERT_EQUAL(2.0f, v.y);
    ASSERT_EQUAL(3.0f, v.z);
}

TEST_CASE(TestVector3VectorAddition)
{
    Vec3F a(1.0f, 2.0f, 3.0f);
    Vec3F b(4.0f, 5.0f, 6.0f);
    Vec3F result = a + b;
    ASSERT_EQUAL(5.0f, result.x);
    ASSERT_EQUAL(7.0f, result.y);
    ASSERT_EQUAL(9.0f, result.z);
}

TEST_CASE(TestVector3VectorSubtraction)
{
    Vec3F a(10.0f, 8.0f, 6.0f);
    Vec3F b(3.0f, 2.0f, 1.0f);
    Vec3F result = a - b;
    ASSERT_EQUAL(7.0f, result.x);
    ASSERT_EQUAL(6.0f, result.y);
    ASSERT_EQUAL(5.0f, result.z);
}

TEST_CASE(TestVector3ScalarMultiplication)
{
    Vec3F v(2.0f, 3.0f, 4.0f);
    Vec3F result = v * 3.0f;
    ASSERT_EQUAL(6.0f, result.x);
    ASSERT_EQUAL(9.0f, result.y);
    ASSERT_EQUAL(12.0f, result.z);
}

TEST_CASE(TestVector3IndexOperator)
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

TEST_CASE(TestVector3Dot)
{
    Vec3F a(1.0f, 2.0f, 3.0f);
    Vec3F b(4.0f, 5.0f, 6.0f);
    float result = dot(a, b);
    ASSERT_EQUAL(32.0f, result); // 1*4 + 2*5 + 3*6 = 4 + 10 + 18 = 32
}

TEST_CASE(TestVector3Cross)
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

TEST_CASE(TestVector3Length)
{
    Vec3F v(3.0f, 4.0f, 0.0f);
    float len = length(v);
    ASSERT(std::abs(len - 5.0f) < 0.001f);
    
    Vec3F unit_vector(1.0f, 0.0f, 0.0f);
    float unit_len = length(unit_vector);
    ASSERT(std::abs(unit_len - 1.0f) < 0.001f);
}

TEST_CASE(TestVector3Normalize)
{
    Vec3F v(3.0f, 4.0f, 0.0f);
    Vec3F normalized = normalize(v);
    float len = length(normalized);
    ASSERT(std::abs(len - 1.0f) < 0.001f);
    ASSERT(std::abs(normalized.x - 0.6f) < 0.001f);
    ASSERT(std::abs(normalized.y - 0.8f) < 0.001f);
    ASSERT(std::abs(normalized.z - 0.0f) < 0.001f);
}

TEST_CASE(TestVector3CompoundAssignment)
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

TEST_CASE(TestVector4DefaultConstructor)
{
    Vec4F v;
    ASSERT_EQUAL(0.0f, v.x);
    ASSERT_EQUAL(0.0f, v.y);
    ASSERT_EQUAL(0.0f, v.z);
    ASSERT_EQUAL(0.0f, v.w);
}

TEST_CASE(TestVector4FourValueConstructor)
{
    Vec4F v(1.0f, 2.0f, 3.0f, 4.0f);
    ASSERT_EQUAL(1.0f, v.x);
    ASSERT_EQUAL(2.0f, v.y);
    ASSERT_EQUAL(3.0f, v.z);
    ASSERT_EQUAL(4.0f, v.w);
}

TEST_CASE(TestVector4IndexOperator)
{
    Vec4F v(1.0f, 2.0f, 3.0f, 4.0f);
    ASSERT_EQUAL(1.0f, v[0]);
    ASSERT_EQUAL(2.0f, v[1]);
    ASSERT_EQUAL(3.0f, v[2]);
    ASSERT_EQUAL(4.0f, v[3]);
    
    v[3] = 10.0f;
    ASSERT_EQUAL(10.0f, v.w);
}

TEST_CASE(TestVector4Dot)
{
    Vec4F a(1.0f, 2.0f, 3.0f, 4.0f);
    Vec4F b(2.0f, 3.0f, 4.0f, 5.0f);
    float result = dot(a, b);
    ASSERT_EQUAL(40.0f, result); // 1*2 + 2*3 + 3*4 + 4*5 = 2 + 6 + 12 + 20 = 40
}

TEST_CASE(TestVector4Length)
{
    Vec4F v(1.0f, 2.0f, 2.0f, 0.0f);
    float len = length(v);
    float expected = 3.0f; // sqrt(1 + 4 + 4 + 0) = sqrt(9) = 3
    ASSERT(std::abs(len - expected) < 0.001f);
}

TEST_CASE(TestVector4AsAxisAngle)
{
    Vec4F v(1.0f, 0.0f, 0.0f, 3.14159f);
    auto [axis, angle] = v.as_axis_angle();
    ASSERT_EQUAL(1.0f, axis.x);
    ASSERT_EQUAL(0.0f, axis.y);
    ASSERT_EQUAL(0.0f, axis.z);
    ASSERT(std::abs(angle - 3.14159f) < 0.001f);
}

TEST_CASE(TestVector4Equality)
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

TEST_CASE(TestIntegerVectors)
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

TEST_CASE(TestDoubleVectors)
{
    Vec3D a(1.5, 2.5, 3.5);
    Vec3D b(0.5, 1.5, 2.5);
    Vec3D result = a - b;
    ASSERT_EQUAL(1.0, result.x);
    ASSERT_EQUAL(1.0, result.y);
    ASSERT_EQUAL(1.0, result.z);
}

TEST_CASE(TestZeroVectors)
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

TEST_CASE(TestConstexprEvaluation)
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
    stdromano::set_log_level(stdromano::LogLevel::Debug);

    TestRunner runner;
    
    runner.add_test("Vector2 Default Constructor", TestVector2DefaultConstructor);
    runner.add_test("Vector2 Single Value Constructor", TestVector2SingleValueConstructor);
    runner.add_test("Vector2 Two Value Constructor", TestVector2TwoValueConstructor);
    runner.add_test("Vector2 Copy Constructor", TestVector2CopyConstructor);
    runner.add_test("Vector2 Assignment", TestVector2Assignment);
    runner.add_test("Vector2 Unary Minus", TestVector2UnaryMinus);
    runner.add_test("Vector2 Index Operator", TestVector2IndexOperator);
    runner.add_test("Vector2 Vector Addition", TestVector2VectorAddition);
    runner.add_test("Vector2 Vector Subtraction", TestVector2VectorSubtraction);
    runner.add_test("Vector2 Vector Multiplication", TestVector2VectorMultiplication);
    runner.add_test("Vector2 Vector Division", TestVector2VectorDivision);
    runner.add_test("Vector2 Scalar Operations", TestVector2ScalarOperations);
    runner.add_test("Vector2 Compound Assignment", TestVector2CompoundAssignment);
    runner.add_test("Vector2 Scalar Compound Assignment", TestVector2ScalarCompoundAssignment);
    runner.add_test("Vector2 Equality", TestVector2Equality);
    runner.add_test("Vector2 Equal With Abs Error", TestVector2EqualWithAbsError);
    runner.add_test("Vector2 Dot Product", TestVector2Dot);
    runner.add_test("Vector2 Length", TestVector2Length);
    runner.add_test("Vector2 Length Squared", TestVector2Length2);
    runner.add_test("Vector2 Normalize", TestVector2Normalize);
    
    runner.add_test("Vector3 Default Constructor", TestVector3DefaultConstructor);
    runner.add_test("Vector3 Single Value Constructor", TestVector3SingleValueConstructor);
    runner.add_test("Vector3 Three Value Constructor", TestVector3ThreeValueConstructor);
    runner.add_test("Vector3 Vector Addition", TestVector3VectorAddition);
    runner.add_test("Vector3 Vector Subtraction", TestVector3VectorSubtraction);
    runner.add_test("Vector3 Scalar Multiplication", TestVector3ScalarMultiplication);
    runner.add_test("Vector3 Index Operator", TestVector3IndexOperator);
    runner.add_test("Vector3 Dot Product", TestVector3Dot);
    runner.add_test("Vector3 Cross Product", TestVector3Cross);
    runner.add_test("Vector3 Length", TestVector3Length);
    runner.add_test("Vector3 Normalize", TestVector3Normalize);
    runner.add_test("Vector3 Compound Assignment", TestVector3CompoundAssignment);
    
    runner.add_test("Vector4 Default Constructor", TestVector4DefaultConstructor);
    runner.add_test("Vector4 Four Value Constructor", TestVector4FourValueConstructor);
    runner.add_test("Vector4 Index Operator", TestVector4IndexOperator);
    runner.add_test("Vector4 Dot Product", TestVector4Dot);
    runner.add_test("Vector4 Length", TestVector4Length);
    runner.add_test("Vector4 As Axis Angle", TestVector4AsAxisAngle);
    runner.add_test("Vector4 Equality", TestVector4Equality);
    
    runner.add_test("Integer Vectors", TestIntegerVectors);
    runner.add_test("Double Vectors", TestDoubleVectors);
    runner.add_test("Zero Vectors", TestZeroVectors);
    runner.add_test("Constexpr Evaluation", TestConstexprEvaluation);
    
    runner.run_all();

    return 0;
}