// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - Present Romain Augier
// All rights reserved.

#include "stdromano/random.hpp"

#if defined(STDROMANO_WIN)
#include <Windows.h>
#include <bcrypt.h>
#elif defined(STDROMANO_LINUX)
#include <sys/random.h>
#endif // defined(STDROMANO_WIN)

#include <limits>

STDROMANO_NAMESPACE_BEGIN

std::uint32_t random_seed() noexcept
{
    std::uint32_t value;

#if defined(STDROMANO_WIN)
    NTSTATUS status = BCryptGenRandom(nullptr,
                                      reinterpret_cast<PUCHAR>(&value),
                                      sizeof(value),
                                      BCRYPT_USE_SYSTEM_PREFERRED_RNG);

    if (!BCRYPT_SUCCESS(status))
        return std::numeric_limits<std::uint32_t>::max();

#elif defined(STDROMANO_LINUX)
    ssize_t res = getrandom(&value, sizeof(value), 0);

    if(res != sizeof(value))
        return std::numeric_limits<std::uint32_t>::max();

#else
#error "random_seed not implemented on this platform"
#endif // defined(STDROMANO_WIN)

    return value;
}

STDROMANO_FORCE_INLINE std::uint64_t rotl(const std::uint64_t x, std::int32_t k)
{
    return (x << k) | (x >> (64 - k));
}

constexpr float tofloat32 = bit_cast<std::uint32_t, float>(0x2f800004UL);


std::uint64_t xoshiro_random_uint64(const std::uint64_t seed) noexcept
{
    std::uint64_t s[4];
    s[0] = seed;

    for(int i = 1; i < 4; i++)
    {
        s[i] = s[i - 1] + 0x9e3779b97f4a7c15ULL;
        s[i] = (s[i] ^ (s[i] >> 30)) * 0xbf58476d1ce4e5b9ULL;
        s[i] = (s[i] ^ (s[i] >> 27)) * 0x94d049bb133111ebULL;
        s[i] = s[i] ^ (s[i] >> 31);
    }

    const std::uint64_t result = rotl(s[0] + s[3], 23) + s[0];
    const std::uint64_t t = s[1] << 17;

    s[2] ^= s[0];
    s[3] ^= s[1];
    s[1] ^= s[2];
    s[0] ^= s[3];

    s[2] ^= t;
    s[3] = rotl(s[3], 45);

    return result;
}

static thread_local std::uint64_t xoshiro_s[4] = {0x123456789abcdef0ULL, 0x42, 0x1337, 0xdeadbeef};

void seed_xoshiro(std::uint64_t seed) noexcept
{
    xoshiro_s[0] = seed;

    for(int i = 1; i < 4; i++)
    {
        xoshiro_s[i] = xoshiro_s[i - 1] + 0x9e3779b97f4a7c15ULL;
        xoshiro_s[i] = (xoshiro_s[i] ^ (xoshiro_s[i] >> 30)) * 0xbf58476d1ce4e5b9ULL;
        xoshiro_s[i] = (xoshiro_s[i] ^ (xoshiro_s[i] >> 27)) * 0x94d049bb133111ebULL;
        xoshiro_s[i] = xoshiro_s[i] ^ (xoshiro_s[i] >> 31);
    }
}

std::uint64_t xoshiro_next_uint64() noexcept
{
    const std::uint64_t result = rotl(xoshiro_s[0] + xoshiro_s[3], 23) + xoshiro_s[0];
    const std::uint64_t t = xoshiro_s[1] << 17;

    xoshiro_s[2] ^= xoshiro_s[0];
    xoshiro_s[3] ^= xoshiro_s[1];
    xoshiro_s[1] ^= xoshiro_s[2];
    xoshiro_s[0] ^= xoshiro_s[3];

    xoshiro_s[2] ^= t;
    xoshiro_s[3] = rotl(xoshiro_s[3], 45);

    return result;
}

float xoshiro_next_float() noexcept
{
    return static_cast<float>(xoshiro_next_uint64() >> 32) * tofloat32;
}

STDROMANO_NAMESPACE_END
