// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - Present Romain Augier
// All rights reserved.

#include "stdromano/cpu.hpp"

#include <cstdio>
#include <cstring>

#if defined(STDROMANO_WIN)
#include <Windows.h>
#include <powerbase.h>

typedef struct _PROCESSOR_POWER_INFORMATION
{
    ULONG Number;
    ULONG MaxMhz;
    ULONG CurrentMhz;
    ULONG MhzLimit;
    ULONG MaxIdleState;
    ULONG CurrentIdleState;
} PROCESSOR_POWER_INFORMATION, *PPROCESSOR_POWER_INFORMATION;

#elif defined(STDROMANO_UNIX)

#if !defined(__USE_POSIX199309)
#define __USE_POSIX199309
#endif /* !defined(__USE_POSIX199309) */
#include <time.h>
#include <unistd.h>

#if defined(STDROMANO_APPLE)
#include <sys/sysctl.h>
#include <sys/types.h>
#endif /* defined(STDROMANO_APPLE) */

#if defined(STDROMANO_LINUX) && defined(STDROMANO_ARM)
#include <sys/auxv.h>
#endif /* defined(STDROMANO_LINUX) && defined(STDROMANO_ARM) */

#if defined(STDROMANO_FREEBSD) && defined(STDROMANO_ARM)
#include <sys/auxv.h>
#include <sys/elf_common.h>
#endif /* defined(STDROMANO_FREEBSD) && defined(STDROMANO_ARM) */

#endif /* defined(STDROMANO_WIN) */

STDROMANO_NAMESPACE_BEGIN

static bool g_cpu_features[CPUFeature_COUNT] = {};
static char g_cpu_name[STDROMANO_CPU_NAME_SZ] = {};
static std::uint32_t g_cpu_freq_mhz = 0;
static std::size_t g_cpu_caches_sizes[3] = {};

static std::uint64_t g_get_frequency_counter = 0;
static std::uint32_t g_frequency = 0;
static std::uint32_t g_refresh_rate = 10000;

/* Keep in sync with the CPUFeature enum */
static const char* g_cpu_features_names[CPUFeature_COUNT] = {
    "MMX",        "SSE",      "SSE2",     "SSE3",      "SSSE3",       "SSE4.1",   "SSE4.2",
    "AVX",        "AVX2",     "FMA3",     "F16C",      "BMI1",        "BMI2",     "LZCNT",
    "POPCNT",     "AES",      "PCLMULQDQ","SHA",       "RDRAND",      "RDSEED",   "ADX",
    "AVX512F",    "AVX512DQ", "AVX512IFMA","AVX512PF", "AVX512ER",    "AVX512CD", "AVX512BW",
    "AVX512VL",   "AVX512VBMI","AMX-TILE","AMX-INT8",  "AMX-BF16",
    "NEON",       "FP",       "AdvSIMD",  "AES",       "PMULL",       "SHA1",     "SHA2",
    "SHA3",       "SHA512",   "CRC32",    "LSE",       "RDM",         "DotProd",  "FP16",
    "FHM",        "FCMA",     "JSCVT",    "FRINTTS",   "I8MM",        "BF16",     "SVE",
    "SVE2",       "SM3",      "SM4",      "DIT",
};

/********************************/
/* Features detection */
/********************************/

#if defined(STDROMANO_INTEL)

STDROMANO_FORCE_INLINE std::uint64_t cpu_xgetbv(std::uint32_t index) noexcept
{
#if defined(STDROMANO_MSVC)
    return _xgetbv(index);
#else
    std::uint32_t eax, edx;
    asm volatile(".byte 0x0f, 0x01, 0xd0" : "=a"(eax), "=d"(edx) : "c"(index));
    return (static_cast<std::uint64_t>(edx) << 32) | eax;
#endif /* defined(STDROMANO_MSVC) */
}

static void cpu_check_features() noexcept
{
    std::int32_t regs[4];
    std::memset(regs, 0, sizeof(regs));

    cpuid(regs, 0);

    const std::int32_t max_leaf = regs[0];

    if(max_leaf >= 1)
    {
        cpuid(regs, 1);

        g_cpu_features[CPUFeature_MMX] = (regs[3] & (1 << 23)) != 0;
        g_cpu_features[CPUFeature_SSE] = (regs[3] & (1 << 25)) != 0;
        g_cpu_features[CPUFeature_SSE2] = (regs[3] & (1 << 26)) != 0;
        g_cpu_features[CPUFeature_SSE3] = (regs[2] & (1 << 0)) != 0;
        g_cpu_features[CPUFeature_PCLMULQDQ] = (regs[2] & (1 << 1)) != 0;
        g_cpu_features[CPUFeature_SSSE3] = (regs[2] & (1 << 9)) != 0;
        g_cpu_features[CPUFeature_SSE4_1] = (regs[2] & (1 << 19)) != 0;
        g_cpu_features[CPUFeature_SSE4_2] = (regs[2] & (1 << 20)) != 0;
        g_cpu_features[CPUFeature_POPCNT] = (regs[2] & (1 << 23)) != 0;
        g_cpu_features[CPUFeature_AES] = (regs[2] & (1 << 25)) != 0;
        g_cpu_features[CPUFeature_F16C] = (regs[2] & (1 << 29)) != 0;
        g_cpu_features[CPUFeature_RDRAND] = (regs[2] & (1 << 30)) != 0;

        /* AVX and AVX512 also need the OS to save the ymm/zmm state, check xcr0 */
        const bool os_uses_xsave = (regs[2] & (1 << 27)) != 0;
        const bool cpu_has_avx = (regs[2] & (1 << 28)) != 0;
        const bool cpu_has_fma = (regs[2] & (1 << 12)) != 0;

        bool os_saves_ymm = false;
        bool os_saves_zmm = false;

        if(os_uses_xsave)
        {
            const std::uint64_t xcr0 = cpu_xgetbv(0);
            os_saves_ymm = (xcr0 & 0x6) == 0x6;
            os_saves_zmm = os_saves_ymm && ((xcr0 & 0xE0) == 0xE0);
        }

        g_cpu_features[CPUFeature_AVX] = cpu_has_avx && os_saves_ymm;
        g_cpu_features[CPUFeature_FMA3] = cpu_has_fma && os_saves_ymm;

        if(max_leaf >= 7)
        {
            cpuidex(regs, 7, 0);

            g_cpu_features[CPUFeature_BMI1] = (regs[1] & (1 << 3)) != 0;
            g_cpu_features[CPUFeature_AVX2] = ((regs[1] & (1 << 5)) != 0) && os_saves_ymm;
            g_cpu_features[CPUFeature_BMI2] = (regs[1] & (1 << 8)) != 0;
            g_cpu_features[CPUFeature_RDSEED] = (regs[1] & (1 << 18)) != 0;
            g_cpu_features[CPUFeature_ADX] = (regs[1] & (1 << 19)) != 0;
            g_cpu_features[CPUFeature_SHA] = (regs[1] & (1 << 29)) != 0;

            g_cpu_features[CPUFeature_AVX512F] = ((regs[1] & (1 << 16)) != 0) && os_saves_zmm;
            g_cpu_features[CPUFeature_AVX512DQ] = ((regs[1] & (1 << 17)) != 0) && os_saves_zmm;
            g_cpu_features[CPUFeature_AVX512IFMA] = ((regs[1] & (1 << 21)) != 0) && os_saves_zmm;
            g_cpu_features[CPUFeature_AVX512PF] = ((regs[1] & (1 << 26)) != 0) && os_saves_zmm;
            g_cpu_features[CPUFeature_AVX512ER] = ((regs[1] & (1 << 27)) != 0) && os_saves_zmm;
            g_cpu_features[CPUFeature_AVX512CD] = ((regs[1] & (1 << 28)) != 0) && os_saves_zmm;
            g_cpu_features[CPUFeature_AVX512BW] = ((regs[1] & (1 << 30)) != 0) && os_saves_zmm;
            g_cpu_features[CPUFeature_AVX512VL] = ((regs[1] & (1 << 31)) != 0) && os_saves_zmm;
            g_cpu_features[CPUFeature_AVX512VBMI] = ((regs[2] & (1 << 1)) != 0) && os_saves_zmm;

            g_cpu_features[CPUFeature_AMX_BF16] = (regs[3] & (1 << 22)) != 0;
            g_cpu_features[CPUFeature_AMX_TILE] = (regs[3] & (1 << 24)) != 0;
            g_cpu_features[CPUFeature_AMX_INT8] = (regs[3] & (1 << 25)) != 0;
        }
    }

    cpuid(regs, 0x80000000);

    if(static_cast<std::uint32_t>(regs[0]) >= 0x80000001)
    {
        cpuid(regs, 0x80000001);
        g_cpu_features[CPUFeature_LZCNT] = (regs[2] & (1 << 5)) != 0;
    }
}

#elif defined(STDROMANO_ARM)

#if defined(STDROMANO_APPLE)

static bool cpu_sysctl_flag(const char* name) noexcept
{
    std::int32_t value = 0;
    std::size_t size = sizeof(value);

    return sysctlbyname(name, &value, &size, nullptr, 0) == 0 && value != 0;
}

static void cpu_check_features() noexcept
{
    /* Every arm64 Apple cpu implements the base FP/AdvSIMD set */
    g_cpu_features[CPUFeature_NEON] = true;
    g_cpu_features[CPUFeature_FP] = true;
    g_cpu_features[CPUFeature_AdvSIMD] = true;

    g_cpu_features[CPUFeature_AES_ARM] = cpu_sysctl_flag("hw.optional.arm.FEAT_AES");
    g_cpu_features[CPUFeature_PMULL] = cpu_sysctl_flag("hw.optional.arm.FEAT_PMULL");
    g_cpu_features[CPUFeature_SHA1] = cpu_sysctl_flag("hw.optional.arm.FEAT_SHA1");
    g_cpu_features[CPUFeature_SHA2] = cpu_sysctl_flag("hw.optional.arm.FEAT_SHA256");
    g_cpu_features[CPUFeature_SHA3] = cpu_sysctl_flag("hw.optional.arm.FEAT_SHA3");
    g_cpu_features[CPUFeature_SHA512] = cpu_sysctl_flag("hw.optional.arm.FEAT_SHA512");
    g_cpu_features[CPUFeature_CRC32] = cpu_sysctl_flag("hw.optional.armv8_crc32");
    g_cpu_features[CPUFeature_LSE] = cpu_sysctl_flag("hw.optional.arm.FEAT_LSE");
    g_cpu_features[CPUFeature_RDM] = cpu_sysctl_flag("hw.optional.arm.FEAT_RDM");
    g_cpu_features[CPUFeature_DotProd] = cpu_sysctl_flag("hw.optional.arm.FEAT_DotProd");
    g_cpu_features[CPUFeature_FP16] = cpu_sysctl_flag("hw.optional.arm.FEAT_FP16");
    g_cpu_features[CPUFeature_FHM] = cpu_sysctl_flag("hw.optional.arm.FEAT_FHM");
    g_cpu_features[CPUFeature_FCMA] = cpu_sysctl_flag("hw.optional.arm.FEAT_FCMA");
    g_cpu_features[CPUFeature_JSCVT] = cpu_sysctl_flag("hw.optional.arm.FEAT_JSCVT");
    g_cpu_features[CPUFeature_FRINTTS] = cpu_sysctl_flag("hw.optional.arm.FEAT_FRINTTS");
    g_cpu_features[CPUFeature_I8MM] = cpu_sysctl_flag("hw.optional.arm.FEAT_I8MM");
    g_cpu_features[CPUFeature_BF16] = cpu_sysctl_flag("hw.optional.arm.FEAT_BF16");
}

#elif defined(STDROMANO_WIN)

static void cpu_check_features() noexcept
{
    g_cpu_features[CPUFeature_NEON] = IsProcessorFeaturePresent(PF_ARM_NEON_INSTRUCTIONS_AVAILABLE) != 0;
    g_cpu_features[CPUFeature_FP] = g_cpu_features[CPUFeature_NEON];
    g_cpu_features[CPUFeature_AdvSIMD] = g_cpu_features[CPUFeature_NEON];

    g_cpu_features[CPUFeature_CRC32] = IsProcessorFeaturePresent(PF_ARM_V8_CRC32_INSTRUCTIONS_AVAILABLE) != 0;
    g_cpu_features[CPUFeature_AES_ARM] = IsProcessorFeaturePresent(PF_ARM_V8_CRYPTO_INSTRUCTIONS_AVAILABLE) != 0;
    g_cpu_features[CPUFeature_PMULL] = g_cpu_features[CPUFeature_AES_ARM];
    g_cpu_features[CPUFeature_SHA1] = g_cpu_features[CPUFeature_AES_ARM];
    g_cpu_features[CPUFeature_SHA2] = g_cpu_features[CPUFeature_AES_ARM];
    g_cpu_features[CPUFeature_LSE] = IsProcessorFeaturePresent(PF_ARM_V81_ATOMIC_INSTRUCTIONS_AVAILABLE) != 0;

#if defined(PF_ARM_V82_DP_INSTRUCTIONS_AVAILABLE)
    g_cpu_features[CPUFeature_DotProd] = IsProcessorFeaturePresent(PF_ARM_V82_DP_INSTRUCTIONS_AVAILABLE) != 0;
#endif /* defined(PF_ARM_V82_DP_INSTRUCTIONS_AVAILABLE) */
#if defined(PF_ARM_V83_JSCVT_INSTRUCTIONS_AVAILABLE)
    g_cpu_features[CPUFeature_JSCVT] = IsProcessorFeaturePresent(PF_ARM_V83_JSCVT_INSTRUCTIONS_AVAILABLE) != 0;
#endif /* defined(PF_ARM_V83_JSCVT_INSTRUCTIONS_AVAILABLE) */
#if defined(PF_ARM_SVE_INSTRUCTIONS_AVAILABLE)
    g_cpu_features[CPUFeature_SVE] = IsProcessorFeaturePresent(PF_ARM_SVE_INSTRUCTIONS_AVAILABLE) != 0;
#endif /* defined(PF_ARM_SVE_INSTRUCTIONS_AVAILABLE) */
#if defined(PF_ARM_SVE2_INSTRUCTIONS_AVAILABLE)
    g_cpu_features[CPUFeature_SVE2] = IsProcessorFeaturePresent(PF_ARM_SVE2_INSTRUCTIONS_AVAILABLE) != 0;
#endif /* defined(PF_ARM_SVE2_INSTRUCTIONS_AVAILABLE) */
#if defined(PF_ARM_V82_FP16_INSTRUCTIONS_AVAILABLE)
    g_cpu_features[CPUFeature_FP16] = IsProcessorFeaturePresent(PF_ARM_V82_FP16_INSTRUCTIONS_AVAILABLE) != 0;
#endif /* defined(PF_ARM_V82_FP16_INSTRUCTIONS_AVAILABLE) */
#if defined(PF_ARM_V82_I8MM_INSTRUCTIONS_AVAILABLE)
    g_cpu_features[CPUFeature_I8MM] = IsProcessorFeaturePresent(PF_ARM_V82_I8MM_INSTRUCTIONS_AVAILABLE) != 0;
#endif /* defined(PF_ARM_V82_I8MM_INSTRUCTIONS_AVAILABLE) */
#if defined(PF_ARM_V86_BF16_INSTRUCTIONS_AVAILABLE)
    g_cpu_features[CPUFeature_BF16] = IsProcessorFeaturePresent(PF_ARM_V86_BF16_INSTRUCTIONS_AVAILABLE) != 0;
#endif /* defined(PF_ARM_V86_BF16_INSTRUCTIONS_AVAILABLE) */
}

#elif defined(STDROMANO_LINUX) || defined(STDROMANO_FREEBSD)

/* Bit positions are stable ABI, we decode them by hand to avoid depending on asm/hwcap.h */
static void cpu_decode_hwcaps(unsigned long hwcap, unsigned long hwcap2) noexcept
{
    g_cpu_features[CPUFeature_FP] = (hwcap & (1UL << 0)) != 0;
    g_cpu_features[CPUFeature_AdvSIMD] = (hwcap & (1UL << 1)) != 0;
    g_cpu_features[CPUFeature_NEON] = g_cpu_features[CPUFeature_AdvSIMD];
    g_cpu_features[CPUFeature_AES_ARM] = (hwcap & (1UL << 3)) != 0;
    g_cpu_features[CPUFeature_PMULL] = (hwcap & (1UL << 4)) != 0;
    g_cpu_features[CPUFeature_SHA1] = (hwcap & (1UL << 5)) != 0;
    g_cpu_features[CPUFeature_SHA2] = (hwcap & (1UL << 6)) != 0;
    g_cpu_features[CPUFeature_CRC32] = (hwcap & (1UL << 7)) != 0;
    g_cpu_features[CPUFeature_LSE] = (hwcap & (1UL << 8)) != 0;
    g_cpu_features[CPUFeature_FP16] = (hwcap & (1UL << 9)) != 0 && (hwcap & (1UL << 10)) != 0;
    g_cpu_features[CPUFeature_RDM] = (hwcap & (1UL << 12)) != 0;
    g_cpu_features[CPUFeature_JSCVT] = (hwcap & (1UL << 13)) != 0;
    g_cpu_features[CPUFeature_FCMA] = (hwcap & (1UL << 14)) != 0;
    g_cpu_features[CPUFeature_SHA3] = (hwcap & (1UL << 17)) != 0;
    g_cpu_features[CPUFeature_SM3] = (hwcap & (1UL << 18)) != 0;
    g_cpu_features[CPUFeature_SM4] = (hwcap & (1UL << 19)) != 0;
    g_cpu_features[CPUFeature_DotProd] = (hwcap & (1UL << 20)) != 0;
    g_cpu_features[CPUFeature_SHA512] = (hwcap & (1UL << 21)) != 0;
    g_cpu_features[CPUFeature_SVE] = (hwcap & (1UL << 22)) != 0;
    g_cpu_features[CPUFeature_FHM] = (hwcap & (1UL << 23)) != 0;
    g_cpu_features[CPUFeature_DIT] = (hwcap & (1UL << 24)) != 0;

    g_cpu_features[CPUFeature_SVE2] = (hwcap2 & (1UL << 1)) != 0;
    g_cpu_features[CPUFeature_FRINTTS] = (hwcap2 & (1UL << 8)) != 0;
    g_cpu_features[CPUFeature_I8MM] = (hwcap2 & (1UL << 13)) != 0;
    g_cpu_features[CPUFeature_BF16] = (hwcap2 & (1UL << 14)) != 0;
}

static void cpu_check_features() noexcept
{
#if defined(STDROMANO_LINUX)
    cpu_decode_hwcaps(getauxval(AT_HWCAP), getauxval(AT_HWCAP2));
#else
    unsigned long hwcap = 0;
    unsigned long hwcap2 = 0;

    elf_aux_info(AT_HWCAP, &hwcap, sizeof(hwcap));
    elf_aux_info(AT_HWCAP2, &hwcap2, sizeof(hwcap2));

    cpu_decode_hwcaps(hwcap, hwcap2);
#endif /* defined(STDROMANO_LINUX) */
}

#else

static void cpu_check_features() noexcept
{
#if defined(STDROMANO_AARCH64)
    /* FP and AdvSIMD are mandatory in armv8-a */
    g_cpu_features[CPUFeature_FP] = true;
    g_cpu_features[CPUFeature_AdvSIMD] = true;
    g_cpu_features[CPUFeature_NEON] = true;
#endif /* defined(STDROMANO_AARCH64) */
}

#endif /* defined(STDROMANO_APPLE) */

#else

static void cpu_check_features() noexcept {}

#endif /* defined(STDROMANO_INTEL) */

bool cpu_has_feature(std::uint32_t feature) noexcept
{
    if(feature >= CPUFeature_COUNT)
        return false;

    return g_cpu_features[feature];
}

const char* cpu_get_feature_name(std::uint32_t feature) noexcept
{
    if(feature >= CPUFeature_COUNT)
        return "Unknown";

    return g_cpu_features_names[feature];
}

void cpu_print_features() noexcept
{
    std::printf("CPU (%s): %s\n", STDROMANO_PLATFORM_STR, g_cpu_name);

    for(std::uint32_t i = 0; i < CPUFeature_COUNT; ++i)
    {
        if(g_cpu_features[i])
        {
            std::printf("  %s\n", g_cpu_features_names[i]);
        }
    }
}

/********************************/
/* Name */
/********************************/

static void cpu_check_name() noexcept
{
    std::memset(g_cpu_name, 0, sizeof(g_cpu_name));

#if defined(STDROMANO_INTEL)
    std::int32_t regs[12];

    cpuid(&regs[0], 0x80000000);

    if(static_cast<std::uint32_t>(regs[0]) < 0x80000004)
        return;

    cpuid(&regs[0], 0x80000002);
    cpuid(&regs[4], 0x80000003);
    cpuid(&regs[8], 0x80000004);

    std::memcpy(g_cpu_name, regs, 12 * sizeof(std::uint32_t));
    g_cpu_name[12 * sizeof(std::uint32_t)] = '\0';
#elif defined(STDROMANO_APPLE)
    std::size_t size = sizeof(g_cpu_name);

    if(sysctlbyname("machdep.cpu.brand_string", g_cpu_name, &size, nullptr, 0) != 0)
    {
        g_cpu_name[0] = '\0';
    }
#elif defined(STDROMANO_UNIX)
    std::FILE* f = std::fopen("/proc/cpuinfo", "r");

    if(f == nullptr)
        return;

    char line[512];

    while(std::fgets(line, sizeof(line), f) != nullptr)
    {
        /* "model name" on x86, "CPU part"/"Model" on arm depending on the kernel */
        const char* keys[3] = { "model name", "Model", "CPU part" };

        for(std::size_t k = 0; k < 3; ++k)
        {
            if(std::strncmp(line, keys[k], std::strlen(keys[k])) != 0)
                continue;

            const char* colon = std::strchr(line, ':');

            if(colon == nullptr)
                continue;

            colon++;

            while(*colon == ' ')
                colon++;

            std::strncpy(g_cpu_name, colon, sizeof(g_cpu_name) - 1);

            const std::size_t len = std::strlen(g_cpu_name);

            if(len > 0 && g_cpu_name[len - 1] == '\n')
                g_cpu_name[len - 1] = '\0';

            std::fclose(f);

            return;
        }
    }

    std::fclose(f);
#endif /* defined(STDROMANO_INTEL) */
}

bool cpu_get_name(char* name) noexcept
{
    if(g_cpu_name[0] == '\0')
    {
        name[0] = '\0';
        return false;
    }

    std::memcpy(name, g_cpu_name, STDROMANO_CPU_NAME_SZ);

    return true;
}

/********************************/
/* Frequency */
/********************************/

static std::uint32_t _get_cpu_frequency() noexcept
{
    if(g_get_frequency_counter % g_refresh_rate == 0)
    {
#if defined(STDROMANO_WIN)
        SYSTEM_INFO system_info;
        system_info.dwNumberOfProcessors = 0;

        GetSystemInfo(&system_info);

        const UINT num_cpus = system_info.dwNumberOfProcessors == 0 ? 1 : system_info.dwNumberOfProcessors;
        const ULONG size = num_cpus * sizeof(PROCESSOR_POWER_INFORMATION);

        LPBYTE p_buffer = (LPBYTE)LocalAlloc(LPTR, size);

        if(!p_buffer)
            return 0;

        if(CallNtPowerInformation(ProcessorInformation, NULL, 0, p_buffer, size) != 0)
        {
            LocalFree(p_buffer);
            return 0;
        }

        const PROCESSOR_POWER_INFORMATION* ppi = (PROCESSOR_POWER_INFORMATION*)p_buffer;
        const ULONG current = ppi->CurrentMhz;

        LocalFree(p_buffer);

        g_frequency = static_cast<std::uint32_t>(current);
#elif defined(STDROMANO_APPLE)
        std::uint64_t freq = 0;
        std::size_t size = sizeof(freq);

        /* Only available on intel macs, Apple Silicon does not expose the cpu frequency */
        if(sysctlbyname("hw.cpufrequency", &freq, &size, nullptr, 0) == 0)
        {
            g_frequency = static_cast<std::uint32_t>(freq / 1000000ULL);
        }
        else
        {
            g_frequency = 0;
        }
#elif defined(STDROMANO_INTEL) && defined(STDROMANO_UNIX)
        const std::uint64_t start = cpu_rdtsc();

        struct timespec wait_duration;
        wait_duration.tv_sec = 0;
        wait_duration.tv_nsec = 1000000;

        nanosleep(&wait_duration, nullptr);

        const std::uint64_t end = cpu_rdtsc();

        const double frequency = static_cast<double>(end - start) * 1000.0;

        g_frequency = static_cast<std::uint32_t>(frequency / 1000000.0);
#elif defined(STDROMANO_UNIX)
        /* cntvct_el0 does not tick at the cpu frequency, ask the kernel instead */
        std::FILE* f = std::fopen("/sys/devices/system/cpu/cpu0/cpufreq/scaling_cur_freq", "r");

        if(f == nullptr)
        {
            f = std::fopen("/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_max_freq", "r");
        }

        if(f != nullptr)
        {
            unsigned long khz = 0;

            if(std::fscanf(f, "%lu", &khz) == 1)
            {
                g_frequency = static_cast<std::uint32_t>(khz / 1000);
            }

            std::fclose(f);
        }
#endif /* defined(STDROMANO_WIN) */
    }

    g_get_frequency_counter++;

    return g_frequency;
}

/********************************/
/* Caches */
/********************************/

#if defined(STDROMANO_APPLE)

static std::size_t cpu_sysctl_size(const char* name) noexcept
{
    std::uint64_t value = 0;
    std::size_t size = sizeof(value);

    if(sysctlbyname(name, &value, &size, nullptr, 0) != 0)
        return 0;

    return static_cast<std::size_t>(value);
}

#endif /* defined(STDROMANO_APPLE) */

static void cpu_check_caches() noexcept
{
    std::memset(g_cpu_caches_sizes, 0, sizeof(g_cpu_caches_sizes));

#if defined(STDROMANO_INTEL)
    std::int32_t regs[4];
    std::memset(regs, 0, sizeof(regs));

    cpuid(regs, 0);

    char cpu_vendor[13];
    std::memset(cpu_vendor, 0, sizeof(cpu_vendor));

    std::memcpy(cpu_vendor + 0, &regs[1], 4);
    std::memcpy(cpu_vendor + 4, &regs[3], 4);
    std::memcpy(cpu_vendor + 8, &regs[2], 4);

    if(std::strncmp(cpu_vendor, "GenuineIntel", 12) == 0)
    {
        for(std::int32_t cache_index = 0; cache_index < 10; ++cache_index)
        {
            cpuidex(regs, 4, cache_index);

            const std::int32_t cache_type = regs[0] & 0x1F;

            if(cache_type == 0)
                break;

            const std::int32_t level = (regs[0] >> 5) & 0x7;
            const std::int32_t is_data_or_unified = (cache_type == 1 || cache_type == 3);

            if(!is_data_or_unified)
                continue;

            const std::int32_t ways = ((regs[1] >> 22) & 0x3FF) + 1;
            const std::int32_t partitions = ((regs[1] >> 12) & 0x3FF) + 1;
            const std::int32_t line_size = (regs[1] & 0xFFF) + 1;
            const std::int32_t sets = regs[2] + 1;

            const std::size_t cache_size = static_cast<std::size_t>(ways) * partitions * line_size * sets;

            if(level == 1)
                g_cpu_caches_sizes[0] = cache_size;
            else if(level == 2)
                g_cpu_caches_sizes[1] = cache_size;
            else if(level == 3)
                g_cpu_caches_sizes[2] = cache_size;
        }
    }
    else if(std::strncmp(cpu_vendor, "AuthenticAMD", 12) == 0)
    {
        cpuid(regs, 0x80000000);

        const std::uint32_t max_ext_leaf = static_cast<std::uint32_t>(regs[0]);

        if(max_ext_leaf >= 0x80000005)
        {
            cpuid(regs, 0x80000005);
            g_cpu_caches_sizes[0] = static_cast<std::size_t>(regs[2] >> 24) * 1024;
        }

        if(max_ext_leaf >= 0x80000006)
        {
            cpuid(regs, 0x80000006);
            g_cpu_caches_sizes[1] = static_cast<std::size_t>((regs[2] >> 16) & 0xFFFF) * 1024;
            g_cpu_caches_sizes[2] = static_cast<std::size_t>(regs[3] >> 18) * 512 * 1024;
        }
    }
#elif defined(STDROMANO_APPLE)
    g_cpu_caches_sizes[0] = cpu_sysctl_size("hw.l1dcachesize");
    g_cpu_caches_sizes[1] = cpu_sysctl_size("hw.l2cachesize");
    g_cpu_caches_sizes[2] = cpu_sysctl_size("hw.l3cachesize");

    if(g_cpu_caches_sizes[1] == 0)
    {
        /* Apple Silicon reports caches per performance level, level 0 being the p-cores */
        g_cpu_caches_sizes[0] = cpu_sysctl_size("hw.perflevel0.l1dcachesize");
        g_cpu_caches_sizes[1] = cpu_sysctl_size("hw.perflevel0.l2cachesize");
    }
#elif defined(STDROMANO_UNIX)
#if defined(_SC_LEVEL1_DCACHE_SIZE)
    const long l1 = sysconf(_SC_LEVEL1_DCACHE_SIZE);
    const long l2 = sysconf(_SC_LEVEL2_CACHE_SIZE);
    const long l3 = sysconf(_SC_LEVEL3_CACHE_SIZE);

    g_cpu_caches_sizes[0] = l1 > 0 ? static_cast<std::size_t>(l1) : 0;
    g_cpu_caches_sizes[1] = l2 > 0 ? static_cast<std::size_t>(l2) : 0;
    g_cpu_caches_sizes[2] = l3 > 0 ? static_cast<std::size_t>(l3) : 0;
#endif /* defined(_SC_LEVEL1_DCACHE_SIZE) */

    /* sysconf returns 0 for the cache sizes on most aarch64 kernels, fallback on sysfs */
    for(std::uint32_t i = 0; i < 4; ++i)
    {
        char path[256];
        std::snprintf(path, sizeof(path), "/sys/devices/system/cpu/cpu0/cache/index%u/level", i);

        std::FILE* f = std::fopen(path, "r");

        if(f == nullptr)
            break;

        unsigned int level = 0;
        const bool has_level = std::fscanf(f, "%u", &level) == 1;
        std::fclose(f);

        if(!has_level || level == 0 || level > 3)
            continue;

        if(g_cpu_caches_sizes[level - 1] != 0)
            continue;

        std::snprintf(path, sizeof(path), "/sys/devices/system/cpu/cpu0/cache/index%u/size", i);

        f = std::fopen(path, "r");

        if(f == nullptr)
            continue;

        unsigned long size = 0;
        char unit = 'K';

        if(std::fscanf(f, "%lu%c", &size, &unit) >= 1)
        {
            const std::size_t multiplier = unit == 'M' ? 1024 * 1024 : 1024;
            g_cpu_caches_sizes[level - 1] = static_cast<std::size_t>(size) * multiplier;
        }

        std::fclose(f);
    }
#endif /* defined(STDROMANO_INTEL) */
}

void cpu_check() noexcept
{
    cpu_check_features();
    cpu_check_name();
    cpu_check_caches();

#if defined(STDROMANO_INTEL)
    std::int32_t regs[4];
    std::memset(regs, 0, sizeof(regs));

    cpuid(regs, 0);

    if(regs[0] >= 0x16)
    {
        cpuid(regs, 0x16);
        g_cpu_freq_mhz = static_cast<std::uint32_t>(regs[0]);
    }

    if(g_cpu_freq_mhz == 0)
    {
        g_cpu_freq_mhz = _get_cpu_frequency();
    }
#else
    g_cpu_freq_mhz = _get_cpu_frequency();
#endif /* defined(STDROMANO_INTEL) */
}

std::uint32_t cpu_get_frequency() noexcept
{
    return g_cpu_freq_mhz;
}

std::uint32_t cpu_get_current_frequency() noexcept
{
    return _get_cpu_frequency();
}

void cpu_get_current_frequency_set_refresh_rate(std::uint32_t refresh_rate) noexcept
{
    g_refresh_rate = refresh_rate == 0 ? 1 : refresh_rate;
}

std::size_t cpu_get_cache_size(std::uint32_t cache) noexcept
{
    if(cache > 2)
        return 0;

    return g_cpu_caches_sizes[cache];
}

STDROMANO_NAMESPACE_END
