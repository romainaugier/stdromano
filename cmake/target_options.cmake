# SPDX-License-Identifier: BSD-3-Clause 
# Copyright (c) 2025 - Present Romain Augier 
# All rights reserved. 

function(set_target_options target_name)
    if(CMAKE_C_COMPILER_ID STREQUAL "Clang")
        set(ROMANO_CLANG 1)
        set(CMAKE_C_FLAGS "-Wall -pedantic-errors")
        set(CMAKE_CXX_FLAGS "-Wall -pedantic-errors")

        target_compile_options(${target_name} PRIVATE $<$<CONFIG:Debug,RelWithDebInfo>:-fsanitize=leak -fsanitize=address>)
        target_compile_options(${target_name} PRIVATE $<$<CONFIG:Release,RelWithDebInfo>:-O3 -mavx2 -mfma)

        target_link_options(${target_name} PRIVATE $<$<CONFIG:Debug,RelWithDebInfo>:-fsanitize=address>)
    elseif (CMAKE_C_COMPILER_ID STREQUAL "GNU")
        set(ROMANO_GCC 1)
        set(CMAKE_C_FLAGS "-D_FORTIFY_SOURCES=2 -pipe -Wall -pedantic-errors")
        set(CMAKE_CXX_FLAGS "-D_FORTIFY_SOURCES=2 -pipe -Wall -pedantic-errors")

        target_compile_options(${target_name} PRIVATE $<$<CONFIG:Debug,RelWithDebInfo>:-fsanitize=leak -fsanitize=address>)
        target_compile_options(${target_name} PRIVATE $<$<CONFIG:Release,RelWithDebInfo>:-O3 -ftree-vectorizer-verbose=2 -mveclibabi=svml -mavx2 -mfma>)

        target_link_options(${target_name} PRIVATE $<$<CONFIG:Debug,RelWithDebInfo>:-fsanitize=address>)
    elseif (CMAKE_C_COMPILER_ID STREQUAL "Intel")
        set(ROMANO_INTEL 1)
    elseif (CMAKE_C_COMPILER_ID STREQUAL "MSVC")
        set(ROMANO_MSVC 1)
        include(find_avx)

        # 4710 is "Function not inlined", we don't care it pollutes more than tells useful information about the code
        # 5045 is "Compiler will insert Spectre mitigation for memory load if /Qspectre switch specified", again we don't care
        set(CMAKE_C_FLAGS "/Wall /wd4710 /wd5045") 
        set(CMAKE_CXX_FLAGS "/Wall /wd4710 /wd5045") 

        target_compile_options(${target_name} PRIVATE $<$<CONFIG:Debug,RelWithDebInfo>:/fsanitize=address /utf-8>)
        target_compile_options(${target_name} PRIVATE $<$<CONFIG:Release,RelWithDebInfo>:/O2 /GF /Ot /Oy /GT /GL /Oi ${AVX_FLAGS} /Zi /Gm- /Zc:inline /Qpar /utf-8>)

        # 4300 is "ignoring '/INCREMENTAL' because input module contains ASAN metadata", and we do not care
        set_target_properties(${target_name} PROPERTIES LINK_FLAGS "/ignore:4300")
    endif()

    # Provides the macro definition DEBUG_BUILD
    target_compile_definitions(${target_name} PRIVATE $<$<CONFIG:Debug>:DEBUG_BUILD>)
endfunction()