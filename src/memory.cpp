// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - Present Romain Augier
// All rights reserved.

#include "stdromano/memory.h"

#include "jemalloc/jemalloc.h"

STDROMANO_NAMESPACE_BEGIN

void* mem_alloc(const size_t size) noexcept 
{ 
    return je_malloc(size); 
}

void* mem_calloc(const size_t count, const size_t size) noexcept
{
    return je_calloc(count, size);
}

void* mem_realloc(void* ptr, const size_t size) noexcept 
{ 
    return je_realloc(ptr, size);
}

void* mem_crealloc(void* ptr, const size_t size) noexcept
{
    void* new_ptr = je_realloc(ptr, size);

    if(new_ptr != nullptr)
    {
        std::memset(new_ptr, 0, size);
    }

    return new_ptr;
}

void mem_free(void* ptr) noexcept
{ 
    je_free(ptr);
}

void* mem_aligned_alloc(const size_t size, const size_t alignment) noexcept
{
    return je_aligned_alloc(alignment, size);
}

void mem_aligned_free(void* ptr) noexcept 
{ 
    je_free(ptr);
}

constexpr const char* units[4] = {"Bytes", "Gb", "Mb", "Kb"};

void format_byte_size(float size, char* buffer) noexcept
{
    size_t unit = 0;

    if(size > 1000000000)
    {
        unit = 1;
        size = size / 1000000000;
    }
    else if(size > 1000000)
    {
        unit = 2;
        size = size / 1000000;
    }
    else if(size > 1e3)
    {
        unit = 3;
        size = size / 1000;
    }

    std::snprintf(buffer, 16, "%.02f %s", size, units[unit]);
}


STDROMANO_NAMESPACE_END