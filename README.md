# STDRomano

C++ 17 data structures and utilities.

## Acknowledgment

This library is based on the great work of some existing libraries:
 - [FMT](https://github.com/fmtlib/fmt)
 - [spdlog](https://github.com/gabime/spdlog)
 - [MoodyCamel Concurrent Queue](https://github.com/cameron314/concurrentqueue)
 - [jemalloc](https://github.com/jemalloc/jemalloc)

## CMake

To use as a CMake package, you can use the following lines in your CMake configuration:
```cmake
# Find package
set(stdromano_DIR "${CMAKE_SOURCE_DIR}/stdromano/install/cmake")
find_package(stdromano CONFIG REQUIRED)

# Include directories
include_directories(${stdromano_INCLUDE_DIR})

# Library linking
target_link_libraries(${EXEC} PUBLIC stdromano::stdromano)

# Runtime dependencies (for Windows)
add_custom_command(
    TARGET ${EXEC} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
        ${stdromano_RUNTIME_DEPENDENCIES}
        $<TARGET_FILE_DIR:${EXEC}>
    COMMAND_EXPAND_LISTS
)

install(
    FILES ${stdromano_RUNTIME_DEPENDENCIES}
    DESTINATION ${CMAKE_INSTALL_BINDIR}
)
```