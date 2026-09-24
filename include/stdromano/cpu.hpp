// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - Present Romain Augier
// All rights reserved.

#pragma once

#if !defined(__STDROMANO_CPU)
#define __STDROMANO_CPU

#include "stdromano/stdromano.hpp"

#if defined(STDROMANO_MSVC)
#include <intrin.h>
#elif defined(STDROMANO_INTEL)
#include <x86intrin.h>
#endif /* defined(STDROMANO_MSVC) */

STDROMANO_NAMESPACE_BEGIN

#if defined(STDROMANO_MSVC) && defined(STDROMANO_AARCH64)
#define STDROMANO_ARM64_SYSREG(op0, op1, crn, crm, op2)                                            \
    ((((op0) & 1) << 14) | (((op1) & 7) << 11) | (((crn) & 15) << 7) | (((crm) & 15) << 3) |       \
     ((op2) & 7))
#define STDROMANO_ARM64_CNTVCT STDROMANO_ARM64_SYSREG(3, 3, 14, 0, 2)
#define STDROMANO_ARM64_CNTFRQ STDROMANO_ARM64_SYSREG(3, 3, 14, 0, 0)
#endif /* defined(STDROMANO_MSVC) && defined(STDROMANO_AARCH64) */

#if defined(STDROMANO_INTEL)

#if defined(STDROMANO_MSVC)
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

#endif /* defined(STDROMANO_INTEL) */

/*
    Reads the cpu timestamp counter.

    On x86 this is rdtsc and the returned value is a cycle count (at the invariant tsc rate).
    On aarch64 this is the virtual counter cntvct_el0, which does NOT tick at the cpu frequency
    (24 MHz on Apple Silicon, 1-100 MHz on other implementations). Use cpu_rdtsc_frequency() to
    convert the delta to seconds, and never assume the value is a cycle count.
*/
STDROMANO_FORCE_INLINE std::uint64_t cpu_rdtsc() noexcept
{
#if defined(STDROMANO_INTEL)
    return __rdtsc();
#elif defined(STDROMANO_AARCH64)
#if defined(STDROMANO_MSVC)
    return static_cast<std::uint64_t>(_ReadStatusReg(STDROMANO_ARM64_CNTVCT));
#else
    std::uint64_t value;
    asm volatile("mrs %0, cntvct_el0" : "=r"(value));
    return value;
#endif /* defined(STDROMANO_MSVC) */
#else
    return 0;
#endif /* defined(STDROMANO_INTEL) */
}

/* Returns the tick rate of cpu_rdtsc() in Hz, or 0 when the counter ticks at the cpu frequency */
STDROMANO_FORCE_INLINE std::uint64_t cpu_rdtsc_frequency() noexcept
{
#if defined(STDROMANO_AARCH64)
#if defined(STDROMANO_MSVC)
    return static_cast<std::uint64_t>(_ReadStatusReg(STDROMANO_ARM64_CNTFRQ));
#else
    std::uint64_t value;
    asm volatile("mrs %0, cntfrq_el0" : "=r"(value));
    return value;
#endif /* defined(STDROMANO_MSVC) */
#else
    return 0;
#endif /* defined(STDROMANO_AARCH64) */
}

enum CPUFeature : std::uint32_t
{
    /* x86_64 features */
    CPUFeature_MMX = 0,
    CPUFeature_SSE,
    CPUFeature_SSE2,
    CPUFeature_SSE3,
    CPUFeature_SSSE3,
    CPUFeature_SSE4_1,
    CPUFeature_SSE4_2,
    CPUFeature_AVX,
    CPUFeature_AVX2,
    CPUFeature_FMA3,
    CPUFeature_F16C,
    CPUFeature_BMI1,
    CPUFeature_BMI2,
    CPUFeature_LZCNT,
    CPUFeature_POPCNT,
    CPUFeature_AES,
    CPUFeature_PCLMULQDQ,
    CPUFeature_SHA,
    CPUFeature_RDRAND,
    CPUFeature_RDSEED,
    CPUFeature_ADX,
    CPUFeature_AVX512F,
    CPUFeature_AVX512DQ,
    CPUFeature_AVX512IFMA,
    CPUFeature_AVX512PF,
    CPUFeature_AVX512ER,
    CPUFeature_AVX512CD,
    CPUFeature_AVX512BW,
    CPUFeature_AVX512VL,
    CPUFeature_AVX512VBMI,
    CPUFeature_AMX_TILE,
    CPUFeature_AMX_INT8,
    CPUFeature_AMX_BF16,

    /* aarch64 features */
    CPUFeature_NEON,
    CPUFeature_FP,
    CPUFeature_AdvSIMD,
    CPUFeature_AES_ARM,
    CPUFeature_PMULL,
    CPUFeature_SHA1,
    CPUFeature_SHA2,
    CPUFeature_SHA3,
    CPUFeature_SHA512,
    CPUFeature_CRC32,
    CPUFeature_LSE,
    CPUFeature_RDM,
    CPUFeature_DotProd,
    CPUFeature_FP16,
    CPUFeature_FHM,
    CPUFeature_FCMA,
    CPUFeature_JSCVT,
    CPUFeature_FRINTTS,
    CPUFeature_I8MM,
    CPUFeature_BF16,
    CPUFeature_SVE,
    CPUFeature_SVE2,
    CPUFeature_SM3,
    CPUFeature_SM4,
    CPUFeature_DIT,

    CPUFeature_COUNT,
};

/* Must be called once before any other cpu_* function (done in lib_entry) */
STDROMANO_API void cpu_check() noexcept;

STDROMANO_API bool cpu_has_feature(std::uint32_t feature) noexcept;

/* Returns the name of the feature, as written in the enum above */
STDROMANO_API const char* cpu_get_feature_name(std::uint32_t feature) noexcept;

/* Prints the features detected on the current cpu */
STDROMANO_API void cpu_print_features() noexcept;

#define STDROMANO_CPU_NAME_SZ 64

/* The string must be allocated before, and should be STDROMANO_CPU_NAME_SZ bytes */
/* Returns false if the name can't be found */
STDROMANO_API bool cpu_get_name(char* name) noexcept;

/* Returns the cpu frequency in MHz, found during initialization (via cpuid or system calls) */
/* Returns 0 when the platform does not expose it (Apple Silicon for example) */
STDROMANO_API std::uint32_t cpu_get_frequency() noexcept;

/* Returns the current cpu frequency in MHz (via system calls) */
STDROMANO_API std::uint32_t cpu_get_current_frequency() noexcept;

STDROMANO_API void cpu_get_current_frequency_set_refresh_rate(std::uint32_t refresh_rate) noexcept;

enum CPUCache_ : std::uint32_t
{
    CPUCache_L1,
    CPUCache_L2,
    CPUCache_L3,
};

STDROMANO_API std::size_t cpu_get_cache_size(std::uint32_t cache) noexcept;

STDROMANO_NAMESPACE_END

#endif /* !defined(__STDROMANO_CPU) */
