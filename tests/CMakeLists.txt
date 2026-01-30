# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2025 - Present Romain Augier
# All rights reserved.

include(target_options)

file(GLOB_RECURSE TEST_FILES *.cpp)

set(NEEDS_OPENCL_LINKING test_densematrix test_opencl)

foreach(test_file ${TEST_FILES})
    get_filename_component(TESTNAME ${test_file} NAME_WLE)
    message(STATUS "Adding stdromano test : ${TESTNAME}")

    add_executable(${TESTNAME} ${test_file})
    target_compile_definitions(${TESTNAME} PRIVATE -DTESTS_DATA_DIR="${TESTS_DATA_DIR}")
    set_target_options(${TESTNAME})
    set_target_properties(${TESTNAME} PROPERTIES CXX_STANDARD 17)
    target_link_libraries(${TESTNAME} ${PROJECT_NAME})

    # Special case for command line parser test
    if(${TESTNAME} STREQUAL "test_command_line_parser")
        set(TEST_PARAMETERS "--str_arg=arg string value" "--int_arg=573849" "--bool_arg:False" "--bool_store_true" "-another-int-arg=-47381" "--" "a_command_after_args")
        add_test(NAME ${TESTNAME} COMMAND ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${TESTNAME} ${TEST_PARAMETERS})
    else()
        add_test(NAME ${TESTNAME} COMMAND ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${TESTNAME})
    endif()

    set_property(TEST ${TESTNAME} PROPERTY ENVIRONMENT "STDROMANO_LOG_LEVEL=debug")
endforeach()

# Copy clang asan dll to the tests directory when building in debug mode
# along pdb files

if(CMAKE_C_COMPILER_ID STREQUAL "MSVC")
    get_filename_component(CL_DIR ${CMAKE_C_COMPILER} DIRECTORY)

    set(ASAN_COPY_COMMAND
        ${CMAKE_COMMAND} -E copy_if_different ${CL_DIR}/clang_rt.asan_dynamic-x86_64.dll $<TARGET_FILE_DIR:${TESTNAME}>
    )

    add_custom_command(
        TARGET ${TESTNAME} POST_BUILD
        COMMAND "$<$<CONFIG:Debug,RelWithDebInfo>:${ASAN_COPY_COMMAND}>"
        COMMAND_EXPAND_LISTS
    )

    set(PDB_COPY_COMMAND
        ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE_DIR:${PROJECT_NAME}>/$<TARGET_FILE_BASE_NAME:${PROJECT_NAME}>.pdb $<TARGET_FILE_DIR:${TESTNAME}>)

    add_custom_command(
        TARGET ${TESTNAME} POST_BUILD
        COMMAND "$<$<CONFIG:Debug,RelWithDebInfo>:${PDB_COPY_COMMAND}>"
        COMMAND_EXPAND_LISTS
    )
endif()

if(WIN32)
    add_custom_command(
        TARGET ${TESTNAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
            $<TARGET_RUNTIME_DLLS:${TESTNAME}>
            $<TARGET_FILE_DIR:${TESTNAME}>
        COMMAND_EXPAND_LISTS
    )
endif()

add_custom_command(
    TARGET ${TESTNAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/src/cl
        $<TARGET_FILE_DIR:${TESTNAME}>/cl
)
