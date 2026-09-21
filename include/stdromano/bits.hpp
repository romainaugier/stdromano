// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - Present Romain Augier
// All rights reserved.

#pragma once

#if !defined(__STDROMANO_BITS)
#define __STDROMANO_BITS

#include "stdromano/memory.hpp"

#if defined(STDROMANO_MSVC)
#include <intrin.h>
#elif defined(STDROMANO_INTEL)
#include <immintrin.h>
#endif /* defined(STDROMANO_MSVC) */

STDROMANO_NAMESPACE_BEGIN

#define BIT32(bit) (static_cast<uint32_t>(1UL) << (bit))
#define HAS_BIT32(i, b) (static_cast<uint32_t>(i) & BIT32((b)))
#define SET_BIT32(i, b) (static_cast<uint32_t>(i) |= BIT32((b)))
#define UNSET_BIT32(i, b) (static_cast<uint32_t>(i) &= ~BIT32((b)))
#define TOGGLE_BIT32(i, b) (static_cast<uint32_t>(i) ^= BIT32((b)))

#define BIT64(bit) (static_cast<uint64_t>(1ULL) << (bit))
#define HAS_BIT64(i, b) (static_cast<uint64_t>(i) & BIT64((b)))
#define SET_BIT64(i, b) (static_cast<uint64_t>(i) |= BIT64((b)))
#define UNSET_BIT64(i, b) (static_cast<uint64_t>(i) &= ~BIT64((b)))
#define TOGGLE_BIT64(i, b) (static_cast<uint64_t>(i) ^= BIT64((b)))

STDROMANO_FORCE_INLINE size_t bit_ceil(size_t n) noexcept
{
    if(n <= 1)
    {
        return 1;
    }

    size_t power = 2;
    n--;

    while(n >>= 1)
    {
        power <<= 1;
    }

    return power;
}

STDROMANO_FORCE_INLINE uint32_t round_u32_to_next_pow2(uint32_t x) noexcept
{
    x--;
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;
    return x++;
}

STDROMANO_FORCE_INLINE uint64_t round_u64_to_next_pow2(uint64_t x) noexcept
{
    x--;
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;
    x |= x >> 32;
    return x++;
}

STDROMANO_FORCE_INLINE uint64_t popcount_u32(const uint32_t x) noexcept
{
#if defined(STDROMANO_MSVC)
    return __popcnt(x);
#elif defined(STDROMANO_GCC) || defined(STDROMANO_CLANG)
    return __builtin_popcountl(x);
#else
    /* A good compiler will replace it with popcnt instruction if available */
    uint32_t v = x;
    v = v - ((v >> 1) & (uint32_t)~(uint32_t)0/3);
    v = (v & (uint32_t)~(uint32_t)0/15*3) + ((v >> 2) & (uint32_t)~(uint32_t)0/15*3);
    v = (v + (v >> 4)) & (uint32_t)~(uint32_t)0/255*15;
    return (uint32_t)(v * ((uint32_t)~(uint32_t)0/255)) >> (sizeof(uint32_t) - 1) * 8;
#endif /* defined(STDROMANO_WIN) */
}

STDROMANO_FORCE_INLINE uint64_t popcount_u64(const uint64_t x) noexcept
{
#if defined(STDROMANO_MSVC)
    return __popcnt64(x);
#elif defined(STDROMANO_GCC) || defined(STDROMANO_CLANG)
    return __builtin_popcountll(x);
#else
    /* A good compiler will replace it with popcnt instruction if available */
    uint64_t v = x;
    v = v - ((v >> 1) & (uint64_t)~(uint64_t)0/3);
    v = (v & (uint64_t)~(uint64_t)0/15*3) + ((v >> 2) & (uint64_t)~(uint64_t)0/15*3);
    v = (v + (v >> 4)) & (uint64_t)~(uint64_t)0/255*15;
    return (uint64_t)(v * ((uint64_t)~(uint64_t)0/255)) >> (sizeof(uint64_t) - 1) * 8;
#endif /* defined(STDROMANO_WIN) */
}

STDROMANO_FORCE_INLINE uint32_t clz_u64(const uint64_t x) noexcept
{
#if defined(STDROMANO_MSVC)
    unsigned long trailing_zero = 0;

    if(_BitScanReverse64(&trailing_zero, x))
    {
        return 63UL - trailing_zero;
    }

    return 32UL;
#elif defined(STDROMANO_GCC) || defined(STDROMANO_CLANG)
    return __builtin_clzll(x);
#endif /* defined(STDROMANO_MSVC) */
}

STDROMANO_FORCE_INLINE uint32_t ctz_u64(const uint64_t x) noexcept
{
#if defined(STDROMANO_MSVC)
    unsigned long trailing_zero = 0UL;

    if(_BitScanForward64(&trailing_zero, x))
    {
        return trailing_zero;
    }

    return 63UL;
#elif defined(STDROMANO_GCC) || defined(STDROMANO_CLANG)
    return __builtin_ctzll(x);
#endif /* defined(STDROMANO_MSVC) */
}

/*
    pext is a bmi2 instruction, there is no aarch64 equivalent so we fallback on the
    classic bit gathering loop (also used on x86 when the target has no bmi2)
*/
#if defined(STDROMANO_INTEL) && (defined(__BMI2__) || defined(STDROMANO_MSVC))
#define STDROMANO_HAS_PEXT
#endif /* defined(STDROMANO_INTEL) && (defined(__BMI2__) || defined(STDROMANO_MSVC)) */

STDROMANO_FORCE_INLINE uint32_t pext_u32(const uint32_t x, const uint32_t y) noexcept
{
#if defined(STDROMANO_HAS_PEXT)
    return _pext_u32(x, y);
#else
    uint32_t res = 0;
    uint32_t mask = y;
    uint32_t bit = 1;

    while(mask != 0)
    {
        const uint32_t lsb = mask & (~mask + 1);

        if((x & lsb) != 0)
            res |= bit;

        mask ^= lsb;
        bit <<= 1;
    }

    return res;
#endif /* defined(STDROMANO_HAS_PEXT) */
}

STDROMANO_FORCE_INLINE uint64_t pext_u64(const uint64_t x, const uint64_t y) noexcept
{
#if defined(STDROMANO_HAS_PEXT)
    return _pext_u64(x, y);
#else
    uint64_t res = 0;
    uint64_t mask = y;
    uint64_t bit = 1;

    while(mask != 0)
    {
        const uint64_t lsb = mask & (~mask + 1);

        if((x & lsb) != 0)
            res |= bit;

        mask ^= lsb;
        bit <<= 1;
    }

    return res;
#endif /* defined(STDROMANO_HAS_PEXT) */
}

STDROMANO_FORCE_INLINE uint8_t abs_u8(const int8_t x) noexcept
{
    const uint8_t mask = x >> 7U;
    return (uint8_t)((mask ^ x) - mask);
}

STDROMANO_FORCE_INLINE uint16_t abs_u16(const int16_t x) noexcept
{
    const uint16_t mask = x >> 15U;
    return (uint16_t)((mask ^ x) - mask);
}

STDROMANO_FORCE_INLINE uint32_t abs_u32(const int32_t x) noexcept
{
    const uint32_t mask = x >> 31U;
    return (uint32_t)((mask ^ x) - mask);
}

STDROMANO_FORCE_INLINE uint64_t abs_u64(const int64_t x) noexcept
{
    const uint64_t mask = x >> 63U;
    return (uint64_t)((mask ^ x) - mask);
}

STDROMANO_FORCE_INLINE uint64_t lsb_u64(const uint64_t x) noexcept
{
    return x & -x;
}

STDROMANO_FORCE_INLINE uint64_t clsb_u64(const uint64_t x) noexcept
{
    return x & (x - 1ULL);
}

template<typename From, typename To>
#if defined(STDROMANO_GCC) || defined(STDROMANO_CLANG)
constexpr
#endif
STDROMANO_FORCE_INLINE To bit_cast(From x) noexcept
{
    static_assert(sizeof(To) == sizeof(From), "size mismatch");

#if defined(STDROMANO_GCC) || defined(STDROMANO_CLANG)
    return __builtin_bit_cast(To, x);
#else
    To to;
    std::memcpy(&to, &x, sizeof(To));
    return to;
#endif // defined(STDROMANO_GCC) || defined(STDROMANO_CLANG)
}

STDROMANO_NAMESPACE_END

#endif // !defined(__STDROMANO_BITS)
