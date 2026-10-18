// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - Present Romain Augier
// All rights reserved.

#include "stdromano/maths.hpp"

#include "fixtures.hpp"

#include <cmath>
#include <limits>

using namespace stdromano;

STDROMANO_TEST_CASE(maths_classification)
{
    const float finf = std::numeric_limits<float>::infinity();
    const float fnan = std::numeric_limits<float>::quiet_NaN();
    const double dinf = std::numeric_limits<double>::infinity();
    const double dnan = std::numeric_limits<double>::quiet_NaN();

    STDROMANO_CHECK(maths::isinf(finf));
    STDROMANO_CHECK(maths::isinf(-finf));
    STDROMANO_CHECK(!maths::isinf(fnan));
    STDROMANO_CHECK(!maths::isinf(1.0f));
    STDROMANO_CHECK(maths::isnan(fnan));
    STDROMANO_CHECK(!maths::isnan(finf));
    STDROMANO_CHECK(!maths::isnan(0.0f));
    STDROMANO_CHECK(maths::isfinite(std::numeric_limits<float>::max()));
    STDROMANO_CHECK(maths::isfinite(std::numeric_limits<float>::denorm_min()));
    STDROMANO_CHECK(!maths::isfinite(finf));
    STDROMANO_CHECK(!maths::isfinite(fnan));

    STDROMANO_CHECK(maths::isinf(dinf));
    STDROMANO_CHECK(!maths::isinf(dnan));
    STDROMANO_CHECK(maths::isnan(dnan));
    STDROMANO_CHECK(!maths::isnan(-dinf));
    STDROMANO_CHECK(!maths::isfinite(dinf));
    STDROMANO_CHECK(maths::isfinite(-1.0));
}

STDROMANO_TEST_CASE(maths_angles)
{
    STDROMANO_CHECK_NEAR(maths::deg2rad(180.0f), maths::constants<float>::pi, 1e-6f);
    STDROMANO_CHECK_NEAR(maths::deg2rad(90.0), maths::constants<double>::pi_over_two, 1e-15);
    STDROMANO_CHECK_NEAR(maths::rad2deg(maths::constants<float>::pi), 180.0f, 1e-4f);
    STDROMANO_CHECK_NEAR(maths::rad2deg(maths::deg2rad(37.5)), 37.5, 1e-12);

    /* the CUDA deg2rad was dividing instead of multiplying, keep the direction pinned */
    STDROMANO_CHECK_LT(maths::deg2rad(45.0f), 1.0f);
    STDROMANO_CHECK_GT(maths::rad2deg(1.0f), 57.0f);
}

STDROMANO_TEST_CASE(maths_sign_copysign)
{
    STDROMANO_CHECK_EQ(maths::sign(3.0f), 1.0f);
    STDROMANO_CHECK_EQ(maths::sign(-3.0f), -1.0f);
    STDROMANO_CHECK_EQ(maths::sign(-0.0f), -1.0f);
    STDROMANO_CHECK_EQ(maths::sign(0.0), 1.0);
    STDROMANO_CHECK_EQ(maths::copysign(2.0f, -0.5f), -2.0f);
    STDROMANO_CHECK_EQ(maths::copysign(-2.0, 0.5), 2.0);
}

STDROMANO_TEST_CASE(maths_min_max_clamp)
{
    STDROMANO_CHECK_EQ(maths::min(1.0f, 2.0f), 1.0f);
    STDROMANO_CHECK_EQ(maths::max(1.0f, 2.0f), 2.0f);
    STDROMANO_CHECK_EQ(maths::min(-4, 3), -4);
    STDROMANO_CHECK_EQ(maths::max(-4, 3), 3);
    STDROMANO_CHECK_EQ(maths::clamp(1.5f), 1.0f);
    STDROMANO_CHECK_EQ(maths::clamp(-1.5f), 0.0f);
    STDROMANO_CHECK_EQ(maths::clamp(5.0, 2.0, 4.0), 4.0);
    STDROMANO_CHECK_EQ(maths::clampz(-2.0f, 10.0f), 0.0f);

    static_assert(maths::min(1, 2) == 1, "min must stay constexpr on the host");
    static_assert(maths::clamp(3.0f, 0.0f, 2.0f) == 2.0f, "clamp must stay constexpr on the host");
}

STDROMANO_TEST_CASE(maths_interpolation)
{
    STDROMANO_CHECK_EQ(maths::lerp(2.0f, 6.0f, 0.0f), 2.0f);
    STDROMANO_CHECK_EQ(maths::lerp(2.0f, 6.0f, 1.0f), 6.0f);
    STDROMANO_CHECK_EQ(maths::lerp(2.0f, 6.0f, 0.5f), 4.0f);
    STDROMANO_CHECK_EQ(maths::fit(5.0f, 0.0f, 10.0f, 100.0f, 200.0f), 150.0f);
    STDROMANO_CHECK_EQ(maths::fit01(0.25f, 4.0f, 8.0f), 5.0f);
    STDROMANO_CHECK_EQ(maths::frac(2.75f), 0.75f);
    STDROMANO_CHECK_EQ(maths::frac(-0.25f), 0.75f);
}

STDROMANO_TEST_CASE(maths_fused)
{
    STDROMANO_CHECK_EQ(maths::fma(2.0f, 3.0f, 1.0f), 7.0f);
    STDROMANO_CHECK_EQ(maths::fms(2.0f, 3.0f, 1.0f), 5.0f);
    STDROMANO_CHECK_EQ(maths::nfma(2.0f, 3.0f, 1.0f), -5.0f);
    STDROMANO_CHECK_EQ(maths::nfms(2.0f, 3.0f, 1.0f), -7.0f);
    STDROMANO_CHECK_EQ(maths::fma(2.0, 3.0, 1.0), 7.0);
    STDROMANO_CHECK_EQ(maths::fms(2.0, 3.0, 1.0), 5.0);
    STDROMANO_CHECK_EQ(maths::nfma(2.0, 3.0, 1.0), -5.0);
    STDROMANO_CHECK_EQ(maths::nfms(2.0, 3.0, 1.0), -7.0);
}

STDROMANO_TEST_CASE(maths_sincos)
{
    float s, c;
    maths::sincos(0.5f, &s, &c);
    STDROMANO_CHECK_EQ(s, std::sin(0.5f));
    STDROMANO_CHECK_EQ(c, std::cos(0.5f));

    double sd, cd;
    maths::sincos(1.25, &sd, &cd);
    STDROMANO_CHECK_EQ(sd, std::sin(1.25));
    STDROMANO_CHECK_EQ(cd, std::cos(1.25));
}

STDROMANO_TEST_CASE(maths_differential_against_std)
{
    const auto report = fuzz::run_property(fixtures::options("maths_vs_std", 2000), [](fuzz::Source& source) {
        const float x = source.finite<float>(-1e4f, 1e4f);
        const float y = source.finite<float>(-1e4f, 1e4f);
        const float p = source.finite<float>(1e-3f, 1e4f);

        STDROMANO_FUZZ_CHECK_EQ(maths::abs(x), std::fabs(x));
        STDROMANO_FUZZ_CHECK_EQ(maths::floor(x), std::floor(x));
        STDROMANO_FUZZ_CHECK_EQ(maths::ceil(x), std::ceil(x));
        STDROMANO_FUZZ_CHECK_EQ(maths::sqrt(p), std::sqrt(p));
        STDROMANO_FUZZ_CHECK_EQ(maths::rcp_safe(p), 1.0f / p);
        STDROMANO_FUZZ_CHECK_EQ(maths::atan2(y, x), std::atan2(y, x));
        STDROMANO_FUZZ_CHECK_EQ(maths::min(x, y), std::min(x, y));
        STDROMANO_FUZZ_CHECK_EQ(maths::max(x, y), std::max(x, y));
#if defined(STDROMANO_HAS_FMA_INTRINSICS) || defined(STDROMANO_AARCH64)
        STDROMANO_FUZZ_CHECK_EQ(maths::fma(x, y, p), std::fma(x, y, p));
#endif /* defined(STDROMANO_HAS_FMA_INTRINSICS) || defined(STDROMANO_AARCH64) */

        return true;
    });

    TESTS_REQUIRE_PROPERTY(report);
}

STDROMANO_TEST_CASE(maths_approximations_error_bounds)
{
    /* rcp and rsqrt are refined estimates, they must stay within a few ulps of the exact value */
    const auto report = fuzz::run_property(fixtures::options("maths_rcp_rsqrt", 2000), [](fuzz::Source& source) {
        const float x = source.finite<float>(1e-30f, 1e30f);

        const float rcp_error = std::fabs(maths::rcp(x) * x - 1.0f);
        const float rsqrt_error = std::fabs(maths::rsqrt(x) * std::sqrt(x) - 1.0f);

        STDROMANO_FUZZ_CHECK(rcp_error < 1e-6f);
        STDROMANO_FUZZ_CHECK(rsqrt_error < 1e-6f);

        return true;
    });

    TESTS_REQUIRE_PROPERTY(report);
}

STDROMANO_TEST_MAIN()
