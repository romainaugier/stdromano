// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - Present Romain Augier
// All rights reserved.

#include "stdromano/mutex.hpp"

#include <climits>

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

STDROMANO_NAMESPACE_END
