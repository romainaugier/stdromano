// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - Present Romain Augier
// All rights reserved.

#pragma once

#if !defined(__STDROMANO_MEMORY)
#define __STDROMANO_MEMORY

#include "stdromano/stdromano.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <immintrin.h>

#if defined(STDROMANO_GCC)
#include <alloca.h>
#endif /* defined(STDROMANO_GCC) */

STDROMANO_NAMESPACE_BEGIN

/* jemalloc wrappers */

STDROMANO_API void* mem_alloc(const size_t size) noexcept;

STDROMANO_API void* mem_calloc(const size_t count, const size_t size) noexcept;

STDROMANO_API void* mem_realloc(void* ptr, const size_t size) noexcept;

STDROMANO_API void* mem_crealloc(void* ptr, const size_t size) noexcept;

STDROMANO_API void mem_free(void* ptr) noexcept;

#if defined(STDROMANO_MSVC)
#define mem_alloca(size) _malloca(size)
#elif defined(STDROMANO_GCC)
#define mem_alloca(size) __builtin_alloca(size)
#endif /* defined(STDROMANO_MSVC) */

STDROMANO_API void* mem_aligned_alloc(const size_t size, const size_t alignment) noexcept;

STDROMANO_API void mem_aligned_free(void* ptr) noexcept;

static STDROMANO_FORCE_INLINE void mem_swap(void* a, void* b, const size_t size) noexcept
{
    unsigned char* p;
    unsigned char* q;
    unsigned char* const sentry = (unsigned char*)a + size;

    for(p = static_cast<unsigned char*>(a), q = static_cast<unsigned char*>(b); p < sentry; ++p, ++q)
    {
        const unsigned char t = *p;
        *p = *q;
        *q = t;
    }
}

STDROMANO_API void format_byte_size(float size, char* buffer) noexcept;

STDROMANO_NAMESPACE_END

#endif // !defined(__STDROMANO_MEMORY)