# SPDX-License-Identifier: BSD-3-Clause 
# Copyright (c) 2025 - Present Romain Augier 
# All rights reserved. 

include(CheckCXXCompilerFlag)

# Architecture specific optimization flags, shared by gcc and clang
function(get_arch_compile_options out_var)
    set(arch_options)

    if(STDROMANO_ARCH_X86_64 OR STDROMANO_ARCH_X86)
        list(APPEND arch_options -mavx2 -mfma)
    elseif(STDROMANO_ARCH_AARCH64)
        if(APPLE)
            # -mcpu=apple-m1 is the lowest common denominator of the apple silicon cpus,
            # newer ones (apple-m3, apple-m4) need a recent clang
            check_cxx_compiler_flag("-mcpu=apple-m1" HAS_MCPU_APPLE_M1)

            if(HAS_MCPU_APPLE_M1)
                list(APPEND arch_options -mcpu=apple-m1)
            endif()
        else()
            # NEON is mandatory in armv8-a, nothing to enable, but let the compiler use
            # the extensions of the machine we are building on when it can
            check_cxx_compiler_flag("-mcpu=native" HAS_MCPU_NATIVE)

            if(HAS_MCPU_NATIVE AND NOT CMAKE_CROSSCOMPILING)
                list(APPEND arch_options -mcpu=native)
            endif()
        endif()
    endif()

    set(${out_var} ${arch_options} PARENT_SCOPE)
endfunction()

function(set_target_options target_name)
    get_arch_compile_options(ARCH_COMPILE_OPTIONS)

    # AppleClang is a distinct compiler id, MATCHES catches both
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        set(ROMANO_CLANG 1)

        # -fsanitize=leak is not implemented on darwin (asan has its own leak detector)
        if(APPLE)
            target_compile_options(${target_name} PRIVATE $<$<CONFIG:Debug,RelWithDebInfo>:-fsanitize=address>)
        else()
            target_compile_options(${target_name} PRIVATE $<$<CONFIG:Debug,RelWithDebInfo>:-fsanitize=leak -fsanitize=address>)
        endif()

        target_compile_options(${target_name} PRIVATE -Wall -pedantic-errors)
        target_compile_options(${target_name} PRIVATE ${ARCH_COMPILE_OPTIONS})
        target_compile_options(${target_name} PRIVATE $<$<CONFIG:Release,RelWithDebInfo>:-O3>)

        target_link_options(${target_name} PRIVATE $<$<CONFIG:Debug,RelWithDebInfo>:-fsanitize=address>)
    elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        set(ROMANO_GCC 1)
        set(CMAKE_C_FLAGS "-D_FORTIFY_SOURCES=2 -pipe -Wall -pedantic-errors")
        set(CMAKE_CXX_FLAGS "-D_FORTIFY_SOURCES=2 -pipe -Wall -pedantic-errors")

        if(${ADDRSAN})
            target_compile_options(${target_name} PRIVATE $<$<CONFIG:Debug,RelWithDebInfo>:-fsanitize=address>)
            target_link_options(${target_name} PRIVATE $<$<CONFIG:Debug,RelWithDebInfo>:-fsanitize=address>)
        endif()

        if(${LEAKSAN})
            target_compile_options(${target_name} PRIVATE $<$<CONFIG:Debug,RelWithDebInfo>:-fsanitize=leak>)
            target_link_options(${target_name} PRIVATE $<$<CONFIG:Debug,RelWithDebInfo>:-fsanitize=leak>)
        endif()

        if(${UBSAN})
            target_compile_options(${target_name} PRIVATE $<$<CONFIG:Debug,RelWithDebInfo>:-fsanitize=undefined -fno-sanitize-recover=undefined>)
            target_link_options(${target_name} PRIVATE $<$<CONFIG:Debug,RelWithDebInfo>:-fsanitize=undefined -fno-sanitize-recover=undefined>)
        endif()

        if(${THREADSAN})
            target_compile_options(${target_name} PRIVATE $<$<CONFIG:Debug,RelWithDebInfo>:-fsanitize=thread>)
            target_link_options(${target_name} PRIVATE $<$<CONFIG:Debug,RelWithDebInfo>:-fsanitize=thread>)
        endif()

        set(COMPILE_OPTIONS -D_FORTIFY_SOURCES=2 -pipe -Wall -pedantic-errors $<$<CONFIG:Release,RelWithDebInfo>:-O3 -ftree-vectorizer-verbose=2> ${ARCH_COMPILE_OPTIONS})

        # -mveclibabi=svml is x86 only
        if(STDROMANO_ARCH_X86_64 OR STDROMANO_ARCH_X86)
            list(APPEND COMPILE_OPTIONS -mveclibabi=svml)
        endif()

        target_compile_options(${target_name} PRIVATE ${COMPILE_OPTIONS})
    elseif (CMAKE_CXX_COMPILER_ID STREQUAL "Intel")
        set(ROMANO_INTEL 1)
    elseif (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        set(ROMANO_MSVC 1)

        if(STDROMANO_ARCH_X86_64 OR STDROMANO_ARCH_X86)
            include(find_avx)
        else()
            # msvc has no /arch: flag for arm64, neon is always available
            set(AVX_FLAGS)
        endif()

        if(${ADDRSAN})
            if(STDROMANO_ARCH_X86_64 OR STDROMANO_ARCH_X86)
                target_compile_options(${target_name} PRIVATE $<$<CONFIG:Debug,RelWithDebInfo>:/fsanitize=address>)
            else()
                message(WARNING "MSVC AddressSanitizer is only available on x86 and x64, ignoring ADDRSAN for ${target_name}")
            endif()
        endif()

        if(STDROMANO_ARCH_X86)
            set(FRAME_POINTER_OPTION /Oy)
        else()
            set(FRAME_POINTER_OPTION)
        endif()

        # 4710 is "Function not inlined", we don't care it pollutes more than tells useful information about the code
        # 5045 is "Compiler will insert Spectre mitigation for memory load if /Qspectre switch specified", again we don't care
        # 4324 is " structure was padded due to alignment specifier", again we don't care (it appears only in HashSet::Bucket for now)
        # 4146 is " unary minus operator applied to unsigned type", again we don't care (it appears only in lsb_u64)
        set(COMPILE_OPTIONS /W4 /wd4710 /wd5045 /wd4324 /wd4146 /utf-8 ${AVX_FLAGS} $<$<CONFIG:Release,RelWithDebInfo>:/O2 /GF /Ot ${FRAME_POINTER_OPTION} /GT /GL /Oi /Zi /Gm- /Zc:inline>)

        target_compile_options(${target_name} PRIVATE ${COMPILE_OPTIONS})

        # 4300 is "ignoring '/INCREMENTAL' because input module contains ASAN metadata", and we do not care
        target_link_options(${target_name} PRIVATE /ignore:4300 /NODEFAULTLIB:library)
    endif()

    # Code coverage, -O0 comes after the optimization flags above so it takes precedence
    if(COVERAGE AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(${target_name} PRIVATE --coverage -O0 -g)
        target_link_options(${target_name} PRIVATE --coverage)
    endif()

    # Provides the macro definition DEBUG_BUILD
    target_compile_definitions(${target_name} PRIVATE $<$<CONFIG:Debug>:DEBUG_BUILD>)
endfunction()