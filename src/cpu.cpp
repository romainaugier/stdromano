// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - Present Romain Augier
// All rights reserved.

#include "stdromano/cpu.hpp"

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

#elif defined(STDROMANO_LINUX)
#include <x86intrin.h>

#if !defined(__USE_POSIX199309)
#define __USE_POSIX199309
#endif /* !defined(__USE_POSIX199309) */
#include <time.h>
#endif /* defined(STDROMANO_WIN) */

STDROMANO_NAMESPACE_BEGIN

static uint64_t g_get_frequency_counter = 0;
static uint32_t g_frequency = 0;
static uint32_t g_refresh_rate = 10000;

uint32_t _get_cpu_frequency() noexcept
{
    if(g_get_frequency_counter % g_refresh_rate == 0)
    {
#if defined(STDROMANO_WIN)
        PROCESSOR_POWER_INFORMATION* ppi;
        LONG ret;
        ULONG size;
        LPBYTE p_buffer = NULL;
        ULONG current;
        UINT num_cpus;
        SYSTEM_INFO system_info;
        system_info.dwNumberOfProcessors = 0;

        GetSystemInfo(&system_info);

        num_cpus = system_info.dwNumberOfProcessors == 0 ? 1 : system_info.dwNumberOfProcessors;

        size = num_cpus * sizeof(PROCESSOR_POWER_INFORMATION);
        p_buffer = (BYTE*)LocalAlloc(LPTR, size);

        if(!p_buffer)
        {
            return 0;
        }

        ret = CallNtPowerInformation(ProcessorInformation, NULL, 0, p_buffer, size);

        if(ret != 0)
        {
            return 0;
        }

        ppi = (PROCESSOR_POWER_INFORMATION*)p_buffer;
        current = ppi->CurrentMhz;

        LocalFree(p_buffer);

        g_frequency = (uint32_t)current;
#elif defined(STDROMANO_LINUX)
        const uint64_t start = cpu_rdtsc();

        struct timespec wait_duration;
        wait_duration.tv_sec = 0;
        wait_duration.tv_nsec = 1000000;

        nanosleep(&wait_duration, NULL);

        const uint64_t end = cpu_rdtsc();

        const double frequency = (double)(end - start) * 1000;

        g_frequency = (uint32_t)(frequency / 1000000);
#endif /* defined(STDROMANO_WIN) */
    }

    g_get_frequency_counter++;

    return g_frequency;
}

static uint32_t g_cpu_freq_mhz = 0;
static std::size_t g_cpu_caches_sizes[3];

void cpu_check() noexcept
{
    /* CPU Frequency */
    int regs[4];
    std::memset(regs, 0, 4 * sizeof(int));

    cpuid(regs, 0);

    if(regs[0] >= 0x16)
    {
        cpuid(regs, 0x16);
        g_cpu_freq_mhz = regs[0];
    }
    else
    {
        g_cpu_freq_mhz = _get_cpu_frequency();
    }

    /* Caches size */

    cpuid(regs, 0);

    char cpu_vendor[13];

    std::memcpy(cpu_vendor + 0, &regs[1], 4);
    std::memcpy(cpu_vendor + 4, &regs[3], 4);
    std::memcpy(cpu_vendor + 8, &regs[2], 4);

    std::memset(regs, 0, 4 * sizeof(int));
    std::memset(g_cpu_caches_sizes, 0, 3 * sizeof(std::size_t));

    if(std::strncmp(cpu_vendor, "GenuineIntel", 12) == 0)
    {
        for (int cache_index = 0; cache_index < 10; ++cache_index)
        {
            cpuidex(regs, 4, cache_index);

            int cache_type = regs[0] & 0x1F;
            if (cache_type == 0) break;

            int level = (regs[0] >> 5) & 0x7;
            int is_data_or_unified = (cache_type == 1 || cache_type == 3);
            if (!is_data_or_unified) continue;

            int ways = ((regs[1] >> 22) & 0x3FF) + 1;
            int partitions = ((regs[1] >> 12) & 0x3FF) + 1;
            int line_size = (regs[1] & 0xFFF) + 1;
            int sets = regs[2] + 1;

            std::size_t cache_size = static_cast<std::size_t>(ways) * partitions * line_size * sets;

            if (level == 1) g_cpu_caches_sizes[0] = cache_size;
            else if (level == 2) g_cpu_caches_sizes[1] = cache_size;
            else if (level == 3) g_cpu_caches_sizes[2] = cache_size;
        }
    }
    else if(std::strncmp(cpu_vendor, "AuthenticAMD", 12) == 0)
    {
        int max_ext_leaf;
        cpuid(regs, 0x80000000);
        max_ext_leaf = regs[0];

        if (max_ext_leaf >= 0x80000005)
        {
            cpuid(regs, 0x80000005);
            g_cpu_caches_sizes[0] = (regs[2] >> 24) * 1024;
        }

        if (max_ext_leaf >= 0x80000006)
        {
            cpuid(regs, 0x80000006);
            g_cpu_caches_sizes[1] = ((regs[2] >> 16) & 0xFFFF) * 1024;
            g_cpu_caches_sizes[2] = (regs[3] >> 18) * 512 * 1024;
        }
    }
}

bool cpu_get_name(char* name) noexcept
{
    int32_t regs[12];

    cpuid(&regs[0], 0x80000000);

    if(regs[0] < static_cast<int32_t>(0x80000004))
    {
        name[0] = '\0';
        return false;
    }

    cpuid(&regs[0], 0x80000002);
    cpuid(&regs[4], 0x80000003);
    cpuid(&regs[8], 0x80000004);

    std::memcpy(name, regs, 12 * sizeof(uint32_t));

    name[12 * sizeof(uint32_t)] = '\0';

    return true;
}

uint32_t cpu_get_frequency() noexcept
{
    return g_cpu_freq_mhz;
}

uint32_t cpu_get_current_frequency() noexcept
{
    return _get_cpu_frequency();
}

void cpu_get_current_frequency_set_refresh_rate(uint32_t refresh_rate) noexcept
{
    g_refresh_rate = refresh_rate;
}

std::size_t cpu_get_cache_size(std::uint32_t cache) noexcept
{
    if(cache > 2)
    {
        return 0;
    }

    return g_cpu_caches_sizes[cache];
}

STDROMANO_NAMESPACE_END