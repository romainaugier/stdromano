# stdromano Roadmap

This document tracks the feature set, progress, and future plans for the `stdromano` C++17 library.

## Data Structures
- :white_check_mark: String
- :white_check_mark: Dynamic Vector
- :white_check_mark: Stack-based Dynamic Vector
- :white_check_mark: HashMap
- :white_check_mark: HashSet
- :white_check_mark: Optional

## Algorithms
- :white_check_mark: Random Number Generators
- :white_check_mark: C++23 like ranges (enumeration)
- :white_check_mark: Endian conversion
- :white_check_mark: Hash functions (fnv1a, int, murmur3...)

## Utilities
- :white_check_mark: Cross-platform bit manipulation
- :white_check_mark: Cross-platform filesystem interaction
- :white_check_mark: Cross-platform atomics
- :white_check_mark: Cross-platform CPU features detection
- :white_check_mark: Cross-platform environment variables manipulation
- :white_check_mark: Profiling
- :white_check_mark: Logger (based on spdlog)
- :white_check_mark: ThreadPool (lockfree, based on concurrentqueue), with a global threadpool provided, per-job waiter, easy job submission via lambda functions
- :white_check_mark: Threading primitives (Thread, Locks)
- :clock9: Regular Expressions matching (needs backtracking, not done yet, for now only matches greedily)
- :white_check_mark: OpenCL (kernel management, execution, resource caching)
- :white_check_mark: Memory allocators (based on jemalloc)
- :clock9: JSON parser, serializer, manipulation (done but needs optimization)
- :white_check_mark: Python 3.8-3.14 parser
- :white_check_mark: Command-line arguments parser
- :white_check_mark: Rust-like Expected<T> for better error handling across platforms
- :white_check_mark: Loop-guard for infinite while loop, better error handling and debugging

## Maths
- :white_check_mark: Float 32/64 base functions
- :clock9: Float 32/64 Dense Matrix
- :clock9: Dense Matrix ops on GPU (via OpenCL)
- :clock9: Dense Matrix solvers
- :white_check_mark: 2D/3D transforms
- :white_check_mark: 2D/3D/4D vectors
