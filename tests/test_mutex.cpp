// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - Present Romain Augier
// All rights reserved.

#include "stdromano/mutex.hpp"
#include "stdromano/threading.hpp"

#include "fixtures.hpp"

#include <atomic>
#include <chrono>
#include <cstdint>
#include <thread>
#include <vector>

using namespace stdromano;

static Mutex g_static_mutex;

STDROMANO_TEST_CASE(mutex_lock_and_try_lock)
{
    Mutex mutex;

    STDROMANO_REQUIRE(mutex.try_lock());
    STDROMANO_CHECK(!mutex.try_lock());
    mutex.unlock();

    mutex.lock();
    STDROMANO_CHECK(!mutex.try_lock());
    mutex.unlock();

    STDROMANO_CHECK(mutex.try_lock());
    mutex.unlock();
}

STDROMANO_TEST_CASE(mutex_static_storage)
{
    g_static_mutex.lock();
    STDROMANO_CHECK(!g_static_mutex.try_lock());
    g_static_mutex.unlock();
}

STDROMANO_TEST_CASE(scoped_lock_locks_for_its_scope)
{
    Mutex mutex;

    {
        ScopedLock lock(mutex);
        STDROMANO_CHECK(!mutex.try_lock());
    }

    STDROMANO_REQUIRE(mutex.try_lock());

    {
        ScopedLock lock(mutex, adopt_lock);
        STDROMANO_CHECK(!mutex.try_lock());
    }

    STDROMANO_CHECK(mutex.try_lock());
    mutex.unlock();
}

STDROMANO_TEST_CASE(unique_lock_modes)
{
    Mutex mutex;

    {
        UniqueLock lock(mutex);
        STDROMANO_CHECK(lock.owns_lock());
        STDROMANO_CHECK(lock.mutex() == &mutex);

        lock.unlock();
        STDROMANO_CHECK(!lock);
        STDROMANO_CHECK(mutex.try_lock());
        mutex.unlock();

        lock.lock();
        STDROMANO_CHECK(lock.owns_lock());
        STDROMANO_CHECK(!mutex.try_lock());
    }

    {
        UniqueLock lock(mutex, defer_lock);
        STDROMANO_CHECK(!lock.owns_lock());
        STDROMANO_CHECK(lock.try_lock());
        STDROMANO_CHECK(!mutex.try_lock());
    }

    {
        mutex.lock();
        UniqueLock lock(mutex, try_to_lock);
        STDROMANO_CHECK(!lock.owns_lock());
        mutex.unlock();
    }

    {
        UniqueLock lock(mutex, try_to_lock);
        STDROMANO_CHECK(lock.owns_lock());
    }

    {
        mutex.lock();
        UniqueLock lock(mutex, adopt_lock);
        STDROMANO_CHECK(lock.owns_lock());
    }

    STDROMANO_CHECK(mutex.try_lock());
    mutex.unlock();
}

STDROMANO_TEST_CASE(unique_lock_move_and_release)
{
    Mutex mutex;
    Mutex other;

    UniqueLock a(mutex);
    UniqueLock b(std::move(a));
    STDROMANO_CHECK(!a.owns_lock());
    STDROMANO_CHECK(a.mutex() == nullptr);
    STDROMANO_CHECK(b.owns_lock());

    UniqueLock c(other);
    c = std::move(b);
    STDROMANO_CHECK(other.try_lock());
    other.unlock();
    STDROMANO_CHECK(c.owns_lock());
    STDROMANO_CHECK(c.mutex() == &mutex);

    Mutex* released = c.release();
    STDROMANO_CHECK(released == &mutex);
    STDROMANO_CHECK(!c.owns_lock());
    STDROMANO_CHECK(!mutex.try_lock());
    released->unlock();

    STDROMANO_CHECK(mutex.try_lock());
    mutex.unlock();
}

STDROMANO_TEST_CASE(mutex_mutual_exclusion_under_contention)
{
    constexpr std::size_t NUM_THREADS = 8;
    constexpr std::size_t NUM_ITERATIONS = 100000;

    Mutex mutex;
    std::uint64_t counter = 0;
    bool inside = false;
    std::atomic<bool> overlap{false};

    std::vector<std::thread> threads;

    for(std::size_t i = 0; i < NUM_THREADS; ++i)
    {
        threads.emplace_back([&]() {
            for(std::size_t j = 0; j < NUM_ITERATIONS; ++j)
            {
                ScopedLock lock(mutex);

                if(inside)
                    overlap.store(true);

                inside = true;
                ++counter;
                inside = false;
            }
        });
    }

    for(std::thread& thread : threads)
        thread.join();

    STDROMANO_CHECK(!overlap.load());
    STDROMANO_CHECK_EQ(counter, NUM_THREADS * NUM_ITERATIONS);
}

STDROMANO_TEST_CASE(mutex_waiters_sleep_and_wake_up)
{
    constexpr std::size_t NUM_THREADS = 8;

    Mutex mutex;
    std::atomic<std::size_t> started{0};
    std::size_t acquired = 0;

    mutex.lock();

    std::vector<std::thread> threads;

    for(std::size_t i = 0; i < NUM_THREADS; ++i)
    {
        threads.emplace_back([&]() {
            started.fetch_add(1);

            ScopedLock lock(mutex);
            ++acquired;
        });
    }

    while(started.load() != NUM_THREADS)
        thread_yield();

    // Long enough for the waiters to exhaust their spin and go to sleep in the kernel
    thread_sleep(50);

    STDROMANO_CHECK_EQ(acquired, 0u);

    mutex.unlock();

    for(std::thread& thread : threads)
        thread.join();

    STDROMANO_CHECK_EQ(acquired, NUM_THREADS);
    STDROMANO_CHECK(mutex.try_lock());
    mutex.unlock();
}

STDROMANO_TEST_CASE(futex_wait_returns_when_value_differs)
{
    volatile std::uint32_t word = 1;

    detail::futex_wait(&word, 0);

    STDROMANO_CHECK_EQ(word, 1u);
}

STDROMANO_TEST_CASE(futex_wake_all_wakes_every_waiter)
{
    constexpr std::size_t NUM_THREADS = 8;

    Atomic<std::uint32_t> word{0};
    volatile std::uint32_t* address = reinterpret_cast<volatile std::uint32_t*>(&word);
    std::atomic<std::size_t> woken{0};

    std::vector<std::thread> threads;

    for(std::size_t i = 0; i < NUM_THREADS; ++i)
    {
        threads.emplace_back([&]() {
            while(word.load(MemoryOrder::Acquire) == 0)
                detail::futex_wait(address, 0);

            woken.fetch_add(1);
        });
    }

    thread_sleep(20);

    word.store(1, MemoryOrder::Release);
    detail::futex_wake_all(address);

    for(std::thread& thread : threads)
        thread.join();

    STDROMANO_CHECK_EQ(woken.load(), NUM_THREADS);
}

static double elapsed_ms(const std::chrono::steady_clock::time_point start) noexcept
{
    return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
}

STDROMANO_TEST_CASE(mutex_try_lock_for_zero_tries_once)
{
    Mutex mutex;

    STDROMANO_REQUIRE(mutex.try_lock_for(0));
    STDROMANO_CHECK(!mutex.try_lock_for(0));
    mutex.unlock();
}

STDROMANO_TEST_CASE(mutex_try_lock_for_times_out)
{
    Mutex mutex;
    std::atomic<bool> held{false};
    std::atomic<bool> release{false};

    std::thread holder([&]() {
        mutex.lock();
        held.store(true);

        while(!release.load())
            thread_yield();

        mutex.unlock();
    });

    while(!held.load())
        thread_yield();

    for(const std::uint32_t timeout : {1u, 20u, 100u})
    {
        const auto start = std::chrono::steady_clock::now();
        STDROMANO_CHECK(!mutex.try_lock_for(timeout));

        const double elapsed = elapsed_ms(start);
        STDROMANO_CHECK_GE(elapsed, static_cast<double>(timeout) * 0.9);
        STDROMANO_CHECK_LT(elapsed, static_cast<double>(timeout) + 500.0);
    }

    release.store(true);
    holder.join();

    STDROMANO_CHECK(mutex.try_lock());
    mutex.unlock();
}

STDROMANO_TEST_CASE(mutex_try_lock_for_acquires_when_released_in_time)
{
    Mutex mutex;
    mutex.lock();

    std::thread releaser([&]() {
        thread_sleep(30);
        mutex.unlock();
    });

    const auto start = std::chrono::steady_clock::now();
    const bool acquired = mutex.try_lock_for(5000);
    const double elapsed = elapsed_ms(start);

    releaser.join();

    STDROMANO_REQUIRE(acquired);
    STDROMANO_CHECK_LT(elapsed, 2000.0);
    STDROMANO_CHECK(!mutex.try_lock());
    mutex.unlock();
}

STDROMANO_TEST_CASE(mutex_try_lock_for_infinite)
{
    Mutex mutex;
    mutex.lock();

    std::thread releaser([&]() {
        thread_sleep(20);
        mutex.unlock();
    });

    STDROMANO_CHECK(mutex.try_lock_for(Mutex::INFINITE_TIMEOUT));
    releaser.join();
    mutex.unlock();
}

STDROMANO_TEST_CASE(mutex_try_lock_for_mixed_with_lock_under_contention)
{
    constexpr std::size_t NUM_THREADS = 8;
    constexpr std::size_t NUM_ITERATIONS = 20000;

    Mutex mutex;
    std::uint64_t counter = 0;
    std::atomic<std::uint64_t> successes{0};

    std::vector<std::thread> threads;

    for(std::size_t i = 0; i < NUM_THREADS; ++i)
    {
        threads.emplace_back([&, i]() {
            for(std::size_t j = 0; j < NUM_ITERATIONS; ++j)
            {
                if(i % 2 == 0)
                {
                    ScopedLock lock(mutex);
                    ++counter;
                    successes.fetch_add(1);
                }
                else if(mutex.try_lock_for(static_cast<std::uint32_t>(j % 3)))
                {
                    ScopedLock lock(mutex, adopt_lock);
                    ++counter;
                    successes.fetch_add(1);
                }
            }
        });
    }

    for(std::thread& thread : threads)
        thread.join();

    STDROMANO_CHECK_EQ(counter, successes.load());
    STDROMANO_CHECK_GE(counter, (NUM_THREADS / 2) * NUM_ITERATIONS);
    STDROMANO_CHECK(mutex.try_lock());
    mutex.unlock();
}

STDROMANO_TEST_MAIN()
