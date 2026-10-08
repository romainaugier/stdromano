// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - Present Romain Augier
// All rights reserved.

#pragma once

#if !defined(__STDROMANO_MUTEX)
#define __STDROMANO_MUTEX

#include "stdromano/stdromano.hpp"
#include "stdromano/atomic.hpp"

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <type_traits>

STDROMANO_NAMESPACE_BEGIN

DETAIL_NAMESPACE_BEGIN

// Blocks while *address == expected, can return spuriously
STDROMANO_API void futex_wait(volatile std::uint32_t* address, std::uint32_t expected) noexcept;

// Blocks while *address == expected for at most timeout_ns, can return spuriously or early
STDROMANO_API void futex_wait_for(volatile std::uint32_t* address,
                                  std::uint32_t expected,
                                  std::uint64_t timeout_ns) noexcept;

STDROMANO_API void futex_wake_one(volatile std::uint32_t* address) noexcept;

STDROMANO_API void futex_wake_all(volatile std::uint32_t* address) noexcept;

DETAIL_NAMESPACE_END

class STDROMANO_API Mutex
{
    static constexpr std::uint32_t UNLOCKED = 0;
    static constexpr std::uint32_t LOCKED = 1;
    static constexpr std::uint32_t CONTENDED = 2;

    Atomic<std::uint32_t> _state{UNLOCKED};

    void lock_contended() noexcept;

    bool lock_contended_for(const std::uint32_t timeout_ms) noexcept;

    std::uint32_t spin() noexcept;

    STDROMANO_FORCE_INLINE volatile std::uint32_t* state_address() noexcept
    {
        return reinterpret_cast<volatile std::uint32_t*>(&this->_state);
    }

public:
    static constexpr std::uint32_t INFINITE_TIMEOUT = 0xFFFFFFFFul;

    constexpr Mutex() noexcept = default;

    Mutex(const Mutex&) = delete;
    Mutex& operator=(const Mutex&) = delete;
    Mutex(Mutex&&) = delete;
    Mutex& operator=(Mutex&&) = delete;

    STDROMANO_FORCE_INLINE void lock() noexcept
    {
        std::uint32_t expected = UNLOCKED;

        if(!this->_state.compare_exchange(expected,
                                          LOCKED,
                                          MemoryOrder::Acquire,
                                          MemoryOrder::Relaxed))
        {
            this->lock_contended();
        }
    }

    STDROMANO_NO_DISCARD STDROMANO_FORCE_INLINE bool try_lock() noexcept
    {
        std::uint32_t expected = UNLOCKED;

        return this->_state.compare_exchange(expected,
                                             LOCKED,
                                             MemoryOrder::Acquire,
                                             MemoryOrder::Relaxed);
    }

    // timeout_ms == 0 only tries once, INFINITE_TIMEOUT behaves like lock()
    STDROMANO_NO_DISCARD STDROMANO_FORCE_INLINE bool try_lock_for(const std::uint32_t timeout_ms) noexcept
    {
        if(this->try_lock())
            return true;

        return timeout_ms != 0 && this->lock_contended_for(timeout_ms);
    }

    STDROMANO_FORCE_INLINE void unlock() noexcept
    {
        if(this->_state.exchange(UNLOCKED, MemoryOrder::Release) == CONTENDED)
            detail::futex_wake_one(this->state_address());
    }
};

static_assert(sizeof(Atomic<std::uint32_t>) == sizeof(std::uint32_t));
static_assert(std::is_standard_layout_v<Atomic<std::uint32_t>>);

struct DeferLockTag
{
};

struct TryToLockTag
{
};

struct AdoptLockTag
{
};

inline constexpr DeferLockTag defer_lock{};
inline constexpr TryToLockTag try_to_lock{};
inline constexpr AdoptLockTag adopt_lock{};

template <typename M>
class ScopedLock
{
    M& _mutex;

public:
    using mutex_type = M;

    explicit ScopedLock(M& mutex) noexcept : _mutex(mutex)
    {
        this->_mutex.lock();
    }

    ScopedLock(M& mutex, AdoptLockTag) noexcept : _mutex(mutex) {}

    ~ScopedLock()
    {
        this->_mutex.unlock();
    }

    ScopedLock(const ScopedLock&) = delete;
    ScopedLock& operator=(const ScopedLock&) = delete;
};

template <typename M>
class UniqueLock
{
    M* _mutex = nullptr;
    bool _owns = false;

public:
    using mutex_type = M;

    UniqueLock() noexcept = default;

    explicit UniqueLock(M& mutex) noexcept : _mutex(&mutex), _owns(true)
    {
        mutex.lock();
    }

    UniqueLock(M& mutex, DeferLockTag) noexcept : _mutex(&mutex) {}

    UniqueLock(M& mutex, TryToLockTag) noexcept : _mutex(&mutex), _owns(mutex.try_lock()) {}

    UniqueLock(M& mutex, AdoptLockTag) noexcept : _mutex(&mutex), _owns(true) {}

    ~UniqueLock()
    {
        if(this->_owns)
            this->_mutex->unlock();
    }

    UniqueLock(const UniqueLock&) = delete;
    UniqueLock& operator=(const UniqueLock&) = delete;

    UniqueLock(UniqueLock&& other) noexcept : _mutex(other._mutex), _owns(other._owns)
    {
        other._mutex = nullptr;
        other._owns = false;
    }

    UniqueLock& operator=(UniqueLock&& other) noexcept
    {
        if(this != &other)
        {
            if(this->_owns)
                this->_mutex->unlock();

            this->_mutex = other._mutex;
            this->_owns = other._owns;
            other._mutex = nullptr;
            other._owns = false;
        }

        return *this;
    }

    void lock() noexcept
    {
        STDROMANO_ASSERT(this->_mutex != nullptr && !this->_owns,
                         "UniqueLock::lock() without mutex or already owned");
        this->_mutex->lock();
        this->_owns = true;
    }

    STDROMANO_NO_DISCARD bool try_lock() noexcept
    {
        STDROMANO_ASSERT(this->_mutex != nullptr && !this->_owns,
                         "UniqueLock::try_lock() without mutex or already owned");
        this->_owns = this->_mutex->try_lock();
        return this->_owns;
    }

    void unlock() noexcept
    {
        STDROMANO_ASSERT(this->_owns, "UniqueLock::unlock() on a lock that is not owned");
        this->_mutex->unlock();
        this->_owns = false;
    }

    STDROMANO_NO_DISCARD M* release() noexcept
    {
        M* mutex = this->_mutex;
        this->_mutex = nullptr;
        this->_owns = false;
        return mutex;
    }

    STDROMANO_FORCE_INLINE bool owns_lock() const noexcept
    {
        return this->_owns;
    }

    STDROMANO_FORCE_INLINE explicit operator bool() const noexcept
    {
        return this->_owns;
    }

    STDROMANO_FORCE_INLINE M* mutex() const noexcept
    {
        return this->_mutex;
    }
};

STDROMANO_NAMESPACE_END

#endif // !defined(__STDROMANO_MUTEX)
