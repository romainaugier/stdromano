// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - Present Romain Augier
// All rights reserved.

#pragma once

#if !defined(__STDROMANO_RANDOM)
#define __STDROMANO_RANDOM

#include "stdromano/bits.hpp"

#include <atomic>

STDROMANO_NAMESPACE_BEGIN

// Cryptographically secure random seed (uses bcrypt on Windows, urandom on Linux)
STDROMANO_API std::uint32_t random_seed() noexcept;

/* Very fast pseudo random generator with a good distribution */

STDROMANO_FORCE_INLINE float u32_to_float_01(const std::uint32_t x) noexcept
{
    return static_cast<float>(x >> 8) * (1.0f / 16777216.0f);
}

STDROMANO_FORCE_INLINE std::uint32_t u32_to_range(const std::uint32_t x,
                                                  const std::uint32_t low,
                                                  const std::uint32_t high) noexcept
{
    if(high <= low)
        return low;

    const std::uint64_t span = static_cast<std::uint64_t>(high - low);

    return low + static_cast<std::uint32_t>((static_cast<std::uint64_t>(x) * span) >> 32u);
}

STDROMANO_FORCE_INLINE float pcg_float(std::uint32_t state) noexcept
{
    const std::uint32_t state2 = state * 747796405u + 2891336453u;
    const std::uint32_t word = ((state2 >> ((state2 >> 28u) + 4u)) ^ state2) * 277803737u;
    state = (word >> 22u) ^ word;

    return u32_to_float_01(state);
}

STDROMANO_FORCE_INLINE std::uint32_t wang_hash(std::uint32_t seed) noexcept
{
    seed = (seed ^ 61u) ^ (seed >> 16u);
    seed *= 9u;
    seed = seed ^ (seed >> 4u);
    seed *= 0x27d4eb2du;
    seed = seed ^ (seed >> 15u);
    return 1u + seed;
}

STDROMANO_FORCE_INLINE std::uint32_t xorshift32(std::uint32_t state) noexcept
{
    state ^= state << 13u;
    state ^= state >> 17u;
    state ^= state << 5u;
    return state;
}

STDROMANO_FORCE_INLINE float wang_hash_float(std::uint32_t state) noexcept
{
    return u32_to_float_01(xorshift32(wang_hash(state)));
}

STDROMANO_FORCE_INLINE std::uint32_t random_int_range(const std::uint32_t state,
                                                      const std::uint32_t low,
                                                      const std::uint32_t high) noexcept
{
    return u32_to_range(xorshift32(wang_hash(state)), low, high);
}

STDROMANO_API std::uint64_t xoshiro_random_uint64(const std::uint64_t seed) noexcept;

STDROMANO_API void seed_xoshiro(std::uint64_t seed) noexcept;

STDROMANO_API std::uint64_t xoshiro_next_uint64() noexcept;

STDROMANO_API float xoshiro_next_float() noexcept;

// Per-thread random generators

static thread_local std::uint32_t _state = random_seed();

STDROMANO_FORCE_INLINE std::uint32_t next_random_uint32() noexcept
{
    const std::uint32_t state = ++_state;

    return xorshift32(wang_hash(state));
}

STDROMANO_FORCE_INLINE float next_random_float_01() noexcept
{
    const std::uint32_t state = ++_state;

    return wang_hash_float(state);
}

STDROMANO_FORCE_INLINE std::uint32_t next_random_int_range(const std::uint32_t low,
                                                           const std::uint32_t high) noexcept
{
    const std::uint32_t state = ++_state;

    return random_int_range(state, low, high);
}

STDROMANO_NAMESPACE_END

#endif /* !defined(__STDROMANO_RANDOM) */
