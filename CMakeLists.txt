# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2025 - Present Romain Augier
# All rights reserved.

include(CMakePrintHelpers)

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

add_subdirectory(src)

if(RUN_TESTS EQUAL 1)
    add_subdirectory(tests)
endif()