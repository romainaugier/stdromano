// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - Present Romain Augier
// All rights reserved.

#include "stdromano/hash.hpp"
#include "stdromano/endian.hpp"

STDROMANO_NAMESPACE_BEGIN

uint32_t hash_murmur3(const void* key, const size_t len, const uint32_t seed) noexcept
{
    size_t len2 = len;
    const uint8_t* data = static_cast<const uint8_t*>(key);
    const size_t orig_len = len;
    uint32_t h = seed;

    if((((uintptr_t)key & 3) == 0))
    {
        while(len2 >= sizeof(uint32_t))
        {
            uint32_t k = *(const uint32_t*)(const void*)data;

            k = htole32(k);

            k *= 0xcc9e2d51;
            k = (k << 15) | (k >> 17);
            k *= 0x1b873593;

            h ^= k;
            h = (h << 13) | (h >> 19);
            h = h * 5 + 0xe6546b64;

            data += sizeof(uint32_t);
            len2 -= sizeof(uint32_t);
        }
    }
    else
    {
        while(len2 >= sizeof(uint32_t))
        {
            uint32_t k;

            k = data[0];
            k |= data[1] << 8;
            k |= data[2] << 16;
            k |= data[3] << 24;

            k *= 0xcc9e2d51;
            k = (k << 15) | (k >> 17);
            k *= 0x1b873593;

            h ^= k;
            h = (h << 13) | (h >> 19);
            h = h * 5 + 0xe6546b64;

            data += sizeof(uint32_t);
            len2 -= sizeof(uint32_t);
        }
    }

    /*
     * Handle the last few bytes of the input array.
     */
    uint32_t k = 0;

    switch(len2)
    {
        case 3:
            k ^= data[2] << 16;
            /* FALLTHROUGH */
        case 2:
            k ^= data[1] << 8;
            /* FALLTHROUGH */
        case 1:
            k ^= data[0];
            k *= 0xcc9e2d51;
            k = (k << 15) | (k >> 17);
            k *= 0x1b873593;
            h ^= k;
    }

    /*
     * Finalisation mix: force all bits of a hash block to avalanche.
     */
    h ^= orig_len;
    h ^= h >> 16;
    h *= 0x85ebca6b;
    h ^= h >> 13;
    h *= 0xc2b2ae35;
    h ^= h >> 16;

    return h;
}

STDROMANO_NAMESPACE_END