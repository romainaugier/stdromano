// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - Present Romain Augier
// All rights reserved.

#include "stdromano/cpu.h"

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

static uint64_t _get_frequency_counter = 0;
static uint32_t _frequency = 0;
static uint32_t _refresh = 10000;

uint32_t _get_cpu_frequency() noexcept
{
    if(_get_frequency_counter % _refresh == 0)
    {
#if defined(STDROMANO_WIN)
        PROCESSOR_POWER_INFORMATION* ppi;
        LONG ret;
        size_t size;
        LPBYTE p_buffer = NULL;
        ULONG current;
        ULONG max;
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

        _frequency = (uint32_t)current;
#elif defined(STDROMANO_LINUX)
        const uint64_t start = cpu_rdtsc();

        struct timespec wait_duration;
        wait_duration.tv_sec = 0;
        wait_duration.tv_nsec = 1000000;

        nanosleep(&wait_duration, NULL);

        const uint64_t end = cpu_rdtsc();

        const double frequency = (double)(end - start) * 1000;

        _frequency = (uint32_t)(frequency / 1000000);
#endif /* defined(STDROMANO_WIN) */
    }

    _get_frequency_counter++;

    return _frequency;
}

static uint32_t _cpu_freq_mhz = 0;

void cpu_check() noexcept
{
    /* CPU Frequency */
    int regs[4];
    std::memset(regs, 0, 4 * sizeof(int));

    cpuid(regs, 0);

    if(regs[0] >= 0x16)
    {
        cpuid(regs, 0x16);
        _cpu_freq_mhz = regs[0];
    }
    else
    {
        _cpu_freq_mhz = _get_cpu_frequency();
    }
}

void cpu_get_name(char* name) noexcept
{
    int32_t regs[12];

    cpuid(&regs[0], 0x80000000);

    if(regs[0] < 0x80000004)
    {
        name[0] = '\0';
        return;
    }

    cpuid(&regs[0], 0x80000002);
    cpuid(&regs[4], 0x80000003);
    cpuid(&regs[8], 0x80000004);

    std::memcpy(name, regs, 12 * sizeof(uint32_t));

    name[12 * sizeof(uint32_t)] = '\0';
}

uint32_t cpu_get_frequency() noexcept { return _cpu_freq_mhz; }

uint32_t cpu_get_current_frequency() noexcept { return _get_cpu_frequency(); }

void cpu_get_current_frequency_set_refresh_frequency(const uint32_t refresh_frequency) noexcept
{
    _refresh = refresh_frequency;
}

STDROMANO_NAMESPACE_END