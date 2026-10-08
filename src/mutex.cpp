// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - Present Romain Augier
// All rights reserved.

#include "stdromano/mutex.hpp"

#include <climits>
#include <ctime>

#if defined(STDROMANO_WIN)
#include <Windows.h>
#if defined(STDROMANO_MSVC)
#pragma comment(lib, "Synchronization.lib")
#endif /* defined(STDROMANO_MSVC) */
#elif defined(STDROMANO_LINUX)
#include <linux/futex.h>
#include <sys/syscall.h>
#include <unistd.h>
#elif defined(STDROMANO_APPLE)
// Private but stable since macOS 10.12, used by libc++ for std::atomic::wait
extern "C" int __ulock_wait(std::uint32_t operation,
                            void* address,
                            std::uint64_t value,
                            std::uint32_t timeout_us);
extern "C" int __ulock_wake(std::uint32_t operation, void* address, std::uint64_t wake_value);
#elif defined(STDROMANO_FREEBSD)
#include <sys/types.h>
#include <sys/umtx.h>
#elif defined(STDROMANO_OPENBSD)
#include <sys/futex.h>
#include <sys/time.h>
#elif defined(STDROMANO_NETBSD)
#include <sys/futex.h>
#include <sys/syscall.h>
#include <unistd.h>
#elif defined(STDROMANO_DRAGONFLY)
#include <unistd.h>
#endif /* defined(STDROMANO_WIN) */

#if defined(STDROMANO_MSVC) && (defined(STDROMANO_X86_64) || defined(STDROMANO_X86))
#include <immintrin.h>
#endif /* defined(STDROMANO_MSVC) && (defined(STDROMANO_X86_64) || defined(STDROMANO_X86)) */

STDROMANO_NAMESPACE_BEGIN

DETAIL_NAMESPACE_BEGIN

#if defined(STDROMANO_APPLE)
static constexpr std::uint32_t UL_COMPARE_AND_WAIT = 1;
static constexpr std::uint32_t ULF_WAKE_ALL = 0x00000100;
static constexpr std::uint32_t ULF_NO_ERRNO = 0x01000000;
#endif /* defined(STDROMANO_APPLE) */

STDROMANO_FORCE_INLINE static void cpu_relax() noexcept
{
#if defined(STDROMANO_MSVC)
#if defined(STDROMANO_X86_64) || defined(STDROMANO_X86)
    _mm_pause();
#elif defined(STDROMANO_ARM)
    __yield();
#endif /* defined(STDROMANO_X86_64) || defined(STDROMANO_X86) */
#else
#if defined(STDROMANO_X86_64) || defined(STDROMANO_X86)
    __builtin_ia32_pause();
#elif defined(STDROMANO_ARM)
    __asm__ __volatile__("yield" ::: "memory");
#endif /* defined(STDROMANO_X86_64) || defined(STDROMANO_X86) */
#endif /* defined(STDROMANO_MSVC) */
}

STDROMANO_FORCE_INLINE static std::uint32_t* as_word(volatile std::uint32_t* address) noexcept
{
    return const_cast<std::uint32_t*>(address);
}

void futex_wait(volatile std::uint32_t* address, std::uint32_t expected) noexcept
{
#if defined(STDROMANO_WIN)
    WaitOnAddress(address, &expected, sizeof(std::uint32_t), INFINITE);
#elif defined(STDROMANO_LINUX)
    syscall(SYS_futex, as_word(address), FUTEX_WAIT_PRIVATE, expected, nullptr, nullptr, 0);
#elif defined(STDROMANO_APPLE)
    __ulock_wait(UL_COMPARE_AND_WAIT | ULF_NO_ERRNO, as_word(address), expected, 0);
#elif defined(STDROMANO_FREEBSD)
    _umtx_op(as_word(address),
             UMTX_OP_WAIT_UINT_PRIVATE,
             static_cast<u_long>(expected),
             nullptr,
             nullptr);
#elif defined(STDROMANO_OPENBSD)
    futex(address, FUTEX_WAIT | FUTEX_PRIVATE_FLAG, static_cast<int>(expected), nullptr, nullptr);
#elif defined(STDROMANO_NETBSD)
    syscall(SYS___futex,
            as_word(address),
            FUTEX_WAIT | FUTEX_PRIVATE_FLAG,
            static_cast<int>(expected),
            nullptr,
            nullptr,
            0,
            0);
#elif defined(STDROMANO_DRAGONFLY)
    umtx_sleep(reinterpret_cast<volatile const int*>(address), static_cast<int>(expected), 0);
#endif /* defined(STDROMANO_WIN) */
}

static std::uint64_t monotonic_ns() noexcept
{
#if defined(STDROMANO_WIN)
    static const std::uint64_t frequency = []() {
        LARGE_INTEGER value;
        QueryPerformanceFrequency(&value);
        return static_cast<std::uint64_t>(value.QuadPart);
    }();

    LARGE_INTEGER counter;
    QueryPerformanceCounter(&counter);

    const std::uint64_t ticks = static_cast<std::uint64_t>(counter.QuadPart);

    return (ticks / frequency) * 1000000000ull + ((ticks % frequency) * 1000000000ull) / frequency;
#else
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);

    return static_cast<std::uint64_t>(now.tv_sec) * 1000000000ull +
           static_cast<std::uint64_t>(now.tv_nsec);
#endif /* defined(STDROMANO_WIN) */
}

#if !defined(STDROMANO_WIN)
static struct timespec to_timespec(const std::uint64_t ns) noexcept
{
    struct timespec ts;
    ts.tv_sec = static_cast<time_t>(ns / 1000000000ull);
    ts.tv_nsec = static_cast<long>(ns % 1000000000ull);
    return ts;
}
#endif /* !defined(STDROMANO_WIN) */

void futex_wait_for(volatile std::uint32_t* address,
                    std::uint32_t expected,
                    std::uint64_t timeout_ns) noexcept
{
#if defined(STDROMANO_WIN)
    const std::uint64_t timeout_ms = (timeout_ns + 999999ull) / 1000000ull;
    const DWORD wait_ms = timeout_ms >= INFINITE ? INFINITE - 1 : static_cast<DWORD>(timeout_ms);
    WaitOnAddress(address, &expected, sizeof(std::uint32_t), wait_ms);
#elif defined(STDROMANO_LINUX)
    const struct timespec ts = to_timespec(timeout_ns);
    syscall(SYS_futex, as_word(address), FUTEX_WAIT_PRIVATE, expected, &ts, nullptr, 0);
#elif defined(STDROMANO_APPLE)
    const std::uint64_t timeout_us = (timeout_ns + 999ull) / 1000ull;
    const std::uint32_t wait_us = timeout_us >= 0xFFFFFFFFull ? 0xFFFFFFFEu
                                                               : static_cast<std::uint32_t>(timeout_us);
    __ulock_wait(UL_COMPARE_AND_WAIT | ULF_NO_ERRNO, as_word(address), expected, wait_us);
#elif defined(STDROMANO_FREEBSD)
    struct timespec ts = to_timespec(timeout_ns);
    _umtx_op(as_word(address),
             UMTX_OP_WAIT_UINT_PRIVATE,
             static_cast<u_long>(expected),
             nullptr,
             &ts);
#elif defined(STDROMANO_OPENBSD)
    const struct timespec ts = to_timespec(timeout_ns);
    futex(address, FUTEX_WAIT | FUTEX_PRIVATE_FLAG, static_cast<int>(expected), &ts, nullptr);
#elif defined(STDROMANO_NETBSD)
    const struct timespec ts = to_timespec(timeout_ns);
    syscall(SYS___futex,
            as_word(address),
            FUTEX_WAIT | FUTEX_PRIVATE_FLAG,
            static_cast<int>(expected),
            &ts,
            nullptr,
            0,
            0);
#elif defined(STDROMANO_DRAGONFLY)
    const std::uint64_t timeout_us = (timeout_ns + 999ull) / 1000ull;
    const int wait_us = timeout_us >= static_cast<std::uint64_t>(INT_MAX) ? INT_MAX
                                                                          : static_cast<int>(timeout_us);
    umtx_sleep(reinterpret_cast<volatile const int*>(address), static_cast<int>(expected), wait_us);
#endif /* defined(STDROMANO_WIN) */
}

void futex_wake_one(volatile std::uint32_t* address) noexcept
{
#if defined(STDROMANO_WIN)
    WakeByAddressSingle(as_word(address));
#elif defined(STDROMANO_LINUX)
    syscall(SYS_futex, as_word(address), FUTEX_WAKE_PRIVATE, 1, nullptr, nullptr, 0);
#elif defined(STDROMANO_APPLE)
    __ulock_wake(UL_COMPARE_AND_WAIT | ULF_NO_ERRNO, as_word(address), 0);
#elif defined(STDROMANO_FREEBSD)
    _umtx_op(as_word(address), UMTX_OP_WAKE_PRIVATE, 1, nullptr, nullptr);
#elif defined(STDROMANO_OPENBSD)
    futex(address, FUTEX_WAKE | FUTEX_PRIVATE_FLAG, 1, nullptr, nullptr);
#elif defined(STDROMANO_NETBSD)
    syscall(SYS___futex, as_word(address), FUTEX_WAKE | FUTEX_PRIVATE_FLAG, 1, nullptr, nullptr, 0, 0);
#elif defined(STDROMANO_DRAGONFLY)
    umtx_wakeup(reinterpret_cast<volatile const int*>(address), 1);
#endif /* defined(STDROMANO_WIN) */
}

void futex_wake_all(volatile std::uint32_t* address) noexcept
{
#if defined(STDROMANO_WIN)
    WakeByAddressAll(as_word(address));
#elif defined(STDROMANO_LINUX)
    syscall(SYS_futex, as_word(address), FUTEX_WAKE_PRIVATE, INT_MAX, nullptr, nullptr, 0);
#elif defined(STDROMANO_APPLE)
    __ulock_wake(UL_COMPARE_AND_WAIT | ULF_NO_ERRNO | ULF_WAKE_ALL, as_word(address), 0);
#elif defined(STDROMANO_FREEBSD)
    _umtx_op(as_word(address), UMTX_OP_WAKE_PRIVATE, INT_MAX, nullptr, nullptr);
#elif defined(STDROMANO_OPENBSD)
    futex(address, FUTEX_WAKE | FUTEX_PRIVATE_FLAG, INT_MAX, nullptr, nullptr);
#elif defined(STDROMANO_NETBSD)
    syscall(SYS___futex,
            as_word(address),
            FUTEX_WAKE | FUTEX_PRIVATE_FLAG,
            INT_MAX,
            nullptr,
            nullptr,
            0,
            0);
#elif defined(STDROMANO_DRAGONFLY)
    umtx_wakeup(reinterpret_cast<volatile const int*>(address), 0);
#endif /* defined(STDROMANO_WIN) */
}

DETAIL_NAMESPACE_END

std::uint32_t Mutex::spin() noexcept
{
    static constexpr std::uint32_t SPIN_COUNT = 100;

    for(std::uint32_t i = 0;; ++i)
    {
        const std::uint32_t state = this->_state.load(MemoryOrder::Relaxed);

        // Waiters are already sleeping when CONTENDED, spinning would only delay us
        if(state != LOCKED || i == SPIN_COUNT)
            return state;

        detail::cpu_relax();
    }
}

void Mutex::lock_contended() noexcept
{
    std::uint32_t state = this->spin();

    if(state == UNLOCKED)
    {
        if(this->_state.compare_exchange(state, LOCKED, MemoryOrder::Acquire, MemoryOrder::Relaxed))
        {
            return;
        }
    }

    // Once we've waited we can't know if other waiters remain, so we always take it as CONTENDED
    for(;;)
    {
        if(state != CONTENDED && this->_state.exchange(CONTENDED, MemoryOrder::Acquire) == UNLOCKED)
            return;

        detail::futex_wait(this->state_address(), CONTENDED);

        state = this->spin();
    }
}

bool Mutex::lock_contended_for(const std::uint32_t timeout_ms) noexcept
{
    if(timeout_ms == INFINITE_TIMEOUT)
    {
        this->lock_contended();
        return true;
    }

    const std::uint64_t deadline = detail::monotonic_ns() +
                                   static_cast<std::uint64_t>(timeout_ms) * 1000000ull;

    std::uint32_t state = this->spin();

    if(state == UNLOCKED)
    {
        if(this->_state.compare_exchange(state, LOCKED, MemoryOrder::Acquire, MemoryOrder::Relaxed))
        {
            return true;
        }
    }

    // Leaving CONTENDED behind on timeout only costs the next unlock() a useless wake
    for(;;)
    {
        if(state != CONTENDED &&
           this->_state.exchange(CONTENDED, MemoryOrder::Acquire) == UNLOCKED)
        {
            return true;
        }

        const std::uint64_t now = detail::monotonic_ns();

        if(now >= deadline)
        {
            return false;
        }

        detail::futex_wait_for(this->state_address(), CONTENDED, deadline - now);

        state = this->spin();
    }
}

STDROMANO_NAMESPACE_END
