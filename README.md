# stdromano

C++ 17 data structures and utilities.

## Acknowledgment

This library is based on the great work of some existing libraries:
 - [FMT](https://github.com/fmtlib/fmt)
 - [spdlog](https://github.com/gabime/spdlog)
 - [MoodyCamel Concurrent Queue](https://github.com/cameron314/concurrentqueue)
 - [jemalloc](https://github.com/jemalloc/jemalloc)
 - [OpenCL](https://www.khronos.org/opencl/)

## Build and tests

Build scripts are used to build, test and install for different targets (they are basically wrappers around CMake command line). They also take care of cloning and bootstraping vcpkg the first time.
The following arguments are used:
 - `--debug`: builds in debug (default is release)
 - `--reldebug`: builds in reldebug (default is release)
 - `--tests`: builds and runs tests
 - `--clean`: clean the previous build/install
 - `--install`: creates an installation (default directory is $(pwd)/install)
 - `--addrsan`: builds using the address sanitizer
 - `--ubsan`: builds using the undefined behavior sanitizer (only available with gcc)
 - `--threadsan`: builds using the thread sanitizer (only available with gcc)
 - `--version:<x.x.x>`: specifies the build version (note that is should be used like: `--version:MAJOR.MINOR.FIX`)
 - `--installdir:<path>`: specifies where to install the library

Using the buildscript on Windows:
```bat
build ...
```

Using the buildscript on Linux:
```bash
./build.sh ...
```

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
