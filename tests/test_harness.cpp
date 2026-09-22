// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - Present Romain Augier
// All rights reserved.

// The harness tests itself by running inner runners whose cases fail on purpose: failure scopes
// nest, so those failures belong to the inner case and do not fail this one

#include "stdromano/test.hpp"

#include <atomic>
#include <cstdlib>
#include <fstream>
#include <thread>

using namespace stdromano;

// Inner runs are expected to fail, so their output is silenced
class QuietLogs
{
    spdlog::level::level_enum _previous;

public:
    QuietLogs() : _previous(spdlog::get_level()) { spdlog::set_level(spdlog::level::off); }
    ~QuietLogs() { spdlog::set_level(this->_previous); }
};

static void set_env(const char* name, const char* value)
{
#if defined(STDROMANO_WIN)
    _putenv_s(name, value);
#else
    setenv(name, value, 1);
#endif // defined(STDROMANO_WIN)
}

static void unset_env(const char* name)
{
#if defined(STDROMANO_WIN)
    _putenv_s(name, "");
#else
    unsetenv(name);
#endif // defined(STDROMANO_WIN)
}

STDROMANO_TEST_CASE(passing_case_returns_zero)
{
    test::TestRunner runner;
    runner.add_test("passes", []() { STDROMANO_CHECK_EQ(1 + 1, 2); });

    QuietLogs quiet;
    STDROMANO_CHECK_EQ(runner.run_all(), 0);
}

STDROMANO_TEST_CASE(check_is_not_fatal)
{
    bool reached_end = false;

    test::TestRunner runner;
    runner.add_test("fails", [&]() {
        STDROMANO_CHECK_EQ(1, 2);
        STDROMANO_CHECK(false);
        reached_end = true;
    });

    QuietLogs quiet;
    STDROMANO_CHECK_EQ(runner.run_all(), 1);
    STDROMANO_CHECK(reached_end);
}

STDROMANO_TEST_CASE(require_ends_the_case_only)
{
    bool reached_end = false;
    bool second_case_ran = false;

    test::TestRunner runner;
    runner.add_test("fails", [&]() {
        STDROMANO_REQUIRE(false);
        reached_end = true;
    });
    runner.add_test("runs anyway", [&]() { second_case_ran = true; });

    QuietLogs quiet;
    STDROMANO_CHECK_EQ(runner.run_all(), 1);
    STDROMANO_CHECK(!reached_end);
    STDROMANO_CHECK(second_case_ran);
}

STDROMANO_TEST_CASE(uncaught_exceptions_fail_the_case)
{
    test::TestRunner runner;
    runner.add_test("throws std", []() { throw std::runtime_error("boom"); });
    runner.add_test("throws other", []() { throw 42; });

    QuietLogs quiet;
    STDROMANO_CHECK_EQ(runner.run_all(), 1);
}

// The code under test often catches std::exception: AbortCase must go through it
STDROMANO_TEST_CASE(abort_case_is_not_a_std_exception)
{
    bool swallowed = false;

    test::TestRunner runner;
    runner.add_test("fails", [&]() {
        try
        {
            STDROMANO_REQUIRE(false);
        }
        catch(const std::exception&)
        {
            swallowed = true;
        }
    });

    QuietLogs quiet;
    STDROMANO_CHECK_EQ(runner.run_all(), 1);
    STDROMANO_CHECK(!swallowed);
}

STDROMANO_TEST_CASE(comparisons)
{
    STDROMANO_CHECK_NE(1, 2);
    STDROMANO_CHECK_LT(1, 2);
    STDROMANO_CHECK_LE(2, 2);
    STDROMANO_CHECK_GT(2, 1);
    STDROMANO_CHECK_GE(2, 2);
    STDROMANO_CHECK_NEAR(1.0, 1.0 + 1e-9, 1e-6);
    STDROMANO_CHECK_EQ(StringD::make_ref("abc"), StringD::make_ref("abc"));

    test::TestRunner runner;
    runner.add_test("near fails", []() { STDROMANO_CHECK_NEAR(1.0, 2.0, 1e-6); });
    // NaN is near nothing, itself included
    runner.add_test("nan is never near", []() {
        STDROMANO_CHECK_NEAR(std::nan(""), std::nan(""), 1e9);
    });

    QuietLogs quiet;
    STDROMANO_CHECK_EQ(runner.run_all(), 1);
}

struct Unprintable
{
    bool operator==(const Unprintable&) const { return true; }
};

enum class Color : std::uint8_t
{
    Red = 3
};

// Values are printed when a check fails, and types fmt cannot print must still compile
STDROMANO_TEST_CASE(values_of_any_type_compile)
{
    STDROMANO_CHECK_EQ(Unprintable{}, Unprintable{});
    STDROMANO_CHECK_EQ(Color::Red, Color::Red);
    STDROMANO_CHECK_EQ(std::string("a"), std::string("a"));
}

STDROMANO_TEST_CASE(throws_checks)
{
    STDROMANO_CHECK_THROWS(throw std::runtime_error("x"), std::runtime_error);
    STDROMANO_CHECK_NOTHROW(1 + 1);

    test::TestRunner runner;
    runner.add_test("wrong type", []() { STDROMANO_CHECK_THROWS(throw 42, std::runtime_error); });
    runner.add_test("does not throw", []() { STDROMANO_CHECK_THROWS(1 + 1, std::runtime_error); });
    runner.add_test("throws", []() { STDROMANO_CHECK_NOTHROW(throw std::runtime_error("x")); });

    QuietLogs quiet;
    STDROMANO_CHECK_EQ(runner.run_all(), 1);
}

// A check failing in a worker thread belongs to the case the main thread is running
STDROMANO_TEST_CASE(failures_from_other_threads_count)
{
    test::TestRunner runner;
    runner.add_test("fails in a thread", []() {
        std::thread worker([]() { STDROMANO_CHECK(false); });
        worker.join();
    });

    QuietLogs quiet;
    STDROMANO_CHECK_EQ(runner.run_all(), 1);
}

STDROMANO_TEST_CASE(filtering)
{
    std::atomic<int> alpha{0};
    std::atomic<int> beta{0};

    test::TestRunner runner;
    runner.add_test("alpha_case", [&]() { alpha.fetch_add(1); });
    runner.add_test("beta_case", [&]() { beta.fetch_add(1); });

    char program[] = "test";
    char filter[] = "alpha";
    char* argv[] = {program, filter, nullptr};

    QuietLogs quiet;
    STDROMANO_CHECK_EQ(runner.run(2, argv), 0);
    STDROMANO_CHECK_EQ(alpha.load(), 1);
    STDROMANO_CHECK_EQ(beta.load(), 0);

    // A filter matching nothing is an error, not a silent pass
    char nothing[] = "gamma";
    char* none_argv[] = {program, nothing, nullptr};
    STDROMANO_CHECK_EQ(runner.run(2, none_argv), 1);

    // --list runs nothing
    char list[] = "--list";
    char* list_argv[] = {program, list, nullptr};
    STDROMANO_CHECK_EQ(runner.run(2, list_argv), 0);
    STDROMANO_CHECK_EQ(alpha.load(), 1);

    set_env("STDROMANO_TEST_FILTER", "beta");
    STDROMANO_CHECK_EQ(runner.run_all(), 0);
    unset_env("STDROMANO_TEST_FILTER");

    STDROMANO_CHECK_EQ(beta.load(), 1);
}

STDROMANO_TEST_CASE(temp_paths)
{
    const StringD first = test::temp_path("first.txt");
    const StringD second = test::temp_path("second.txt");

    STDROMANO_CHECK_NE(first, second);

    {
        std::ofstream file(first.c_str());
        STDROMANO_REQUIRE(file.is_open());
        file << "content";
    }

    std::ifstream file(first.c_str());
    std::string content;
    file >> content;

    STDROMANO_CHECK_EQ(content, std::string("content"));
}

STDROMANO_TEST_MAIN()
