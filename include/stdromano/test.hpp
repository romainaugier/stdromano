// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - Present Romain Augier
// All rights reserved.

// A small test harness, usable by any project linking stdromano
//
//     #include "stdromano/test.hpp"
//
//     STDROMANO_TEST_CASE(addition)
//     {
//         STDROMANO_CHECK_EQ(1 + 1, 2);      // non fatal: the case goes on
//         STDROMANO_REQUIRE(ptr != nullptr); // fatal: ends the case, the others still run
//     }
//
//     STDROMANO_TEST_MAIN()
//
// Arguments select the cases whose name contains them, --list lists the cases and -v logs at
// debug level. STDROMANO_TEST_FILTER=<substring> selects cases too, for runners that pass no
// arguments (ctest)

#pragma once

#if !defined(__STDROMANO_TEST_HARNESS)
#define __STDROMANO_TEST_HARNESS

#include "stdromano/stdromano.hpp"
#include "stdromano/string.hpp"

#include "spdlog/spdlog.h"

#include <chrono>
#include <cmath>
#include <cstdlib>
#include <functional>
#include <type_traits>
#include <vector>

STDROMANO_NAMESPACE_BEGIN

namespace test {

// Thrown by the REQUIRE macros to end the current case. Not a std::exception, so a
// catch(const std::exception&) in the code under test does not swallow it
struct STDROMANO_API AbortCase
{
};

using TestFunc = std::function<void()>;

// Records a failure in the innermost running case. Thread-safe: checks can fail from worker
// threads, and a failure is counted even if the thread swallows the exception that follows
STDROMANO_API void report_failure(const char* file, int line, const char* message) noexcept;

// Failure scopes: a case opens one, report_failure() counts into the innermost one, and closing
// it returns how many failures it collected. Scopes nest, which lets a runner run inside a case
STDROMANO_API void begin_scope() noexcept;
STDROMANO_API std::size_t end_scope() noexcept;
STDROMANO_API bool in_scope() noexcept;

// A path inside a directory unique to this process, created on first use and removed at exit
STDROMANO_API StringD temp_path(const char* name);

DETAIL_NAMESPACE_BEGIN

STDROMANO_API bool matches_filter(const char* name, int argc, char** argv) noexcept;

STDROMANO_API void configure_logging(int argc, char** argv) noexcept;

STDROMANO_API bool wants_list(int argc, char** argv) noexcept;

template <typename T>
StringD to_display(const T& value)
{
    if constexpr(std::is_enum_v<T>)
        return StringD::make_fmt("{}", static_cast<std::underlying_type_t<T>>(value));
    else if constexpr(fmt::is_formattable<T>::value)
        return StringD::make_fmt("{}", value);
    else
        return StringD::make_from_c_str("<unprintable>");
}

[[noreturn]] inline void abort_case()
{
    // Outside of a case (a plain main), there is nothing to end but the process
    if(!in_scope())
        std::abort();

    throw AbortCase{};
}

DETAIL_NAMESPACE_END

class TestRunner
{
    struct TestCase
    {
        const char* name;
        TestFunc func;
    };

    std::vector<TestCase> _tests;
    const char* _name = nullptr;

public:
    explicit TestRunner(const char* name = nullptr) : _name(name) {}

    void add_test(const char* name, TestFunc func) { this->_tests.push_back({name, std::move(func)}); }

    std::size_t size() const noexcept { return this->_tests.size(); }

    // Runs the selected cases, returns 0 when all of them passed and 1 otherwise: meant to be
    // returned from main
    int run_all() { return this->run(0, nullptr); }

    int run(int argc, char** argv)
    {
        if(detail::wants_list(argc, argv))
        {
            for(const auto& test : this->_tests)
                spdlog::info("{}", test.name);

            return 0;
        }

        detail::configure_logging(argc, argv);

        std::size_t passed = 0;
        std::size_t failed = 0;
        std::vector<const char*> failures;

        if(this->_name != nullptr)
            spdlog::info("== {} ==", this->_name);

        for(const auto& test : this->_tests)
        {
            if(!detail::matches_filter(test.name, argc, argv))
                continue;

            spdlog::info("-- {}", test.name);

            const auto start = std::chrono::steady_clock::now();

            begin_scope();

            try
            {
                test.func();
            }
            catch(const AbortCase&)
            {
                // Already reported by the macro that threw
            }
            catch(const std::exception& e)
            {
                report_failure(__FILE__, __LINE__, StringD::make_fmt("uncaught exception: {}", e.what()).c_str());
            }
            catch(...)
            {
                report_failure(__FILE__, __LINE__, "uncaught exception of unknown type");
            }

            const std::size_t case_failures = end_scope();

            const double ms =
                std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();

            if(case_failures == 0)
            {
                ++passed;
                spdlog::info("   passed ({:.2f} ms)", ms);
            }
            else
            {
                ++failed;
                failures.push_back(test.name);
                spdlog::error("   FAILED: {} failure(s) ({:.2f} ms)", case_failures, ms);
            }
        }

        if(passed + failed == 0)
        {
            spdlog::error("No case matched the filter");
            return 1;
        }

        if(failed == 0)
        {
            spdlog::info("== {} passed ==", passed);
            return 0;
        }

        spdlog::error("== {} passed, {} failed ==", passed, failed);

        for(const char* name : failures)
            spdlog::error("   {}", name);

        return 1;
    }
};

// The runner that STDROMANO_TEST_CASE registers into. One per executable
inline TestRunner& default_runner()
{
    static TestRunner runner;
    return runner;
}

struct Registrar
{
    Registrar(const char* name, TestFunc func) { default_runner().add_test(name, std::move(func)); }
};

} // namespace test

STDROMANO_NAMESPACE_END

#define STDROMANO_TEST_CASE(name)                                                                  \
    static void CONCAT(stdromano_test_case_, name)();                                              \
    static const stdromano::test::Registrar CONCAT(stdromano_test_registrar_,                      \
                                                   name)(#name, CONCAT(stdromano_test_case_, name)); \
    static void CONCAT(stdromano_test_case_, name)()

#define STDROMANO_TEST_MAIN()                                                                      \
    int main(int argc, char** argv) { return stdromano::test::default_runner().run(argc, argv); }

#define STDROMANO_DETAIL_FAIL(fatal, message)                                                      \
    do                                                                                             \
    {                                                                                              \
        stdromano::test::report_failure(__FILE__, __LINE__, (message));                            \
        if(fatal)                                                                                  \
            stdromano::test::detail::abort_case();                                                 \
    } while(0)

#define STDROMANO_DETAIL_CHECK(fatal, condition)                                                   \
    do                                                                                             \
    {                                                                                              \
        if(!(condition))                                                                           \
            STDROMANO_DETAIL_FAIL(fatal, "expected: " #condition);                                 \
    } while(0)

#define STDROMANO_DETAIL_CHECK_MSG(fatal, condition, message)                                      \
    do                                                                                             \
    {                                                                                              \
        if(!(condition))                                                                           \
            STDROMANO_DETAIL_FAIL(fatal,                                                           \
                                  stdromano::StringD::make_fmt("expected: {}: {}",                 \
                                                               #condition,                         \
                                                               (message))                          \
                                      .c_str());                                                   \
    } while(0)

#define STDROMANO_DETAIL_COMPARE(fatal, lhs, rhs, op)                                              \
    do                                                                                             \
    {                                                                                              \
        const auto& stdromano_lhs = (lhs);                                                         \
        const auto& stdromano_rhs = (rhs);                                                         \
        if(!(stdromano_lhs op stdromano_rhs))                                                      \
            STDROMANO_DETAIL_FAIL(                                                                 \
                fatal,                                                                             \
                stdromano::StringD::make_fmt("expected: {} " #op " {} ({} vs {})",                 \
                                             #lhs,                                                 \
                                             #rhs,                                                 \
                                             stdromano::test::detail::to_display(stdromano_lhs),   \
                                             stdromano::test::detail::to_display(stdromano_rhs))   \
                    .c_str());                                                                     \
    } while(0)

// NaN never compares near anything, including NaN
#define STDROMANO_DETAIL_NEAR(fatal, lhs, rhs, epsilon)                                            \
    do                                                                                             \
    {                                                                                              \
        const double stdromano_lhs = static_cast<double>(lhs);                                     \
        const double stdromano_rhs = static_cast<double>(rhs);                                     \
        if(!(std::fabs(stdromano_lhs - stdromano_rhs) <= static_cast<double>(epsilon)))            \
            STDROMANO_DETAIL_FAIL(fatal,                                                           \
                                  stdromano::StringD::make_fmt("expected: {} ~= {} ({} vs {}, "    \
                                                               "epsilon {})",                      \
                                                               #lhs,                               \
                                                               #rhs,                               \
                                                               stdromano_lhs,                      \
                                                               stdromano_rhs,                      \
                                                               static_cast<double>(epsilon))       \
                                      .c_str());                                                   \
    } while(0)

#define STDROMANO_DETAIL_THROWS(fatal, expression, exception_type)                                 \
    do                                                                                             \
    {                                                                                              \
        bool stdromano_caught = false;                                                             \
        try                                                                                        \
        {                                                                                          \
            (void)(expression);                                                                    \
        }                                                                                          \
        catch(const exception_type&)                                                               \
        {                                                                                          \
            stdromano_caught = true;                                                               \
        }                                                                                          \
        catch(const stdromano::test::AbortCase&)                                                   \
        {                                                                                          \
            throw;                                                                                 \
        }                                                                                          \
        catch(...)                                                                                 \
        {                                                                                          \
            STDROMANO_DETAIL_FAIL(fatal, #expression " threw, but not " #exception_type);          \
            stdromano_caught = true;                                                               \
        }                                                                                          \
        if(!stdromano_caught)                                                                      \
            STDROMANO_DETAIL_FAIL(fatal, #expression " did not throw " #exception_type);           \
    } while(0)

#define STDROMANO_DETAIL_NOTHROW(fatal, expression)                                                \
    do                                                                                             \
    {                                                                                              \
        try                                                                                        \
        {                                                                                          \
            (void)(expression);                                                                    \
        }                                                                                          \
        catch(const stdromano::test::AbortCase&)                                                   \
        {                                                                                          \
            throw;                                                                                 \
        }                                                                                          \
        catch(...)                                                                                 \
        {                                                                                          \
            STDROMANO_DETAIL_FAIL(fatal, #expression " threw");                                    \
        }                                                                                          \
    } while(0)

// Non fatal: the failure is recorded and the case goes on

#define STDROMANO_CHECK(condition) STDROMANO_DETAIL_CHECK(false, condition)
#define STDROMANO_CHECK_MSG(condition, message) STDROMANO_DETAIL_CHECK_MSG(false, condition, message)
#define STDROMANO_CHECK_EQ(lhs, rhs) STDROMANO_DETAIL_COMPARE(false, lhs, rhs, ==)
#define STDROMANO_CHECK_NE(lhs, rhs) STDROMANO_DETAIL_COMPARE(false, lhs, rhs, !=)
#define STDROMANO_CHECK_LT(lhs, rhs) STDROMANO_DETAIL_COMPARE(false, lhs, rhs, <)
#define STDROMANO_CHECK_LE(lhs, rhs) STDROMANO_DETAIL_COMPARE(false, lhs, rhs, <=)
#define STDROMANO_CHECK_GT(lhs, rhs) STDROMANO_DETAIL_COMPARE(false, lhs, rhs, >)
#define STDROMANO_CHECK_GE(lhs, rhs) STDROMANO_DETAIL_COMPARE(false, lhs, rhs, >=)
#define STDROMANO_CHECK_NEAR(lhs, rhs, epsilon) STDROMANO_DETAIL_NEAR(false, lhs, rhs, epsilon)
#define STDROMANO_CHECK_THROWS(expression, type) STDROMANO_DETAIL_THROWS(false, expression, type)
#define STDROMANO_CHECK_NOTHROW(expression) STDROMANO_DETAIL_NOTHROW(false, expression)

// Fatal: the failure is recorded and the case ends

#define STDROMANO_REQUIRE(condition) STDROMANO_DETAIL_CHECK(true, condition)
#define STDROMANO_REQUIRE_MSG(condition, message) STDROMANO_DETAIL_CHECK_MSG(true, condition, message)
#define STDROMANO_REQUIRE_EQ(lhs, rhs) STDROMANO_DETAIL_COMPARE(true, lhs, rhs, ==)
#define STDROMANO_REQUIRE_NE(lhs, rhs) STDROMANO_DETAIL_COMPARE(true, lhs, rhs, !=)
#define STDROMANO_REQUIRE_LT(lhs, rhs) STDROMANO_DETAIL_COMPARE(true, lhs, rhs, <)
#define STDROMANO_REQUIRE_LE(lhs, rhs) STDROMANO_DETAIL_COMPARE(true, lhs, rhs, <=)
#define STDROMANO_REQUIRE_GT(lhs, rhs) STDROMANO_DETAIL_COMPARE(true, lhs, rhs, >)
#define STDROMANO_REQUIRE_GE(lhs, rhs) STDROMANO_DETAIL_COMPARE(true, lhs, rhs, >=)
#define STDROMANO_REQUIRE_NEAR(lhs, rhs, epsilon) STDROMANO_DETAIL_NEAR(true, lhs, rhs, epsilon)
#define STDROMANO_REQUIRE_THROWS(expression, type) STDROMANO_DETAIL_THROWS(true, expression, type)
#define STDROMANO_REQUIRE_NOTHROW(expression) STDROMANO_DETAIL_NOTHROW(true, expression)

#define STDROMANO_FAIL(message) STDROMANO_DETAIL_FAIL(true, message)

#endif // !defined(__STDROMANO_TEST_HARNESS)
