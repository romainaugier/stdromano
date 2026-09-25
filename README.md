# stdromano

[![codecov](https://codecov.io/gh/romainaugier/stdromano/branch/main/graph/badge.svg)](https://codecov.io/gh/romainaugier/stdromano)

C++ 17 data structures and utilities.

## Acknowledgment

This library is based on the great work of some existing libraries:
 - [FMT](https://github.com/fmtlib/fmt)
 - [spdlog](https://github.com/gabime/spdlog)
 - [MoodyCamel Concurrent Queue](https://github.com/cameron314/concurrentqueue)
 - [mimalloc](https://github.com/microsoft/mimalloc)
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
 - `--vcpkgpath:<path>`: specifies an existing vcpkg repository
 - `--benchmarks`: build the benchmarks in the /benchmarks directory
 - `--opencl`: builds opencl support
 - `--coverage`: builds the coverage using gcov (only available on Linux)

Using the buildscript on Windows:
```bat
build ...
```

Using the buildscript on Linux:
```bash
./build.sh ...
```

nasm is required on Linux to assemble the code written in assembly (on Windows, it is automatically detected). Assembly code can be found in src/asm.

Sanitizers are mutually exclusive because they cannot all be used at the same time as they require different instrumentation of the code. --ubsan and --threadsan are only available with Clang and GCC.

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

## Tests

Tests cover every functions and module of stdromano. They are a great way to see how to use the functions/classes, you can treat them as examples, and live in /tests.

### Writing tests and fuzzers

The test harness and the fuzzer are part of the library (`stdromano/test.hpp`, `stdromano/fuzz.hpp`),
so any project linking stdromano can use them.

```cpp
#include "stdromano/test.hpp"

STDROMANO_TEST_CASE(addition)
{
    STDROMANO_CHECK_EQ(1 + 1, 2);      // non fatal: the case goes on
    STDROMANO_REQUIRE(ptr != nullptr); // fatal: ends the case, the others still run
}

STDROMANO_TEST_MAIN()
```

A test binary takes substrings to select cases, `--list` to list them and `-v` to log at debug
level. `STDROMANO_TEST_FILTER` selects cases when no argument can be passed (ctest).

The fuzzer draws values from a `Source` backed either by a seeded PRNG or by a byte buffer, so the
same property runs under the built-in runner, a single seed replay, or libFuzzer:

```cpp
const auto report = stdromano::fuzz::run_property(options, [](stdromano::fuzz::Source& source) {
    const int a = source.integer<int>();          // biased towards 0, +-1, limits, powers of two
    const double d = source.floating<double>();   // NaN, +-inf, -0, subnormals...
    STDROMANO_FUZZ_CHECK_EQ(my_round_trip(a, d), std::make_pair(a, d));
    return true;
});

STDROMANO_REQUIRE_MSG(report.passed(), report.describe());
```

`run_input` fuzzes raw bytes instead, from a corpus and a dictionary, and minimizes the input that
fails. Runs are reproducible: the seed is fixed unless overridden, and every failure (crashes
included) prints the seed that replays it.

Environment overrides:
`ROMANO_FUZZ_SEED`, `ROMANO_FUZZ_ITERATIONS`, `ROMANO_FUZZ_SCALE`, `ROMANO_FUZZ_SECONDS`,
`ROMANO_FUZZ_REPLAY`.
