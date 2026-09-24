# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2025 - Present Romain Augier
# All rights reserved.

include(target_options)

file(GLOB TEST_FILES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/test_*.cpp)

if(NOT ENABLE_OPENCL)
    list(FILTER TEST_FILES EXCLUDE REGEX "test_opencl\\.cpp$")
endif()

set(STDROMANO_TESTS_TIMEOUT 300 CACHE STRING "Timeout of a single stdromano test, in seconds")

if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    get_filename_component(CL_DIR ${CMAKE_CXX_COMPILER} DIRECTORY)

    if(STDROMANO_ARCH_X86_64)
        set(ASAN_RUNTIME_DLL ${CL_DIR}/clang_rt.asan_dynamic-x86_64.dll)
    elseif(STDROMANO_ARCH_AARCH64)
        set(ASAN_RUNTIME_DLL ${CL_DIR}/clang_rt.asan_dynamic-aarch64.dll)
    elseif(STDROMANO_ARCH_X86)
        set(ASAN_RUNTIME_DLL ${CL_DIR}/clang_rt.asan_dynamic-i386.dll)
    endif()
endif()

foreach(test_file ${TEST_FILES})
    get_filename_component(TESTNAME ${test_file} NAME_WLE)
    message(STATUS "Adding stdromano test : ${TESTNAME}")

    add_executable(${TESTNAME} ${test_file})
    target_compile_definitions(${TESTNAME} PRIVATE TESTS_DATA_DIR="${TESTS_DATA_DIR}")
    target_include_directories(${TESTNAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    set_target_options(${TESTNAME})
    set_target_properties(${TESTNAME} PROPERTIES CXX_STANDARD 17)
    target_link_libraries(${TESTNAME} ${PROJECT_NAME})

    add_test(NAME ${TESTNAME}
             COMMAND ${TESTNAME}
             WORKING_DIRECTORY $<TARGET_FILE_DIR:${TESTNAME}>)

    set_tests_properties(${TESTNAME} PROPERTIES TIMEOUT ${STDROMANO_TESTS_TIMEOUT})

    if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        if(ADDRSAN AND ASAN_RUNTIME_DLL)
            add_custom_command(
                TARGET ${TESTNAME} POST_BUILD
                COMMAND "$<$<CONFIG:Debug,RelWithDebInfo>:${CMAKE_COMMAND};-E;copy_if_different;${ASAN_RUNTIME_DLL};$<TARGET_FILE_DIR:${TESTNAME}>>"
                COMMAND_EXPAND_LISTS
            )
        endif()

        add_custom_command(
            TARGET ${TESTNAME} POST_BUILD
            COMMAND "$<$<CONFIG:Debug,RelWithDebInfo>:${CMAKE_COMMAND};-E;copy_if_different;$<TARGET_FILE_DIR:${PROJECT_NAME}>/$<TARGET_FILE_BASE_NAME:${PROJECT_NAME}>.pdb;$<TARGET_FILE_DIR:${TESTNAME}>>"
            COMMAND_EXPAND_LISTS
        )
    endif()

    if(WIN32)
        add_custom_command(
            TARGET ${TESTNAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_RUNTIME_DLLS:${TESTNAME}>
                $<TARGET_FILE_DIR:${TESTNAME}>
            COMMAND_EXPAND_LISTS
        )
    endif()

    if(ENABLE_OPENCL)
        add_custom_command(
            TARGET ${TESTNAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
                ${PROJECT_SOURCE_DIR}/src/cl
                $<TARGET_FILE_DIR:${TESTNAME}>/cl
        )
    endif()
endforeach()
