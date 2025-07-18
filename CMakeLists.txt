# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2025 - Present Romain Augier
# All rights reserved.

include(CMakePrintHelpers)
include(FetchContent)
include(ExternalProject)

cmake_minimum_required(VERSION 3.15)

project(stdromano VERSION ${VERSION})

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/finders")

set(CMAKE_VERBOSE_MAKEFILE ON)

if(RUN_TESTS EQUAL 1)
    message(STATUS "RUN_TESTS enabled, building and running tests")
    enable_testing()
endif()

# concurrentqueue
set(concurrentqueue_INCLUDE_DIRS "${CMAKE_SOURCE_DIR}/concurrentqueue")
include_directories(${concurrentqueue_INCLUDE_DIRS})

# spdlog
set(spdlog_INCLUDE_DIRS "${CMAKE_SOURCE_DIR}/spdlog/include")
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

add_subdirectory(src)

if(RUN_TESTS EQUAL 1)
    add_subdirectory(tests)
endif()