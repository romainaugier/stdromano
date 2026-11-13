// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - Present Romain Augier
// All rights reserved.

#include "stdromano/atomic.hpp"
#include "stdromano/threading.hpp"

#include "fixtures.hpp"

#include <set>
#include <stdexcept>
#include <vector>

#if defined(STDROMANO_WIN)
#include <Windows.h>
#else
#include <ctime>
#endif /* defined(STDROMANO_WIN) */

using namespace stdromano;

static std::uint64_t process_cpu_time_ms() noexcept
{
#if defined(STDROMANO_WIN)
    FILETIME creation, exit, kernel, user;
    GetProcessTimes(GetCurrentProcess(), &creation, &exit, &kernel, &user);

    const std::uint64_t kernel_100ns = (static_cast<std::uint64_t>(kernel.dwHighDateTime) << 32) |
                                       kernel.dwLowDateTime;
    const std::uint64_t user_100ns = (static_cast<std::uint64_t>(user.dwHighDateTime) << 32) |
                                     user.dwLowDateTime;

    return (kernel_100ns + user_100ns) / 10000ull;
#else
    struct timespec now;
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &now);

    return static_cast<std::uint64_t>(now.tv_sec) * 1000ull +
           static_cast<std::uint64_t>(now.tv_nsec) / 1000000ull;
#endif /* defined(STDROMANO_WIN) */
}

// Every work blocks until all of them run at the same time, or gives up after a while
template <typename Pool>
static bool burst_runs_on_every_worker(Pool& pool, const std::size_t num_workers)
{
    Atomic<std::size_t> running{0};
    Atomic<std::size_t> all_running{0};
    ThreadPoolWaiter waiter;

    for(std::size_t i = 0; i < num_workers; ++i)
    {
        pool.add_work([&]() {
            running.fetch_add(1);

            for(std::size_t tries = 0; tries < 1000 && all_running.load() == 0; ++tries)
            {
                if(running.load() == num_workers)
                    all_running.store(1);
                else
                    thread_sleep(1);
            }
        }, &waiter);
    }

    waiter.wait();

    return all_running.load() == 1;
}

// Spinning workers burn at least IDLE_WINDOW_MS of cpu time during the window, parked ones none
static constexpr std::size_t IDLE_WINDOW_MS = 300;
static constexpr std::uint64_t IDLE_CPU_BUDGET_MS = 100;

class CountingWork : public ThreadPoolWork
{
    Atomic<std::size_t>* _counter;
    std::size_t _amount;

public:
    CountingWork(Atomic<std::size_t>* counter, const std::size_t amount) : _counter(counter),
                                                                           _amount(amount)
    {
    }

    void execute() override
    {
        this->_counter->fetch_add(this->_amount);
    }
};

STDROMANO_TEST_CASE(threads_run_and_join)
{
    Atomic<std::size_t> counter{0};

    Thread first([&]() {
        thread_sleep(20);
        counter.fetch_add(1);
    });

    Thread second([&]() { counter.fetch_add(10); });

    first.start();
    second.start();

    first.join();
    second.join();

    STDROMANO_CHECK_EQ(counter.load(), 11u);
}

STDROMANO_TEST_CASE(thread_destructor_joins)
{
    Atomic<std::size_t> counter{0};

    {
        Thread thread([&]() {
            thread_sleep(10);
            counter.fetch_add(1);
        });

        thread.start();
    }

    STDROMANO_CHECK_EQ(counter.load(), 1u);
}

STDROMANO_TEST_CASE(threads_have_distinct_ids)
{
    const std::size_t main_id = thread_get_id();
    std::size_t worker_id = main_id;

    Thread thread([&]() { worker_id = thread_get_id(); });
    thread.start();
    thread.join();

    STDROMANO_CHECK_NE(worker_id, main_id);
    STDROMANO_CHECK_EQ(thread_get_id(), main_id);
}

STDROMANO_TEST_CASE(threadpool_runs_every_work)
{
    ThreadPool pool(4);

    STDROMANO_CHECK(pool.is_started());
    STDROMANO_CHECK_EQ(pool.num_workers(), 4u);

    Atomic<std::size_t> counter{0};

    for(std::size_t i = 0; i < 1000; ++i)
        STDROMANO_REQUIRE(pool.add_work(new CountingWork(&counter, 1)));

    for(std::size_t i = 0; i < 1000; ++i)
        STDROMANO_REQUIRE(pool.add_work([&]() { counter.fetch_add(2); }));

    pool.wait();

    STDROMANO_CHECK_EQ(counter.load(), 3000u);
}

STDROMANO_TEST_CASE(threadpool_waiter_waits_for_its_own_work)
{
    ThreadPool pool(3);

    Atomic<std::size_t> counter{0};
    ThreadPoolWaiter waiter;

    for(std::size_t i = 0; i < 200; ++i)
        pool.add_work([&]() {
            thread_sleep(1);
            counter.fetch_add(1);
        }, &waiter);

    waiter.wait();

    STDROMANO_CHECK_EQ(counter.load(), 200u);
}

STDROMANO_TEST_CASE(threadpool_survives_throwing_work)
{
    fixtures::QuietLogs quiet;

    ThreadPool pool(2);

    Atomic<std::size_t> counter{0};
    ThreadPoolWaiter waiter;

    for(std::size_t i = 0; i < 20; ++i)
    {
        pool.add_work([&, i]() {
            if(i % 2 == 0)
                throw std::runtime_error("expected failure");

            counter.fetch_add(1);
        }, &waiter);
    }

    waiter.wait();

    STDROMANO_CHECK_EQ(counter.load(), 10u);
}

STDROMANO_TEST_CASE(threadpool_destruction_drops_pending_work)
{
    Atomic<std::size_t> counter{0};

    {
        ThreadPool pool(1);
        pool.set_max_active_workers(0);

        for(std::size_t i = 0; i < 100; ++i)
            pool.add_work([&]() { counter.fetch_add(1); });
    }

    STDROMANO_CHECK_EQ(counter.load(), 0u);
}

STDROMANO_TEST_CASE(threadpool_idle_workers_do_not_spin)
{
    ThreadPool pool(4);

    Atomic<std::size_t> counter{0};

    for(std::size_t i = 0; i < 100; ++i)
        pool.add_work([&]() { counter.fetch_add(1); });

    pool.wait();

    thread_sleep(50);

    const std::uint64_t start = process_cpu_time_ms();
    thread_sleep(IDLE_WINDOW_MS);
    const std::uint64_t idle_cpu_time = process_cpu_time_ms() - start;

    STDROMANO_CHECK_EQ(counter.load(), 100u);
    STDROMANO_CHECK_LT(idle_cpu_time, IDLE_CPU_BUDGET_MS);
}

STDROMANO_TEST_CASE(threadpool_wakes_parked_workers)
{
    ThreadPool pool(4);

    Atomic<std::size_t> counter{0};

    for(std::size_t round = 0; round < 50; ++round)
    {
        thread_sleep(round % 4);

        ThreadPoolWaiter waiter;

        for(std::size_t i = 0; i <= round % 7; ++i)
            pool.add_work([&]() { counter.fetch_add(1); }, &waiter);

        waiter.wait();
    }

    std::size_t expected = 0;

    for(std::size_t round = 0; round < 50; ++round)
        expected += round % 7 + 1;

    STDROMANO_CHECK_EQ(counter.load(), expected);
}

STDROMANO_TEST_CASE(threadpool_burst_wakes_every_parked_worker)
{
    ThreadPool pool(4);

    for(std::size_t round = 0; round < 5; ++round)
    {
        thread_sleep(20);

        STDROMANO_CHECK(burst_runs_on_every_worker(pool, 4));
    }
}

STDROMANO_TEST_CASE(threadpool_raising_max_active_workers_wakes_parked_workers)
{
    ThreadPool pool(2);
    pool.set_max_active_workers(0);

    Atomic<std::size_t> counter{0};

    for(std::size_t i = 0; i < 100; ++i)
        pool.add_work([&]() { counter.fetch_add(1); });

    thread_sleep(50);

    const std::uint64_t start = process_cpu_time_ms();
    thread_sleep(IDLE_WINDOW_MS);
    const std::uint64_t idle_cpu_time = process_cpu_time_ms() - start;

    STDROMANO_CHECK_EQ(counter.load(), 0u);
    STDROMANO_CHECK_LT(idle_cpu_time, IDLE_CPU_BUDGET_MS);

    pool.set_max_active_workers(2);
    pool.wait();

    STDROMANO_CHECK_EQ(counter.load(), 100u);
}

STDROMANO_TEST_CASE(stealing_threadpool_runs_every_work)
{
    StealingThreadPool pool(4);

    STDROMANO_CHECK(pool.is_started());
    STDROMANO_CHECK_EQ(pool.num_workers(), 4u);

    Atomic<std::size_t> counter{0};

    for(std::size_t i = 0; i < 1000; ++i)
        STDROMANO_REQUIRE(pool.add_work(new CountingWork(&counter, 1)));

    for(std::size_t i = 0; i < 1000; ++i)
        STDROMANO_REQUIRE(pool.add_work([&]() { counter.fetch_add(2); }));

    pool.wait();

    STDROMANO_CHECK_EQ(counter.load(), 3000u);
    STDROMANO_CHECK(!pool.add_work(static_cast<ThreadPoolWork*>(nullptr)));
}

STDROMANO_TEST_CASE(stealing_threadpool_idle_workers_do_not_spin)
{
    StealingThreadPool pool(4);

    Atomic<std::size_t> counter{0};

    for(std::size_t i = 0; i < 100; ++i)
        pool.add_work([&]() { counter.fetch_add(1); });

    pool.wait();

    thread_sleep(50);

    const std::uint64_t start = process_cpu_time_ms();
    thread_sleep(IDLE_WINDOW_MS);
    const std::uint64_t idle_cpu_time = process_cpu_time_ms() - start;

    STDROMANO_CHECK_EQ(counter.load(), 100u);
    STDROMANO_CHECK_LT(idle_cpu_time, IDLE_CPU_BUDGET_MS);
}

STDROMANO_TEST_CASE(stealing_threadpool_wakes_parked_workers)
{
    StealingThreadPool pool(4);

    Atomic<std::size_t> counter{0};

    for(std::size_t round = 0; round < 50; ++round)
    {
        thread_sleep(round % 4);

        ThreadPoolWaiter waiter;

        for(std::size_t i = 0; i <= round % 7; ++i)
            pool.add_work([&]() { counter.fetch_add(1); }, &waiter);

        waiter.wait();
    }

    std::size_t expected = 0;

    for(std::size_t round = 0; round < 50; ++round)
        expected += round % 7 + 1;

    STDROMANO_CHECK_EQ(counter.load(), expected);
}

STDROMANO_TEST_CASE(stealing_threadpool_burst_wakes_every_parked_worker)
{
    StealingThreadPool pool(4);

    for(std::size_t round = 0; round < 5; ++round)
    {
        thread_sleep(20);

        STDROMANO_CHECK(burst_runs_on_every_worker(pool, 4));
    }
}

STDROMANO_TEST_CASE(stealing_threadpool_wakes_for_concurrent_producers)
{
    StealingThreadPool pool(4);

    Atomic<std::size_t> counter{0};
    ThreadPoolWaiter waiter;

    auto produce = [&]() {
        for(std::size_t i = 0; i < 2000; ++i)
        {
            pool.add_work([&]() { counter.fetch_add(1); }, &waiter);

            if(i % 100 == 0)
                thread_sleep(1);
        }
    };

    Thread first(produce);
    Thread second(produce);
    Thread third(produce);

    first.start();
    second.start();
    third.start();

    first.join();
    second.join();
    third.join();

    waiter.wait();

    STDROMANO_CHECK_EQ(counter.load(), 6000u);
}

STDROMANO_TEST_CASE(stealing_threadpool_destruction_wakes_parked_workers)
{
    Atomic<std::size_t> counter{0};

    {
        StealingThreadPool pool(4);

        pool.add_work([&]() { counter.fetch_add(1); });
        pool.wait();

        thread_sleep(20);
    }

    STDROMANO_CHECK_EQ(counter.load(), 1u);
}

STDROMANO_TEST_CASE(global_threadpool_with_waiter)
{
    Atomic<std::size_t> counter{0};
    ThreadPoolWaiter waiter;

    for(std::size_t i = 0; i < 100; ++i)
        global_threadpool().add_work([&]() { counter.fetch_add(1); }, &waiter);

    waiter.wait();

    STDROMANO_CHECK_EQ(counter.load(), 100u);
}

STDROMANO_TEST_CASE(fuzz_pools_account_for_every_work)
{
    const auto report = fuzz::run_property(fixtures::options("threadpool_accounting", 60), [](fuzz::Source& source) {
        const std::size_t workers = source.range<std::size_t>(1, 6);
        const std::size_t count = source.range<std::size_t>(0, 300);

        std::vector<std::size_t> amounts(count);
        std::size_t expected = 0;

        for(auto& amount : amounts)
        {
            amount = source.range<std::size_t>(0, 1000);
            expected += amount;
        }

        Atomic<std::size_t> simple_total{0};
        Atomic<std::size_t> stealing_total{0};

        {
            ThreadPool simple(static_cast<std::int64_t>(workers));
            StealingThreadPool stealing(static_cast<std::int64_t>(workers));

            ThreadPoolWaiter simple_waiter;
            ThreadPoolWaiter stealing_waiter;

            for(const std::size_t amount : amounts)
            {
                if(source.boolean())
                    simple.add_work(new CountingWork(&simple_total, amount), &simple_waiter);
                else
                    simple.add_work([&simple_total, amount]() { simple_total.fetch_add(amount); }, &simple_waiter);

                stealing.add_work([&stealing_total, amount]() { stealing_total.fetch_add(amount); },
                                  &stealing_waiter);
            }

            if(source.boolean())
            {
                simple_waiter.wait();
                stealing_waiter.wait();
            }
            else
            {
                simple.wait();
                stealing.wait();
            }

            STDROMANO_FUZZ_CHECK_EQ(simple_total.load(), expected);
            STDROMANO_FUZZ_CHECK_EQ(stealing_total.load(), expected);
        }

        return true;
    });

    TESTS_REQUIRE_PROPERTY(report);
}

STDROMANO_TEST_MAIN()
