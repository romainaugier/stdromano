# SPDX-License-Identifier: BSD-3-Clause 
# Copyright (c) 2025 - Present Romain Augier 
# All rights reserved. 

include(GNUInstallDirs)
include(target_options)

file(GLOB_RECURSE src_files *.cpp)

add_definitions(-DSTDROMANO_BUILD_SHARED)

add_library(${PROJECT_NAME} SHARED ${src_files})
set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "")
set_target_properties(${PROJECT_NAME} PROPERTIES CXX_STANDARD 17)
set_target_properties(${PROJECT_NAME} PROPERTIES DEBUG_POSTFIX "d")

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

install(
    FILES $<TARGET_RUNTIME_DLLS:${PROJECT_NAME}>
    DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(
    DIRECTORY "${PROJECT_SOURCE_DIR}/include/${PROJECT_NAME}"
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

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