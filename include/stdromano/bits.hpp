// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - Present Romain Augier
// All rights reserved.

#pragma once

#if !defined(__STDROMANO_BITS)
#define __STDROMANO_BITS

#include "stdromano/stdromano.hpp"

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

STDROMANO_FORCE_INLINE std::size_t bit_ceil(std::size_t n) noexcept
{
    if(n <= 1)
        return 1;

    std::size_t power = 2;
    n--;

    while(n >>= 1)
        power <<= 1;

    return power;
}

STDROMANO_FORCE_INLINE std::uint32_t round_u32_to_next_pow2(std::uint32_t x) noexcept
{
    x--;

    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;

    return x + 1;
}

STDROMANO_FORCE_INLINE std::uint64_t round_u64_to_next_pow2(std::uint64_t x) noexcept
{
    x--;

    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;
    x |= x >> 32;

    return x + 1;
}

STDROMANO_FORCE_INLINE std::uint64_t popcount_u32(const std::uint32_t x) noexcept
{
#if defined(STDROMANO_MSVC) && defined(STDROMANO_AARCH64)
    return _CountOneBits(x);
#elif defined(STDROMANO_MSVC)
    return __popcnt(x);
#elif defined(STDROMANO_GCC) || defined(STDROMANO_CLANG)
    return __builtin_popcountl(x);
#else
    std::uint32_t v = x;
    v = v - ((v >> 1) & (std::uint32_t)~(std::uint32_t)0/3);
    v = (v & (std::uint32_t)~(std::uint32_t)0/15*3) + ((v >> 2) & (std::uint32_t)~(std::uint32_t)0/15*3);
    v = (v + (v >> 4)) & (std::uint32_t)~(std::uint32_t)0/255*15;
    return (std::uint32_t)(v * ((std::uint32_t)~(std::uint32_t)0/255)) >> (sizeof(std::uint32_t) - 1) * 8;
#endif /* defined(STDROMANO_WIN) */
}

STDROMANO_FORCE_INLINE std::uint64_t popcount_u64(const std::uint64_t x) noexcept
{
#if defined(STDROMANO_MSVC) && defined(STDROMANO_AARCH64)
    return _CountOneBits64(x);
#elif defined(STDROMANO_MSVC)
    return __popcnt64(x);
#elif defined(STDROMANO_GCC) || defined(STDROMANO_CLANG)
    return __builtin_popcountll(x);
#else
    std::uint64_t v = x;
    v = v - ((v >> 1) & (std::uint64_t)~(std::uint64_t)0/3);
    v = (v & (std::uint64_t)~(std::uint64_t)0/15*3) + ((v >> 2) & (std::uint64_t)~(std::uint64_t)0/15*3);
    v = (v + (v >> 4)) & (std::uint64_t)~(std::uint64_t)0/255*15;
    return (std::uint64_t)(v * ((std::uint64_t)~(std::uint64_t)0/255)) >> (sizeof(std::uint64_t) - 1) * 8;
#endif /* defined(STDROMANO_WIN) */
}

STDROMANO_FORCE_INLINE std::uint32_t clz_u64(const std::uint64_t x) noexcept
{
#if defined(STDROMANO_MSVC)
    unsigned long trailing_zero = 0;

    if(_BitScanReverse64(&trailing_zero, x))
        return 63UL - trailing_zero;

    return 64UL;
#elif defined(STDROMANO_GCC) || defined(STDROMANO_CLANG)
    return __builtin_clzll(x);
#endif /* defined(STDROMANO_MSVC) */
}

STDROMANO_FORCE_INLINE std::uint32_t ctz_u64(const std::uint64_t x) noexcept
{
#if defined(STDROMANO_MSVC)
    unsigned long trailing_zero = 0UL;

    if(_BitScanForward64(&trailing_zero, x))
        return trailing_zero;

    return 64UL;
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

STDROMANO_FORCE_INLINE std::uint32_t pext_u32(const std::uint32_t x,
                                              const std::uint32_t y) noexcept
{
#if defined(STDROMANO_HAS_PEXT)
    return _pext_u32(x, y);
#else
    std::uint32_t res = 0;
    std::uint32_t mask = y;
    std::uint32_t bit = 1;

    while(mask != 0)
    {
        const std::uint32_t lsb = mask & (~mask + 1);

        if((x & lsb) != 0)
            res |= bit;

        mask ^= lsb;
        bit <<= 1;
    }

    return res;
#endif /* defined(STDROMANO_HAS_PEXT) */
}

STDROMANO_FORCE_INLINE std::uint64_t pext_u64(const std::uint64_t x,
                                              const std::uint64_t y) noexcept
{
#if defined(STDROMANO_HAS_PEXT)
    return _pext_u64(x, y);
#else
    std::uint64_t res = 0;
    std::uint64_t mask = y;
    std::uint64_t bit = 1;

    while(mask != 0)
    {
        const std::uint64_t lsb = mask & (~mask + 1);

        if((x & lsb) != 0)
            res |= bit;

        mask ^= lsb;
        bit <<= 1;
    }

    return res;
#endif /* defined(STDROMANO_HAS_PEXT) */
}

STDROMANO_FORCE_INLINE std::uint8_t abs_u8(const std::int8_t x) noexcept
{
    const std::uint8_t mask = x >> 7U;
    return (std::uint8_t)((mask ^ x) - mask);
}

STDROMANO_FORCE_INLINE std::uint16_t abs_u16(const std::int16_t x) noexcept
{
    const std::uint16_t mask = x >> 15U;
    return (std::uint16_t)((mask ^ x) - mask);
}

STDROMANO_FORCE_INLINE std::uint32_t abs_u32(const std::int32_t x) noexcept
{
    const std::uint32_t mask = x >> 31U;
    return (std::uint32_t)((mask ^ x) - mask);
}

STDROMANO_FORCE_INLINE std::uint64_t abs_u64(const std::int64_t x) noexcept
{
    const std::uint64_t mask = x >> 63U;
    return (std::uint64_t)((mask ^ x) - mask);
}

STDROMANO_FORCE_INLINE std::uint64_t lsb_u64(const std::uint64_t x) noexcept
{
    return x & -x;
}

STDROMANO_FORCE_INLINE std::uint64_t clsb_u64(const std::uint64_t x) noexcept
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
