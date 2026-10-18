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
 - `--cuda`: builds the CUDA tests (host/device parity of the maths, needs the CUDA toolkit)
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

## CUDA

The maths, linear algebra and hashing headers can be used from CUDA code, in host, device and
host/device functions, with nvcc or clang. Nothing needs to be built for that, the headers detect
CUDA by themselves:

- `STDROMANO_CUDA` is defined when compiling a `.cu` file (nvcc's `__CUDACC__` or clang's `__CUDA__`)
- `STDROMANO_CUDA_DEVICE` is defined in the device compilation pass only (`__CUDA_ARCH__`)
- `STDROMANO_HOST`, `STDROMANO_DEVICE` and `STDROMANO_HOST_DEVICE` expand to the CUDA execution space
  qualifiers, and to nothing outside of CUDA, so they can be used in shared code

```cpp
#include "stdromano/linalg/random.hpp"
#include "stdromano/linalg/transform.hpp"

using namespace stdromano;

STDROMANO_HOST_DEVICE Vec3F primary_ray_dir(const Transform44F& camera, const float fov, const Vec2F& ndc)
{
    const float scale = maths::tan(maths::deg2rad(fov * 0.5f));
    return normalize(camera.transform_dir(Vec3F(ndc * scale, -1.0f)));
}

__global__ void render(float4* pixels, const float* camera_transform, const float fov)
{
    const Transform44F camera = Transform44F::from_data(camera_transform);
    const Vec2F jitter = pcg2d_hash_float(Vec2U(blockIdx.x, threadIdx.x));
    // ...
}
```

Host/device headers: `maths.hpp`, `bits.hpp`, `random.hpp` (except the thread-safe `ts_*`
generators and `random_seed_*`), `linalg/vector.hpp`, `linalg/transform.hpp` (except
`Transform33::inverse`) and `linalg/random.hpp`. `STDROMANO_ASSERT` works in device code too
(device `printf` then `__trap`).

On the device, the functions mapping to libm (`sqrt`, `exp`, `sin`, `pow`...) call the CUDA math
library and follow the compilation flags, with `--use_fast_math` they become the approximate
intrinsics. The other ones use dedicated instructions whatever the flags:

| Function | Host | Device |
| --- | --- | --- |
| `rcp` | rcpss/frecpe + Newton-Raphson | `__fdividef` (MUFU.RCP) |
| `rcp_safe` | division | `__frcp_rn`, correctly rounded even with fast math |
| `rsqrt` | rsqrtss/frsqrte + Newton-Raphson | `rsqrtf` (MUFU.RSQ) |
| `fma`, `fms`, `nfma`, `nfms` | fma3, `std::fma` on arm | `__fmaf_rn`, a single rounding |
| `min`, `max` | compare and select | `fminf`/`fmaxf`, one instruction |
| `isnan`, `isinf`, `isfinite` | compiler builtins | bit tests, they survive finite-math flags |
| `popcount`, `clz`, `ctz`, 64 bits high product | compiler builtins | `__popc`, `__clzll`, `__ffsll`, `__umul64hi` |

The vector types have the layout of the CUDA ones: `Vec2F`/`Vec4F` are 8/16 bytes aligned like
`float2`/`float4` (so device loads and stores are vectorized), `Vec3F` is packed like `float3`, and
the same goes for the `int` (`Vec*I`), `unsigned int` (`Vec*U`) and `double2` (`Vec2D`) versions.
This is checked with static asserts when compiling with CUDA, so buffers can be reinterpreted
(`reinterpret_cast<const Vec3F*>(normals)`).

A CUDA vector type and the stdromano vector with the same component type and count convert
implicitly both ways (`float3` and `Vec3F`, `uint4` and `Vec4U`...), so a `Vec3F` can be passed to
OptiX functions taking a `float3`, and `pcg4d_hash(make_uint4(...))` works. Any other conversion
stays explicit (`Vec3F(f4)` drops `w`, `Vec3F(i3)` converts the components) or does not exist
(`Vec3D` to `float3`). `to_cuda()` makes a conversion visible where you want it to be.

Template functions do not deduce through conversions: `dot(f3, v)` or `f3 + v` do not compile, but
`v + f3` does. Convert once when reading from CUDA types (`const Vec3F n = normals[i];`) and stay in
stdromano types afterwards. Avoid mixing with headers defining operators on the CUDA types (like the
CUDA samples' `helper_math.h`), `v + f3` would become ambiguous.

Floating-point vector division by a scalar multiplies by the reciprocal, and `normalize` uses the
`rsqrt` approximation (`normalize_safe` is exact).

The parity test (`--cuda`) evaluates every function on the host and on the device and checks the
results match, bit for bit where they must (`rcp_safe`, `fma`, `sqrt`, `floor`, the hashes...) and
within the CUDA math library error bounds for the others. It needs the default (non fast math)
compilation flags. Without a CUDA device it only checks that the device code compiles.

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
if(stdromano_HAS_RUNTIME_DEPENDENCIES)
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
endif()
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
