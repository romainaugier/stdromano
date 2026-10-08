// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - Present Romain Augier
// All rights reserved.

// Usage: bench_mutex
// Compares stdromano::Mutex against std::mutex, best of RUNS runs per case

#include "stdromano/mutex.hpp"
#include "stdromano/threading.hpp"

#include <chrono>
#include <cstdint>
#include <cstdio>
#include <mutex>
#include <thread>
#include <vector>

using namespace stdromano;

using Clock = std::chrono::steady_clock;

static constexpr std::size_t RUNS = 5;
static constexpr std::size_t TOTAL_LOCKS = 4000000;

template <typename M>
static double run_once(const std::size_t num_threads, const std::size_t work)
{
    M mutex;
    volatile std::uint64_t shared = 0;
    const std::size_t iterations = TOTAL_LOCKS / num_threads;

    std::vector<std::thread> threads;

    const auto start = Clock::now();

    for(std::size_t t = 0; t < num_threads; ++t)
    {
        threads.emplace_back([&]() {
            volatile std::uint64_t local = 0;

            for(std::size_t i = 0; i < iterations; ++i)
            {
                mutex.lock();

                for(std::size_t w = 0; w < work; ++w)
                    shared = shared + 1;

                mutex.unlock();

                for(std::size_t w = 0; w < work; ++w)
                    local = local + 1;
            }
        });
    }

    for(std::thread& thread : threads)
        thread.join();

    return std::chrono::duration<double, std::milli>(Clock::now() - start).count();
}

template <typename M>
static double run_best(const std::size_t num_threads, const std::size_t work)
{
    double best = run_once<M>(num_threads, work);

    for(std::size_t i = 1; i < RUNS; ++i)
    {
        const double elapsed = run_once<M>(num_threads, work);
        best = elapsed < best ? elapsed : best;
    }

    return best;
}

int main()
{
    const std::size_t num_procs = get_num_procs();
    const std::size_t thread_counts[] = {1, 2, num_procs, num_procs * 2};
    const std::size_t works[] = {0, 50};

    std::printf("%zu hardware threads, %zu locks per case\n\n", num_procs, TOTAL_LOCKS);
    std::printf("%-8s %-6s %14s %18s %9s\n", "threads", "work", "std::mutex", "stdromano::Mutex", "speedup");

    std::size_t previous = 0;

    for(const std::size_t num_threads : thread_counts)
    {
        if(num_threads == previous)
            continue;

        previous = num_threads;

        for(const std::size_t work : works)
        {
            const double std_ms = run_best<std::mutex>(num_threads, work);
            const double stdromano_ms = run_best<Mutex>(num_threads, work);

            std::printf("%-8zu %-6zu %11.1f ms %15.1f ms %8.2fx\n",
                        num_threads,
                        work,
                        std_ms,
                        stdromano_ms,
                        std_ms / stdromano_ms);
        }
    }

    return 0;
}
