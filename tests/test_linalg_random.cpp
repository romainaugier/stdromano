// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - Present Romain Augier
// All rights reserved.

#include "stdromano/linalg/random.hpp"

#include "fixtures.hpp"

using namespace stdromano;

/* Reference values computed with an independent implementation of the paper's code */

STDROMANO_TEST_CASE(pcg2d_reference_values)
{
    STDROMANO_CHECK_EQ(pcg2d_hash(Vec2U(0U, 0U)), Vec2U(0x18E431A7U, 0x055DF4D1U));
    STDROMANO_CHECK_EQ(pcg2d_hash(Vec2U(12U, 345U)), Vec2U(0x0ADAEDDCU, 0x98DBE451U));
}

STDROMANO_TEST_CASE(pcg3d_reference_values)
{
    STDROMANO_CHECK_EQ(pcg3d_hash(Vec3U(1U, 2U, 3U)), Vec3U(0xFA9F79A6U, 0x48F2F44CU, 0x596F5AB1U));
}

STDROMANO_TEST_CASE(pcg4d_reference_values)
{
    STDROMANO_CHECK_EQ(pcg4d_hash(Vec4U(1U, 2U, 3U, 4U)), Vec4U(0x3622CD16U, 0xF11471D8U, 0xE1109B3FU, 0x02B94C2FU));
}

STDROMANO_TEST_CASE(pcg_hash_float_range)
{
    const auto report = fuzz::run_property(fixtures::options("pcg_hash_float_range", 5000), [](fuzz::Source& source) {
        const Vec4U seed(source.integer<std::uint32_t>(),
                         source.integer<std::uint32_t>(),
                         source.integer<std::uint32_t>(),
                         source.integer<std::uint32_t>());

        const Vec2F f2 = pcg2d_hash_float(Vec2U(seed.x, seed.y));
        const Vec3F f3 = pcg3d_hash_float(seed.xyz());
        const Vec4F f4 = pcg4d_hash_float(seed);

        STDROMANO_FUZZ_CHECK(min_component(f2) >= 0.0f && max_component(f2) < 1.0f);
        STDROMANO_FUZZ_CHECK(min_component(f3) >= 0.0f && max_component(f3) < 1.0f);
        STDROMANO_FUZZ_CHECK(min_component(f4) >= 0.0f && max_component(f4) < 1.0f);

        return true;
    });

    TESTS_REQUIRE_PROPERTY(report);
}

STDROMANO_TEST_CASE(pcg_hash_float_extremes)
{
    /* inputs hashing to 0xFFFFFFFF must still map below 1 */
    STDROMANO_CHECK_LT(detail::u32_to_float_01(0xFFFFFFFFU), 1.0f);
    STDROMANO_CHECK_EQ(detail::u32_to_float_01(0U), 0.0f);
}

STDROMANO_TEST_CASE(pcg_hash_avalanche)
{
    /* flipping a single input bit should flip about half of the output bits */
    std::uint64_t flipped = 0;
    std::uint64_t total = 0;

    for(std::uint32_t i = 0; i < 4096; ++i)
    {
        const Vec3U v(i * 2654435761U, i ^ 0xA5A5A5A5U, i + 17U);
        const Vec3U h = pcg3d_hash(v);

        for(std::uint32_t bit = 0; bit < 32; bit += 7)
        {
            const Vec3U h2 = pcg3d_hash(Vec3U(v.x ^ (1U << bit), v.y, v.z));
            const Vec3U diff = h ^ h2;
            flipped += popcount_u32(diff.x) + popcount_u32(diff.y) + popcount_u32(diff.z);
            total += 96;
        }
    }

    const double ratio = static_cast<double>(flipped) / static_cast<double>(total);
    STDROMANO_CHECK_GT(ratio, 0.4);
    STDROMANO_CHECK_LT(ratio, 0.6);
}

STDROMANO_TEST_MAIN()
