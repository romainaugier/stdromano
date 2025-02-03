#include "stdromano/cpu.h"

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
}

void STDROMANO_LIB_EXIT lib_exit(void)
{
#if STDROMANO_DEBUG
    std::printf("stdromano exit\n");
#endif // STDROMANO_DEBUG
}

#if defined(STDROMANO_WIN)
BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) 
{
    switch (ul_reason_for_call) {
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