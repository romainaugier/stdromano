// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - Present Romain Augier
// All rights reserved.

#pragma once

#if !defined(__STDROMANO_HASH)
#define __STDROMANO_HASH

#include "stdromano/stdromano.hpp"

#include <cstring>

STDROMANO_NAMESPACE_BEGIN

static STDROMANO_FORCE_INLINE uint32_t hash_fnv1a(char* data, const size_t n)
{
    uint32_t result = static_cast<uint32_t>(0x811c9dc5UL);

#if defined(STDROMANO_GCC)
#pragma GCC unroll 0
#endif /* defined(ROMANO_CLANG) || defined(ROMANO_GCC) */
    for(size_t i = 0; i < n; i++)
    {
        result ^= static_cast<uint32_t>(data[i]);
        result *= static_cast<uint32_t>(0x01000193UL);
    }

    return result;
}

static STDROMANO_FORCE_INLINE uint32_t hash_fnv1a(const char* data)
{
    uint32_t result = static_cast<uint32_t>(0x811c9dc5UL);

#if defined(STDROMANO_GCC)
#pragma GCC unroll 0
#endif /* defined(ROMANO_CLANG) || defined(ROMANO_GCC) */
    while(*data != '\0')
    {
        result ^= static_cast<uint32_t>(*data);
        result *= static_cast<uint32_t>(0x01000193UL);

        data++;
    }

    return result;
}

/* fnv1a_pippip hash */
#if !defined(FNV1A_MAX_KEY_SIZE)
#define FNV1A_MAX_KEY_SIZE 512
#endif /* !defined(FNV1A_MAX_KEY_SIZE) */

#define _PADr_KAZE(x, n) (((x) << (n)) >> (n))

static STDROMANO_FORCE_INLINE uint32_t hash_fnv1a_pippip(const char* str, size_t n)
{
    assert(n < (FNV1A_MAX_KEY_SIZE - 8));

    const uint32_t PRIME = static_cast<uint32_t>(591798841UL);

    uint32_t hash32;
    uint64_t hash64 = static_cast<uint64_t>(14695981039346656037ULL);
    size_t cycles, nd_head;

    char _str[FNV1A_MAX_KEY_SIZE];
    std::memcpy(_str, str, n * sizeof(char));

    char* _str_ptr = _str;

    if(n > 8)
    {
        cycles = ((n - 1) >> 4) + 1;
        nd_head = n - (cycles << 3);

#if defined(STDROMANO_GCC)
#pragma GCC unroll 0
#endif /* defined(STDROMANO_CLANG) || defined(STDROMANO_GCC) */

        for(; cycles--; _str_ptr += 8)
        {
            hash64 = (hash64 ^ (*(uint64_t*)(_str_ptr))) * PRIME;
            hash64 = (hash64 ^ (*(uint64_t*)(_str_ptr + nd_head))) * PRIME;
        }
    }
    else
    {
        hash64 = (hash64 ^ _PADr_KAZE(*(uint64_t*)(_str_ptr + 0), (8 - n) << 3)) * PRIME;
    }

    hash32 = (uint32_t)(hash64 ^ (hash64 >> 32));

    return hash32 ^ (hash32 >> 16);
}

static STDROMANO_FORCE_INLINE uint32_t hash_u32(uint32_t x)
{
    x = ((x >> 16) ^ x) * static_cast<uint32_t>(0x45d9f3bUL);
    x = ((x >> 16) ^ x) * static_cast<uint32_t>(0x45d9f3bUL);
    x = (x >> 16) ^ x;

    return x;
}

static STDROMANO_FORCE_INLINE uint64_t hash_u64(uint64_t x)
{
    x = (x ^ (x >> 30)) * static_cast<uint64_t>(0xbf58476d1ce4e5b9ULL);
    x = (x ^ (x >> 27)) * static_cast<uint64_t>(0x94d049bb133111ebULL);
    x = x ^ (x >> 31);
    return x;
}

static STDROMANO_FORCE_INLINE int64_t hash_murmur_64(int64_t x)
{
    x ^= x >> 33;
    x *= 0xff51afd7ed558ccdL;
    x ^= x >> 33;
    x *= 0xc4ceb9fe1a85ec53L;
    x ^= x >> 33;
    return x;
}

STDROMANO_API uint32_t hash_murmur3(const void* key,
                                    const size_t len,
                                    const uint32_t seed) noexcept;

STDROMANO_NAMESPACE_END

#endif /* !defined(__STDROMANO_HASH) */