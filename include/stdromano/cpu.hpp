// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - Present Romain Augier
// All rights reserved.

#if !defined(__STDROMANO_CPU)
#define __STDROMANO_CPU

#include "stdromano/stdromano.hpp"

#if defined(STDROMANO_MSVC)
#include <intrin.h>
#else
#include <x86intrin.h>
#endif /* defined(STDROMANO_MSVC) */

STDROMANO_NAMESPACE_BEGIN

#if defined(STDROMANO_MSVC)
#include <intrin.h>
#define cpuid(__regs, __eax) __cpuid(__regs, __eax)
#define cpuidex(__regs, __eax, __ecx) __cpuidex(__regs, __eax, __ecx)
#else
#define cpuid(__regs, __eax)                                                                       \
    asm volatile("cpuid"                                                                           \
                 : "=a"((__regs)[0]), "=b"((__regs)[1]), "=c"((__regs)[2]), "=d"((__regs)[3])      \
                 : "a"(__eax), "c"(0))
#define cpuidex(__regs, __eax, __ecx)                                                              \
    asm volatile("cpuid"                                                                           \
                 : "=a"((__regs)[0]), "=b"((__regs)[1]), "=c"((__regs)[2]), "=d"((__regs)[3])      \
                 : "a"(__eax), "c"(__ecx))
#endif /* defined(STDROMANO_MSVC) */

STDROMANO_FORCE_INLINE uint64_t cpu_rdtsc() noexcept
{
    return __rdtsc();
}

STDROMANO_API void cpu_check() noexcept;

/* The string must be allocated before, and should be sizeof(int) * 12 + 1 (49 bytes) */
/* Returns false if the name can't be found */
STDROMANO_API bool cpu_get_name(char* name) noexcept;

/* Returns the cpu frequency in MHz, found during initialization (via cpuid or system calls) */
STDROMANO_API uint32_t cpu_get_frequency() noexcept;

/* Returns the current cpu frequency in MHz (via system calls) */
STDROMANO_API uint32_t cpu_get_current_frequency() noexcept;

STDROMANO_API void cpu_get_current_frequency_set_refresh_rate(uint32_t refresh_rate) noexcept;

enum CPUCache_ : std::uint32_t
{
    CPUCache_L1,
    CPUCache_L2,
    CPUCache_L3,
};

STDROMANO_API std::size_t cpu_get_cache_size(std::uint32_t cache) noexcept;

STDROMANO_NAMESPACE_END

#endif /* !defined(__STDROMANO_CPU) */