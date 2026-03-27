// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - Present Romain Augier
// All rights reserved.

#include "stdromano/expected.hpp"
#include "test.hpp"
#include "spdlog/spdlog.h"

/* ---------- Ok / Expected<void> ---------- */

TEST_CASE(test_ok)
{
    ASSERT(stdromano::Ok().has_error() == false);
}

TEST_CASE(test_ok_has_value)
{
    auto ok = stdromano::Ok();
    ASSERT(ok.has_value() == true);
}

TEST_CASE(test_ok_bool_conversion)
{
    auto ok = stdromano::Ok();
    ASSERT(static_cast<bool>(ok) == true);
}

TEST_CASE(test_expected_void_error)
{
    stdromano::Expected<void> e(stdromano::Error(stdromano::StringD("something failed")));
    ASSERT(e.has_error() == true);
    ASSERT(e.has_value() == false);
    ASSERT(static_cast<bool>(e) == false);
}

TEST_CASE(test_expected_void_error_message)
{
    stdromano::Expected<void> e(stdromano::Error(stdromano::StringD("disk full")));
    ASSERT(e.error().message == stdromano::StringD("disk full"));
}

TEST_CASE(test_expected_void_on_error_called)
{
    stdromano::Expected<void> e(stdromano::Error(stdromano::StringD("oops")));
    bool called = false;
    e.on_error([&](const stdromano::Error& err) { called = true; });
    ASSERT(called == true);
}

TEST_CASE(test_expected_void_on_error_not_called)
{
    auto ok = stdromano::Ok();
    bool called = false;
    ok.on_error([&](const stdromano::Error& err) { called = true; });
    ASSERT(called == false);
}

TEST_CASE(test_expected_void_on_error_chaining)
{
    stdromano::Expected<void> e(stdromano::Error(stdromano::StringD("err")));
    int count = 0;
    e.on_error([&](const stdromano::Error&) { count++; })
     .on_error([&](const stdromano::Error&) { count++; });
    ASSERT(count == 2);
}

/* ---------- Error struct ---------- */

TEST_CASE(test_error_default_construct)
{
    stdromano::Error err;
    ASSERT(err.message == stdromano::StringD(""));
}

TEST_CASE(test_error_with_message)
{
    stdromano::Error err(stdromano::StringD("bad input"));
    ASSERT(err.message == stdromano::StringD("bad input"));
}

/* ---------- Expected<T> with value ---------- */

TEST_CASE(test_expected_int_value)
{
    stdromano::Expected<int> e(42);
    ASSERT(e.has_value() == true);
    ASSERT(e.has_error() == false);
    ASSERT(static_cast<bool>(e) == true);
    ASSERT(e.value() == 42);
}

TEST_CASE(test_expected_int_error)
{
    stdromano::Expected<int> e(stdromano::Error(stdromano::StringD("no int")));
    ASSERT(e.has_value() == false);
    ASSERT(e.has_error() == true);
    ASSERT(static_cast<bool>(e) == false);
}

TEST_CASE(test_expected_int_error_message)
{
    stdromano::Expected<int> e(stdromano::Error(stdromano::StringD("overflow")));
    ASSERT(e.error().message == stdromano::StringD("overflow"));
}

TEST_CASE(test_expected_const_value)
{
    const stdromano::Expected<int> e(7);
    ASSERT(e.value() == 7);
}

/* ---------- unwrap ---------- */

TEST_CASE(test_unwrap_success)
{
    stdromano::Expected<int> e(99);
    ASSERT(e.unwrap() == 99);
}

TEST_CASE(test_unwrap_throws_on_error)
{
    stdromano::Expected<int> e(stdromano::Error(stdromano::StringD("fail")));
    bool threw = false;
    try
    {
        e.unwrap();
    }
    catch(const std::runtime_error&)
    {
        threw = true;
    }
    ASSERT(threw == true);
}

/* ---------- value_or ---------- */

TEST_CASE(test_value_or_with_value)
{
    stdromano::Expected<int> e(10);
    bool called = false;
    int v = e.value_or([&](const stdromano::Error&) { called = true; });
    ASSERT(v == 10);
    ASSERT(called == false);
}

TEST_CASE(test_value_or_with_error_calls_lambda)
{
    stdromano::Expected<int> e(stdromano::Error(stdromano::StringD("missing")));
    bool called = false;
    try
    {
        e.value_or([&](const stdromano::Error&) { called = true; });
    }
    catch(...)
    {
        // std::get will throw since the variant holds Error, not T
    }
    ASSERT(called == true);
}

/* ---------- Expected<T> with non-trivial types ---------- */

TEST_CASE(test_expected_string_value)
{
    stdromano::Expected<stdromano::StringD> e(stdromano::StringD("hello"));
    ASSERT(e.has_value() == true);
    ASSERT(e.value() == stdromano::StringD("hello"));
}

TEST_CASE(test_expected_string_error)
{
    stdromano::Expected<stdromano::StringD> e(stdromano::Error(stdromano::StringD("nope")));
    ASSERT(e.has_error() == true);
    ASSERT(e.error().message == stdromano::StringD("nope"));
}

TEST_CASE(test_expected_double_value)
{
    stdromano::Expected<double> e(3.14);
    ASSERT(e.has_value() == true);
    ASSERT(e.value() == 3.14);
}

/* ---------- Move semantics ---------- */

TEST_CASE(test_expected_move_construct)
{
    stdromano::Expected<int> a(123);
    stdromano::Expected<int> b(std::move(a));
    ASSERT(b.has_value() == true);
    ASSERT(b.value() == 123);
}

TEST_CASE(test_expected_void_default_construct)
{
    stdromano::Expected<void> e;
    ASSERT(e.has_value() == true);
    ASSERT(e.has_error() == false);
}

int main()
{
    spdlog::set_level(spdlog::level::trace);
    spdlog::info("Starting expected test");

    TestRunner runner;
    runner.add_test("test_ok", test_ok);
    runner.add_test("test_ok_has_value", test_ok_has_value);
    runner.add_test("test_ok_bool_conversion", test_ok_bool_conversion);
    runner.add_test("test_expected_void_error", test_expected_void_error);
    runner.add_test("test_expected_void_error_message", test_expected_void_error_message);
    runner.add_test("test_expected_void_on_error_called", test_expected_void_on_error_called);
    runner.add_test("test_expected_void_on_error_not_called", test_expected_void_on_error_not_called);
    runner.add_test("test_expected_void_on_error_chaining", test_expected_void_on_error_chaining);
    runner.add_test("test_error_default_construct", test_error_default_construct);
    runner.add_test("test_error_with_message", test_error_with_message);
    runner.add_test("test_expected_int_value", test_expected_int_value);
    runner.add_test("test_expected_int_error", test_expected_int_error);
    runner.add_test("test_expected_int_error_message", test_expected_int_error_message);
    runner.add_test("test_expected_const_value", test_expected_const_value);
    runner.add_test("test_unwrap_success", test_unwrap_success);
    runner.add_test("test_unwrap_throws_on_error", test_unwrap_throws_on_error);
    runner.add_test("test_value_or_with_value", test_value_or_with_value);
    runner.add_test("test_value_or_with_error_calls_lambda", test_value_or_with_error_calls_lambda);
    runner.add_test("test_expected_string_value", test_expected_string_value);
    runner.add_test("test_expected_string_error", test_expected_string_error);
    runner.add_test("test_expected_double_value", test_expected_double_value);
    runner.add_test("test_expected_move_construct", test_expected_move_construct);
    runner.add_test("test_expected_void_default_construct", test_expected_void_default_construct);
    runner.run_all();

    spdlog::info("Finished expected test");

    return 0;
}
