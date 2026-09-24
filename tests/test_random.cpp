// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - Present Romain Augier
// All rights reserved.

#include "stdromano/random.hpp"
#include "stdromano/hashset.hpp"
#include "stdromano/vector.hpp"
#include "stdromano/threading.hpp"

#include "fixtures.hpp"

#include <algorithm>


STDROMANO_TEST_CASE(random_seed_nonzero)
{
    bool any_nonzero = false;

    for(std::int32_t i = 0; i < 32; ++i)
    {
        if(stdromano::random_seed() != 0)
        {
            any_nonzero = true;
            break;
        }
    }

    STDROMANO_CHECK(any_nonzero);
}

STDROMANO_TEST_CASE(random_seed_uniqueness)
{
    stdromano::HashSet<std::uint32_t> seeds;

    constexpr std::uint32_t count = 256;

    for(std::uint32_t i = 0; i < count; ++i)
        seeds.insert(stdromano::random_seed());

    STDROMANO_CHECK(seeds.size() >= static_cast<size_t>(count - 1));
}

STDROMANO_TEST_CASE(pcg_float_range)
{
    for(std::uint32_t i = 0; i < 10000; ++i)
    {
        const float val = stdromano::pcg_float(i);
        STDROMANO_CHECK(val >= 0.0f);
        STDROMANO_CHECK(val < 1.0f);
    }
}

STDROMANO_TEST_CASE(pcg_float_deterministic)
{
    for(std::uint32_t i = 0; i < 1000; ++i)
    {
        const float a = stdromano::pcg_float(i);
        const float b = stdromano::pcg_float(i);
        STDROMANO_CHECK(a == b);
    }
}

STDROMANO_TEST_CASE(pcg_float_distribution)
{
    std::uint32_t bins[10] = {};

    constexpr std::uint32_t n = 100000;

    for(std::uint32_t i = 0; i < n; ++i)
    {
        const float val = stdromano::pcg_float(i);
        std::uint32_t bin = static_cast<std::uint32_t>(val * 10.0f);

        if(bin >= 10)
            bin = 9;

        ++bins[bin];
    }

    for(std::uint32_t i = 0; i < 10; ++i)
        STDROMANO_CHECK(bins[i] > n / 20);
}

STDROMANO_TEST_CASE(wang_hash_nonzero)
{
    for(std::uint32_t i = 0; i < 10000; ++i)
        STDROMANO_CHECK(stdromano::wang_hash(i) >= 1u);
}

STDROMANO_TEST_CASE(wang_hash_deterministic)
{
    for(std::uint32_t i = 0; i < 1000; ++i)
        STDROMANO_CHECK(stdromano::wang_hash(i) == stdromano::wang_hash(i));
}

STDROMANO_TEST_CASE(wang_hash_avalanche)
{
    const std::uint32_t h0 = stdromano::wang_hash(0);
    const std::uint32_t h1 = stdromano::wang_hash(1);
    STDROMANO_CHECK(h0 != h1);

    std::uint32_t diff = h0 ^ h1;
    std::int32_t bit_diff = 0;

    while(diff)
    {
        bit_diff += diff & 1u;
        diff >>= 1u;
    }
    STDROMANO_CHECK(bit_diff >= 4);
}

STDROMANO_TEST_CASE(xorshift32_nonzero_propagation)
{

    std::uint32_t state = 1u;

    for(std::uint32_t i = 0; i < 10000; ++i)
    {
        state = stdromano::xorshift32(state);
        STDROMANO_CHECK(state != 0u);
    }
}

STDROMANO_TEST_CASE(xorshift32_no_short_cycle)
{
    stdromano::HashSet<std::uint32_t> seen;
    std::uint32_t state = 42u;

    for(std::uint32_t i = 0; i < 1000; ++i)
    {
        state = stdromano::xorshift32(state);
        seen.insert(state);
    }

    STDROMANO_CHECK(seen.size() == 1000);
}

STDROMANO_TEST_CASE(wang_hash_float_range)
{
    for(std::uint32_t i = 0; i < 10000; ++i)
    {
        const float val = stdromano::wang_hash_float(i);
        STDROMANO_CHECK(val >= 0.0f);
        STDROMANO_CHECK(val < 1.0f);
    }
}

STDROMANO_TEST_CASE(wang_hash_float_deterministic)
{
    for(std::uint32_t i = 0; i < 1000; ++i)
    {
        STDROMANO_CHECK(stdromano::wang_hash_float(i) == stdromano::wang_hash_float(i));
    }
}

STDROMANO_TEST_CASE(random_int_range_bounds)
{
    constexpr std::uint32_t low = 10;
    constexpr std::uint32_t high = 50;

    for(std::uint32_t i = 0; i < 10000; ++i)
    {
        const std::uint32_t val = stdromano::random_int_range(i, low, high);
        STDROMANO_CHECK(val >= low);
        STDROMANO_CHECK(val < high);
    }
}

STDROMANO_TEST_CASE(random_int_range_single_value)
{
    for(std::uint32_t i = 0; i < 100; ++i)
    {
        const std::uint32_t val = stdromano::random_int_range(i, 5, 6);
        STDROMANO_CHECK(val == 5);
    }
}

STDROMANO_TEST_CASE(random_int_range_coverage)
{
    constexpr std::uint32_t low = 0;
    constexpr std::uint32_t high = 5;

    stdromano::HashSet<std::uint32_t> seen;

    for(std::uint32_t i = 0; i < 10000; ++i)
        seen.insert(stdromano::random_int_range(i, low, high));

    for(std::uint32_t v = low; v < high; ++v)
        STDROMANO_CHECK(seen.count(v) > 0);
}

STDROMANO_TEST_CASE(xoshiro_random_uint64_deterministic)
{
    const std::uint64_t a = stdromano::xoshiro_random_uint64(12345);
    const std::uint64_t b = stdromano::xoshiro_random_uint64(12345);
    STDROMANO_CHECK(a == b);
}

STDROMANO_TEST_CASE(xoshiro_random_uint64_different_seeds)
{
    const std::uint64_t a = stdromano::xoshiro_random_uint64(1);
    const std::uint64_t b = stdromano::xoshiro_random_uint64(2);
    STDROMANO_CHECK(a != b);
}

STDROMANO_TEST_CASE(xoshiro_next_uint64_sequence)
{
    stdromano::seed_xoshiro(42);
    stdromano::HashSet<std::uint64_t> seen;

    for(std::uint32_t i = 0; i < 1000; ++i)
        seen.insert(stdromano::xoshiro_next_uint64());

    STDROMANO_CHECK(seen.size() == 1000);
}

STDROMANO_TEST_CASE(xoshiro_next_float_range)
{
    stdromano::seed_xoshiro(99);

    for(std::uint64_t i = 0; i < 10000; ++i)
    {
        const float val = stdromano::xoshiro_next_float();
        STDROMANO_CHECK(val >= 0.0f);
        STDROMANO_CHECK(val < 1.0f);
    }
}

STDROMANO_TEST_CASE(xoshiro_seeding_resets_state)
{
    stdromano::seed_xoshiro(7);
    const std::uint64_t first_a = stdromano::xoshiro_next_uint64();
    const std::uint64_t second_a = stdromano::xoshiro_next_uint64();

    stdromano::seed_xoshiro(7);
    const std::uint64_t first_b = stdromano::xoshiro_next_uint64();
    const std::uint64_t second_b = stdromano::xoshiro_next_uint64();

    STDROMANO_CHECK(first_a == first_b);
    STDROMANO_CHECK(second_a == second_b);
}

STDROMANO_TEST_CASE(next_random_uint32_uniqueness)
{
    stdromano::HashSet<std::uint32_t> seen;

    for(std::uint32_t i = 0; i < 10000; ++i)
        seen.insert(stdromano::next_random_uint32());

    STDROMANO_CHECK(seen.size() > 9900);
}

STDROMANO_TEST_CASE(next_random_float_01_range)
{
    for(std::uint32_t i = 0; i < 10000; ++i)
    {
        const float val = stdromano::next_random_float_01();
        STDROMANO_CHECK(val >= 0.0f);
        STDROMANO_CHECK(val < 1.0f);
    }
}

STDROMANO_TEST_CASE(next_random_int_range_bounds)
{
    constexpr std::uint32_t low = 100;
    constexpr std::uint32_t high = 200;

    for(std::uint32_t i = 0; i < 10000; ++i)
    {
        const std::uint32_t val = stdromano::next_random_int_range(low, high);
        STDROMANO_CHECK(val >= low);
        STDROMANO_CHECK(val < high);
    }
}

STDROMANO_TEST_CASE(thread_safety)
{
    constexpr std::uint32_t num_threads = 8;
    constexpr std::uint32_t per_thread = 10000;

    stdromano::Vector<stdromano::Vector<std::uint32_t>> results(num_threads);
    stdromano::ThreadPoolWaiter waiter;

    for(std::uint32_t i = 0; i < num_threads; i++)
    {
        stdromano::global_threadpool().add_work([&, i](){
            for(std::uint32_t j = 0; j < per_thread; j++)
                results[i].push_back(stdromano::next_random_uint32());
        }, &waiter);
    }

    waiter.wait();

    stdromano::HashSet<std::uint32_t> all;

    for(const auto& v : results)
        for(std::uint32_t val : v)
            all.insert(val);

    const std::size_t total = num_threads * per_thread;

    STDROMANO_CHECK(all.size() > total * 9 / 10);
}

STDROMANO_TEST_CASE(float_mapping_excludes_one)
{
    STDROMANO_CHECK_LE(stdromano::u32_to_float_01(0xFFFFFFFFu), 1.0f);
    STDROMANO_CHECK_EQ(stdromano::u32_to_float_01(0u), 0.0f);
    STDROMANO_CHECK_EQ(stdromano::u32_to_range(0xFFFFFFFFu, 0, 10), 9u);
    STDROMANO_CHECK_EQ(stdromano::u32_to_range(0u, 3, 10), 3u);
    STDROMANO_CHECK_EQ(stdromano::u32_to_range(123u, 7, 7), 7u);
}

STDROMANO_TEST_CASE(fuzz_generators_stay_in_range)
{
    const auto report = stdromano::fuzz::run_property(fixtures::options("random_ranges", 5000), [](stdromano::fuzz::Source& source) {
        const std::uint32_t state = source.integer<std::uint32_t>();
        const std::uint32_t a = source.integer<std::uint32_t>();
        const std::uint32_t b = source.integer<std::uint32_t>();

        const std::uint32_t low = std::min(a, b);
        const std::uint32_t high = std::max(a, b);

        const float pcg = stdromano::pcg_float(state);
        const float wang = stdromano::wang_hash_float(state);

        STDROMANO_FUZZ_CHECK(pcg >= 0.0f && pcg < 1.0f);
        STDROMANO_FUZZ_CHECK(wang >= 0.0f && wang < 1.0f);

        const std::uint32_t value = stdromano::random_int_range(state, low, high);

        if(low == high)
            STDROMANO_FUZZ_CHECK_EQ(value, low);
        else
            STDROMANO_FUZZ_CHECK(value >= low && value < high);

        STDROMANO_FUZZ_CHECK_EQ(stdromano::xoshiro_random_uint64(state), stdromano::xoshiro_random_uint64(state));

        return true;
    });

    TESTS_REQUIRE_PROPERTY(report);
}

STDROMANO_TEST_MAIN()
