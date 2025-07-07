// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - Present Romain Augier
// All rights reserved.

#pragma once

#if !defined(__STDROMANO_MEMORY)
#define __STDROMANO_MEMORY

#include "stdromano/stdromano.h"

#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <immintrin.h>
#include <memory>
#include <new>
#include <type_traits>
#include <vector>


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

/* STL-like allocators wrapping alloc functions above */

template <typename T>
class STDROMANO_API Allocator
{
public:
    using value_type = T;
    using propagate_on_container_move_assignment = std::true_type;
    using is_always_equal = std::false_type;

    Allocator() = default;

    template <typename U>
    constexpr Allocator(const Allocator<U>&) noexcept {}

    [[nodiscard]] T* allocate(const std::size_t n)
    {
        if(n > this->max_size())
        {
            throw std::bad_alloc();
        }

        if(auto p = static_cast<T*>(mem_alloc(n * sizeof(T))))
        {
            return p;
        }

        throw std::bad_alloc();
    }

    void deallocate(T* p, std::size_t) noexcept
    {
        mem_free(p);
    }

    [[nodiscard]] constexpr std::size_t max_size() const noexcept
    {
        return std::numeric_limits<std::size_t>::max() / sizeof(T);
    }
};

template <typename T, std::size_t Alignment>
class STDROMANO_API AlignedAllocator 
{
public:
    using value_type = T;
    using propagate_on_container_move_assignment = std::true_type;
    using is_always_equal = std::false_type;

    static_assert(Alignment >= alignof(T), "Alignment must be at least as strict as T alignment");
    static_assert((Alignment & (Alignment - 1)) == 0, "Alignment must be a power of two");

    AlignedAllocator() = default;

    template <typename U>
    constexpr AlignedAllocator(const AlignedAllocator<U, Alignment>&) noexcept {}

    [[nodiscard]] T* allocate(const std::size_t n)
    {
        if(n > this->max_size())
        {
            throw std::bad_alloc();
        }

        if(auto p = static_cast<T*>(mem_aligned_alloc(n * sizeof(T), Alignment)))
        {
            return p;
        }

        throw std::bad_alloc();
    }

    void deallocate(T* p, std::size_t) noexcept
    {
        mem_aligned_free(p);
    }

    [[nodiscard]] constexpr std::size_t max_size() const noexcept
    {
        return std::numeric_limits<std::size_t>::max() / sizeof(T);
    }
};

static STDROMANO_FORCE_INLINE void mem_swap(void* a, void* b, const std::size_t size) noexcept
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
    static constexpr std::uint32_t ARENA_BLOCK_SIZE = 16384; /* 16 Kb */

    struct Block
    {
        void* _address;
        std::size_t _size;
        std::size_t _offset;
        Block* _prev;
        Block* _next;

        Block(void* address, std::size_t size)
            : _address(address),
              _size(size),
              _offset(0),
              _prev(nullptr),
              _next(nullptr)
        {
        }
    };

    struct Destructor
    {
        void (*destroy_func)(void*);
        void* object_ptr;
        Destructor* next;
    };

    Block* _current_block;

    std::size_t _capacity;

    std::size_t _block_size;

    Destructor* _destructors = nullptr;

    static Block* allocate_block(const std::size_t size) noexcept;

    STDROMANO_FORCE_INLINE void* current_address() const noexcept
    {
        return static_cast<void*>(static_cast<char*>(this->_current_block->_address) +
                                  this->_current_block->_offset);
    }

    STDROMANO_FORCE_INLINE std::size_t align_offset(const size_t alignment) const noexcept
    {
        const uintptr_t current_addr = reinterpret_cast<uintptr_t>(this->current_address());
        const uintptr_t aligned_addr = (current_addr + alignment - 1) & ~(alignment - 1);
        return this->_current_block->_offset + (aligned_addr - current_addr);
    }

    STDROMANO_FORCE_INLINE bool check_resize(const std::uint32_t size) const noexcept
    {
        return (this->_current_block->_offset + size) > this->_current_block->_size;
    }

    void grow() noexcept;

    template <typename T>
    static void dtor_func(void* ptr)
    {
        reinterpret_cast<T*>(ptr)->~T();
    }

public:
    Arena(const std::size_t initial_size, const std::size_t block_size = ARENA_BLOCK_SIZE);

    ~Arena();

    void clear() noexcept;

    template <typename T, typename... Args>
    T* emplace(Args... args) noexcept
    {
        constexpr bool needs_destructor = !std::is_trivially_destructible<T>::value;
        constexpr size_t object_size = sizeof(T);
        constexpr size_t dtor_size = needs_destructor ? sizeof(Destructor) : 0;
        constexpr size_t total_size = object_size + dtor_size;

        if(this->check_resize(total_size))
        {
            this->grow();
        }

        this->_current_block->_offset = this->align_offset(alignof(T));

        void* object_address = this->current_address();

        this->_current_block->_offset += object_size;

        T* object = ::new(object_address) T(args...);

        if(needs_destructor)
        {
            Destructor* dtor = static_cast<Destructor*>(this->current_address());
            this->_current_block->_offset += dtor_size;

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

        Block* current = this->_current_block;
        std::size_t total_capacity = 0;

        while(current != nullptr)
        {
            if(offset < (total_capacity + static_cast<std::size_t>(current->_size)))
            {
                const std::size_t base_offset = offset - total_capacity;

                return static_cast<void*>(static_cast<char*>(current->_address) + base_offset);
            }

            total_capacity += static_cast<std::size_t>(current->_size);
            current = current->_next;
        }

        return nullptr;
    }
};

STDROMANO_NAMESPACE_END

#endif // !defined(__STDROMANO_MEMORY)