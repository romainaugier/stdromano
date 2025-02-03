// SPDX-License-Identifier: BSD-3-Clause 
// Copyright (c) 2025 - Present Romain Augier 
// All rights reserved. 

#pragma once

#if !defined(__STDROMANO_RANDOM)
#define __STDROMANO_RANDOM

#include "stdromano/stdromano.h"

#include <atomic>

STDROMANO_NAMESPACE_BEGIN

/* Very fast pseudo random generator with a good distribution */

STDROMANO_FORCE_INLINE float pcg_float(unsigned int state) noexcept
{
    constexpr uint32_t tofloat = 0x2f800004u;

    const unsigned int state2 = state * 747796405u + 2891336453u;
    const unsigned int word = ((state2 >> ((state2 >> 28u) + 4u)) ^ state2) * 277803737u;
    state = (word >> 22u) ^ word;

    return static_cast<float>(state) * reinterpret_cast<const float&>(tofloat);
}

STDROMANO_FORCE_INLINE uint32_t wang_hash(uint32_t seed) noexcept
{
    seed = (seed ^ 61u) ^ (seed >> 16u);
    seed *= 9u;
    seed = seed ^ (seed >> 4u);
    seed *= 0x27d4eb2du;
    seed = seed ^ (seed >> 15u);
    return 1u + seed;
}

STDROMANO_FORCE_INLINE uint32_t xorshift32(uint32_t state) noexcept
{
    state ^= state << 13u;
    state ^= state >> 17u;
    state ^= state << 5u;
    return state;
}

STDROMANO_FORCE_INLINE float wang_hash_float(uint32_t state) noexcept
{
    constexpr uint32_t tofloat = 0x2f800004u;

    const uint32_t x = xorshift32(wang_hash(state));

    return static_cast<float>(x) * reinterpret_cast<const float&>(tofloat);
}

STDROMANO_FORCE_INLINE uint32_t random_int_range(const uint32_t state, const uint32_t low, const uint32_t high) noexcept
{
    return static_cast<uint32_t>(wang_hash_float(state) * (static_cast<float>(high) - static_cast<float>(low))) + low;
}

/* Thread safe random generators */

static std::atomic<uint32_t> _state;

STDROMANO_FORCE_INLINE uint32_t next_random_uint32() noexcept
{
    const uint32_t state = ++_state;

    return xorshift32(wang_hash(state));
}

STDROMANO_FORCE_INLINE float next_random_float_01() noexcept
{
    const uint32_t state = ++_state;

    return wang_hash_float(state);
}

STDROMANO_FORCE_INLINE uint32_t next_random_int_range(const uint32_t low, const uint32_t high) noexcept
{
    const uint32_t state = ++_state;

    return random_int_range(state, low, high);
}

STDROMANO_NAMESPACE_END

#endif /* !defined(__STDROMANO_RANDOM) */