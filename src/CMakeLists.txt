# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2025 - Present Romain Augier
# All rights reserved.

include(GNUInstallDirs)
include(target_options)

file(GLOB_RECURSE src_files *.cpp)

if(WIN32)
    enable_language(ASM_MASM)
    file(GLOB_RECURSE asm_files asm/x64_windows/*.asm)
elseif(UNIX)
    enable_language(ASM_NASM)
    file(GLOB_RECURSE asm_files asm/x64_linux/*.asm)
    set(CMAKE_ASM_NASM_OBJECT_FORMAT "elf64")
else()
    message(FATAL_ERROR "Platform not supported")
endif()

add_library(asm_objects OBJECT ${asm_files})

add_definitions(-DSTDROMANO_BUILD_SHARED)

add_library(${PROJECT_NAME} SHARED ${src_files} $<TARGET_OBJECTS:asm_objects>)

set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "")
set_target_properties(${PROJECT_NAME} PROPERTIES CXX_STANDARD 17)
set_target_properties(${PROJECT_NAME} PROPERTIES DEBUG_POSTFIX "d")
set_target_properties(${PROJECT_NAME} PROPERTIES LINKER_LANGUAGE CXX)
set_target_properties(${PROJECT_NAME} PROPERTIES EXPORT_LINK_INTERFACE_LANGUAGES CXX)

set_target_options(${PROJECT_NAME})

if(WIN32)
    set_target_properties(${PROJECT_NAME} PROPERTIES CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
endif()

if(UNIX)
    target_compile_options(${PROJECT_NAME} PUBLIC "-pthread")
    target_link_libraries(${PROJECT_NAME} PUBLIC ${MATH_LIBRARY})
    target_link_libraries(${PROJECT_NAME} PUBLIC ${GTK_LIBRARIES})
elseif(WIN32)
    target_link_libraries(${PROJECT_NAME} PUBLIC wsock32 ws2_32 Shlwapi Pathcch PowrProf)
endif()

target_link_libraries(${PROJECT_NAME} PRIVATE jemalloc)

if(ENABLE_OPENCL)
    target_link_libraries(${PROJECT_NAME} PUBLIC OpenCL::OpenCL)
endif()

target_include_directories(${PROJECT_NAME}
                           PUBLIC
                           $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
                           $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

install(
    TARGETS ${PROJECT_NAME}
    EXPORT "${PROJECT_NAME}Targets"
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(CODE [[
    file(GET_RUNTIME_DEPENDENCIES
         EXECUTABLES $<TARGET_FILE:stdromano>
         RESOLVED_DEPENDENCIES_VAR RESOLVED_DEPS
         UNRESOLVED_DEPENDENCIES_VAR UNRESOLVED_DEPS
         PRE_EXCLUDE_REGEXES "api-ms-" "ext-ms-" "hvsifiletrust" "pdmutilities" "aclui")
        #  POST_EXCLUDE_REGEXES ".*[Ss]ystem32/.*\\.dll")

    set(FILTERED_DEPS "")
    foreach(FILE ${RESOLVED_DEPS})
        if(FILE MATCHES "[Ss]ystem32")
            if(FILE MATCHES "OpenCL\\.dll$")
                list(APPEND FILTERED_DEPS "${FILE}")
            endif()
        else()
            list(APPEND FILTERED_DEPS "${FILE}")
        endif()
    endforeach()
    set(RESOLVED_DEPS ${FILTERED_DEPS})

    foreach(FILE ${RESOLVED_DEPS})
        file(INSTALL
             DESTINATION "${CMAKE_INSTALL_PREFIX}/bin"
             TYPE SHARED_LIBRARY
             FOLLOW_SYMLINK_CHAIN
             FILES "${FILE}")

    endforeach()
]])

install(
    DIRECTORY "${PROJECT_SOURCE_DIR}/include/${PROJECT_NAME}"
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

if(ENABLE_OPENCL)
    install(
        DIRECTORY "${PROJECT_SOURCE_DIR}/src/cl"
        DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
endif()

install(
    DIRECTORY ${spdlog_INCLUDE_DIRS}/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING PATTERN "*.h"
)

install(
    FILES ${concurrentqueue_INCLUDE_DIRS}/concurrentqueue.h
          ${concurrentqueue_INCLUDE_DIRS}/blockingconcurrentqueue.h
          ${concurrentqueue_INCLUDE_DIRS}/lightweightsemaphore.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

if(ENABLE_OPENCL)
    install(
        DIRECTORY ${OpenCL_INCLUDE_DIR}/CL/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/CL
        FILES_MATCHING PATTERN "*.h"
    )

    install(
        DIRECTORY ${OpenCLHPP_INCLUDE_DIR}/CL/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/CL
        FILES_MATCHING PATTERN "*.hpp"
    )
endif()

install(
    EXPORT "${PROJECT_NAME}Targets"
    FILE "${PROJECT_NAME}Targets.cmake"
    NAMESPACE "${PROJECT_NAME}::"
    DESTINATION cmake
)

include(CMakePackageConfigHelpers)

configure_package_config_file(
    "${CMAKE_SOURCE_DIR}/${PROJECT_NAME}Deps.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Deps.cmake"
    PATH_VARS CMAKE_INSTALL_INCLUDEDIR CMAKE_INSTALL_BINDIR
    INSTALL_DESTINATION ${CMAKE_INSTALL_PREFIX}/cmake
)

configure_package_config_file(
    "${CMAKE_SOURCE_DIR}/${PROJECT_NAME}Config.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
    PATH_VARS CMAKE_INSTALL_INCLUDEDIR CMAKE_INSTALL_BINDIR
    INSTALL_DESTINATION ${CMAKE_INSTALL_PREFIX}/cmake
)

write_basic_package_version_file(
    "${PROJECT_NAME}ConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY AnyNewerVersion
)

install(
    FILES "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
          "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake"
          "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Deps.cmake"
    DESTINATION cmake
)
