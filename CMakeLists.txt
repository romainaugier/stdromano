# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2025 - Present Romain Augier
# All rights reserved.

include(CMakePrintHelpers)
include(FetchContent)
include(ExternalProject)

cmake_minimum_required(VERSION 3.15)

# MSVC is not an assembler for language ASM
if(POLICY CMP0194)
    cmake_policy(SET CMP0194 NEW)
endif()

list(APPEND VCPKG_OVERLAY_TRIPLETS "${CMAKE_CURRENT_SOURCE_DIR}/cmake/triplets")

project(stdromano VERSION ${VERSION} LANGUAGES CXX)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

# Target architecture detection (CMAKE_OSX_ARCHITECTURES wins when cross building on apple)
if(CMAKE_OSX_ARCHITECTURES)
    list(GET CMAKE_OSX_ARCHITECTURES 0 STDROMANO_TARGET_ARCH)
else()
    set(STDROMANO_TARGET_ARCH "${CMAKE_SYSTEM_PROCESSOR}")
endif()

string(TOLOWER "${STDROMANO_TARGET_ARCH}" STDROMANO_TARGET_ARCH)

if(STDROMANO_TARGET_ARCH MATCHES "^(x86_64|amd64|x64)$")
    set(STDROMANO_ARCH_X86_64 ON)
elseif(STDROMANO_TARGET_ARCH MATCHES "^(aarch64|arm64|armv8.*)$")
    set(STDROMANO_ARCH_AARCH64 ON)
elseif(STDROMANO_TARGET_ARCH MATCHES "^(i.86|x86)$")
    set(STDROMANO_ARCH_X86 ON)
elseif(STDROMANO_TARGET_ARCH MATCHES "^(arm.*)$")
    set(STDROMANO_ARCH_AARCH32 ON)
else()
    message(WARNING "Unknown target architecture \"${STDROMANO_TARGET_ARCH}\", building without any architecture specific code")
endif()

message(STATUS "stdromano target architecture: ${STDROMANO_TARGET_ARCH}")

# The hand written strcmp kernels are x86-64 nasm (elf64) and masm, they are only assembled
# on windows and linux, src/CMakeLists.txt defines STDROMANO_ASM_STRCMP when they are
if(STDROMANO_ARCH_X86_64 AND (WIN32 OR (UNIX AND NOT APPLE AND NOT CMAKE_SYSTEM_NAME MATCHES "BSD")))
    set(STDROMANO_BUILD_ASM ON)
endif()

if(STDROMANO_BUILD_ASM)
    if(WIN32)
        enable_language(ASM_MASM)
    else()
        find_program(NASM_EXEC "nasm")

        if(NASM_EXEC)
            enable_language(ASM_NASM)
            set(CMAKE_ASM_NASM_OBJECT_FORMAT "elf64")
        else()
            message(STATUS "nasm not found, falling back on the C++ strcmp kernels")
            set(STDROMANO_BUILD_ASM OFF)
        endif()
    endif()
else()
    message(STATUS "No assembly kernels on this platform, using the C++ ones")
endif()

set(CMAKE_VERBOSE_MAKEFILE ON)

# Enable OpenCL utilities and accelerators for linear algebra operations
option(ENABLE_OPENCL
       "Builds the OpenCL utilities, and accelerators for linear algebra"
       OFF)

option(BUILD_BENCHMARKS "Builds the benchmarks in benchmarks/" OFF)

# Enable testing
if(RUN_TESTS EQUAL 1)
    message(STATUS "RUN_TESTS enabled, building and running tests")
    enable_testing()
endif()

# concurrentqueue
set(concurrentqueue_INCLUDE_DIRS "${CMAKE_SOURCE_DIR}/ext/concurrentqueue")
include_directories(${concurrentqueue_INCLUDE_DIRS})

# spdlog
set(spdlog_INCLUDE_DIRS "${CMAKE_SOURCE_DIR}/ext/spdlog/include")
include_directories(${spdlog_INCLUDE_DIRS})

# gtk (file dialogs on linux and the bsds, macOS needs its own NSOpenPanel implementation)
if(UNIX AND NOT APPLE)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(GTK QUIET IMPORTED_TARGET "gtk+-2.0")

    if(GTK_FOUND)
        set(STDROMANO_ENABLE_GTK ON)
        include_directories(${GTK_INCLUDE_DIRS})
        add_definitions(-DSTDROMANO_ENABLE_GTK)
    else()
        message(WARNING "gtk+-2.0 not found, the file dialogs will not be available")
    endif()
endif()

# mimalloc
if(VCPKG_TARGET_TRIPLET)
    set(STDROMANO_VCPKG_TRIPLET ${VCPKG_TARGET_TRIPLET})
else()
    if(STDROMANO_ARCH_AARCH64)
        set(STDROMANO_VCPKG_ARCH arm64)
    elseif(STDROMANO_ARCH_X86)
        set(STDROMANO_VCPKG_ARCH x86)
    else()
        set(STDROMANO_VCPKG_ARCH x64)
    endif()

    if(WIN32)
        set(STDROMANO_VCPKG_TRIPLET ${STDROMANO_VCPKG_ARCH}-windows)
    elseif(APPLE)
        set(STDROMANO_VCPKG_TRIPLET ${STDROMANO_VCPKG_ARCH}-osx)
    else()
        set(STDROMANO_VCPKG_TRIPLET ${STDROMANO_VCPKG_ARCH}-linux)
    endif()
endif()

find_package(mimalloc CONFIG QUIET)

if(mimalloc_FOUND)
    if(TARGET mimalloc)
        set(MIMALLOC_TARGET mimalloc)
    elseif(TARGET mimalloc-static)
        set(MIMALLOC_TARGET mimalloc-static)
    endif()

    message(STATUS "Found mimalloc (target: ${MIMALLOC_TARGET})")
else()
    message(STATUS "mimalloc cmake config not found, looking for the headers and the library")

    find_path(MIMALLOC_INCLUDE_DIR mimalloc.h
        PATHS
        ${CMAKE_PREFIX_PATH}/include
        ${CMAKE_INSTALL_PREFIX}/include
        /opt/homebrew/include
        /usr/local/include
        $ENV{VCPKG_ROOT}/installed/${STDROMANO_VCPKG_TRIPLET}/include
        PATH_SUFFIXES mimalloc mimalloc-2.1
    )

    find_library(MIMALLOC_LIBRARY
        NAMES mimalloc mimalloc-static libmimalloc
        PATHS
        ${CMAKE_PREFIX_PATH}/lib
        ${CMAKE_INSTALL_PREFIX}/lib
        /opt/homebrew/lib
        /usr/local/lib
        $ENV{VCPKG_ROOT}/installed/${STDROMANO_VCPKG_TRIPLET}/lib
    )

    if(MIMALLOC_INCLUDE_DIR AND MIMALLOC_LIBRARY)
        message(STATUS "mimalloc include dir: ${MIMALLOC_INCLUDE_DIR}")
        message(STATUS "mimalloc library: ${MIMALLOC_LIBRARY}")

        add_library(mimalloc_imported UNKNOWN IMPORTED GLOBAL)

        set_target_properties(mimalloc_imported PROPERTIES
            IMPORTED_LOCATION ${MIMALLOC_LIBRARY}
            INTERFACE_INCLUDE_DIRECTORIES ${MIMALLOC_INCLUDE_DIR}
        )

        set(MIMALLOC_TARGET mimalloc_imported)
    else()
        message(FATAL_ERROR "Cannot find mimalloc (brew install mimalloc / vcpkg install mimalloc), exiting")
    endif()
endif()

# OpenCL
if(ENABLE_OPENCL)
    if(APPLE)
        # macOS ships OpenCL as a (deprecated) system framework, there is nothing to install
        find_library(OPENCL_FRAMEWORK OpenCL REQUIRED)

        message(STATUS "Found the OpenCL framework: ${OPENCL_FRAMEWORK}")

        if(NOT TARGET OpenCL::OpenCL)
            add_library(OpenCL::OpenCL INTERFACE IMPORTED GLOBAL)

            set_target_properties(OpenCL::OpenCL PROPERTIES
                INTERFACE_LINK_LIBRARIES "${OPENCL_FRAMEWORK}"
            )
        endif()

        set(OpenCL_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/ext/opencl_hpp/include")
    elseif(UNIX)
        find_package(PkgConfig QUIET)

        if(PkgConfig_FOUND)
            pkg_check_modules(OPENCL_PKG QUIET OpenCL)
        endif()

        find_library(SYSTEM_OPENCL_LIB
            NAMES OpenCL OpenCL.so OpenCL.so.1
            HINTS /usr/lib64 /lib64 /usr/lib /usr/local/lib
        )

        find_path(SYSTEM_OPENCL_INCLUDE_DIR
            NAMES CL/cl.h
            HINTS /usr/include /usr/local/include /usr/include/CL
        )

        if(SYSTEM_OPENCL_LIB AND SYSTEM_OPENCL_INCLUDE_DIR)
            message(STATUS "Found system OpenCL: ${SYSTEM_OPENCL_LIB}, includes: ${SYSTEM_OPENCL_INCLUDE_DIR}")

            if(TARGET OpenCL::OpenCL)
                message(STATUS "Overriding existing OpenCL::OpenCL imported target to system OpenCL")

                set_target_properties(OpenCL::OpenCL PROPERTIES
                    IMPORTED_LOCATION "${SYSTEM_OPENCL_LIB}"
                    INTERFACE_INCLUDE_DIRECTORIES "${SYSTEM_OPENCL_INCLUDE_DIR}"
                )
            else()
                add_library(OpenCL::OpenCL UNKNOWN IMPORTED GLOBAL)

                set_target_properties(OpenCL::OpenCL PROPERTIES
                    IMPORTED_LOCATION "${SYSTEM_OPENCL_LIB}"
                    INTERFACE_INCLUDE_DIRECTORIES "${SYSTEM_OPENCL_INCLUDE_DIR}"
                )
            endif()

            set(OpenCL_INCLUDE_DIR ${SYSTEM_OPENCL_INCLUDE_DIR})
        else()
            message(STATUS "System OpenCL not found via find_library/find_path; will try find_package or vcpkg fallback")

            find_package(OpenCL QUIET)

            if(NOT TARGET OpenCL::OpenCL AND OpenCL_LIBRARY)
                add_library(OpenCL::OpenCL UNKNOWN IMPORTED GLOBAL)
                set_target_properties(OpenCL::OpenCL PROPERTIES
                    IMPORTED_LOCATION "${OpenCL_LIBRARY}"
                    INTERFACE_INCLUDE_DIRECTORIES "${OpenCL_INCLUDE_DIR}"
                )
            endif()
        endif()

    elseif(WIN32)
        # Usually OpenCL comes with CUDA on windows (this is how I install it) so we ask for Cuda toolkit and
        # then use the OpenCL provided by Nvidia
        find_package(CUDAToolkit)

        if(NOT ${CUDAToolkit_FOUND})
            message(STATUS "Cannot find OpenCL via Cuda Toolkit, downloading it using vcpkg")

            file(READ ${CMAKE_SOURCE_DIR}/vcpkg.json JSON_CONTENT)

            string(REGEX REPLACE
                "(\"dependencies\"[ \t]*:[ \t]*\\[[^]]*)(\\])"
                "\\1, \"opencl\"\\2"
                JSON_CONTENT
                "${JSON_CONTENT}")

            file(WRITE ${CMAKE_SOURCE_DIR}/vcpkg.json "${JSON_CONTENT}")

            execute_process(
                COMMAND vcpkg install
                WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/vcpkg
            )
            set(OpenCL_ROOT "${CMAKE_SOURCE_DIR}/vcpkg/packages/opencl_${STDROMANO_VCPKG_TRIPLET}")
        else()
            set(OpenCL_ROOT "${CUDA_TOOLKIT_ROOT_DIR}")
        endif()

        find_package(OpenCL REQUIRED)
    endif()

    include_directories(${OpenCL_INCLUDE_DIR})

    set(OpenCLHPP_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/ext/opencl_hpp/include")
    include_directories(${OpenCLHPP_INCLUDE_DIR})

    add_definitions(-DSTDROMANO_ENABLE_OPENCL)
endif()

add_subdirectory(src)

set(TESTS_DATA_DIR "${CMAKE_SOURCE_DIR}/tests_data")

if(RUN_TESTS EQUAL 1)
    add_subdirectory(tests)
endif()

if(BUILD_BENCHMARKS)
    message(STATUS "BUILD_BENCHMARKS enabled, building benchmarks")
    add_subdirectory(benchmarks)
endif()
