// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - Present Romain Augier
// All rights reserved.

#include "stdromano/random.hpp"
#include "stdromano/hashset.hpp"
#include "stdromano/vector.hpp"
#include "stdromano/threading.hpp"

#include "fixtures.hpp"

#include <algorithm>

namespace {

struct SplitMixGenerator
{
    using State = std::uint64_t;

    static constexpr const char* name = "splitmix";

    static State make(const std::uint64_t seed) { return seed; }

    static std::uint32_t next_u32(State& s) { return stdromano::next_random_u32(s); }
    static std::uint64_t next_u64(State& s) { return stdromano::next_random_u64(s); }
    static float next_float(State& s) { return stdromano::next_random_float(s); }
    static double next_double(State& s) { return stdromano::next_random_double(s); }

    static std::uint32_t next_u32_in_range(State& s, std::uint32_t l, std::uint32_t h) { return stdromano::next_random_u32_in_range(s, l, h); }
    static std::uint64_t next_u64_in_range(State& s, std::uint64_t l, std::uint64_t h) { return stdromano::next_random_u64_in_range(s, l, h); }
    static float next_float_in_range(State& s, float l, float h) { return stdromano::next_random_float_in_range(s, l, h); }
    static double next_double_in_range(State& s, double l, double h) { return stdromano::next_random_double_in_range(s, l, h); }

    static std::uint32_t random_u32(std::uint64_t seed) { return stdromano::random_u32(seed); }
    static std::uint64_t random_u64(std::uint64_t seed) { return stdromano::random_u64(seed); }
    static float random_float(std::uint64_t seed) { return stdromano::random_float(seed); }
    static double random_double(std::uint64_t seed) { return stdromano::random_double(seed); }

    static std::uint32_t random_u32_in_range(std::uint64_t seed, std::uint32_t l, std::uint32_t h) { return stdromano::random_u32_in_range(seed, l, h); }
    static std::uint64_t random_u64_in_range(std::uint64_t seed, std::uint64_t l, std::uint64_t h) { return stdromano::random_u64_in_range(seed, l, h); }
    static float random_float_in_range(std::uint64_t seed, float l, float h) { return stdromano::random_float_in_range(seed, l, h); }
    static double random_double_in_range(std::uint64_t seed, double l, double h) { return stdromano::random_double_in_range(seed, l, h); }

    static std::uint32_t ts_u32() { return stdromano::ts_next_random_u32(); }
    static std::uint64_t ts_u64() { return stdromano::ts_next_random_u64(); }
    static float ts_float() { return stdromano::ts_next_random_float(); }
    static double ts_double() { return stdromano::ts_next_random_double(); }

    static std::uint32_t ts_u32_in_range(std::uint32_t l, std::uint32_t h) { return stdromano::ts_next_random_u32_in_range(l, h); }
    static std::uint64_t ts_u64_in_range(std::uint64_t l, std::uint64_t h) { return stdromano::ts_next_random_u64_in_range(l, h); }
    static float ts_float_in_range(float l, float h) { return stdromano::ts_next_random_float_in_range(l, h); }
    static double ts_double_in_range(double l, double h) { return stdromano::ts_next_random_double_in_range(l, h); }
};

struct WangGenerator
{
    using State = std::uint32_t;

    static constexpr const char* name = "wang";

    static State make(const std::uint64_t seed) { return static_cast<State>(seed); }

    static std::uint32_t next_u32(State& s) { return stdromano::wang_next_random_u32(s); }
    static std::uint64_t next_u64(State& s) { return stdromano::wang_next_random_u64(s); }
    static float next_float(State& s) { return stdromano::wang_next_random_float(s); }
    static double next_double(State& s) { return stdromano::wang_next_random_double(s); }

    static std::uint32_t next_u32_in_range(State& s, std::uint32_t l, std::uint32_t h) { return stdromano::wang_next_random_u32_in_range(s, l, h); }
    static std::uint64_t next_u64_in_range(State& s, std::uint64_t l, std::uint64_t h) { return stdromano::wang_next_random_u64_in_range(s, l, h); }
    static float next_float_in_range(State& s, float l, float h) { return stdromano::wang_next_random_float_in_range(s, l, h); }
    static double next_double_in_range(State& s, double l, double h) { return stdromano::wang_next_random_double_in_range(s, l, h); }

    static std::uint32_t random_u32(std::uint64_t seed) { return stdromano::wang_random_u32(make(seed)); }
    static std::uint64_t random_u64(std::uint64_t seed) { return stdromano::wang_random_u64(make(seed)); }
    static float random_float(std::uint64_t seed) { return stdromano::wang_random_float(make(seed)); }
    static double random_double(std::uint64_t seed) { return stdromano::wang_random_double(make(seed)); }

    static std::uint32_t random_u32_in_range(std::uint64_t seed, std::uint32_t l, std::uint32_t h) { return stdromano::wang_random_u32_in_range(make(seed), l, h); }
    static std::uint64_t random_u64_in_range(std::uint64_t seed, std::uint64_t l, std::uint64_t h) { return stdromano::wang_random_u64_in_range(make(seed), l, h); }
    static float random_float_in_range(std::uint64_t seed, float l, float h) { return stdromano::wang_random_float_in_range(make(seed), l, h); }
    static double random_double_in_range(std::uint64_t seed, double l, double h) { return stdromano::wang_random_double_in_range(make(seed), l, h); }

    static std::uint32_t ts_u32() { return stdromano::ts_wang_next_random_u32(); }
    static std::uint64_t ts_u64() { return stdromano::ts_wang_next_random_u64(); }
    static float ts_float() { return stdromano::ts_wang_next_random_float(); }
    static double ts_double() { return stdromano::ts_wang_next_random_double(); }

    static std::uint32_t ts_u32_in_range(std::uint32_t l, std::uint32_t h) { return stdromano::ts_wang_next_random_u32_in_range(l, h); }
    static std::uint64_t ts_u64_in_range(std::uint64_t l, std::uint64_t h) { return stdromano::ts_wang_next_random_u64_in_range(l, h); }
    static float ts_float_in_range(float l, float h) { return stdromano::ts_wang_next_random_float_in_range(l, h); }
    static double ts_double_in_range(double l, double h) { return stdromano::ts_wang_next_random_double_in_range(l, h); }
};

struct PcgGenerator
{
    using State = std::uint64_t;

    static constexpr const char* name = "pcg";

    static State make(const std::uint64_t seed) { return stdromano::pcg_seed(seed); }

    static std::uint32_t next_u32(State& s) { return stdromano::pcg_next_random_u32(s); }
    static std::uint64_t next_u64(State& s) { return stdromano::pcg_next_random_u64(s); }
    static float next_float(State& s) { return stdromano::pcg_next_random_float(s); }
    static double next_double(State& s) { return stdromano::pcg_next_random_double(s); }

    static std::uint32_t next_u32_in_range(State& s, std::uint32_t l, std::uint32_t h) { return stdromano::pcg_next_random_u32_in_range(s, l, h); }
    static std::uint64_t next_u64_in_range(State& s, std::uint64_t l, std::uint64_t h) { return stdromano::pcg_next_random_u64_in_range(s, l, h); }
    static float next_float_in_range(State& s, float l, float h) { return stdromano::pcg_next_random_float_in_range(s, l, h); }
    static double next_double_in_range(State& s, double l, double h) { return stdromano::pcg_next_random_double_in_range(s, l, h); }

    static std::uint32_t random_u32(std::uint64_t seed) { return stdromano::pcg_random_u32(seed); }
    static std::uint64_t random_u64(std::uint64_t seed) { return stdromano::pcg_random_u64(seed); }
    static float random_float(std::uint64_t seed) { return stdromano::pcg_random_float(seed); }
    static double random_double(std::uint64_t seed) { return stdromano::pcg_random_double(seed); }

    static std::uint32_t random_u32_in_range(std::uint64_t seed, std::uint32_t l, std::uint32_t h) { return stdromano::pcg_random_u32_in_range(seed, l, h); }
    static std::uint64_t random_u64_in_range(std::uint64_t seed, std::uint64_t l, std::uint64_t h) { return stdromano::pcg_random_u64_in_range(seed, l, h); }
    static float random_float_in_range(std::uint64_t seed, float l, float h) { return stdromano::pcg_random_float_in_range(seed, l, h); }
    static double random_double_in_range(std::uint64_t seed, double l, double h) { return stdromano::pcg_random_double_in_range(seed, l, h); }

    static std::uint32_t ts_u32() { return stdromano::ts_pcg_next_random_u32(); }
    static std::uint64_t ts_u64() { return stdromano::ts_pcg_next_random_u64(); }
    static float ts_float() { return stdromano::ts_pcg_next_random_float(); }
    static double ts_double() { return stdromano::ts_pcg_next_random_double(); }

    static std::uint32_t ts_u32_in_range(std::uint32_t l, std::uint32_t h) { return stdromano::ts_pcg_next_random_u32_in_range(l, h); }
    static std::uint64_t ts_u64_in_range(std::uint64_t l, std::uint64_t h) { return stdromano::ts_pcg_next_random_u64_in_range(l, h); }
    static float ts_float_in_range(float l, float h) { return stdromano::ts_pcg_next_random_float_in_range(l, h); }
    static double ts_double_in_range(double l, double h) { return stdromano::ts_pcg_next_random_double_in_range(l, h); }
};

struct XoshiroGenerator
{
    using State = stdromano::XoshiroState;

    static constexpr const char* name = "xoshiro";

    static State make(const std::uint64_t seed) { return stdromano::xoshiro_seed(seed); }

    static std::uint32_t next_u32(State& s) { return stdromano::xoshiro_next_random_u32(s); }
    static std::uint64_t next_u64(State& s) { return stdromano::xoshiro_next_random_u64(s); }
    static float next_float(State& s) { return stdromano::xoshiro_next_random_float(s); }
    static double next_double(State& s) { return stdromano::xoshiro_next_random_double(s); }

    static std::uint32_t next_u32_in_range(State& s, std::uint32_t l, std::uint32_t h) { return stdromano::xoshiro_next_random_u32_in_range(s, l, h); }
    static std::uint64_t next_u64_in_range(State& s, std::uint64_t l, std::uint64_t h) { return stdromano::xoshiro_next_random_u64_in_range(s, l, h); }
    static float next_float_in_range(State& s, float l, float h) { return stdromano::xoshiro_next_random_float_in_range(s, l, h); }
    static double next_double_in_range(State& s, double l, double h) { return stdromano::xoshiro_next_random_double_in_range(s, l, h); }

    static std::uint32_t random_u32(std::uint64_t seed) { return stdromano::xoshiro_random_u32(seed); }
    static std::uint64_t random_u64(std::uint64_t seed) { return stdromano::xoshiro_random_u64(seed); }
    static float random_float(std::uint64_t seed) { return stdromano::xoshiro_random_float(seed); }
    static double random_double(std::uint64_t seed) { return stdromano::xoshiro_random_double(seed); }

    static std::uint32_t random_u32_in_range(std::uint64_t seed, std::uint32_t l, std::uint32_t h) { return stdromano::xoshiro_random_u32_in_range(seed, l, h); }
    static std::uint64_t random_u64_in_range(std::uint64_t seed, std::uint64_t l, std::uint64_t h) { return stdromano::xoshiro_random_u64_in_range(seed, l, h); }
    static float random_float_in_range(std::uint64_t seed, float l, float h) { return stdromano::xoshiro_random_float_in_range(seed, l, h); }
    static double random_double_in_range(std::uint64_t seed, double l, double h) { return stdromano::xoshiro_random_double_in_range(seed, l, h); }

    static std::uint32_t ts_u32() { return stdromano::ts_xoshiro_next_random_u32(); }
    static std::uint64_t ts_u64() { return stdromano::ts_xoshiro_next_random_u64(); }
    static float ts_float() { return stdromano::ts_xoshiro_next_random_float(); }
    static double ts_double() { return stdromano::ts_xoshiro_next_random_double(); }

    static std::uint32_t ts_u32_in_range(std::uint32_t l, std::uint32_t h) { return stdromano::ts_xoshiro_next_random_u32_in_range(l, h); }
    static std::uint64_t ts_u64_in_range(std::uint64_t l, std::uint64_t h) { return stdromano::ts_xoshiro_next_random_u64_in_range(l, h); }
    static float ts_float_in_range(float l, float h) { return stdromano::ts_xoshiro_next_random_float_in_range(l, h); }
    static double ts_double_in_range(double l, double h) { return stdromano::ts_xoshiro_next_random_double_in_range(l, h); }
};

template <typename F>
void for_each_generator(F&& f)
{
    f(SplitMixGenerator{});
    f(WangGenerator{});
    f(PcgGenerator{});
    f(XoshiroGenerator{});
}

template <typename T>
bool bins_are_uniform(const T* bins, const std::uint32_t num_bins, const std::uint32_t samples)
{
    const double expected = static_cast<double>(samples) / static_cast<double>(num_bins);

    for(std::uint32_t i = 0; i < num_bins; ++i)
    {
        const double deviation = std::fabs(static_cast<double>(bins[i]) - expected) / expected;

        if(deviation > 0.05)
            return false;
    }

    return true;
}

constexpr std::uint64_t large_high = (1ULL << 63) + 12345ULL;

} // namespace

/* Seeds */

STDROMANO_TEST_CASE(random_seed_u32_nonzero)
{
    bool any_nonzero = false;

    for(std::int32_t i = 0; i < 32 && !any_nonzero; ++i)
        any_nonzero = stdromano::random_seed_u32() != 0;

    STDROMANO_CHECK(any_nonzero);
}

STDROMANO_TEST_CASE(random_seed_u64_nonzero)
{
    bool any_nonzero = false;

    for(std::int32_t i = 0; i < 32 && !any_nonzero; ++i)
        any_nonzero = stdromano::random_seed_u64() != 0;

    STDROMANO_CHECK(any_nonzero);
}

STDROMANO_TEST_CASE(random_seed_u32_uniqueness)
{
    stdromano::HashSet<std::uint32_t> seeds;

    constexpr std::uint32_t count = 256;

    for(std::uint32_t i = 0; i < count; ++i)
        seeds.insert(stdromano::random_seed_u32());

    STDROMANO_CHECK(seeds.size() >= static_cast<std::size_t>(count - 1));
}

STDROMANO_TEST_CASE(random_seed_u64_uniqueness)
{
    stdromano::HashSet<std::uint64_t> seeds;

    constexpr std::uint32_t count = 256;

    for(std::uint32_t i = 0; i < count; ++i)
        seeds.insert(stdromano::random_seed_u64());

    STDROMANO_CHECK_EQ(seeds.size(), static_cast<std::size_t>(count));
}

STDROMANO_TEST_CASE(random_seed_u64_uses_high_bits)
{
    bool any_high_bits = false;

    for(std::int32_t i = 0; i < 32 && !any_high_bits; ++i)
        any_high_bits = (stdromano::random_seed_u64() >> 32) != 0;

    STDROMANO_CHECK(any_high_bits);
}

/* Internals */

STDROMANO_TEST_CASE(mul_u64_wide_known_values)
{
    std::uint64_t low;

    STDROMANO_CHECK_EQ(stdromano::detail::mul_u64_wide(0, 12345, low), 0ULL);
    STDROMANO_CHECK_EQ(low, 0ULL);

    STDROMANO_CHECK_EQ(stdromano::detail::mul_u64_wide(1ULL << 32, 1ULL << 32, low), 1ULL);
    STDROMANO_CHECK_EQ(low, 0ULL);

    STDROMANO_CHECK_EQ(stdromano::detail::mul_u64_wide(~0ULL, ~0ULL, low), 0xFFFFFFFFFFFFFFFEULL);
    STDROMANO_CHECK_EQ(low, 1ULL);

    STDROMANO_CHECK_EQ(stdromano::detail::mul_u64_wide(0x123456789ABCDEF0ULL, 0x0FEDCBA987654321ULL, low), 0x0121FA00AD77D742ULL);
    STDROMANO_CHECK_EQ(low, 0x2236D88FE5618CF0ULL);
}

STDROMANO_TEST_CASE(float_mapping_bounds)
{
    STDROMANO_CHECK_EQ(stdromano::detail::u32_to_float_01(0U), 0.0f);
    STDROMANO_CHECK_LT(stdromano::detail::u32_to_float_01(0xFFFFFFFFU), 1.0f);
    STDROMANO_CHECK_EQ(stdromano::detail::u64_to_double_01(0ULL), 0.0);
    STDROMANO_CHECK_LT(stdromano::detail::u64_to_double_01(~0ULL), 1.0);
}

STDROMANO_TEST_CASE(unit_to_range_never_reaches_high)
{
    const float almost_one = stdromano::detail::u32_to_float_01(0xFFFFFFFFU);

    STDROMANO_CHECK_LT(stdromano::detail::unit_to_range(almost_one, 1.0f, 1.0000001f), 1.0000001f);
    STDROMANO_CHECK_LT(stdromano::detail::unit_to_range(almost_one, -1e30f, 1e30f), 1e30f);
    STDROMANO_CHECK_EQ(stdromano::detail::unit_to_range(0.5f, 3.0f, 3.0f), 3.0f);
    STDROMANO_CHECK_EQ(stdromano::detail::unit_to_range(0.5f, 3.0f, 1.0f), 3.0f);
}

/* Hash primitives */

STDROMANO_TEST_CASE(splitmix_reference_values)
{
    std::uint64_t state = 1234567;

    STDROMANO_CHECK_EQ(stdromano::next_random_u64(state), 6457827717110365317ULL);
    STDROMANO_CHECK_EQ(stdromano::next_random_u64(state), 3203168211198807973ULL);
    STDROMANO_CHECK_EQ(stdromano::next_random_u64(state), 9817491932198370423ULL);
    STDROMANO_CHECK_EQ(stdromano::next_random_u64(state), 4593380528125082431ULL);
    STDROMANO_CHECK_EQ(stdromano::next_random_u64(state), 16408922859458223821ULL);
}

STDROMANO_TEST_CASE(wang_hash_nonzero)
{
    for(std::uint32_t i = 0; i < 10000; ++i)
        STDROMANO_CHECK(stdromano::wang_hash(i) >= 1U);
}

STDROMANO_TEST_CASE(wang_hash_avalanche)
{
    std::uint32_t diff = stdromano::wang_hash(0) ^ stdromano::wang_hash(1);

    STDROMANO_CHECK_GE(stdromano::popcount_u32(diff), 4ULL);
}

STDROMANO_TEST_CASE(xorshift32_nonzero_propagation)
{
    std::uint32_t state = 1U;

    for(std::uint32_t i = 0; i < 10000; ++i)
    {
        state = stdromano::xorshift32(state);
        STDROMANO_CHECK(state != 0U);
    }
}

STDROMANO_TEST_CASE(xorshift32_no_short_cycle)
{
    stdromano::HashSet<std::uint32_t> seen;
    std::uint32_t state = 42U;

    for(std::uint32_t i = 0; i < 1000; ++i)
    {
        state = stdromano::xorshift32(state);
        seen.insert(state);
    }

    STDROMANO_CHECK_EQ(seen.size(), static_cast<std::size_t>(1000));
}

STDROMANO_TEST_CASE(pcg_hash_is_injective_on_sample)
{
    stdromano::HashSet<std::uint32_t> seen;

    for(std::uint32_t i = 0; i < 10000; ++i)
        seen.insert(stdromano::pcg_hash(i));

    STDROMANO_CHECK_EQ(seen.size(), static_cast<std::size_t>(10000));
}

/* u32 seed overloads that don't go through the u64 stream */

STDROMANO_TEST_CASE(u32_seed_hashes)
{
    for(std::uint32_t i = 0; i < 1000; ++i)
    {
        STDROMANO_CHECK_EQ(stdromano::random_u32(i), stdromano::detail::lowbias32(i));
        STDROMANO_CHECK_EQ(stdromano::pcg_random_u32(i), stdromano::pcg_hash(i));
        STDROMANO_CHECK_EQ(stdromano::pcg_random_float(i), stdromano::detail::u32_to_float_01(stdromano::pcg_hash(i)));
        STDROMANO_CHECK_EQ(stdromano::random_u64(i), stdromano::random_u64(static_cast<std::uint64_t>(i)));
        STDROMANO_CHECK_EQ(stdromano::pcg_random_double(i), stdromano::pcg_random_double(static_cast<std::uint64_t>(i)));
    }
}

STDROMANO_TEST_CASE(u32_seed_floats_distribution)
{
    std::uint32_t splitmix_bins[10] = {};
    std::uint32_t pcg_bins[10] = {};

    constexpr std::uint32_t n = 100000;

    for(std::uint32_t i = 0; i < n; ++i)
    {
        const float splitmix = stdromano::random_float(i);
        const float pcg = stdromano::pcg_random_float(i);

        STDROMANO_CHECK(splitmix >= 0.0f && splitmix < 1.0f);
        STDROMANO_CHECK(pcg >= 0.0f && pcg < 1.0f);

        ++splitmix_bins[std::min(static_cast<std::uint32_t>(splitmix * 10.0f), 9U)];
        ++pcg_bins[std::min(static_cast<std::uint32_t>(pcg * 10.0f), 9U)];
    }

    STDROMANO_CHECK(bins_are_uniform(splitmix_bins, 10, n));
    STDROMANO_CHECK(bins_are_uniform(pcg_bins, 10, n));
}

/* Generic properties, run on every generator */

STDROMANO_TEST_CASE(stateless_matches_first_stream_value)
{
    for_each_generator([](auto generator) {
        using G = decltype(generator);

        for(std::uint64_t seed = 0; seed < 1000; ++seed)
        {
            const std::uint64_t s = seed * 0x9E3779B97F4A7C15ULL;

            typename G::State a = G::make(s);
            typename G::State b = G::make(s);
            typename G::State c = G::make(s);
            typename G::State d = G::make(s);

            STDROMANO_CHECK_MSG(G::random_u32(s) == G::next_u32(a), G::name);
            STDROMANO_CHECK_MSG(G::random_u64(s) == G::next_u64(b), G::name);
            STDROMANO_CHECK_MSG(G::random_float(s) == G::next_float(c), G::name);
            STDROMANO_CHECK_MSG(G::random_double(s) == G::next_double(d), G::name);

            typename G::State e = G::make(s);
            typename G::State f = G::make(s);
            typename G::State g = G::make(s);
            typename G::State h = G::make(s);

            STDROMANO_CHECK_MSG(G::random_u32_in_range(s, 3, 17) == G::next_u32_in_range(e, 3, 17), G::name);
            STDROMANO_CHECK_MSG(G::random_u64_in_range(s, 3, large_high) == G::next_u64_in_range(f, 3, large_high), G::name);
            STDROMANO_CHECK_MSG(G::random_float_in_range(s, -2.0f, 5.0f) == G::next_float_in_range(g, -2.0f, 5.0f), G::name);
            STDROMANO_CHECK_MSG(G::random_double_in_range(s, -2.0, 5.0) == G::next_double_in_range(h, -2.0, 5.0), G::name);
        }
    });
}

STDROMANO_TEST_CASE(stream_deterministic)
{
    for_each_generator([](auto generator) {
        using G = decltype(generator);

        typename G::State a = G::make(7);
        typename G::State b = G::make(7);

        for(std::uint32_t i = 0; i < 1000; ++i)
            STDROMANO_CHECK_MSG(G::next_u64(a) == G::next_u64(b), G::name);
    });
}

STDROMANO_TEST_CASE(stream_different_seeds)
{
    for_each_generator([](auto generator) {
        using G = decltype(generator);

        typename G::State a = G::make(1);
        typename G::State b = G::make(2);

        std::uint32_t equal = 0;

        for(std::uint32_t i = 0; i < 1000; ++i)
            equal += G::next_u64(a) == G::next_u64(b);

        STDROMANO_CHECK_MSG(equal == 0, G::name);
        STDROMANO_CHECK_MSG(G::random_u64(1) != G::random_u64(2), G::name);
    });
}

STDROMANO_TEST_CASE(stream_u32_uniqueness)
{
    for_each_generator([](auto generator) {
        using G = decltype(generator);

        typename G::State state = G::make(42);
        stdromano::HashSet<std::uint32_t> seen;

        for(std::uint32_t i = 0; i < 10000; ++i)
            seen.insert(G::next_u32(state));

        STDROMANO_CHECK_MSG(seen.size() > 9990, G::name);
    });
}

STDROMANO_TEST_CASE(stream_u64_uniqueness)
{
    for_each_generator([](auto generator) {
        using G = decltype(generator);

        typename G::State state = G::make(42);
        stdromano::HashSet<std::uint64_t> seen;

        for(std::uint32_t i = 0; i < 10000; ++i)
            seen.insert(G::next_u64(state));

        STDROMANO_CHECK_MSG(seen.size() == 10000, G::name);
    });
}

STDROMANO_TEST_CASE(stream_u64_uses_both_halves)
{
    for_each_generator([](auto generator) {
        using G = decltype(generator);

        typename G::State state = G::make(3);

        std::uint64_t or_bits = 0;
        std::uint64_t and_bits = ~0ULL;

        for(std::uint32_t i = 0; i < 1000; ++i)
        {
            const std::uint64_t value = G::next_u64(state);
            or_bits |= value;
            and_bits &= value;
        }

        STDROMANO_CHECK_MSG(or_bits == ~0ULL, G::name);
        STDROMANO_CHECK_MSG(and_bits == 0ULL, G::name);
    });
}

STDROMANO_TEST_CASE(stream_float_double_range_and_mean)
{
    for_each_generator([](auto generator) {
        using G = decltype(generator);

        typename G::State state = G::make(99);

        constexpr std::uint32_t n = 100000;

        double float_sum = 0.0;
        double double_sum = 0.0;

        for(std::uint32_t i = 0; i < n; ++i)
        {
            const float f = G::next_float(state);
            const double d = G::next_double(state);

            STDROMANO_CHECK_MSG(f >= 0.0f && f < 1.0f, G::name);
            STDROMANO_CHECK_MSG(d >= 0.0 && d < 1.0, G::name);

            float_sum += f;
            double_sum += d;
        }

        STDROMANO_CHECK_MSG(std::fabs(float_sum / n - 0.5) < 0.01, G::name);
        STDROMANO_CHECK_MSG(std::fabs(double_sum / n - 0.5) < 0.01, G::name);
    });
}

STDROMANO_TEST_CASE(u32_in_range_bounds_and_uniformity)
{
    for_each_generator([](auto generator) {
        using G = decltype(generator);

        typename G::State state = G::make(5);

        constexpr std::uint32_t low = 10;
        constexpr std::uint32_t high = 17;
        constexpr std::uint32_t n = 70000;

        std::uint32_t bins[high - low] = {};

        for(std::uint32_t i = 0; i < n; ++i)
        {
            const std::uint32_t value = G::next_u32_in_range(state, low, high);

            STDROMANO_CHECK_MSG(value >= low && value < high, G::name);

            if(value >= low && value < high)
                ++bins[value - low];
        }

        STDROMANO_CHECK_MSG(bins_are_uniform(bins, high - low, n), G::name);
    });
}

STDROMANO_TEST_CASE(u64_in_range_bounds_and_uniformity)
{
    for_each_generator([](auto generator) {
        using G = decltype(generator);

        typename G::State state = G::make(6);

        constexpr std::uint64_t low = 1000;
        constexpr std::uint64_t high = 1007;
        constexpr std::uint32_t n = 70000;

        std::uint32_t bins[high - low] = {};

        for(std::uint32_t i = 0; i < n; ++i)
        {
            const std::uint64_t value = G::next_u64_in_range(state, low, high);

            STDROMANO_CHECK_MSG(value >= low && value < high, G::name);

            if(value >= low && value < high)
                ++bins[value - low];
        }

        STDROMANO_CHECK_MSG(bins_are_uniform(bins, static_cast<std::uint32_t>(high - low), n), G::name);
    });
}

STDROMANO_TEST_CASE(in_range_large_spans)
{
    for_each_generator([](auto generator) {
        using G = decltype(generator);

        typename G::State state = G::make(8);

        std::uint32_t u32_upper_half = 0;
        std::uint32_t u64_upper_half = 0;

        constexpr std::uint32_t n = 10000;

        for(std::uint32_t i = 0; i < n; ++i)
        {
            const std::uint32_t a = G::next_u32_in_range(state, 1, 0xFFFFFFFFU);
            const std::uint64_t b = G::next_u64_in_range(state, 1, large_high);

            STDROMANO_CHECK_MSG(a >= 1 && a < 0xFFFFFFFFU, G::name);
            STDROMANO_CHECK_MSG(b >= 1 && b < large_high, G::name);

            u32_upper_half += a >= 0x80000000U;
            u64_upper_half += b >= large_high / 2;
        }

        STDROMANO_CHECK_MSG(u32_upper_half > n * 45 / 100 && u32_upper_half < n * 55 / 100, G::name);
        STDROMANO_CHECK_MSG(u64_upper_half > n * 45 / 100 && u64_upper_half < n * 55 / 100, G::name);
    });
}

STDROMANO_TEST_CASE(in_range_degenerate_ranges)
{
    for_each_generator([](auto generator) {
        using G = decltype(generator);

        typename G::State state = G::make(9);

        for(std::uint32_t i = 0; i < 100; ++i)
        {
            STDROMANO_CHECK_MSG(G::next_u32_in_range(state, 5, 6) == 5, G::name);
            STDROMANO_CHECK_MSG(G::next_u32_in_range(state, 5, 5) == 5, G::name);
            STDROMANO_CHECK_MSG(G::next_u32_in_range(state, 9, 2) == 9, G::name);

            STDROMANO_CHECK_MSG(G::next_u64_in_range(state, 5, 6) == 5, G::name);
            STDROMANO_CHECK_MSG(G::next_u64_in_range(state, 5, 5) == 5, G::name);
            STDROMANO_CHECK_MSG(G::next_u64_in_range(state, 9, 2) == 9, G::name);

            STDROMANO_CHECK_MSG(G::next_float_in_range(state, 2.0f, 2.0f) == 2.0f, G::name);
            STDROMANO_CHECK_MSG(G::next_float_in_range(state, 3.0f, 1.0f) == 3.0f, G::name);

            STDROMANO_CHECK_MSG(G::next_double_in_range(state, 2.0, 2.0) == 2.0, G::name);
            STDROMANO_CHECK_MSG(G::next_double_in_range(state, 3.0, 1.0) == 3.0, G::name);
        }
    });
}

STDROMANO_TEST_CASE(in_range_coverage)
{
    for_each_generator([](auto generator) {
        using G = decltype(generator);

        typename G::State state = G::make(10);

        stdromano::HashSet<std::uint32_t> seen;

        for(std::uint32_t i = 0; i < 10000; ++i)
            seen.insert(G::next_u32_in_range(state, 0, 5));

        STDROMANO_CHECK_MSG(seen.size() == 5, G::name);
    });
}

STDROMANO_TEST_CASE(float_double_in_range_bounds)
{
    for_each_generator([](auto generator) {
        using G = decltype(generator);

        typename G::State state = G::make(11);

        float float_min = 1e30f;
        float float_max = -1e30f;
        double double_min = 1e300;
        double double_max = -1e300;

        for(std::uint32_t i = 0; i < 100000; ++i)
        {
            const float f = G::next_float_in_range(state, -3.5f, 2.25f);
            const double d = G::next_double_in_range(state, -3.5, 2.25);

            STDROMANO_CHECK_MSG(f >= -3.5f && f < 2.25f, G::name);
            STDROMANO_CHECK_MSG(d >= -3.5 && d < 2.25, G::name);

            float_min = std::min(float_min, f);
            float_max = std::max(float_max, f);
            double_min = std::min(double_min, d);
            double_max = std::max(double_max, d);
        }

        STDROMANO_CHECK_MSG(float_min < -3.4f && float_max > 2.15f, G::name);
        STDROMANO_CHECK_MSG(double_min < -3.4 && double_max > 2.15, G::name);
    });
}

/* Thread-safe generators */

STDROMANO_TEST_CASE(ts_ranges)
{
    for_each_generator([](auto generator) {
        using G = decltype(generator);

        for(std::uint32_t i = 0; i < 10000; ++i)
        {
            const float f = G::ts_float();
            const double d = G::ts_double();

            STDROMANO_CHECK_MSG(f >= 0.0f && f < 1.0f, G::name);
            STDROMANO_CHECK_MSG(d >= 0.0 && d < 1.0, G::name);

            const std::uint32_t a = G::ts_u32_in_range(100, 200);
            const std::uint64_t b = G::ts_u64_in_range(100, large_high);
            const float c = G::ts_float_in_range(-1.0f, 1.0f);
            const double e = G::ts_double_in_range(-1.0, 1.0);

            STDROMANO_CHECK_MSG(a >= 100 && a < 200, G::name);
            STDROMANO_CHECK_MSG(b >= 100 && b < large_high, G::name);
            STDROMANO_CHECK_MSG(c >= -1.0f && c < 1.0f, G::name);
            STDROMANO_CHECK_MSG(e >= -1.0 && e < 1.0, G::name);
        }
    });
}

STDROMANO_TEST_CASE(ts_uniqueness)
{
    for_each_generator([](auto generator) {
        using G = decltype(generator);

        stdromano::HashSet<std::uint32_t> seen_u32;
        stdromano::HashSet<std::uint64_t> seen_u64;

        for(std::uint32_t i = 0; i < 10000; ++i)
        {
            seen_u32.insert(G::ts_u32());
            seen_u64.insert(G::ts_u64());
        }

        STDROMANO_CHECK_MSG(seen_u32.size() > 9990, G::name);
        STDROMANO_CHECK_MSG(seen_u64.size() == 10000, G::name);
    });
}

STDROMANO_TEST_CASE(ts_thread_safety)
{
    for_each_generator([](auto generator) {
        using G = decltype(generator);

        constexpr std::uint32_t num_threads = 8;
        constexpr std::uint32_t per_thread = 10000;

        stdromano::Vector<stdromano::Vector<std::uint64_t>> results(num_threads);
        stdromano::ThreadPoolWaiter waiter;

        for(std::uint32_t i = 0; i < num_threads; i++)
        {
            stdromano::global_threadpool().add_work(
                [&results, i, per_thread]() {
                    for(std::uint32_t j = 0; j < per_thread; j++)
                        results[i].push_back(G::ts_u64());
                },
                &waiter);
        }

        waiter.wait();

        stdromano::HashSet<std::uint64_t> all;

        for(const auto& values : results)
            for(const std::uint64_t value : values)
                all.insert(value);

        const std::size_t total = num_threads * per_thread;

        STDROMANO_CHECK_MSG(all.size() > total * 99 / 100, G::name);
    });
}

/* Fuzzing */

STDROMANO_TEST_CASE(fuzz_generators_stay_in_range)
{
    const auto report = stdromano::fuzz::run_property(fixtures::options("random_ranges", 5000), [](stdromano::fuzz::Source& source) {
        const std::uint64_t seed = source.integer<std::uint64_t>();

        const std::uint32_t a32 = source.integer<std::uint32_t>();
        const std::uint32_t b32 = source.integer<std::uint32_t>();
        const std::uint64_t a64 = source.integer<std::uint64_t>();
        const std::uint64_t b64 = source.integer<std::uint64_t>();

        const std::uint32_t low32 = std::min(a32, b32);
        const std::uint32_t high32 = std::max(a32, b32);
        const std::uint64_t low64 = std::min(a64, b64);
        const std::uint64_t high64 = std::max(a64, b64);

        bool ok = true;

        for_each_generator([&](auto generator) {
            using G = decltype(generator);

            const std::uint32_t value32 = G::random_u32_in_range(seed, low32, high32);
            const std::uint64_t value64 = G::random_u64_in_range(seed, low64, high64);
            const float f = G::random_float(seed);
            const double d = G::random_double(seed);

            ok &= low32 == high32 ? value32 == low32 : (value32 >= low32 && value32 < high32);
            ok &= low64 == high64 ? value64 == low64 : (value64 >= low64 && value64 < high64);
            ok &= f >= 0.0f && f < 1.0f;
            ok &= d >= 0.0 && d < 1.0;
            ok &= G::random_u64(seed) == G::random_u64(seed);
        });

        STDROMANO_FUZZ_CHECK(ok);

        return true;
    });

    TESTS_REQUIRE_PROPERTY(report);
}

STDROMANO_TEST_MAIN()