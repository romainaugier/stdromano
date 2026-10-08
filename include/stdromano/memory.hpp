// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - Present Romain Augier
// All rights reserved.

#pragma once

#if !defined(__STDROMANO_MEMORY)
#define __STDROMANO_MEMORY

#include "stdromano/stdromano.hpp"
#include "stdromano/atomic.hpp"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <new>
#include <type_traits>
#include <utility>

#if defined(STDROMANO_UNIX)
#include <alloca.h>
#endif /* defined(STDROMANO_UNIX) */

STDROMANO_NAMESPACE_BEGIN

/* mimalloc wrappers */

DETAIL_NAMESPACE_BEGIN

STDROMANO_API void* mem_alloc(const std::size_t size) noexcept;

STDROMANO_API void* mem_calloc(const std::size_t count, const std::size_t size) noexcept;

STDROMANO_API void* mem_realloc(void* ptr, const std::size_t size) noexcept;

STDROMANO_API void* mem_crealloc(void* ptr, const std::size_t size) noexcept;

STDROMANO_API void mem_free(void* ptr) noexcept;

STDROMANO_API void* mem_aligned_alloc(const std::size_t size, const std::size_t alignment) noexcept;

STDROMANO_API void mem_aligned_free(void* ptr) noexcept;

DETAIL_NAMESPACE_END

template<typename T = void>
STDROMANO_FORCE_INLINE T* mem_alloc(const std::size_t size) noexcept
{
    return reinterpret_cast<T*>(detail::mem_alloc(size));
}

template<typename T>
STDROMANO_FORCE_INLINE T* mem_calloc(const std::size_t count, const std::size_t size) noexcept
{
    return reinterpret_cast<T*>(detail::mem_calloc(count, size));
}

template<typename T>
STDROMANO_FORCE_INLINE T* mem_realloc(T* ptr, const std::size_t size) noexcept
{
    return reinterpret_cast<T*>(detail::mem_realloc(reinterpret_cast<void*>(ptr), size));
}

template<typename T>
STDROMANO_FORCE_INLINE T* mem_crealloc(T* ptr, const std::size_t size) noexcept
{
    return reinterpret_cast<T*>(detail::mem_crealloc(reinterpret_cast<void*>(ptr), size));
}

template<typename T>
STDROMANO_FORCE_INLINE void mem_free(T* ptr) noexcept
{
    detail::mem_free(reinterpret_cast<void*>(ptr));
}

template<typename T = void>
STDROMANO_FORCE_INLINE T* mem_aligned_alloc(const std::size_t size, const std::size_t alignment) noexcept
{
    return reinterpret_cast<T*>(detail::mem_aligned_alloc(size, alignment));
}

template<typename T>
STDROMANO_FORCE_INLINE void mem_aligned_free(T* ptr) noexcept
{
    detail::mem_aligned_free(reinterpret_cast<void*>(ptr));
}

/* Alloca functions */

#if defined(STDROMANO_MSVC)
#define mem_alloca(size) _malloca(size)
#elif defined(STDROMANO_GCC) || defined(STDROMANO_CLANG)
#define mem_alloca(size) __builtin_alloca(size)
#endif /* defined(STDROMANO_MSVC) */

#define mem_aligned_alloca(ptrname, size, alignment)                                               \
    std::uintptr_t ptrname##_raw =                                                                 \
        reinterpret_cast<std::uintptr_t>(mem_alloca((size) + (alignment) - 1 + sizeof(void*)));    \
    std::uintptr_t ptrname##_aligned =                                                             \
        (ptrname##_raw + sizeof(void*) + (alignment) - 1) & ~((std::uintptr_t)(alignment) - 1);    \
    void* ptrname = reinterpret_cast<void*>(ptrname##_aligned)

// STL-like allocator that allocates memory using the mimalloc wrappers declared above
template <typename T>
class STDROMANO_API Allocator
{
public:
    using value_type = T;
    using propagate_on_container_move_assignment = std::true_type;
    using is_always_equal = std::false_type;

    Allocator() = default;

    template <typename U>
    constexpr Allocator(const Allocator<U>&) noexcept
    {
    }

    [[nodiscard]] T* allocate(const std::size_t n)
    {
        if(n > this->max_size())
        {
            throw std::bad_alloc();
        }

        if(auto p = mem_alloc<T>(n * sizeof(T)))
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

// STL-like allocator that allocates aligned memory using the mimalloc wrappers declared above
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
    constexpr AlignedAllocator(const AlignedAllocator<U, Alignment>&) noexcept
    {
    }

    STDROMANO_NO_DISCARD T* allocate(const std::size_t n)
    {
        if(n > this->max_size())
            throw std::bad_alloc();

        if(auto p = mem_aligned_alloc<T>(n * sizeof(T), Alignment))
            return p;

        throw std::bad_alloc();
    }

    void deallocate(T* p, std::size_t) noexcept
    {
        mem_aligned_free(p);
    }

    STDROMANO_NO_DISCARD constexpr std::size_t max_size() const noexcept
    {
        return std::numeric_limits<std::size_t>::max() / sizeof(T);
    }
};

// constexpr version of memswap
constexpr STDROMANO_FORCE_INLINE void mem_swap(void* a, void* b, const std::size_t size) noexcept
{
    unsigned char* p = nullptr;
    unsigned char* q = nullptr;
    unsigned char* const sentry = (unsigned char*)a + size;

    for(p = static_cast<unsigned char*>(a), q = static_cast<unsigned char*>(b); p < sentry;
        ++p, ++q)
    {
        const unsigned char t = *p;
        *p = *q;
        *q = t;
    }
}

// constexpr version of strlen
constexpr STDROMANO_FORCE_INLINE std::size_t str_len(const char* __restrict str) noexcept
{
    if(str == nullptr)
        return 0;

    std::size_t len = 0;

    while(str[len] != '\0')
        ++len;

    return len;
}

// constexpr version of memcpy
constexpr STDROMANO_FORCE_INLINE void mem_cpy(void* __restrict dst,
                                              const void* __restrict src,
                                              std::size_t count) noexcept
{
    if(dst != nullptr && src != nullptr)
    {
        char* _dst = static_cast<char*>(dst);
        const char* _src = static_cast<const char*>(src);

        for(std::size_t i = 0; i < count; i++)
        {
            _dst[i] = _src[i];
        }
    }
}

// constexpr version of memmove
constexpr STDROMANO_FORCE_INLINE void mem_move(void* __restrict dst,
                                               const void* __restrict src,
                                               std::size_t count) noexcept
{
    if(dst != nullptr && src != nullptr)
    {
        char* _dst = static_cast<char*>(dst);
        const char* _src = static_cast<const char*>(src);

        if(_dst < _src)
        {
            for(std::size_t i = 0; i < count; ++i)
                _dst[i] = _src[i];
        }
        else if(_dst > _src)
        {
            for(std::size_t i = count; i > 0; --i)
                _dst[i - 1] = _src[i - 1];
        }
    }
}

// constexpr version of memset
constexpr STDROMANO_FORCE_INLINE void mem_set(void* __restrict dst,
                                              char value,
                                              std::size_t count) noexcept
{
    if(dst != nullptr)
    {
        char* _dst = static_cast<char*>(dst);

        for(std::size_t i = 0; i < count; i++)
            _dst[i] = value;
    }
}

STDROMANO_API void format_byte_size(float size, char* buffer) noexcept;

// Simple Arena allocator
class STDROMANO_API Arena
{
    static constexpr std::uint32_t ARENA_INITIAL_SIZE =  1048576; /* 1 Mb */
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

    STDROMANO_FORCE_INLINE std::size_t required_size(const std::size_t size,
                                                     const std::size_t alignment) const noexcept
    {
        return this->align_offset(alignment) - this->_current_block->_offset + size;
    }

    STDROMANO_FORCE_INLINE bool check_resize(const std::size_t size) const noexcept
    {
        return (this->_current_block->_offset + size) > this->_current_block->_size;
    }

    Block* first_block() const noexcept;

    void grow(const std::size_t min_size) noexcept;

    void* reserve_aligned(const std::size_t size, const std::size_t alignment) noexcept
    {
        const std::size_t worst_case = size + alignment - 1;

        if(this->check_resize(this->required_size(size, alignment)))
            this->grow(worst_case);

        this->_current_block->_offset = this->align_offset(alignment);

        void* address = this->current_address();

        this->_current_block->_offset += size;

        return address;
    }

    template <typename T>
    static void dtor_func(void* ptr)
    {
        reinterpret_cast<T*>(ptr)->~T();
    }

public:
    Arena(const std::size_t initial_size = ARENA_INITIAL_SIZE,
          const std::size_t block_size = ARENA_BLOCK_SIZE);

    ~Arena();

    Arena(const Arena&) = delete;
    Arena& operator=(const Arena&) = delete;

    void clear() noexcept;

    template <typename T, typename... Args>
    T* emplace(Args... args) noexcept
    {
        void* object_address = this->reserve_aligned(sizeof(T), alignof(T));

        T* object = ::new(object_address) T(args...);

        if constexpr(!std::is_trivially_destructible_v<T>)
        {
            Destructor* dtor = static_cast<Destructor*>(this->reserve_aligned(sizeof(Destructor),
                                                                              alignof(Destructor)));

            dtor->destroy_func = &Arena::dtor_func<T>;
            dtor->object_ptr = object;
            dtor->next = this->_destructors;

            this->_destructors = dtor;
        }

        return object;
    }

    STDROMANO_FORCE_INLINE void* allocate(std::size_t n) noexcept
    {
        if(this->check_resize(n))
            this->grow(n);

        void* address = this->current_address();

        this->_current_block->_offset += n;

        return address;
    }

    STDROMANO_FORCE_INLINE void* allocate_aligned(std::size_t n, std::size_t alignment) noexcept
    {
        return this->reserve_aligned(n, alignment);
    }

    STDROMANO_FORCE_INLINE void* at(const size_t offset) const noexcept
    {
        STDROMANO_ASSERT(offset < this->_capacity, "Out of bounds access");

        Block* current = this->first_block();
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

// Smart pointers

DETAIL_NAMESPACE_BEGIN

template <typename T>
STDROMANO_FORCE_INLINE void* object_address(T* ptr) noexcept
{
    return const_cast<void*>(static_cast<const volatile void*>(ptr));
}

template <typename T, typename... Args>
STDROMANO_FORCE_INLINE T* construct_at(void* block, void* address, Args&&... args)
{
    using U = std::remove_cv_t<T>;

    if constexpr(std::is_nothrow_constructible_v<U, Args...>)
    {
        return ::new(address) U(std::forward<Args>(args)...);
    }
    else
    {
        try
        {
            return ::new(address) U(std::forward<Args>(args)...);
        }
        catch(...)
        {
            mem_aligned_free(block);
            throw;
        }
    }
}

template <typename T>
STDROMANO_FORCE_INLINE void destroy_at(T* ptr) noexcept
{
    if constexpr(!std::is_trivially_destructible_v<T>)
    {
        ptr->~T();
    }
}

class RefCountST
{
    std::uint32_t _strong = 1;
    std::uint32_t _weak = 1;

public:
    STDROMANO_FORCE_INLINE void inc_strong() noexcept
    {
        ++this->_strong;
    }

    STDROMANO_FORCE_INLINE bool dec_strong() noexcept
    {
        return --this->_strong == 0;
    }

    STDROMANO_FORCE_INLINE bool inc_strong_if_alive() noexcept
    {
        if(this->_strong == 0)
            return false;

        ++this->_strong;

        return true;
    }

    STDROMANO_FORCE_INLINE void inc_weak() noexcept
    {
        ++this->_weak;
    }

    STDROMANO_FORCE_INLINE bool dec_weak() noexcept
    {
        return --this->_weak == 0;
    }

    STDROMANO_FORCE_INLINE std::uint32_t strong_count() const noexcept
    {
        return this->_strong;
    }

    STDROMANO_FORCE_INLINE bool is_sole_owner() const noexcept
    {
        return this->_strong == 1 && this->_weak == 1;
    }
};

class RefCountMT
{
    Atomic<std::uint32_t> _strong{1};
    Atomic<std::uint32_t> _weak{1};

public:
    STDROMANO_FORCE_INLINE void inc_strong() noexcept
    {
        this->_strong.fetch_add(1, MemoryOrder::Relaxed);
    }

    STDROMANO_FORCE_INLINE bool dec_strong() noexcept
    {
        return this->_strong.fetch_sub(1, MemoryOrder::AcqRel) == 1;
    }

    STDROMANO_FORCE_INLINE bool inc_strong_if_alive() noexcept
    {
        std::uint32_t count = this->_strong.load(MemoryOrder::Relaxed);

        while(count != 0)
        {
            if(this->_strong.compare_exchange(count,
                                              count + 1,
                                              MemoryOrder::Acquire,
                                              MemoryOrder::Relaxed))
            {
                return true;
            }
        }

        return false;
    }

    STDROMANO_FORCE_INLINE void inc_weak() noexcept
    {
        this->_weak.fetch_add(1, MemoryOrder::Relaxed);
    }

    STDROMANO_FORCE_INLINE bool dec_weak() noexcept
    {
        return this->_weak.fetch_sub(1, MemoryOrder::AcqRel) == 1;
    }

    STDROMANO_FORCE_INLINE std::uint32_t strong_count() const noexcept
    {
        return this->_strong.load(MemoryOrder::Relaxed);
    }

    // Nobody else can create a reference concurrently when we hold the only strong and no weak exists
    STDROMANO_FORCE_INLINE bool is_sole_owner() const noexcept
    {
#if defined(STDROMANO_WIN)
        return false;
#else
        return this->_strong.load(MemoryOrder::Acquire) == 1 &&
               this->_weak.load(MemoryOrder::Acquire) == 1;
#endif /* defined(STDROMANO_WIN) */
    }
};

// Single allocation: [RefCount | padding | T], the smart pointers only hold the T*
template <typename T, bool ThreadSafe>
struct SharedBlock
{
    using RefCount = std::conditional_t<ThreadSafe, RefCountMT, RefCountST>;

    static_assert(std::is_trivially_destructible_v<RefCount>);

    static constexpr std::size_t ALIGNMENT = alignof(T) > alignof(RefCount) ? alignof(T)
                                                                           : alignof(RefCount);
    static constexpr std::size_t OBJECT_OFFSET = (sizeof(RefCount) + alignof(T) - 1) &
                                                 ~(alignof(T) - 1);

    static STDROMANO_FORCE_INLINE RefCount* refcount(T* ptr) noexcept
    {
        return reinterpret_cast<RefCount*>(static_cast<char*>(object_address(ptr)) -
                                           OBJECT_OFFSET);
    }

    template <typename... Args>
    static T* create(Args&&... args)
    {
        void* block = mem_aligned_alloc(OBJECT_OFFSET + sizeof(T), ALIGNMENT);

        if(block == nullptr)
            throw std::bad_alloc();

        ::new(block) RefCount();

        return construct_at<T>(block,
                               static_cast<char*>(block) + OBJECT_OFFSET,
                               std::forward<Args>(args)...);
    }

    static STDROMANO_FORCE_INLINE void release_strong(T* ptr) noexcept
    {
        RefCount* rc = refcount(ptr);

        if(rc->is_sole_owner())
        {
            destroy_at(ptr);
            mem_aligned_free(static_cast<void*>(rc));
            return;
        }

        if(rc->dec_strong())
        {
            destroy_at(ptr);
            release_weak(ptr);
        }
    }

    static STDROMANO_FORCE_INLINE void release_weak(T* ptr) noexcept
    {
        RefCount* rc = refcount(ptr);

        if(rc->dec_weak())
            mem_aligned_free(static_cast<void*>(rc));
    }
};

DETAIL_NAMESPACE_END

template <typename T>
class UniquePtr
{
    static_assert(!std::is_array_v<T>, "UniquePtr does not support arrays");

    template <typename U>
    friend class UniquePtr;

    T* _ptr = nullptr;

public:
    using element_type = T;

    constexpr UniquePtr() noexcept = default;

    constexpr UniquePtr(std::nullptr_t) noexcept {}

    // ptr must come from make_unique() or UniquePtr::release()
    explicit UniquePtr(T* ptr) noexcept : _ptr(ptr) {}

    UniquePtr(const UniquePtr&) = delete;
    UniquePtr& operator=(const UniquePtr&) = delete;

    UniquePtr(UniquePtr&& other) noexcept : _ptr(other.release()) {}

    template <typename U,
              typename = std::enable_if_t<!std::is_same_v<U, T> &&
                                          std::is_convertible_v<U*, T*> &&
                                          std::has_virtual_destructor_v<T>>>
    UniquePtr(UniquePtr<U>&& other) noexcept : _ptr(other.release())
    {
    }

    ~UniquePtr()
    {
        this->reset();
    }

    UniquePtr& operator=(UniquePtr&& other) noexcept
    {
        this->reset(other.release());
        return *this;
    }

    template <typename U,
              typename = std::enable_if_t<!std::is_same_v<U, T> &&
                                          std::is_convertible_v<U*, T*> &&
                                          std::has_virtual_destructor_v<T>>>
    UniquePtr& operator=(UniquePtr<U>&& other) noexcept
    {
        this->reset(other.release());
        return *this;
    }

    UniquePtr& operator=(std::nullptr_t) noexcept
    {
        this->reset();
        return *this;
    }

    STDROMANO_NO_DISCARD STDROMANO_FORCE_INLINE T* release() noexcept
    {
        T* ptr = this->_ptr;
        this->_ptr = nullptr;
        return ptr;
    }

    void reset(T* ptr = nullptr) noexcept
    {
        T* old = this->_ptr;
        this->_ptr = ptr;

        if(old == nullptr)
            return;

        void* block;

        if constexpr(std::is_polymorphic_v<T>)
            block = const_cast<void*>(dynamic_cast<const volatile void*>(old));
        else
            block = detail::object_address(old);

        detail::destroy_at(old);
        mem_aligned_free(block);
    }

    STDROMANO_FORCE_INLINE void swap(UniquePtr& other) noexcept
    {
        T* tmp = this->_ptr;
        this->_ptr = other._ptr;
        other._ptr = tmp;
    }

    STDROMANO_FORCE_INLINE T* get() const noexcept
    {
        return this->_ptr;
    }

    STDROMANO_FORCE_INLINE T& operator*() const noexcept
    {
        STDROMANO_ASSERT(this->_ptr != nullptr, "Dereferencing a null UniquePtr");
        return *this->_ptr;
    }

    STDROMANO_FORCE_INLINE T* operator->() const noexcept
    {
        STDROMANO_ASSERT(this->_ptr != nullptr, "Dereferencing a null UniquePtr");
        return this->_ptr;
    }

    STDROMANO_FORCE_INLINE explicit operator bool() const noexcept
    {
        return this->_ptr != nullptr;
    }
};

template <typename T, typename... Args>
STDROMANO_NO_DISCARD UniquePtr<T> make_unique(Args&&... args)
{
    void* address = mem_aligned_alloc(sizeof(T), alignof(T));

    if(address == nullptr)
        throw std::bad_alloc();

    return UniquePtr<T>(detail::construct_at<T>(address, address, std::forward<Args>(args)...));
}

template <typename T, bool ThreadSafe>
class WeakPtr;

template <typename T, bool ThreadSafe>
class SharedPtr;

DETAIL_NAMESPACE_BEGIN

template <typename T, bool ThreadSafe, typename... Args>
SharedPtr<T, ThreadSafe> make_shared(Args&&... args);

DETAIL_NAMESPACE_END

template <typename T, bool ThreadSafe = true>
class SharedPtr
{
    static_assert(!std::is_array_v<T>, "SharedPtr does not support arrays");

    using Block = detail::SharedBlock<T, ThreadSafe>;

    friend class WeakPtr<T, ThreadSafe>;

    template <typename U, bool TS, typename... Args>
    friend SharedPtr<U, TS> detail::make_shared(Args&&... args);

    T* _ptr = nullptr;

    struct AdoptTag
    {
    };

    STDROMANO_FORCE_INLINE SharedPtr(T* ptr, AdoptTag) noexcept : _ptr(ptr) {}

public:
    using element_type = T;
    using weak_type = WeakPtr<T, ThreadSafe>;

    static constexpr bool THREAD_SAFE = ThreadSafe;

    constexpr SharedPtr() noexcept = default;

    constexpr SharedPtr(std::nullptr_t) noexcept {}

    SharedPtr(const SharedPtr& other) noexcept : _ptr(other._ptr)
    {
        if(this->_ptr != nullptr)
            Block::refcount(this->_ptr)->inc_strong();
    }

    SharedPtr(SharedPtr&& other) noexcept : _ptr(other._ptr)
    {
        other._ptr = nullptr;
    }

    ~SharedPtr()
    {
        if(this->_ptr != nullptr)
            Block::release_strong(this->_ptr);
    }

    SharedPtr& operator=(const SharedPtr& other) noexcept
    {
        SharedPtr(other).swap(*this);
        return *this;
    }

    SharedPtr& operator=(SharedPtr&& other) noexcept
    {
        SharedPtr(std::move(other)).swap(*this);
        return *this;
    }

    SharedPtr& operator=(std::nullptr_t) noexcept
    {
        this->reset();
        return *this;
    }

    STDROMANO_FORCE_INLINE void reset() noexcept
    {
        SharedPtr().swap(*this);
    }

    STDROMANO_FORCE_INLINE void swap(SharedPtr& other) noexcept
    {
        T* tmp = this->_ptr;
        this->_ptr = other._ptr;
        other._ptr = tmp;
    }

    STDROMANO_FORCE_INLINE T* get() const noexcept
    {
        return this->_ptr;
    }

    STDROMANO_FORCE_INLINE T& operator*() const noexcept
    {
        STDROMANO_ASSERT(this->_ptr != nullptr, "Dereferencing a null SharedPtr");
        return *this->_ptr;
    }

    STDROMANO_FORCE_INLINE T* operator->() const noexcept
    {
        STDROMANO_ASSERT(this->_ptr != nullptr, "Dereferencing a null SharedPtr");
        return this->_ptr;
    }

    STDROMANO_FORCE_INLINE explicit operator bool() const noexcept
    {
        return this->_ptr != nullptr;
    }

    STDROMANO_FORCE_INLINE std::uint32_t use_count() const noexcept
    {
        return this->_ptr != nullptr ? Block::refcount(this->_ptr)->strong_count() : 0;
    }
};

template <typename T, bool ThreadSafe = true>
class WeakPtr
{
    using Block = detail::SharedBlock<T, ThreadSafe>;

    T* _ptr = nullptr;

public:
    using element_type = T;

    constexpr WeakPtr() noexcept = default;

    constexpr WeakPtr(std::nullptr_t) noexcept {}

    WeakPtr(const SharedPtr<T, ThreadSafe>& shared) noexcept : _ptr(shared._ptr)
    {
        if(this->_ptr != nullptr)
            Block::refcount(this->_ptr)->inc_weak();
    }

    WeakPtr(const WeakPtr& other) noexcept : _ptr(other._ptr)
    {
        if(this->_ptr != nullptr)
            Block::refcount(this->_ptr)->inc_weak();
    }

    WeakPtr(WeakPtr&& other) noexcept : _ptr(other._ptr)
    {
        other._ptr = nullptr;
    }

    ~WeakPtr()
    {
        if(this->_ptr != nullptr)
            Block::release_weak(this->_ptr);
    }

    WeakPtr& operator=(const WeakPtr& other) noexcept
    {
        WeakPtr(other).swap(*this);
        return *this;
    }

    WeakPtr& operator=(WeakPtr&& other) noexcept
    {
        WeakPtr(std::move(other)).swap(*this);
        return *this;
    }

    WeakPtr& operator=(const SharedPtr<T, ThreadSafe>& shared) noexcept
    {
        WeakPtr(shared).swap(*this);
        return *this;
    }

    STDROMANO_FORCE_INLINE void reset() noexcept
    {
        WeakPtr().swap(*this);
    }

    STDROMANO_FORCE_INLINE void swap(WeakPtr& other) noexcept
    {
        T* tmp = this->_ptr;
        this->_ptr = other._ptr;
        other._ptr = tmp;
    }

    STDROMANO_FORCE_INLINE std::uint32_t use_count() const noexcept
    {
        return this->_ptr != nullptr ? Block::refcount(this->_ptr)->strong_count() : 0;
    }

    STDROMANO_FORCE_INLINE bool expired() const noexcept
    {
        return this->use_count() == 0;
    }

    STDROMANO_NO_DISCARD SharedPtr<T, ThreadSafe> lock() const noexcept
    {
        if(this->_ptr != nullptr && Block::refcount(this->_ptr)->inc_strong_if_alive())
            return SharedPtr<T, ThreadSafe>(this->_ptr, typename SharedPtr<T, ThreadSafe>::AdoptTag{});

        return SharedPtr<T, ThreadSafe>();
    }
};

DETAIL_NAMESPACE_BEGIN

template <typename T, bool ThreadSafe, typename... Args>
SharedPtr<T, ThreadSafe> make_shared(Args&&... args)
{
    return SharedPtr<T, ThreadSafe>(SharedBlock<T, ThreadSafe>::create(std::forward<Args>(args)...),
                                    typename SharedPtr<T, ThreadSafe>::AdoptTag{});
}

DETAIL_NAMESPACE_END

template <typename T, typename... Args>
STDROMANO_NO_DISCARD STDROMANO_FORCE_INLINE SharedPtr<T, true> make_shared(Args&&... args)
{
    return detail::make_shared<T, true>(std::forward<Args>(args)...);
}

template <typename T, typename... Args>
STDROMANO_NO_DISCARD STDROMANO_FORCE_INLINE SharedPtr<T, false> make_shared_st(Args&&... args)
{
    return detail::make_shared<T, false>(std::forward<Args>(args)...);
}

template <typename T>
using SharedPtrST = SharedPtr<T, false>;

template <typename T>
using WeakPtrST = WeakPtr<T, false>;

template <typename T, typename U>
STDROMANO_FORCE_INLINE bool operator==(const UniquePtr<T>& a, const UniquePtr<U>& b) noexcept
{
    return a.get() == b.get();
}

template <typename T, typename U>
STDROMANO_FORCE_INLINE bool operator!=(const UniquePtr<T>& a, const UniquePtr<U>& b) noexcept
{
    return a.get() != b.get();
}

template <typename T>
STDROMANO_FORCE_INLINE bool operator==(const UniquePtr<T>& a, std::nullptr_t) noexcept
{
    return a.get() == nullptr;
}

template <typename T>
STDROMANO_FORCE_INLINE bool operator!=(const UniquePtr<T>& a, std::nullptr_t) noexcept
{
    return a.get() != nullptr;
}

template <typename T, typename U, bool TS>
STDROMANO_FORCE_INLINE bool operator==(const SharedPtr<T, TS>& a, const SharedPtr<U, TS>& b) noexcept
{
    return a.get() == b.get();
}

template <typename T, typename U, bool TS>
STDROMANO_FORCE_INLINE bool operator!=(const SharedPtr<T, TS>& a, const SharedPtr<U, TS>& b) noexcept
{
    return a.get() != b.get();
}

template <typename T, bool TS>
STDROMANO_FORCE_INLINE bool operator==(const SharedPtr<T, TS>& a, std::nullptr_t) noexcept
{
    return a.get() == nullptr;
}

template <typename T, bool TS>
STDROMANO_FORCE_INLINE bool operator!=(const SharedPtr<T, TS>& a, std::nullptr_t) noexcept
{
    return a.get() != nullptr;
}

STDROMANO_NAMESPACE_END

#endif // !defined(__STDROMANO_MEMORY)
