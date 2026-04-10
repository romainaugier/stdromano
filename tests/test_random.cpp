// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - Present Romain Augier
// All rights reserved.

#include "stdromano/random.hpp"
#include "stdromano/hashset.hpp"
#include "stdromano/vector.hpp"
#include "stdromano/threading.hpp"

#include "test.hpp"

INIT_TEST_OBJECT

// random_seed

TEST_CASE(test_random_seed_nonzero)
{
    // A zero seed is technically possible but astronomically unlikely
    // Run multiple times to be safe
    bool any_nonzero = false;

    for(std::int32_t i = 0; i < 32; ++i)
    {
        if(stdromano::random_seed() != 0)
        {
            any_nonzero = true;
            break;
        }
    }

    ASSERT(any_nonzero);
}

TEST_CASE(test_random_seed_uniqueness)
{
    stdromano::HashSet<std::uint32_t> seeds;

    constexpr std::uint32_t count = 256;

    for(std::uint32_t i = 0; i < count; ++i)
        seeds.insert(stdromano::random_seed());

    // With a proper CSPRNG, collisions in 256 draws from a 32-bit space are extremely unlikely
    ASSERT(seeds.size() >= static_cast<size_t>(count - 1));
}

// pcg_float

TEST_CASE(test_pcg_float_range)
{
    for(std::uint32_t i = 0; i < 10000; ++i)
    {
        const float val = stdromano::pcg_float(i);
        ASSERT(val >= 0.0f);
        ASSERT(val <= 1.0f);
    }
}

TEST_CASE(test_pcg_float_deterministic)
{
    for(std::uint32_t i = 0; i < 1000; ++i)
    {
        const float a = stdromano::pcg_float(i);
        const float b = stdromano::pcg_float(i);
        ASSERT(a == b);
    }
}

TEST_CASE(test_pcg_float_distribution)
{
    // Very rough uniformity check: split [0,1) into 10 bins and ensure none are empty
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

    // Each bin should have roughly n/10 = 10000 entries; assert at least 5%
    for(std::uint32_t i = 0; i < 10; ++i)
        ASSERT(bins[i] > n / 20);
}

// wang_hash

TEST_CASE(test_wang_hash_nonzero)
{
    // wang_hash should always return >= 1
    for(std::uint32_t i = 0; i < 10000; ++i)
        ASSERT(stdromano::wang_hash(i) >= 1u);
}

TEST_CASE(test_wang_hash_deterministic)
{
    for(std::uint32_t i = 0; i < 1000; ++i)
        ASSERT(stdromano::wang_hash(i) == stdromano::wang_hash(i));
}

TEST_CASE(test_wang_hash_avalanche)
{
    // Nearby inputs should produce very different outputs
    const std::uint32_t h0 = stdromano::wang_hash(0);
    const std::uint32_t h1 = stdromano::wang_hash(1);
    ASSERT(h0 != h1);

    // Check that at least several bits differ (hamming distance)
    std::uint32_t diff = h0 ^ h1;
    std::int32_t bit_diff = 0;

    while(diff)
    {
        bit_diff += diff & 1u;
        diff >>= 1u;
    }
    //
    // A good hash should flip roughly half the bits; require at least 4
    ASSERT(bit_diff >= 4);
}

// xorshift32

TEST_CASE(test_xorshift32_nonzero_propagation)
{
    // xorshift32 with a nonzero input should produce nonzero output

    std::uint32_t state = 1u;

    for(std::uint32_t i = 0; i < 10000; ++i)
    {
        state = stdromano::xorshift32(state);
        ASSERT(state != 0u);
    }
}

TEST_CASE(test_xorshift32_no_short_cycle)
{
    // Ensure no trivially short cycle (at least 1000 unique values)
    stdromano::HashSet<std::uint32_t> seen;
    std::uint32_t state = 42u;

    for(std::uint32_t i = 0; i < 1000; ++i)
    {
        state = stdromano::xorshift32(state);
        seen.insert(state);
    }

    ASSERT(seen.size() == 1000);
}

// wang_hash_float

TEST_CASE(test_wang_hash_float_range)
{
    for(std::uint32_t i = 0; i < 10000; ++i)
    {
        const float val = stdromano::wang_hash_float(i);
        ASSERT(val >= 0.0f);
        ASSERT(val <= 1.0f);
    }
}

TEST_CASE(test_wang_hash_float_deterministic)
{
    for(std::uint32_t i = 0; i < 1000; ++i)
    {
        ASSERT(stdromano::wang_hash_float(i) == stdromano::wang_hash_float(i));
    }
}

// random_int_range

TEST_CASE(test_random_int_range_bounds)
{
    constexpr std::uint32_t low = 10;
    constexpr std::uint32_t high = 50;

    for(std::uint32_t i = 0; i < 10000; ++i)
    {
        const std::uint32_t val = stdromano::random_int_range(i, low, high);
        ASSERT(val >= low);
        ASSERT(val < high);
    }
}

TEST_CASE(test_random_int_range_single_value)
{
    // When low == high - 1, the only valid result is low
    for(std::uint32_t i = 0; i < 100; ++i)
    {
        const std::uint32_t val = stdromano::random_int_range(i, 5, 6);
        ASSERT(val == 5);
    }
}

TEST_CASE(test_random_int_range_coverage)
{
    // Ensure all values in a small range are eventually hit
    constexpr std::uint32_t low = 0;
    constexpr std::uint32_t high = 5;

    stdromano::HashSet<std::uint32_t> seen;

    for(std::uint32_t i = 0; i < 10000; ++i)
        seen.insert(stdromano::random_int_range(i, low, high));

    for(std::uint32_t v = low; v < high; ++v)
        ASSERT(seen.count(v) > 0);
}

// xoshiro

TEST_CASE(test_xoshiro_random_uint64_deterministic)
{
    const std::uint64_t a = stdromano::xoshiro_random_uint64(12345);
    const std::uint64_t b = stdromano::xoshiro_random_uint64(12345);
    ASSERT(a == b);
}

TEST_CASE(test_xoshiro_random_uint64_different_seeds)
{
    const std::uint64_t a = stdromano::xoshiro_random_uint64(1);
    const std::uint64_t b = stdromano::xoshiro_random_uint64(2);
    ASSERT(a != b);
}

TEST_CASE(test_xoshiro_next_uint64_sequence)
{
    stdromano::seed_xoshiro(42);
    stdromano::HashSet<std::uint64_t> seen;

    for(std::uint32_t i = 0; i < 1000; ++i)
        seen.insert(stdromano::xoshiro_next_uint64());

    ASSERT(seen.size() == 1000);
}

TEST_CASE(test_xoshiro_next_float_range)
{
    stdromano::seed_xoshiro(99);

    for(std::uint64_t i = 0; i < 10000; ++i)
    {
        const float val = stdromano::xoshiro_next_float();
        ASSERT(val >= 0.0f);
        ASSERT(val <= 1.0f);
    }
}

TEST_CASE(test_xoshiro_seeding_resets_state)
{
    stdromano::seed_xoshiro(7);
    const std::uint64_t first_a = stdromano::xoshiro_next_uint64();
    const std::uint64_t second_a = stdromano::xoshiro_next_uint64();

    stdromano::seed_xoshiro(7);
    const std::uint64_t first_b = stdromano::xoshiro_next_uint64();
    const std::uint64_t second_b = stdromano::xoshiro_next_uint64();

    ASSERT(first_a == first_b);
    ASSERT(second_a == second_b);
}

// Thread-safe generators

TEST_CASE(test_next_random_uint32_uniqueness)
{
    stdromano::HashSet<std::uint32_t> seen;

    for(std::uint32_t i = 0; i < 10000; ++i)
        seen.insert(stdromano::next_random_uint32());

    // Allow a tiny number of collisions in 2^32 space
    ASSERT(seen.size() > 9900);
}

TEST_CASE(test_next_random_float_01_range)
{
    for(std::uint32_t i = 0; i < 10000; ++i)
    {
        const float val = stdromano::next_random_float_01();
        ASSERT(val >= 0.0f);
        ASSERT(val <= 1.0f);
    }
}

TEST_CASE(test_next_random_int_range_bounds)
{
    constexpr std::uint32_t low = 100;
    constexpr std::uint32_t high = 200;

    for(std::uint32_t i = 0; i < 10000; ++i)
    {
        const std::uint32_t val = stdromano::next_random_int_range(low, high);
        ASSERT(val >= low);
        ASSERT(val < high);
    }
}

TEST_CASE(test_thread_safety)
{
    // Spawn several threads that each call the atomic generators concurrently
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

    // Collect all values and check for a reasonable level of uniqueness
    stdromano::HashSet<std::uint32_t> all;

    for(const auto& v : results)
        for(std::uint32_t val : v)
            all.insert(val);

    const std::size_t total = num_threads * per_thread;

    // With good hashing, the vast majority should be unique
    ASSERT(all.size() > total * 9 / 10);
}

int main()
{
    TestRunner runner("random");

    // random_seed
    runner.add_test("random_seed_nonzero", test_random_seed_nonzero);
    runner.add_test("random_seed_uniqueness", test_random_seed_uniqueness);

    // pcg_float
    runner.add_test("pcg_float_range", test_pcg_float_range);
    runner.add_test("pcg_float_deterministic", test_pcg_float_deterministic);
    runner.add_test("pcg_float_distribution", test_pcg_float_distribution);

    // wang_hash
    runner.add_test("wang_hash_nonzero", test_wang_hash_nonzero);
    runner.add_test("wang_hash_deterministic", test_wang_hash_deterministic);
    runner.add_test("wang_hash_avalanche", test_wang_hash_avalanche);

    // xorshift32
    runner.add_test("xorshift32_nonzero_propagation", test_xorshift32_nonzero_propagation);
    runner.add_test("xorshift32_no_short_cycle", test_xorshift32_no_short_cycle);

    // wang_hash_float
    runner.add_test("wang_hash_float_range", test_wang_hash_float_range);
    runner.add_test("wang_hash_float_deterministic", test_wang_hash_float_deterministic);

    // random_int_range
    runner.add_test("random_int_range_bounds", test_random_int_range_bounds);
    runner.add_test("random_int_range_single_value", test_random_int_range_single_value);
    runner.add_test("random_int_range_coverage", test_random_int_range_coverage);

    // xoshiro
    runner.add_test("xoshiro_random_uint64_deterministic", test_xoshiro_random_uint64_deterministic);
    runner.add_test("xoshiro_random_uint64_different_seeds", test_xoshiro_random_uint64_different_seeds);
    runner.add_test("xoshiro_next_uint64_sequence", test_xoshiro_next_uint64_sequence);
    runner.add_test("xoshiro_next_float_range", test_xoshiro_next_float_range);
    runner.add_test("xoshiro_seeding_resets_state", test_xoshiro_seeding_resets_state);

    // Thread-safe generators
    runner.add_test("next_random_uint32_uniqueness", test_next_random_uint32_uniqueness);
    runner.add_test("next_random_float_01_range", test_next_random_float_01_range);
    runner.add_test("next_random_int_range_bounds", test_next_random_int_range_bounds);
    runner.add_test("thread_safety", test_thread_safety);

    runner.run_all();

    return 0;
}
