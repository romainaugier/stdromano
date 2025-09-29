# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2025 - Present Romain Augier
# All rights reserved.

include(CMakePrintHelpers)
include(FetchContent)
include(ExternalProject)

cmake_minimum_required(VERSION 3.15)

project(stdromano VERSION ${VERSION} LANGUAGES CXX ASM)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/finders")

set(CMAKE_VERBOSE_MAKEFILE ON)

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

# gtk
if(UNIX)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(GTK REQUIRED IMPORTED_TARGET "gtk+-2.0")

    include_directories(${GTK_INCLUDE_DIRS})
endif()

# jemalloc
if(UNIX)
    find_package(PkgConfig)

    pkg_search_module(JEMALLOC REQUIRED jemalloc)
else()
    set(JEMALLOC_FOUND 0)
endif()

if(JEMALLOC_FOUND)
    find_library(JEMALLOC_LIB jemalloc
        HINTS ${JEMALLOC_LIBRARY_DIRS}
        PATH_SUFFIXES lib
    )

    message(STATUS "Found jemalloc via pkg-config")
    message(STATUS "jemalloc include dirs: ${JEMALLOC_INCLUDE_DIRS}")
    message(STATUS "jemalloc library dirs: ${JEMALLOC_LIBRARY_DIRS}")
    message(STATUS "jemalloc libraries: ${JEMALLOC_LIB}")

    add_library(jemalloc SHARED IMPORTED GLOBAL)

    set_target_properties(jemalloc PROPERTIES
        IMPORTED_LOCATION ${JEMALLOC_LIB}
        INTERFACE_INCLUDE_DIRECTORIES ${JEMALLOC_INCLUDE_DIRS}
    )
else()
    message(STATUS "Looking for jemalloc in vcpkg directories")

    find_path(JEMALLOC_INCLUDE_DIR jemalloc/jemalloc.h
        PATHS
        ${CMAKE_PREFIX_PATH}/include
        ${CMAKE_INSTALL_PREFIX}/include
        $ENV{PROGRAMFILES}/jemalloc/include
        $ENV{VCPKG_ROOT}/installed/x64-windows/include
        $ENV{VCPKG_ROOT}/installed/x64-windows-static/include
    )
    
    find_library(JEMALLOC_LIBRARY
        NAMES jemalloc jemalloc_s
        PATHS
        ${CMAKE_PREFIX_PATH}/lib
        ${CMAKE_INSTALL_PREFIX}/lib
        $ENV{PROGRAMFILES}/jemalloc/lib
        $ENV{VCPKG_ROOT}/installed/x64-windows/lib
        $ENV{VCPKG_ROOT}/installed/x64-windows-static/lib
    )

    find_file(JEMALLOC_DLL jemalloc.dll
        PATHS
        ${CMAKE_PREFIX_PATH}/bin
        ${CMAKE_INSTALL_PREFIX}/bin
        $ENV{PROGRAMFILES}/jemalloc/bin
        $ENV{VCPKG_ROOT}/installed/x64-windows/bin
        $ENV{VCPKG_ROOT}/installed/x64-windows/debug/bin
    )
  
    if(JEMALLOC_INCLUDE_DIR AND JEMALLOC_LIBRARY AND JEMALLOC_DLL)
        message(STATUS "Found jemalloc")
        message(STATUS "jemalloc include dir: ${JEMALLOC_INCLUDE_DIR}")
        message(STATUS "jemalloc library: ${JEMALLOC_LIBRARY}")
        message(STATUS "jemalloc dll: ${JEMALLOC_DLL}")

        add_library(jemalloc SHARED IMPORTED GLOBAL)

        set_target_properties(jemalloc PROPERTIES
            IMPORTED_IMPLIB ${JEMALLOC_LIBRARY}
            IMPORTED_LOCATION ${JEMALLOC_DLL}
            INTERFACE_INCLUDE_DIRECTORIES ${JEMALLOC_INCLUDE_DIR}
        )
    else()
        message(FATAL_ERROR "Cannot find jemalloc, exiting")
    endif()
endif()

# OpenCL
if(UNIX)
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
        set(OpenCL_ROOT "${CMAKE_SOURCE_DIR}/vcpkg/packages/opencl_x64-windows")
    else()
        set(OpenCL_ROOT "${CUDA_TOOLKIT_ROOT_DIR}")
    endif()

    find_package(OpenCL REQUIRED)
endif()

include_directories(${OpenCL_INCLUDE_DIR})

set(OpenCLHPP_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/ext/opencl_hpp/include")
include_directories(${OpenCLHPP_INCLUDE_DIR})

add_subdirectory(src)

if(RUN_TESTS EQUAL 1)
    set(TESTS_DATA_DIR "${CMAKE_SOURCE_DIR}/tests_data")

    add_subdirectory(tests)
endif()