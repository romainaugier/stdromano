// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - Present Romain Augier
// All rights reserved.

#include "stdromano/linalg/vector.hpp"

#include "fixtures.hpp"

#include <algorithm>
#include <cmath>

using namespace stdromano;

STDROMANO_TEST_CASE(vector2_default_constructor)
{
    Vec2F v;
    STDROMANO_CHECK_EQ(v.x, 0.0f);
    STDROMANO_CHECK_EQ(v.y, 0.0f);
}

STDROMANO_TEST_CASE(vector2_single_value_constructor)
{
    Vec2F v(5.0f);
    STDROMANO_CHECK_EQ(v.x, 5.0f);
    STDROMANO_CHECK_EQ(v.y, 5.0f);
}

STDROMANO_TEST_CASE(vector2_two_value_constructor)
{
    Vec2F v(3.0f, 4.0f);
    STDROMANO_CHECK_EQ(v.x, 3.0f);
    STDROMANO_CHECK_EQ(v.y, 4.0f);
}

STDROMANO_TEST_CASE(vector2_copy_constructor)
{
    Vec2I original(10, 20);
    Vec2F copy(original);
    STDROMANO_CHECK_EQ(copy.x, 10.0f);
    STDROMANO_CHECK_EQ(copy.y, 20.0f);
}

STDROMANO_TEST_CASE(vector2_assignment)
{
    Vec2I src(7, 8);
    Vec2F dst;
    dst = src;
    STDROMANO_CHECK_EQ(dst.x, 7.0f);
    STDROMANO_CHECK_EQ(dst.y, 8.0f);
}

STDROMANO_TEST_CASE(vector2_unary_minus)
{
    Vec2F v(3.0f, -4.0f);
    Vec2F negated = -v;
    STDROMANO_CHECK_EQ(negated.x, -3.0f);
    STDROMANO_CHECK_EQ(negated.y, 4.0f);
}

STDROMANO_TEST_CASE(vector2_index_operator)
{
    Vec2F v(1.0f, 2.0f);
    STDROMANO_CHECK_EQ(v[0], 1.0f);
    STDROMANO_CHECK_EQ(v[1], 2.0f);

    v[0] = 10.0f;
    v[1] = 20.0f;
    STDROMANO_CHECK_EQ(v.x, 10.0f);
    STDROMANO_CHECK_EQ(v.y, 20.0f);
}

STDROMANO_TEST_CASE(vector2_vector_addition)
{
    Vec2F a(1.0f, 2.0f);
    Vec2F b(3.0f, 4.0f);
    Vec2F result = a + b;
    STDROMANO_CHECK_EQ(result.x, 4.0f);
    STDROMANO_CHECK_EQ(result.y, 6.0f);
}

STDROMANO_TEST_CASE(vector2_vector_subtraction)
{
    Vec2F a(5.0f, 7.0f);
    Vec2F b(2.0f, 3.0f);
    Vec2F result = a - b;
    STDROMANO_CHECK_EQ(result.x, 3.0f);
    STDROMANO_CHECK_EQ(result.y, 4.0f);
}

STDROMANO_TEST_CASE(vector2_vector_multiplication)
{
    Vec2F a(2.0f, 3.0f);
    Vec2F b(4.0f, 5.0f);
    Vec2F result = a * b;
    STDROMANO_CHECK_EQ(result.x, 8.0f);
    STDROMANO_CHECK_EQ(result.y, 15.0f);
}

STDROMANO_TEST_CASE(vector2_vector_division)
{
    Vec2F a(12.0f, 15.0f);
    Vec2F b(3.0f, 5.0f);
    Vec2F result = a / b;
    STDROMANO_CHECK_EQ(result.x, 4.0f);
    STDROMANO_CHECK_EQ(result.y, 3.0f);
}

STDROMANO_TEST_CASE(vector2_scalar_operations)
{
    Vec2F v(2.0f, 3.0f);

    Vec2F add_result = v + 5.0f;
    STDROMANO_CHECK_EQ(add_result.x, 7.0f);
    STDROMANO_CHECK_EQ(add_result.y, 8.0f);

    Vec2F sub_result = v - 1.0f;
    STDROMANO_CHECK_EQ(sub_result.x, 1.0f);
    STDROMANO_CHECK_EQ(sub_result.y, 2.0f);

    Vec2F mul_result = v * 2.0f;
    STDROMANO_CHECK_EQ(mul_result.x, 4.0f);
    STDROMANO_CHECK_EQ(mul_result.y, 6.0f);

    Vec2F div_result = v / 2.0f;
    STDROMANO_CHECK_EQ(div_result.x, 1.0f);
    STDROMANO_CHECK_EQ(div_result.y, 1.5f);
}

STDROMANO_TEST_CASE(vector2_compound_assignment)
{
    Vec2F v(2.0f, 3.0f);
    Vec2F other(1.0f, 2.0f);

    v += other;
    STDROMANO_CHECK_EQ(v.x, 3.0f);
    STDROMANO_CHECK_EQ(v.y, 5.0f);

    v -= other;
    STDROMANO_CHECK_EQ(v.x, 2.0f);
    STDROMANO_CHECK_EQ(v.y, 3.0f);

    v *= other;
    STDROMANO_CHECK_EQ(v.x, 2.0f);
    STDROMANO_CHECK_EQ(v.y, 6.0f);

    v /= Vec2F(2.0f, 3.0f);
    STDROMANO_CHECK_EQ(v.x, 1.0f);
    STDROMANO_CHECK_EQ(v.y, 2.0f);
}

STDROMANO_TEST_CASE(vector2_scalar_compound_assignment)
{
    Vec2F v(4.0f, 6.0f);

    v += 2.0f;
    STDROMANO_CHECK_EQ(v.x, 6.0f);
    STDROMANO_CHECK_EQ(v.y, 8.0f);

    v -= 2.0f;
    STDROMANO_CHECK_EQ(v.x, 4.0f);
    STDROMANO_CHECK_EQ(v.y, 6.0f);

    v *= 2.0f;
    STDROMANO_CHECK_EQ(v.x, 8.0f);
    STDROMANO_CHECK_EQ(v.y, 12.0f);

    v /= 4.0f;
    STDROMANO_CHECK_EQ(v.x, 2.0f);
    STDROMANO_CHECK_EQ(v.y, 3.0f);
}

STDROMANO_TEST_CASE(vector2_equality)
{
    Vec2F a(1.0f, 2.0f);
    Vec2F b(1.0f, 2.0f);
    Vec2F c(1.0f, 3.0f);

    STDROMANO_CHECK(a == b);
    STDROMANO_CHECK(!(a == c));
    STDROMANO_CHECK(!(a != b));
    STDROMANO_CHECK(a != c);
}

STDROMANO_TEST_CASE(vector2_equal_with_abs_error)
{
    Vec2F a(1.0f, 2.0f);
    Vec2F b(1.001f, 2.001f);

    STDROMANO_CHECK(a.equal_with_abs_error(b, 0.01f));
    STDROMANO_CHECK(!(a.equal_with_abs_error(b, 0.0001f)));
}

STDROMANO_TEST_CASE(vector2_dot)
{
    Vec2F a(3.0f, 4.0f);
    Vec2F b(2.0f, 1.0f);
    float result = dot(a, b);
    STDROMANO_CHECK_EQ(result, 10.0f);
}

STDROMANO_TEST_CASE(vector2_length)
{
    Vec2F v(3.0f, 4.0f);
    float len = length(v);
    float expected = 5.0f;
    STDROMANO_CHECK_NEAR(len, expected, 0.001f);
}

STDROMANO_TEST_CASE(vector2_length2)
{
    Vec2F v(3.0f, 4.0f);
    float len2 = length2(v);
    STDROMANO_CHECK_EQ(len2, 25.0f);
}

STDROMANO_TEST_CASE(vector2_normalize)
{
    Vec2F v(3.0f, 4.0f);
    Vec2F normalized = normalize(v);
    float len = length(normalized);
    STDROMANO_CHECK_NEAR(len, 1.0f, 0.001f);
    STDROMANO_CHECK_NEAR(normalized.x, 0.6f, 0.001f);
    STDROMANO_CHECK_NEAR(normalized.y, 0.8f, 0.001f);
}

STDROMANO_TEST_CASE(vector3_default_constructor)
{
    Vec3F v;
    STDROMANO_CHECK_EQ(v.x, 0.0f);
    STDROMANO_CHECK_EQ(v.y, 0.0f);
    STDROMANO_CHECK_EQ(v.z, 0.0f);
}

STDROMANO_TEST_CASE(vector3_single_value_constructor)
{
    Vec3F v(7.0f);
    STDROMANO_CHECK_EQ(v.x, 7.0f);
    STDROMANO_CHECK_EQ(v.y, 7.0f);
    STDROMANO_CHECK_EQ(v.z, 7.0f);
}

STDROMANO_TEST_CASE(vector3_three_value_constructor)
{
    Vec3F v(1.0f, 2.0f, 3.0f);
    STDROMANO_CHECK_EQ(v.x, 1.0f);
    STDROMANO_CHECK_EQ(v.y, 2.0f);
    STDROMANO_CHECK_EQ(v.z, 3.0f);
}

STDROMANO_TEST_CASE(vector3_vector_addition)
{
    Vec3F a(1.0f, 2.0f, 3.0f);
    Vec3F b(4.0f, 5.0f, 6.0f);
    Vec3F result = a + b;
    STDROMANO_CHECK_EQ(result.x, 5.0f);
    STDROMANO_CHECK_EQ(result.y, 7.0f);
    STDROMANO_CHECK_EQ(result.z, 9.0f);
}

STDROMANO_TEST_CASE(vector3_vector_subtraction)
{
    Vec3F a(10.0f, 8.0f, 6.0f);
    Vec3F b(3.0f, 2.0f, 1.0f);
    Vec3F result = a - b;
    STDROMANO_CHECK_EQ(result.x, 7.0f);
    STDROMANO_CHECK_EQ(result.y, 6.0f);
    STDROMANO_CHECK_EQ(result.z, 5.0f);
}

STDROMANO_TEST_CASE(vector3_scalar_multiplication)
{
    Vec3F v(2.0f, 3.0f, 4.0f);
    Vec3F result = v * 3.0f;
    STDROMANO_CHECK_EQ(result.x, 6.0f);
    STDROMANO_CHECK_EQ(result.y, 9.0f);
    STDROMANO_CHECK_EQ(result.z, 12.0f);
}

STDROMANO_TEST_CASE(vector3_index_operator)
{
    Vec3F v(1.0f, 2.0f, 3.0f);
    STDROMANO_CHECK_EQ(v[0], 1.0f);
    STDROMANO_CHECK_EQ(v[1], 2.0f);
    STDROMANO_CHECK_EQ(v[2], 3.0f);

    v[0] = 10.0f;
    v[1] = 20.0f;
    v[2] = 30.0f;
    STDROMANO_CHECK_EQ(v.x, 10.0f);
    STDROMANO_CHECK_EQ(v.y, 20.0f);
    STDROMANO_CHECK_EQ(v.z, 30.0f);
}

STDROMANO_TEST_CASE(vector3_dot)
{
    Vec3F a(1.0f, 2.0f, 3.0f);
    Vec3F b(4.0f, 5.0f, 6.0f);
    float result = dot(a, b);
    STDROMANO_CHECK_EQ(result, 32.0f);
}

STDROMANO_TEST_CASE(vector3_cross)
{
    Vec3F a(1.0f, 0.0f, 0.0f);
    Vec3F b(0.0f, 1.0f, 0.0f);
    Vec3F result = cross(a, b);
    STDROMANO_CHECK_EQ(result.x, 0.0f);
    STDROMANO_CHECK_EQ(result.y, 0.0f);
    STDROMANO_CHECK_EQ(result.z, 1.0f);

    Vec3F c(2.0f, 3.0f, 4.0f);
    Vec3F d(5.0f, 6.0f, 7.0f);
    Vec3F cross_result = cross(c, d);
    STDROMANO_CHECK_EQ(cross_result.x, -3.0f);
    STDROMANO_CHECK_EQ(cross_result.y, 6.0f);
    STDROMANO_CHECK_EQ(cross_result.z, -3.0f);
}

STDROMANO_TEST_CASE(vector3_length)
{
    Vec3F v(3.0f, 4.0f, 0.0f);
    float len = length(v);
    STDROMANO_CHECK_NEAR(len, 5.0f, 0.001f);

    Vec3F unit_vector(1.0f, 0.0f, 0.0f);
    float unit_len = length(unit_vector);
    STDROMANO_CHECK_NEAR(unit_len, 1.0f, 0.001f);
}

STDROMANO_TEST_CASE(vector3_normalize)
{
    Vec3F v(3.0f, 4.0f, 0.0f);
    Vec3F normalized = normalize(v);
    float len = length(normalized);
    STDROMANO_CHECK_NEAR(len, 1.0f, 0.001f);
    STDROMANO_CHECK_NEAR(normalized.x, 0.6f, 0.001f);
    STDROMANO_CHECK_NEAR(normalized.y, 0.8f, 0.001f);
    STDROMANO_CHECK_NEAR(normalized.z, 0.0f, 0.001f);
}

STDROMANO_TEST_CASE(vector3_compound_assignment)
{
    Vec3F v(2.0f, 4.0f, 6.0f);
    Vec3F other(1.0f, 2.0f, 3.0f);

    v += other;
    STDROMANO_CHECK_EQ(v.x, 3.0f);
    STDROMANO_CHECK_EQ(v.y, 6.0f);
    STDROMANO_CHECK_EQ(v.z, 9.0f);

    v -= other;
    STDROMANO_CHECK_EQ(v.x, 2.0f);
    STDROMANO_CHECK_EQ(v.y, 4.0f);
    STDROMANO_CHECK_EQ(v.z, 6.0f);

    v *= other;
    STDROMANO_CHECK_EQ(v.x, 2.0f);
    STDROMANO_CHECK_EQ(v.y, 8.0f);
    STDROMANO_CHECK_EQ(v.z, 18.0f);
}

STDROMANO_TEST_CASE(vector4_default_constructor)
{
    Vec4F v;
    STDROMANO_CHECK_EQ(v.x, 0.0f);
    STDROMANO_CHECK_EQ(v.y, 0.0f);
    STDROMANO_CHECK_EQ(v.z, 0.0f);
    STDROMANO_CHECK_EQ(v.w, 0.0f);
}

STDROMANO_TEST_CASE(vector4_four_value_constructor)
{
    Vec4F v(1.0f, 2.0f, 3.0f, 4.0f);
    STDROMANO_CHECK_EQ(v.x, 1.0f);
    STDROMANO_CHECK_EQ(v.y, 2.0f);
    STDROMANO_CHECK_EQ(v.z, 3.0f);
    STDROMANO_CHECK_EQ(v.w, 4.0f);
}

STDROMANO_TEST_CASE(vector4_index_operator)
{
    Vec4F v(1.0f, 2.0f, 3.0f, 4.0f);
    STDROMANO_CHECK_EQ(v[0], 1.0f);
    STDROMANO_CHECK_EQ(v[1], 2.0f);
    STDROMANO_CHECK_EQ(v[2], 3.0f);
    STDROMANO_CHECK_EQ(v[3], 4.0f);

    v[3] = 10.0f;
    STDROMANO_CHECK_EQ(v.w, 10.0f);
}

STDROMANO_TEST_CASE(vector4_dot)
{
    Vec4F a(1.0f, 2.0f, 3.0f, 4.0f);
    Vec4F b(2.0f, 3.0f, 4.0f, 5.0f);
    float result = dot(a, b);
    STDROMANO_CHECK_EQ(result, 40.0f);
}

STDROMANO_TEST_CASE(vector4_length)
{
    Vec4F v(1.0f, 2.0f, 2.0f, 0.0f);
    float len = length(v);
    float expected = 3.0f;
    STDROMANO_CHECK_NEAR(len, expected, 0.001f);
}

STDROMANO_TEST_CASE(vector4_axis_angle)
{
    Vec4F v(1.0f, 0.0f, 0.0f, 3.14159f);
    auto [axis, angle] = v.as_axis_angle();
    STDROMANO_CHECK_EQ(axis.x, 1.0f);
    STDROMANO_CHECK_EQ(axis.y, 0.0f);
    STDROMANO_CHECK_EQ(axis.z, 0.0f);
    STDROMANO_CHECK_NEAR(angle, 3.14159f, 0.001f);
}

STDROMANO_TEST_CASE(vector4_equality)
{
    Vec4F a(1.0f, 2.0f, 3.0f, 4.0f);
    Vec4F b(1.0f, 2.0f, 3.0f, 4.0f);
    Vec4F c(1.0f, 2.0f, 3.0f, 5.0f);

    STDROMANO_CHECK(a == b);
    STDROMANO_CHECK(!(a == c));
    STDROMANO_CHECK(!(a != b));
    STDROMANO_CHECK(a != c);
}

STDROMANO_TEST_CASE(integer_vectors)
{
    Vec2I a(5, 3);
    Vec2I b(2, 7);
    Vec2I result = a + b;
    STDROMANO_CHECK_EQ(result.x, 7);
    STDROMANO_CHECK_EQ(result.y, 10);

    Vec3I c(10, 15, 20);
    Vec3I d(2, 3, 4);
    Vec3I div_result = c / d;
    STDROMANO_CHECK_EQ(div_result.x, 5);
    STDROMANO_CHECK_EQ(div_result.y, 5);
    STDROMANO_CHECK_EQ(div_result.z, 5);
}

STDROMANO_TEST_CASE(double_vectors)
{
    Vec3D a(1.5, 2.5, 3.5);
    Vec3D b(0.5, 1.5, 2.5);
    Vec3D result = a - b;
    STDROMANO_CHECK_EQ(result.x, 1.0);
    STDROMANO_CHECK_EQ(result.y, 1.0);
    STDROMANO_CHECK_EQ(result.z, 1.0);
}

STDROMANO_TEST_CASE(zero_vectors)
{
    Vec2F zero_vec;
    Vec2F other(1.0f, 2.0f);

    Vec2F add_result = zero_vec + other;
    STDROMANO_CHECK_EQ(other.x, add_result.x);
    STDROMANO_CHECK_EQ(other.y, add_result.y);

    Vec2F sub_result = other - zero_vec;
    STDROMANO_CHECK_EQ(other.x, sub_result.x);
    STDROMANO_CHECK_EQ(other.y, sub_result.y);
}

STDROMANO_TEST_CASE(constexpr_evaluation)
{
    constexpr Vec2F a(3.0f, 4.0f);
    constexpr Vec2F b(1.0f, 2.0f);
    constexpr Vec2F sum = a + b;
    constexpr float dot_product = dot(a, b);
    constexpr float len_squared = length2(a);

    STDROMANO_CHECK_EQ(sum.x, 4.0f);
    STDROMANO_CHECK_EQ(sum.y, 6.0f);
    STDROMANO_CHECK_EQ(dot_product, 11.0f);
    STDROMANO_CHECK_EQ(len_squared, 25.0f);
}

STDROMANO_TEST_CASE(fuzz_vector3_identities)
{
    const auto report = fuzz::run_property(fixtures::options("vector3_identities", 2000), [](fuzz::Source& source) {
        const Vec3D a(source.finite(-100.0, 100.0), source.finite(-100.0, 100.0), source.finite(-100.0, 100.0));
        const Vec3D b(source.finite(-100.0, 100.0), source.finite(-100.0, 100.0), source.finite(-100.0, 100.0));

        const double scale = std::max(1.0, length2(a) * length2(b));

        STDROMANO_FUZZ_CHECK_EQ(dot(a, b), dot(b, a));
        STDROMANO_FUZZ_CHECK(std::abs(length2(a) - dot(a, a)) <= 1e-12 * std::max(1.0, length2(a)));

        const Vec3D c = cross(a, b);
        STDROMANO_FUZZ_CHECK(std::abs(dot(c, a)) <= 1e-9 * scale);
        STDROMANO_FUZZ_CHECK(std::abs(dot(c, b)) <= 1e-9 * scale);
        STDROMANO_FUZZ_CHECK(cross(b, a).equal_with_abs_error(-c, 1e-12 * scale));

        STDROMANO_FUZZ_CHECK(((a + b) - b).equal_with_abs_error(a, 1e-9));

        if(length(a) > 1e-6)
            STDROMANO_FUZZ_CHECK(std::abs(length(normalize(a)) - 1.0) <= 1e-9);

        return true;
    });

    TESTS_REQUIRE_PROPERTY(report);
}

STDROMANO_TEST_CASE(vector_layout)
{
    static_assert(sizeof(Vec2F) == 8 && alignof(Vec2F) == 8, "Vec2F must match float2");
    static_assert(sizeof(Vec3F) == 12 && alignof(Vec3F) == 4, "Vec3F must match float3");
    static_assert(sizeof(Vec4F) == 16 && alignof(Vec4F) == 16, "Vec4F must match float4");
    static_assert(sizeof(Vec2D) == 16 && alignof(Vec2D) == 16, "Vec2D must match double2");
    static_assert(sizeof(Vec4D) == 32 && alignof(Vec4D) == 16, "Vec4D must keep the CUDA double4 alignment");
    static_assert(sizeof(Vec4U) == 16 && alignof(Vec4U) == 16, "Vec4U must match uint4");
}

STDROMANO_TEST_CASE(vector4_component_division)
{
    const Vec4F a(8.0f, 6.0f, 4.0f, 2.0f);
    const Vec4F b(2.0f, 3.0f, 4.0f, 2.0f);
    STDROMANO_CHECK_EQ(a / b, Vec4F(4.0f, 2.0f, 1.0f, 1.0f));
}

STDROMANO_TEST_CASE(vector_scalar_division)
{
    STDROMANO_CHECK_EQ(Vec3F(3.0f, 6.0f, 9.0f) / 3.0f, Vec3F(3.0f, 6.0f, 9.0f) * (1.0f / 3.0f));
    STDROMANO_CHECK_EQ(Vec3I(7, 8, 9) / 2, Vec3I(3, 4, 4));
    STDROMANO_CHECK_EQ(Vec4U(9u, 18u, 27u, 36u) / 9u, Vec4U(1u, 2u, 3u, 4u));
}

STDROMANO_TEST_CASE(vector_normalize)
{
    const Vec3F v(3.0f, 4.0f, 12.0f);
    STDROMANO_CHECK_NEAR(length(normalize(v)), 1.0f, 1e-6f);
    STDROMANO_CHECK(normalize_safe(v).equal_with_abs_error(Vec3F(3.0f / 13.0f, 4.0f / 13.0f, 12.0f / 13.0f), 1e-7f));
    STDROMANO_CHECK_NEAR(length(normalize(Vec2F(3.0f, 4.0f))), 1.0f, 1e-6f);
    STDROMANO_CHECK_NEAR(length(normalize(Vec4F(1.0f, 2.0f, 3.0f, 4.0f))), 1.0f, 1e-6f);
    STDROMANO_CHECK_EQ(normalize_safe(Vec2D(0.0, -2.0)), Vec2D(0.0, -1.0));
}

STDROMANO_TEST_CASE(vector_swizzle_and_conversions)
{
    const Vec4F v(1.0f, 2.0f, 3.0f, 4.0f);
    STDROMANO_CHECK_EQ(v.xyz(), Vec3F(1.0f, 2.0f, 3.0f));
    STDROMANO_CHECK_EQ(v.xy(), Vec2F(1.0f, 2.0f));
    STDROMANO_CHECK_EQ(Vec4F(Vec3F(1.0f, 2.0f, 3.0f), 4.0f), v);
    STDROMANO_CHECK_EQ(Vec3F(Vec2F(1.0f, 2.0f), 3.0f), v.xyz());

    struct Foreign { float x, y, z, w; };
    const Foreign f{5.0f, 6.0f, 7.0f, 8.0f};
    STDROMANO_CHECK_EQ(Vec4F(f), Vec4F(5.0f, 6.0f, 7.0f, 8.0f));
    STDROMANO_CHECK_EQ(Vec3F(f), Vec3F(5.0f, 6.0f, 7.0f));
    STDROMANO_CHECK_EQ(Vec2I(f), Vec2I(5, 6));

    static_assert(!std::is_convertible_v<Foreign, Vec3F>, "foreign vectors convert explicitly only");
    static_assert(!std::is_convertible_v<Vec4F, Vec3F>, "dropping components must be explicit");
    static_assert(std::is_convertible_v<Vec3I, Vec3F>, "same dimension conversions stay implicit");
}

STDROMANO_TEST_CASE(vector_component_wise_functions)
{
    const Vec3F a(-1.5f, 2.25f, 0.0f);
    const Vec3F b(1.0f, -3.0f, 0.5f);

    STDROMANO_CHECK_EQ(abs(a), Vec3F(1.5f, 2.25f, 0.0f));
    STDROMANO_CHECK_EQ(min(a, b), Vec3F(-1.5f, -3.0f, 0.0f));
    STDROMANO_CHECK_EQ(max(a, b), Vec3F(1.0f, 2.25f, 0.5f));
    STDROMANO_CHECK_EQ(min_component(a), -1.5f);
    STDROMANO_CHECK_EQ(max_component(b), 1.0f);
    STDROMANO_CHECK_EQ(clamp(a), Vec3F(0.0f, 1.0f, 0.0f));
    STDROMANO_CHECK_EQ(clamp(a, -1.0f, 2.0f), Vec3F(-1.0f, 2.0f, 0.0f));
    STDROMANO_CHECK_EQ(floor(a), Vec3F(-2.0f, 2.0f, 0.0f));
    STDROMANO_CHECK_EQ(ceil(a), Vec3F(-1.0f, 3.0f, 0.0f));
    STDROMANO_CHECK_EQ(sign(a), Vec3F(-1.0f, 1.0f, 1.0f));
    STDROMANO_CHECK_EQ(lerp(a, b, 0.0f), a);
    STDROMANO_CHECK_EQ(lerp(a, b, 1.0f), b);
    STDROMANO_CHECK_EQ(lerp(a, b, Vec3F(0.0f, 1.0f, 0.0f)), Vec3F(-1.5f, -3.0f, 0.0f));
    STDROMANO_CHECK_EQ(pow(Vec3F(2.0f, 3.0f, 4.0f), 2.0f), Vec3F(4.0f, 9.0f, 16.0f));
    STDROMANO_CHECK_EQ(rcp_safe(Vec4F(2.0f, 4.0f, 0.5f, 1.0f)), Vec4F(0.5f, 0.25f, 2.0f, 1.0f));
    STDROMANO_CHECK(rcp(Vec2F(3.0f, 7.0f)).equal_with_abs_error(Vec2F(1.0f / 3.0f, 1.0f / 7.0f), 1e-6f));
    STDROMANO_CHECK_EQ(dist2(Vec3F(1.0f, 2.0f, 3.0f), Vec3F(1.0f, 4.0f, 3.0f)), 4.0f);
    STDROMANO_CHECK_EQ(dist(Vec2F(0.0f, 0.0f), Vec2F(3.0f, 4.0f)), 5.0f);
}

STDROMANO_TEST_CASE(vector_classification)
{
    const float nan = std::numeric_limits<float>::quiet_NaN();
    const float inf = std::numeric_limits<float>::infinity();

    STDROMANO_CHECK(isnan(Vec3F(0.0f, nan, 1.0f)));
    STDROMANO_CHECK(!isnan(Vec3F(0.0f, inf, 1.0f)));
    STDROMANO_CHECK(isinf(Vec4F(0.0f, 0.0f, 0.0f, -inf)));
    STDROMANO_CHECK(!isinf(Vec2F(nan, 0.0f)));
    STDROMANO_CHECK(isfinite(Vec2F(1.0f, 2.0f)));
    STDROMANO_CHECK(!isfinite(Vec3F(1.0f, 2.0f, inf)));
    STDROMANO_CHECK(!isfinite(Vec4F(nan)));
}

STDROMANO_TEST_CASE(vector_geometry)
{
    const Vec3F incident(1.0f, -1.0f, 0.0f);
    const Vec3F up(0.0f, 1.0f, 0.0f);
    STDROMANO_CHECK_EQ(reflect(incident, up), Vec3F(1.0f, 1.0f, 0.0f));
    STDROMANO_CHECK_EQ(reflect(Vec2F(1.0f, -1.0f), Vec2F(0.0f, 1.0f)), Vec2F(1.0f, 1.0f));
    STDROMANO_CHECK_EQ(perpendicular(Vec2F(1.0f, 0.0f)), Vec2F(0.0f, 1.0f));

    STDROMANO_CHECK_NEAR(angle(Vec3F(1.0f, 0.0f, 0.0f), Vec3F(0.0f, 2.0f, 0.0f)), maths::constants<float>::pi_over_two, 1e-6f);
    STDROMANO_CHECK_NEAR(angle(Vec2D(1.0, 0.0), Vec2D(-3.0, 0.0)), maths::constants<double>::pi, 1e-12);
    STDROMANO_CHECK_EQ(angle(Vec3D(1.0, 1.0, 0.0), Vec3D(2.0, 2.0, 0.0)), 0.0);
}

STDROMANO_TEST_CASE(vector_integral_bit_operations)
{
    const Vec4U a(0xF0F0F0F0u, 1u, 2u, 0xFFFFFFFFu);
    const Vec4U b(0x0F0F0F0Fu, 3u, 6u, 1u);

    STDROMANO_CHECK_EQ(a ^ b, Vec4U(0xFFFFFFFFu, 2u, 4u, 0xFFFFFFFEu));
    STDROMANO_CHECK_EQ(a & b, Vec4U(0u, 1u, 2u, 1u));
    STDROMANO_CHECK_EQ(a | b, Vec4U(0xFFFFFFFFu, 3u, 6u, 0xFFFFFFFFu));
    STDROMANO_CHECK_EQ(a >> 4u, Vec4U(0x0F0F0F0Fu, 0u, 0u, 0x0FFFFFFFu));
    STDROMANO_CHECK_EQ(b << 1u, Vec4U(0x1E1E1E1Eu, 6u, 12u, 2u));
    STDROMANO_CHECK_EQ(~Vec2U(0u, 0xFFFFFFFFu), Vec2U(0xFFFFFFFFu, 0u));
    STDROMANO_CHECK_EQ(Vec3U(1u, 2u, 3u) ^ 1u, Vec3U(0u, 3u, 2u));

    Vec3U c(1u, 2u, 3u);
    c <<= 2u;
    c ^= Vec3U(1u);
    c >>= 1u;
    STDROMANO_CHECK_EQ(c, Vec3U(2u, 4u, 6u));

    /* unsigned arithmetic wraps like the PCG hashes expect */
    STDROMANO_CHECK_EQ(Vec2U(0xFFFFFFFFu, 2u) * 2u + 1u, Vec2U(0xFFFFFFFFu, 5u));
}

STDROMANO_TEST_MAIN()
