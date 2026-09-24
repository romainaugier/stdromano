// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - Present Romain Augier
// All rights reserved.

#include "stdromano/expected.hpp"

#include "fixtures.hpp"

#include <stdexcept>

STDROMANO_TEST_CASE(ok)
{
    STDROMANO_CHECK(!stdromano::Ok().has_error());
}

STDROMANO_TEST_CASE(ok_has_value)
{
    auto ok = stdromano::Ok();
    STDROMANO_CHECK(ok.has_value());
}

STDROMANO_TEST_CASE(ok_bool_conversion)
{
    auto ok = stdromano::Ok();
    STDROMANO_CHECK(static_cast<bool>(ok));
}

STDROMANO_TEST_CASE(expected_void_error)
{
    stdromano::Expected<void> e(stdromano::Error(stdromano::StringD("something failed")));
    STDROMANO_CHECK(e.has_error());
    STDROMANO_CHECK(!e.has_value());
    STDROMANO_CHECK(!static_cast<bool>(e));
}

STDROMANO_TEST_CASE(expected_void_error_message)
{
    stdromano::Expected<void> e(stdromano::Error(stdromano::StringD("disk full")));
    STDROMANO_CHECK(e.error().message == stdromano::StringD("disk full"));
}

STDROMANO_TEST_CASE(expected_void_on_error_called)
{
    stdromano::Expected<void> e(stdromano::Error(stdromano::StringD("oops")));
    bool called = false;
    e.on_error([&](const stdromano::Error& err) { called = true; });
    STDROMANO_CHECK(called);
}

STDROMANO_TEST_CASE(expected_void_on_error_not_called)
{
    auto ok = stdromano::Ok();
    bool called = false;
    ok.on_error([&](const stdromano::Error& err) { called = true; });
    STDROMANO_CHECK(!called);
}

STDROMANO_TEST_CASE(expected_void_on_error_chaining)
{
    stdromano::Expected<void> e(stdromano::Error(stdromano::StringD("err")));
    int count = 0;
    e.on_error([&](const stdromano::Error&) { count++; })
     .on_error([&](const stdromano::Error&) { count++; });
    STDROMANO_CHECK(count == 2);
}

STDROMANO_TEST_CASE(error_default_construct)
{
    stdromano::Error err;
    STDROMANO_CHECK(err.message == stdromano::StringD(""));
}

STDROMANO_TEST_CASE(error_with_message)
{
    stdromano::Error err(stdromano::StringD("bad input"));
    STDROMANO_CHECK(err.message == stdromano::StringD("bad input"));
}

STDROMANO_TEST_CASE(fmt_error_message)
{
    stdromano::Error err("Error with num {}", 1);
    STDROMANO_CHECK(err.message == stdromano::StringD("Error with num 1"));
}

STDROMANO_TEST_CASE(expected_int_value)
{
    stdromano::Expected<int> e(42);
    STDROMANO_CHECK(e.has_value());
    STDROMANO_CHECK(!e.has_error());
    STDROMANO_CHECK(static_cast<bool>(e));
    STDROMANO_CHECK(e.value() == 42);
}

STDROMANO_TEST_CASE(expected_int_error)
{
    stdromano::Expected<int> e(stdromano::Error(stdromano::StringD("no int")));
    STDROMANO_CHECK(!e.has_value());
    STDROMANO_CHECK(e.has_error());
    STDROMANO_CHECK(!static_cast<bool>(e));
}

STDROMANO_TEST_CASE(expected_int_error_message)
{
    stdromano::Expected<int> e(stdromano::Error(stdromano::StringD("overflow")));
    STDROMANO_CHECK(e.error().message == stdromano::StringD("overflow"));
}

STDROMANO_TEST_CASE(expected_const_value)
{
    const stdromano::Expected<int> e(7);
    STDROMANO_CHECK(e.value() == 7);
}

STDROMANO_TEST_CASE(unwrap_success)
{
    stdromano::Expected<int> e(99);
    STDROMANO_CHECK(e.unwrap() == 99);
}

STDROMANO_TEST_CASE(value_or_with_value)
{
    stdromano::Expected<int> e(10);
    bool called = false;
    int v = e.value_or([&](const stdromano::Error&) { called = true; });
    STDROMANO_CHECK(v == 10);
    STDROMANO_CHECK(!called);
}

STDROMANO_TEST_CASE(value_or_with_error_calls_lambda)
{
    stdromano::Expected<int> e(stdromano::Error(stdromano::StringD("missing")));
    bool called = false;
    try
    {
        e.value_or([&](const stdromano::Error&) { called = true; });
    }
    catch(...)
    {
    }
    STDROMANO_CHECK(called);
}

STDROMANO_TEST_CASE(expected_string_value)
{
    stdromano::Expected<stdromano::StringD> e(stdromano::StringD("hello"));
    STDROMANO_CHECK(e.has_value());
    STDROMANO_CHECK(e.value() == stdromano::StringD("hello"));
}

STDROMANO_TEST_CASE(expected_string_error)
{
    stdromano::Expected<stdromano::StringD> e(stdromano::Error(stdromano::StringD("nope")));
    STDROMANO_CHECK(e.has_error());
    STDROMANO_CHECK(e.error().message == stdromano::StringD("nope"));
}

STDROMANO_TEST_CASE(expected_double_value)
{
    stdromano::Expected<double> e(3.14);
    STDROMANO_CHECK(e.has_value());
    STDROMANO_CHECK(e.value() == 3.14);
}

STDROMANO_TEST_CASE(expected_move_construct)
{
    stdromano::Expected<int> a(123);
    stdromano::Expected<int> b(std::move(a));
    STDROMANO_CHECK(b.has_value());
    STDROMANO_CHECK(b.value() == 123);
}

STDROMANO_TEST_CASE(expected_void_default_construct)
{
    stdromano::Expected<void> e;
    STDROMANO_CHECK(e.has_value());
    STDROMANO_CHECK(!e.has_error());
}

STDROMANO_TEST_CASE(unwrap_throws_runtime_error)
{
    stdromano::Expected<int> e(stdromano::Error(stdromano::StringD("fail")));
    STDROMANO_CHECK_THROWS(e.unwrap(), std::runtime_error);
}

STDROMANO_TEST_CASE(fuzz_value_or_error)
{
    const auto report = stdromano::fuzz::run_property(fixtures::options("expected_value_or_error", 500), [](stdromano::fuzz::Source& source) {
        const bool has_value = source.boolean();
        const int value = source.integer<int>();
        const stdromano::StringD message = source.string(32, "abc ");

        stdromano::Expected<int> e = has_value ? stdromano::Expected<int>(value)
                                                     : stdromano::Expected<int>(stdromano::Error(message.copy()));

        STDROMANO_FUZZ_CHECK_EQ(e.has_value(), has_value);
        STDROMANO_FUZZ_CHECK_EQ(e.has_error(), !has_value);
        STDROMANO_FUZZ_CHECK_EQ(static_cast<bool>(e), has_value);

        if(has_value)
            STDROMANO_FUZZ_CHECK_EQ(e.value(), value);
        else
            STDROMANO_FUZZ_CHECK(e.error().message == message);

        stdromano::Expected<int> moved(std::move(e));
        STDROMANO_FUZZ_CHECK_EQ(moved.has_value(), has_value);

        return true;
    });

    TESTS_REQUIRE_PROPERTY(report);
}

STDROMANO_TEST_MAIN()
