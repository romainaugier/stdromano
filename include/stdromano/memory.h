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
#include <type_traits>

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

    for(p = static_cast<unsigned char*>(a), q = static_cast<unsigned char*>(b); p < sentry;
        ++p, ++q)
    {
        const unsigned char t = *p;
        *p = *q;
        *q = t;
    }
}

STDROMANO_API void format_byte_size(float size, char* buffer) noexcept;

class STDROMANO_API Arena
{
    static constexpr float ARENA_GROWTH_RATE = 1.6180339887f;

    struct Destructor
    {
        void (*destroy_func)(void*);
        void* object_ptr;
        Destructor* next;
    };

    void* _data = nullptr;
    size_t _offset = 0;
    size_t _capacity = 0;

    Destructor* _destructors = nullptr;

    STDROMANO_FORCE_INLINE bool check_resize(const size_t new_size) const noexcept
    {
        return (this->_offset + new_size) >= this->_capacity;
    }

    STDROMANO_FORCE_INLINE size_t align_offset(const size_t alignment) const noexcept
    {
        const uintptr_t current_addr =
                       reinterpret_cast<uintptr_t>(static_cast<char*>(this->_data) + this->_offset);
        const uintptr_t aligned_addr = (current_addr + alignment - 1) & ~(alignment - 1);
        return this->_offset + (aligned_addr - current_addr);
    }

    STDROMANO_FORCE_INLINE void* current_address() const noexcept
    {
        return static_cast<void*>(static_cast<char*>(this->_data) + this->_offset);
    }

    void grow(const size_t min_size) noexcept;

    template <typename T>
    static void dtor_func(void* ptr)
    {
        reinterpret_cast<T*>(ptr)->~T();
    }

  public:
    Arena(const size_t initial_size);

    ~Arena();

    void clear() noexcept;

    void resize(const size_t new_capacity) noexcept;

    template <typename T, typename... Args>
    T* emplace(Args... args) noexcept
    {
        constexpr bool needs_destructor = !std::is_trivially_destructible<T>::value;
        constexpr size_t object_size = sizeof(T);
        constexpr size_t dtor_size = needs_destructor ? sizeof(Destructor) : 0;
        constexpr size_t total_size = object_size + dtor_size;

        if(this->check_resize(total_size))
        {
            this->grow(total_size);
        }

        this->_offset = this->align_offset(alignof(T));

        void* object_address = this->current_address();

        this->_offset += object_size;

        T* object = ::new(object_address) T(args...);

        if(needs_destructor)
        {
            Destructor* dtor = static_cast<Destructor*>(this->current_address());
            this->_offset += dtor_size;

            dtor->destroy_func = &Arena::dtor_func<T>;
            dtor->object_ptr = object;
            dtor->next = this->_destructors;

            this->_destructors = dtor;
        }

        return object;
    }

    STDROMANO_FORCE_INLINE void* at(const size_t offset) const noexcept
    {
        STDROMANO_ASSERT(offset < this->_capacity, "Out of bounds access");

        return offset >= this->_capacity
                              ? nullptr
                              : static_cast<void*>(static_cast<char*>(this->_data) + offset);
    }
};

STDROMANO_NAMESPACE_END

#endif // !defined(__STDROMANO_MEMORY)