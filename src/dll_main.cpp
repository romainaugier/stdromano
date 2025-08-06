// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - Present Romain Augier
// All rights reserved.

#include "stdromano/cpu.hpp"
#include "stdromano/logger.hpp"
#include "stdromano/simd.hpp"

#include <cstdio>

#if defined(STDROMANO_WIN)
#include <Windows.h>
#endif // defined(STDROMANO_WIN)

/*
   In this source file we execute all functions that need to be executed at runtime to check and
   set some global variables (for simd vectorization, cpu frequency for profiling...)

   lib_entry is executed on dlopen / LoadLibrary
   lib_exit is executed on dlclose / CloseLibrary
*/

void STDROMANO_LIB_ENTRY lib_entry(void)
{
#if STDROMANO_DEBUG
    std::printf("stdromano entry\n");
#endif // STDROMANO_DEBUG
    stdromano::cpu_check();
    stdromano::simd_check_vectorization();

    stdromano::log_debug("stdromano vectorization mode: {} (fma: {}, f16c: {})\n",
                         stdromano::simd_get_vectorization_mode_as_string(),
                         stdromano::simd_has_fma(),
                         stdromano::simd_has_f16c());

    stdromano::log_debug("stdromano detected caches size: L1: {}, L2: {}, L3: {}",
                         stdromano::cpu_get_cache_size(stdromano::CPUCache_L1),
                         stdromano::cpu_get_cache_size(stdromano::CPUCache_L2),
                         stdromano::cpu_get_cache_size(stdromano::CPUCache_L2));
}

void STDROMANO_LIB_EXIT lib_exit(void)
{
#if STDROMANO_DEBUG
    std::printf("stdromano exit\n");
#endif // STDROMANO_DEBUG
}

#if defined(STDROMANO_WIN)
BOOL APIENTRY DllMain(STDROMANO_MAYBE_UNUSED HMODULE hModule, 
                      DWORD ul_reason_for_call,
                      STDROMANO_MAYBE_UNUSED LPVOID lpReserved)
{
    switch(ul_reason_for_call)
    {
        case DLL_PROCESS_ATTACH:
            /* Code to run when the DLL is loaded */
            lib_entry();
            break;
        case DLL_THREAD_ATTACH:
            break;
        case DLL_THREAD_DETACH:
            break;
        case DLL_PROCESS_DETACH:
            /* Code to run when the DLL is unloaded */
            lib_exit();
            break;
    }

    return TRUE;
}
#endif // defined(STDROMANO_WIN)