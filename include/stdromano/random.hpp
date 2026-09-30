// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - Present Romain Augier
// All rights reserved.

#pragma once

#if !defined(__STDROMANO_RANDOM)
#define __STDROMANO_RANDOM

#include "stdromano/bits.hpp"

#include <cmath>

STDROMANO_NAMESPACE_BEGIN

// Cryptographically secure random seeds (bcrypt on Windows, getrandom on Linux, arc4random elsewhere)
STDROMANO_API std::uint32_t random_seed_u32() noexcept;

STDROMANO_API std::uint64_t random_seed_u64() noexcept;

namespace detail {

STDROMANO_FORCE_INLINE std::uint64_t mul_u64_wide(const std::uint64_t a,
                                                  const std::uint64_t b,
                                                  std::uint64_t& low) noexcept
{
#if defined(STDROMANO_MSVC) && defined(STDROMANO_X86_64)
    std::uint64_t high;
    low = _umul128(a, b, &high);
    return high;
#elif defined(STDROMANO_MSVC) && defined(STDROMANO_AARCH64)
    low = a * b;
    return __umulh(a, b);
#elif defined(__SIZEOF_INT128__)
    const __uint128_t product = static_cast<__uint128_t>(a) * static_cast<__uint128_t>(b);
    low = static_cast<std::uint64_t>(product);
    return static_cast<std::uint64_t>(product >> 64);
#else
    const std::uint64_t a_low = a & 0xFFFFFFFFULL;
    const std::uint64_t a_high = a >> 32;
    const std::uint64_t b_low = b & 0xFFFFFFFFULL;
    const std::uint64_t b_high = b >> 32;

    const std::uint64_t low_low = a_low * b_low;
    const std::uint64_t high_low = a_high * b_low;
    const std::uint64_t low_high = a_low * b_high;
    const std::uint64_t high_high = a_high * b_high;

    const std::uint64_t middle = (low_low >> 32) + (high_low & 0xFFFFFFFFULL) + low_high;

    low = (middle << 32) | (low_low & 0xFFFFFFFFULL);
    return high_high + (high_low >> 32) + (middle >> 32);
#endif /* defined(STDROMANO_MSVC) && defined(STDROMANO_X86_64) */
}

STDROMANO_FORCE_INLINE std::uint64_t rotl_u64(const std::uint64_t x, const std::uint32_t k) noexcept
{
    return (x << k) | (x >> ((0U - k) & 63U));
}

STDROMANO_FORCE_INLINE std::uint32_t rotr_u32(const std::uint32_t x, const std::uint32_t k) noexcept
{
    return (x >> k) | (x << ((0U - k) & 31U));
}

STDROMANO_FORCE_INLINE std::uint32_t lowbias32(std::uint32_t x) noexcept
{
    x ^= x >> 16;
    x *= 0x7FEB352DU;
    x ^= x >> 15;
    x *= 0x846CA68BU;
    x ^= x >> 16;
    return x;
}

STDROMANO_FORCE_INLINE std::uint64_t join_u32(const std::uint32_t high, const std::uint32_t low) noexcept
{
    return (static_cast<std::uint64_t>(high) << 32) | static_cast<std::uint64_t>(low);
}

STDROMANO_FORCE_INLINE float u32_to_float_01(const std::uint32_t x) noexcept
{
    return static_cast<float>(x >> 8) * (1.0f / 16777216.0f);
}

STDROMANO_FORCE_INLINE double u64_to_double_01(const std::uint64_t x) noexcept
{
    return static_cast<double>(x >> 11) * (1.0 / 9007199254740992.0);
}

template <typename F>
STDROMANO_FORCE_INLINE F unit_to_range(const F t, const F low, const F high) noexcept
{
    if(!(low < high))
        return low;

    const F value = low + (high - low) * t;

    return value < high ? value : std::nextafter(high, low);
}

template <typename NextU32>
STDROMANO_FORCE_INLINE std::uint32_t bounded_u32(NextU32&& next_u32,
                                                 const std::uint32_t low,
                                                 const std::uint32_t high) noexcept
{
    if(high <= low)
        return low;

    const std::uint32_t span = high - low;

    std::uint64_t product = static_cast<std::uint64_t>(next_u32()) * span;
    std::uint32_t product_low = static_cast<std::uint32_t>(product);

    if(product_low < span)
    {
        const std::uint32_t rejection_threshold = (0U - span) % span;

        while(product_low < rejection_threshold)
        {
            product = static_cast<std::uint64_t>(next_u32()) * span;
            product_low = static_cast<std::uint32_t>(product);
        }
    }

    return low + static_cast<std::uint32_t>(product >> 32);
}

template <typename NextU64>
STDROMANO_FORCE_INLINE std::uint64_t bounded_u64(NextU64&& next_u64,
                                                 const std::uint64_t low,
                                                 const std::uint64_t high) noexcept
{
    if(high <= low)
        return low;

    const std::uint64_t span = high - low;

    std::uint64_t product_low;
    std::uint64_t product_high = mul_u64_wide(next_u64(), span, product_low);

    if(product_low < span)
    {
        const std::uint64_t rejection_threshold = (0ULL - span) % span;

        while(product_low < rejection_threshold)
            product_high = mul_u64_wide(next_u64(), span, product_low);
    }

    return low + product_high;
}

} // namespace detail

// All integer ranges are [low, high), float ranges are [low, high) too, empty ranges return low.
// Stateless random_*(seed) functions return the first value of the stream seeded with seed.

/* Hash primitives */

STDROMANO_FORCE_INLINE std::uint32_t wang_hash(std::uint32_t seed) noexcept
{
    seed = (seed ^ 61U) ^ (seed >> 16U);
    seed *= 9U;
    seed = seed ^ (seed >> 4U);
    seed *= 0x27D4EB2DU;
    seed = seed ^ (seed >> 15U);
    return 1U + seed;
}

STDROMANO_FORCE_INLINE std::uint32_t xorshift32(std::uint32_t state) noexcept
{
    state ^= state << 13U;
    state ^= state >> 17U;
    state ^= state << 5U;
    return state;
}

STDROMANO_FORCE_INLINE std::uint32_t pcg_hash(const std::uint32_t seed) noexcept
{
    const std::uint32_t state = seed * 747796405U + 2891336453U;
    const std::uint32_t word = ((state >> ((state >> 28U) + 4U)) ^ state) * 277803737U;
    return (word >> 22U) ^ word;
}

/* SplitMix64, state is a single u64 that can be any value */

STDROMANO_FORCE_INLINE std::uint64_t next_random_u64(std::uint64_t& state) noexcept
{
    state += 0x9E3779B97F4A7C15ULL;

    std::uint64_t z = state;
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
    return z ^ (z >> 31);
}

STDROMANO_FORCE_INLINE std::uint32_t next_random_u32(std::uint64_t& state) noexcept
{
    return static_cast<std::uint32_t>(next_random_u64(state) >> 32);
}

STDROMANO_FORCE_INLINE float next_random_float(std::uint64_t& state) noexcept
{
    return detail::u32_to_float_01(next_random_u32(state));
}

STDROMANO_FORCE_INLINE double next_random_double(std::uint64_t& state) noexcept
{
    return detail::u64_to_double_01(next_random_u64(state));
}

STDROMANO_FORCE_INLINE std::uint32_t next_random_u32_in_range(std::uint64_t& state,
                                                              const std::uint32_t low,
                                                              const std::uint32_t high) noexcept
{
    return detail::bounded_u32([&state]() { return next_random_u32(state); }, low, high);
}

STDROMANO_FORCE_INLINE std::uint64_t next_random_u64_in_range(std::uint64_t& state,
                                                              const std::uint64_t low,
                                                              const std::uint64_t high) noexcept
{
    return detail::bounded_u64([&state]() { return next_random_u64(state); }, low, high);
}

STDROMANO_FORCE_INLINE float next_random_float_in_range(std::uint64_t& state,
                                                        const float low,
                                                        const float high) noexcept
{
    return detail::unit_to_range(next_random_float(state), low, high);
}

STDROMANO_FORCE_INLINE double next_random_double_in_range(std::uint64_t& state,
                                                          const double low,
                                                          const double high) noexcept
{
    return detail::unit_to_range(next_random_double(state), low, high);
}

STDROMANO_FORCE_INLINE std::uint32_t random_u32(const std::uint32_t seed) noexcept
{
    return detail::lowbias32(seed);
}

STDROMANO_FORCE_INLINE std::uint32_t random_u32(const std::uint64_t seed) noexcept
{
    std::uint64_t state = seed;
    return next_random_u32(state);
}

STDROMANO_FORCE_INLINE std::uint64_t random_u64(const std::uint32_t seed) noexcept
{
    std::uint64_t state = seed;
    return next_random_u64(state);
}

STDROMANO_FORCE_INLINE std::uint64_t random_u64(const std::uint64_t seed) noexcept
{
    std::uint64_t state = seed;
    return next_random_u64(state);
}

STDROMANO_FORCE_INLINE float random_float(const std::uint32_t seed) noexcept
{
    return detail::u32_to_float_01(detail::lowbias32(seed));
}

STDROMANO_FORCE_INLINE float random_float(const std::uint64_t seed) noexcept
{
    std::uint64_t state = seed;
    return next_random_float(state);
}

STDROMANO_FORCE_INLINE double random_double(const std::uint32_t seed) noexcept
{
    std::uint64_t state = seed;
    return next_random_double(state);
}

STDROMANO_FORCE_INLINE double random_double(const std::uint64_t seed) noexcept
{
    std::uint64_t state = seed;
    return next_random_double(state);
}

STDROMANO_FORCE_INLINE std::uint32_t random_u32_in_range(const std::uint64_t seed,
                                                         const std::uint32_t low,
                                                         const std::uint32_t high) noexcept
{
    std::uint64_t state = seed;
    return next_random_u32_in_range(state, low, high);
}

STDROMANO_FORCE_INLINE std::uint64_t random_u64_in_range(const std::uint64_t seed,
                                                         const std::uint64_t low,
                                                         const std::uint64_t high) noexcept
{
    std::uint64_t state = seed;
    return next_random_u64_in_range(state, low, high);
}

STDROMANO_FORCE_INLINE float random_float_in_range(const std::uint64_t seed,
                                                   const float low,
                                                   const float high) noexcept
{
    std::uint64_t state = seed;
    return next_random_float_in_range(state, low, high);
}

STDROMANO_FORCE_INLINE double random_double_in_range(const std::uint64_t seed,
                                                     const double low,
                                                     const double high) noexcept
{
    std::uint64_t state = seed;
    return next_random_double_in_range(state, low, high);
}

/* Wang hash + xorshift32 over a u32 counter, state can be any value */

STDROMANO_FORCE_INLINE std::uint32_t wang_next_random_u32(std::uint32_t& state) noexcept
{
    return xorshift32(wang_hash(state++));
}

STDROMANO_FORCE_INLINE std::uint64_t wang_next_random_u64(std::uint32_t& state) noexcept
{
    const std::uint32_t high = wang_next_random_u32(state);
    const std::uint32_t low = wang_next_random_u32(state);
    return detail::join_u32(high, low);
}

STDROMANO_FORCE_INLINE float wang_next_random_float(std::uint32_t& state) noexcept
{
    return detail::u32_to_float_01(wang_next_random_u32(state));
}

STDROMANO_FORCE_INLINE double wang_next_random_double(std::uint32_t& state) noexcept
{
    return detail::u64_to_double_01(wang_next_random_u64(state));
}

STDROMANO_FORCE_INLINE std::uint32_t wang_next_random_u32_in_range(std::uint32_t& state,
                                                                   const std::uint32_t low,
                                                                   const std::uint32_t high) noexcept
{
    return detail::bounded_u32([&state]() { return wang_next_random_u32(state); }, low, high);
}

STDROMANO_FORCE_INLINE std::uint64_t wang_next_random_u64_in_range(std::uint32_t& state,
                                                                   const std::uint64_t low,
                                                                   const std::uint64_t high) noexcept
{
    return detail::bounded_u64([&state]() { return wang_next_random_u64(state); }, low, high);
}

STDROMANO_FORCE_INLINE float wang_next_random_float_in_range(std::uint32_t& state,
                                                             const float low,
                                                             const float high) noexcept
{
    return detail::unit_to_range(wang_next_random_float(state), low, high);
}

STDROMANO_FORCE_INLINE double wang_next_random_double_in_range(std::uint32_t& state,
                                                               const double low,
                                                               const double high) noexcept
{
    return detail::unit_to_range(wang_next_random_double(state), low, high);
}

STDROMANO_FORCE_INLINE std::uint32_t wang_random_u32(const std::uint32_t seed) noexcept
{
    std::uint32_t state = seed;
    return wang_next_random_u32(state);
}

STDROMANO_FORCE_INLINE std::uint64_t wang_random_u64(const std::uint32_t seed) noexcept
{
    std::uint32_t state = seed;
    return wang_next_random_u64(state);
}

STDROMANO_FORCE_INLINE float wang_random_float(const std::uint32_t seed) noexcept
{
    std::uint32_t state = seed;
    return wang_next_random_float(state);
}

STDROMANO_FORCE_INLINE double wang_random_double(const std::uint32_t seed) noexcept
{
    std::uint32_t state = seed;
    return wang_next_random_double(state);
}

STDROMANO_FORCE_INLINE std::uint32_t wang_random_u32_in_range(const std::uint32_t seed,
                                                              const std::uint32_t low,
                                                              const std::uint32_t high) noexcept
{
    std::uint32_t state = seed;
    return wang_next_random_u32_in_range(state, low, high);
}

STDROMANO_FORCE_INLINE std::uint64_t wang_random_u64_in_range(const std::uint32_t seed,
                                                              const std::uint64_t low,
                                                              const std::uint64_t high) noexcept
{
    std::uint32_t state = seed;
    return wang_next_random_u64_in_range(state, low, high);
}

STDROMANO_FORCE_INLINE float wang_random_float_in_range(const std::uint32_t seed,
                                                        const float low,
                                                        const float high) noexcept
{
    std::uint32_t state = seed;
    return wang_next_random_float_in_range(state, low, high);
}

STDROMANO_FORCE_INLINE double wang_random_double_in_range(const std::uint32_t seed,
                                                          const double low,
                                                          const double high) noexcept
{
    std::uint32_t state = seed;
    return wang_next_random_double_in_range(state, low, high);
}

/* PCG32 (XSH-RR), state is a u64, use pcg_seed to turn a seed into a state */

STDROMANO_FORCE_INLINE std::uint32_t pcg_next_random_u32(std::uint64_t& state) noexcept
{
    const std::uint64_t old_state = state;
    state = old_state * 6364136223846793005ULL + 1442695040888963407ULL;

    const std::uint32_t xorshifted = static_cast<std::uint32_t>(((old_state >> 18U) ^ old_state) >> 27U);
    const std::uint32_t rotation = static_cast<std::uint32_t>(old_state >> 59U);
    return detail::rotr_u32(xorshifted, rotation);
}

STDROMANO_FORCE_INLINE std::uint64_t pcg_seed(const std::uint64_t seed) noexcept
{
    std::uint64_t state = 0;
    pcg_next_random_u32(state);
    state += seed;
    pcg_next_random_u32(state);
    return state;
}

STDROMANO_FORCE_INLINE std::uint64_t pcg_next_random_u64(std::uint64_t& state) noexcept
{
    const std::uint32_t high = pcg_next_random_u32(state);
    const std::uint32_t low = pcg_next_random_u32(state);
    return detail::join_u32(high, low);
}

STDROMANO_FORCE_INLINE float pcg_next_random_float(std::uint64_t& state) noexcept
{
    return detail::u32_to_float_01(pcg_next_random_u32(state));
}

STDROMANO_FORCE_INLINE double pcg_next_random_double(std::uint64_t& state) noexcept
{
    return detail::u64_to_double_01(pcg_next_random_u64(state));
}

STDROMANO_FORCE_INLINE std::uint32_t pcg_next_random_u32_in_range(std::uint64_t& state,
                                                                  const std::uint32_t low,
                                                                  const std::uint32_t high) noexcept
{
    return detail::bounded_u32([&state]() { return pcg_next_random_u32(state); }, low, high);
}

STDROMANO_FORCE_INLINE std::uint64_t pcg_next_random_u64_in_range(std::uint64_t& state,
                                                                  const std::uint64_t low,
                                                                  const std::uint64_t high) noexcept
{
    return detail::bounded_u64([&state]() { return pcg_next_random_u64(state); }, low, high);
}

STDROMANO_FORCE_INLINE float pcg_next_random_float_in_range(std::uint64_t& state,
                                                            const float low,
                                                            const float high) noexcept
{
    return detail::unit_to_range(pcg_next_random_float(state), low, high);
}

STDROMANO_FORCE_INLINE double pcg_next_random_double_in_range(std::uint64_t& state,
                                                              const double low,
                                                              const double high) noexcept
{
    return detail::unit_to_range(pcg_next_random_double(state), low, high);
}

STDROMANO_FORCE_INLINE std::uint32_t pcg_random_u32(const std::uint32_t seed) noexcept
{
    return pcg_hash(seed);
}

STDROMANO_FORCE_INLINE std::uint32_t pcg_random_u32(const std::uint64_t seed) noexcept
{
    std::uint64_t state = pcg_seed(seed);
    return pcg_next_random_u32(state);
}

STDROMANO_FORCE_INLINE std::uint64_t pcg_random_u64(const std::uint32_t seed) noexcept
{
    std::uint64_t state = pcg_seed(seed);
    return pcg_next_random_u64(state);
}

STDROMANO_FORCE_INLINE std::uint64_t pcg_random_u64(const std::uint64_t seed) noexcept
{
    std::uint64_t state = pcg_seed(seed);
    return pcg_next_random_u64(state);
}

STDROMANO_FORCE_INLINE float pcg_random_float(const std::uint32_t seed) noexcept
{
    return detail::u32_to_float_01(pcg_hash(seed));
}

STDROMANO_FORCE_INLINE float pcg_random_float(const std::uint64_t seed) noexcept
{
    std::uint64_t state = pcg_seed(seed);
    return pcg_next_random_float(state);
}

STDROMANO_FORCE_INLINE double pcg_random_double(const std::uint32_t seed) noexcept
{
    std::uint64_t state = pcg_seed(seed);
    return pcg_next_random_double(state);
}

STDROMANO_FORCE_INLINE double pcg_random_double(const std::uint64_t seed) noexcept
{
    std::uint64_t state = pcg_seed(seed);
    return pcg_next_random_double(state);
}

STDROMANO_FORCE_INLINE std::uint32_t pcg_random_u32_in_range(const std::uint64_t seed,
                                                             const std::uint32_t low,
                                                             const std::uint32_t high) noexcept
{
    std::uint64_t state = pcg_seed(seed);
    return pcg_next_random_u32_in_range(state, low, high);
}

STDROMANO_FORCE_INLINE std::uint64_t pcg_random_u64_in_range(const std::uint64_t seed,
                                                             const std::uint64_t low,
                                                             const std::uint64_t high) noexcept
{
    std::uint64_t state = pcg_seed(seed);
    return pcg_next_random_u64_in_range(state, low, high);
}

STDROMANO_FORCE_INLINE float pcg_random_float_in_range(const std::uint64_t seed,
                                                       const float low,
                                                       const float high) noexcept
{
    std::uint64_t state = pcg_seed(seed);
    return pcg_next_random_float_in_range(state, low, high);
}

STDROMANO_FORCE_INLINE double pcg_random_double_in_range(const std::uint64_t seed,
                                                         const double low,
                                                         const double high) noexcept
{
    std::uint64_t state = pcg_seed(seed);
    return pcg_next_random_double_in_range(state, low, high);
}

/* xoshiro256++, use xoshiro_seed to turn a seed into a state */

struct XoshiroState
{
    std::uint64_t s[4];
};

STDROMANO_FORCE_INLINE XoshiroState xoshiro_seed(const std::uint64_t seed) noexcept
{
    std::uint64_t splitmix_state = seed;

    XoshiroState state;
    state.s[0] = next_random_u64(splitmix_state);
    state.s[1] = next_random_u64(splitmix_state);
    state.s[2] = next_random_u64(splitmix_state);
    state.s[3] = next_random_u64(splitmix_state);
    return state;
}

STDROMANO_FORCE_INLINE std::uint64_t xoshiro_next_random_u64(XoshiroState& state) noexcept
{
    std::uint64_t* s = state.s;

    const std::uint64_t result = detail::rotl_u64(s[0] + s[3], 23) + s[0];
    const std::uint64_t t = s[1] << 17;

    s[2] ^= s[0];
    s[3] ^= s[1];
    s[1] ^= s[2];
    s[0] ^= s[3];

    s[2] ^= t;
    s[3] = detail::rotl_u64(s[3], 45);

    return result;
}

STDROMANO_FORCE_INLINE std::uint32_t xoshiro_next_random_u32(XoshiroState& state) noexcept
{
    return static_cast<std::uint32_t>(xoshiro_next_random_u64(state) >> 32);
}

STDROMANO_FORCE_INLINE float xoshiro_next_random_float(XoshiroState& state) noexcept
{
    return detail::u32_to_float_01(xoshiro_next_random_u32(state));
}

STDROMANO_FORCE_INLINE double xoshiro_next_random_double(XoshiroState& state) noexcept
{
    return detail::u64_to_double_01(xoshiro_next_random_u64(state));
}

STDROMANO_FORCE_INLINE std::uint32_t xoshiro_next_random_u32_in_range(XoshiroState& state,
                                                                      const std::uint32_t low,
                                                                      const std::uint32_t high) noexcept
{
    return detail::bounded_u32([&state]() { return xoshiro_next_random_u32(state); }, low, high);
}

STDROMANO_FORCE_INLINE std::uint64_t xoshiro_next_random_u64_in_range(XoshiroState& state,
                                                                      const std::uint64_t low,
                                                                      const std::uint64_t high) noexcept
{
    return detail::bounded_u64([&state]() { return xoshiro_next_random_u64(state); }, low, high);
}

STDROMANO_FORCE_INLINE float xoshiro_next_random_float_in_range(XoshiroState& state,
                                                                const float low,
                                                                const float high) noexcept
{
    return detail::unit_to_range(xoshiro_next_random_float(state), low, high);
}

STDROMANO_FORCE_INLINE double xoshiro_next_random_double_in_range(XoshiroState& state,
                                                                  const double low,
                                                                  const double high) noexcept
{
    return detail::unit_to_range(xoshiro_next_random_double(state), low, high);
}

STDROMANO_FORCE_INLINE std::uint32_t xoshiro_random_u32(const std::uint64_t seed) noexcept
{
    XoshiroState state = xoshiro_seed(seed);
    return xoshiro_next_random_u32(state);
}

STDROMANO_FORCE_INLINE std::uint64_t xoshiro_random_u64(const std::uint64_t seed) noexcept
{
    XoshiroState state = xoshiro_seed(seed);
    return xoshiro_next_random_u64(state);
}

STDROMANO_FORCE_INLINE float xoshiro_random_float(const std::uint64_t seed) noexcept
{
    XoshiroState state = xoshiro_seed(seed);
    return xoshiro_next_random_float(state);
}

STDROMANO_FORCE_INLINE double xoshiro_random_double(const std::uint64_t seed) noexcept
{
    XoshiroState state = xoshiro_seed(seed);
    return xoshiro_next_random_double(state);
}

STDROMANO_FORCE_INLINE std::uint32_t xoshiro_random_u32_in_range(const std::uint64_t seed,
                                                                 const std::uint32_t low,
                                                                 const std::uint32_t high) noexcept
{
    XoshiroState state = xoshiro_seed(seed);
    return xoshiro_next_random_u32_in_range(state, low, high);
}

STDROMANO_FORCE_INLINE std::uint64_t xoshiro_random_u64_in_range(const std::uint64_t seed,
                                                                 const std::uint64_t low,
                                                                 const std::uint64_t high) noexcept
{
    XoshiroState state = xoshiro_seed(seed);
    return xoshiro_next_random_u64_in_range(state, low, high);
}

STDROMANO_FORCE_INLINE float xoshiro_random_float_in_range(const std::uint64_t seed,
                                                           const float low,
                                                           const float high) noexcept
{
    XoshiroState state = xoshiro_seed(seed);
    return xoshiro_next_random_float_in_range(state, low, high);
}

STDROMANO_FORCE_INLINE double xoshiro_random_double_in_range(const std::uint64_t seed,
                                                             const double low,
                                                             const double high) noexcept
{
    XoshiroState state = xoshiro_seed(seed);
    return xoshiro_next_random_double_in_range(state, low, high);
}

/* Thread-safe generators, one thread_local state per generator seeded with random_seed_* */

STDROMANO_API std::uint32_t ts_next_random_u32() noexcept;
STDROMANO_API std::uint64_t ts_next_random_u64() noexcept;
STDROMANO_API float ts_next_random_float() noexcept;
STDROMANO_API double ts_next_random_double() noexcept;
STDROMANO_API std::uint32_t ts_next_random_u32_in_range(const std::uint32_t low, const std::uint32_t high) noexcept;
STDROMANO_API std::uint64_t ts_next_random_u64_in_range(const std::uint64_t low, const std::uint64_t high) noexcept;
STDROMANO_API float ts_next_random_float_in_range(const float low, const float high) noexcept;
STDROMANO_API double ts_next_random_double_in_range(const double low, const double high) noexcept;

STDROMANO_API std::uint32_t ts_wang_next_random_u32() noexcept;
STDROMANO_API std::uint64_t ts_wang_next_random_u64() noexcept;
STDROMANO_API float ts_wang_next_random_float() noexcept;
STDROMANO_API double ts_wang_next_random_double() noexcept;
STDROMANO_API std::uint32_t ts_wang_next_random_u32_in_range(const std::uint32_t low, const std::uint32_t high) noexcept;
STDROMANO_API std::uint64_t ts_wang_next_random_u64_in_range(const std::uint64_t low, const std::uint64_t high) noexcept;
STDROMANO_API float ts_wang_next_random_float_in_range(const float low, const float high) noexcept;
STDROMANO_API double ts_wang_next_random_double_in_range(const double low, const double high) noexcept;

STDROMANO_API std::uint32_t ts_pcg_next_random_u32() noexcept;
STDROMANO_API std::uint64_t ts_pcg_next_random_u64() noexcept;
STDROMANO_API float ts_pcg_next_random_float() noexcept;
STDROMANO_API double ts_pcg_next_random_double() noexcept;
STDROMANO_API std::uint32_t ts_pcg_next_random_u32_in_range(const std::uint32_t low, const std::uint32_t high) noexcept;
STDROMANO_API std::uint64_t ts_pcg_next_random_u64_in_range(const std::uint64_t low, const std::uint64_t high) noexcept;
STDROMANO_API float ts_pcg_next_random_float_in_range(const float low, const float high) noexcept;
STDROMANO_API double ts_pcg_next_random_double_in_range(const double low, const double high) noexcept;

STDROMANO_API std::uint32_t ts_xoshiro_next_random_u32() noexcept;
STDROMANO_API std::uint64_t ts_xoshiro_next_random_u64() noexcept;
STDROMANO_API float ts_xoshiro_next_random_float() noexcept;
STDROMANO_API double ts_xoshiro_next_random_double() noexcept;
STDROMANO_API std::uint32_t ts_xoshiro_next_random_u32_in_range(const std::uint32_t low, const std::uint32_t high) noexcept;
STDROMANO_API std::uint64_t ts_xoshiro_next_random_u64_in_range(const std::uint64_t low, const std::uint64_t high) noexcept;
STDROMANO_API float ts_xoshiro_next_random_float_in_range(const float low, const float high) noexcept;
STDROMANO_API double ts_xoshiro_next_random_double_in_range(const double low, const double high) noexcept;

STDROMANO_NAMESPACE_END

#endif /* !defined(__STDROMANO_RANDOM) */