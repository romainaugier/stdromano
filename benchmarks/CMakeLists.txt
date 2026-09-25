# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2025 - Present Romain Augier
# All rights reserved.

include(target_options)

file(GLOB BENCHMARK_FILES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/bench_*.cpp)

foreach(benchmark_file ${BENCHMARK_FILES})
    get_filename_component(BENCHMARK_NAME ${benchmark_file} NAME_WLE)
    message(STATUS "Adding stdromano benchmark : ${BENCHMARK_NAME}")

    add_executable(${BENCHMARK_NAME} ${benchmark_file})
    target_compile_definitions(${BENCHMARK_NAME} PRIVATE TESTS_DATA_DIR="${TESTS_DATA_DIR}")
    set_target_options(${BENCHMARK_NAME})
    set_target_properties(${BENCHMARK_NAME} PROPERTIES CXX_STANDARD 17)
    target_link_libraries(${BENCHMARK_NAME} ${PROJECT_NAME})

    # Windows has no rpath: put the library dll next to the executable
    if(WIN32)
        add_custom_command(
            TARGET ${BENCHMARK_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_RUNTIME_DLLS:${BENCHMARK_NAME}>
                $<TARGET_FILE_DIR:${BENCHMARK_NAME}>
            COMMAND_EXPAND_LISTS
        )
    endif()
endforeach()
