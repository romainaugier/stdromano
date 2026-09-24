// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - Present Romain Augier
// All rights reserved.

#include "stdromano/env.hpp"

#include "fixtures.hpp"

#include <deque>
#include <string>

STDROMANO_TEST_CASE(env_set_and_get)
{
    const bool ok = stdromano::env::set("STDROMANO_TEST_VAR", "hello");
    STDROMANO_CHECK(ok);

    const stdromano::StringD val = stdromano::env::get("STDROMANO_TEST_VAR");
    STDROMANO_CHECK(val == stdromano::StringD("hello"));

    stdromano::env::unset("STDROMANO_TEST_VAR");
}

STDROMANO_TEST_CASE(env_set_and_get_stringd_overload)
{
    const stdromano::StringD name("STDROMANO_TEST_SD");
    const stdromano::StringD value("world");

    STDROMANO_CHECK(stdromano::env::set(name, value));

    const stdromano::StringD got = stdromano::env::get(name);
    STDROMANO_CHECK(got == value);

    stdromano::env::unset(name);
}

STDROMANO_TEST_CASE(env_get_nonexistent)
{
    const stdromano::StringD val = stdromano::env::get("STDROMANO_SURELY_DOES_NOT_EXIST_12345");
    STDROMANO_CHECK(val.empty());
}

STDROMANO_TEST_CASE(env_get_nullptr)
{
    const stdromano::StringD val = stdromano::env::get(static_cast<const char*>(nullptr));
    STDROMANO_CHECK(val.empty());
}

STDROMANO_TEST_CASE(env_set_empty_value)
{
    STDROMANO_CHECK(stdromano::env::set("STDROMANO_TEST_EMPTY", ""));

    const stdromano::StringD val = stdromano::env::get("STDROMANO_TEST_EMPTY");
    STDROMANO_CHECK(val.empty());

    stdromano::env::unset("STDROMANO_TEST_EMPTY");
}

STDROMANO_TEST_CASE(env_set_nullptr_name)
{
    STDROMANO_CHECK(!stdromano::env::set(static_cast<const char*>(nullptr), "value"));
}

STDROMANO_TEST_CASE(env_set_nullptr_value)
{
    STDROMANO_CHECK(!stdromano::env::set("STDROMANO_TEST_NV", static_cast<const char*>(nullptr)));
}

STDROMANO_TEST_CASE(env_set_overwrite_true)
{
    stdromano::env::set("STDROMANO_TEST_OW", "first");
    stdromano::env::set("STDROMANO_TEST_OW", "second", true);

    const stdromano::StringD val = stdromano::env::get("STDROMANO_TEST_OW");
    STDROMANO_CHECK(val == stdromano::StringD("second"));

    stdromano::env::unset("STDROMANO_TEST_OW");
}

STDROMANO_TEST_CASE(env_set_overwrite_false)
{
    stdromano::env::set("STDROMANO_TEST_NOOW", "first");
    stdromano::env::set("STDROMANO_TEST_NOOW", "second", false);

    const stdromano::StringD val = stdromano::env::get("STDROMANO_TEST_NOOW");
    STDROMANO_CHECK(val == stdromano::StringD("first"));

    stdromano::env::unset("STDROMANO_TEST_NOOW");
}

STDROMANO_TEST_CASE(env_set_overwrite_false_when_unset)
{
    stdromano::env::unset("STDROMANO_TEST_NOOW2");

    stdromano::env::set("STDROMANO_TEST_NOOW2", "value", false);

    const stdromano::StringD val = stdromano::env::get("STDROMANO_TEST_NOOW2");
    STDROMANO_CHECK(val == stdromano::StringD("value"));

    stdromano::env::unset("STDROMANO_TEST_NOOW2");
}

STDROMANO_TEST_CASE(env_unset)
{
    stdromano::env::set("STDROMANO_TEST_UNSET", "bye");
    STDROMANO_CHECK(stdromano::env::has("STDROMANO_TEST_UNSET"));

    const bool ok = stdromano::env::unset("STDROMANO_TEST_UNSET");
    STDROMANO_CHECK(ok);
    STDROMANO_CHECK(!stdromano::env::has("STDROMANO_TEST_UNSET"));

    const stdromano::StringD val = stdromano::env::get("STDROMANO_TEST_UNSET");
    STDROMANO_CHECK(val.empty());
}

STDROMANO_TEST_CASE(env_unset_nonexistent)
{
    const bool ok = stdromano::env::unset("STDROMANO_NEVER_SET_XYZ");
    (void)ok;
}

STDROMANO_TEST_CASE(env_unset_nullptr)
{
    STDROMANO_CHECK(!stdromano::env::unset(static_cast<const char*>(nullptr)));
}

STDROMANO_TEST_CASE(env_unset_stringd_overload)
{
    const stdromano::StringD name("STDROMANO_TEST_UNSET_SD");

    stdromano::env::set(name, stdromano::StringD("tmp"));
    STDROMANO_CHECK(stdromano::env::has(name));

    stdromano::env::unset(name);
    STDROMANO_CHECK(!stdromano::env::has(name));
}

STDROMANO_TEST_CASE(env_has_existing)
{
    stdromano::env::set("STDROMANO_TEST_HAS", "1");
    STDROMANO_CHECK(stdromano::env::has("STDROMANO_TEST_HAS"));
    stdromano::env::unset("STDROMANO_TEST_HAS");
}

STDROMANO_TEST_CASE(env_has_missing)
{
    STDROMANO_CHECK(!stdromano::env::has("STDROMANO_MISSING_VAR_999"));
}

STDROMANO_TEST_CASE(env_has_nullptr)
{
    STDROMANO_CHECK(!stdromano::env::has(static_cast<const char*>(nullptr)));
}

STDROMANO_TEST_CASE(env_has_stringd_overload)
{
    const stdromano::StringD name("STDROMANO_TEST_HAS_SD");

    STDROMANO_CHECK(!stdromano::env::has(name));
    stdromano::env::set(name, stdromano::StringD("yes"));
    STDROMANO_CHECK(stdromano::env::has(name));
    stdromano::env::unset(name);
}

STDROMANO_TEST_CASE(env_append_to_existing)
{
    stdromano::env::set("STDROMANO_TEST_APP", "/usr/bin");

    const bool ok = stdromano::env::append("STDROMANO_TEST_APP", "/usr/local/bin");
    STDROMANO_CHECK(ok);

    const stdromano::StringD val = stdromano::env::get("STDROMANO_TEST_APP");

#if defined(STDROMANO_WIN)
    STDROMANO_CHECK(val == stdromano::StringD("/usr/bin;/usr/local/bin"));
#else
    STDROMANO_CHECK(val == stdromano::StringD("/usr/bin:/usr/local/bin"));
#endif

    stdromano::env::unset("STDROMANO_TEST_APP");
}

STDROMANO_TEST_CASE(env_append_to_nonexistent)
{
    stdromano::env::unset("STDROMANO_TEST_APP_NEW");

    const bool ok = stdromano::env::append("STDROMANO_TEST_APP_NEW", "/first");
    STDROMANO_CHECK(ok);

    const stdromano::StringD val = stdromano::env::get("STDROMANO_TEST_APP_NEW");
    STDROMANO_CHECK(val == stdromano::StringD("/first"));

    stdromano::env::unset("STDROMANO_TEST_APP_NEW");
}

STDROMANO_TEST_CASE(env_append_custom_separator)
{
    stdromano::env::set("STDROMANO_TEST_APP_SEP", "a");

    stdromano::env::append("STDROMANO_TEST_APP_SEP", "b", ',');

    const stdromano::StringD val = stdromano::env::get("STDROMANO_TEST_APP_SEP");
    STDROMANO_CHECK(val == stdromano::StringD("a,b"));

    stdromano::env::unset("STDROMANO_TEST_APP_SEP");
}

STDROMANO_TEST_CASE(env_append_multiple)
{
    stdromano::env::set("STDROMANO_TEST_APP_M", "1");
    stdromano::env::append("STDROMANO_TEST_APP_M", "2", ',');
    stdromano::env::append("STDROMANO_TEST_APP_M", "3", ',');

    const stdromano::StringD val = stdromano::env::get("STDROMANO_TEST_APP_M");
    STDROMANO_CHECK(val == stdromano::StringD("1,2,3"));

    stdromano::env::unset("STDROMANO_TEST_APP_M");
}

STDROMANO_TEST_CASE(env_append_nullptr)
{
    STDROMANO_CHECK(!stdromano::env::append(static_cast<const char*>(nullptr), "val"));
    STDROMANO_CHECK(!stdromano::env::append("STDROMANO_TEST_APP_NP", static_cast<const char*>(nullptr)));
}

STDROMANO_TEST_CASE(env_append_stringd_overload)
{
    const stdromano::StringD name("STDROMANO_TEST_APP_SD");
    const stdromano::StringD v1("alpha");
    const stdromano::StringD v2("beta");

    stdromano::env::set(name, v1);
    stdromano::env::append(name, v2, ',');

    const stdromano::StringD val = stdromano::env::get(name);
    STDROMANO_CHECK(val == stdromano::StringD("alpha,beta"));

    stdromano::env::unset(name);
}

STDROMANO_TEST_CASE(env_prepend_to_existing)
{
    stdromano::env::set("STDROMANO_TEST_PRE", "/usr/bin");

    const bool ok = stdromano::env::prepend("STDROMANO_TEST_PRE", "/opt/bin");
    STDROMANO_CHECK(ok);

    const stdromano::StringD val = stdromano::env::get("STDROMANO_TEST_PRE");

#if defined(STDROMANO_WIN)
    STDROMANO_CHECK(val == stdromano::StringD("/opt/bin;/usr/bin"));
#else
    STDROMANO_CHECK(val == stdromano::StringD("/opt/bin:/usr/bin"));
#endif

    stdromano::env::unset("STDROMANO_TEST_PRE");
}

STDROMANO_TEST_CASE(env_prepend_to_nonexistent)
{
    stdromano::env::unset("STDROMANO_TEST_PRE_NEW");

    const bool ok = stdromano::env::prepend("STDROMANO_TEST_PRE_NEW", "/only");
    STDROMANO_CHECK(ok);

    const stdromano::StringD val = stdromano::env::get("STDROMANO_TEST_PRE_NEW");
    STDROMANO_CHECK(val == stdromano::StringD("/only"));

    stdromano::env::unset("STDROMANO_TEST_PRE_NEW");
}

STDROMANO_TEST_CASE(env_prepend_custom_separator)
{
    stdromano::env::set("STDROMANO_TEST_PRE_SEP", "b");

    stdromano::env::prepend("STDROMANO_TEST_PRE_SEP", "a", '|');

    const stdromano::StringD val = stdromano::env::get("STDROMANO_TEST_PRE_SEP");
    STDROMANO_CHECK(val == stdromano::StringD("a|b"));

    stdromano::env::unset("STDROMANO_TEST_PRE_SEP");
}

STDROMANO_TEST_CASE(env_prepend_multiple)
{
    stdromano::env::set("STDROMANO_TEST_PRE_M", "3");
    stdromano::env::prepend("STDROMANO_TEST_PRE_M", "2", ',');
    stdromano::env::prepend("STDROMANO_TEST_PRE_M", "1", ',');

    const stdromano::StringD val = stdromano::env::get("STDROMANO_TEST_PRE_M");
    STDROMANO_CHECK(val == stdromano::StringD("1,2,3"));

    stdromano::env::unset("STDROMANO_TEST_PRE_M");
}

STDROMANO_TEST_CASE(env_prepend_nullptr)
{
    STDROMANO_CHECK(!stdromano::env::prepend(static_cast<const char*>(nullptr), "val"));
    STDROMANO_CHECK(!stdromano::env::prepend("STDROMANO_TEST_PRE_NP", static_cast<const char*>(nullptr)));
}

STDROMANO_TEST_CASE(env_prepend_stringd_overload)
{
    const stdromano::StringD name("STDROMANO_TEST_PRE_SD");
    const stdromano::StringD v1("second");
    const stdromano::StringD v2("first");

    stdromano::env::set(name, v1);
    stdromano::env::prepend(name, v2, ',');

    const stdromano::StringD val = stdromano::env::get(name);
    STDROMANO_CHECK(val == stdromano::StringD("first,second"));

    stdromano::env::unset(name);
}

STDROMANO_TEST_CASE(env_expand_single_variable)
{
    stdromano::env::set("STDROMANO_TEST_EXP", "expanded_value");

#if defined(STDROMANO_WIN)
    const stdromano::StringD result = stdromano::env::expand("%STDROMANO_TEST_EXP%");
#else
    const stdromano::StringD result = stdromano::env::expand("$STDROMANO_TEST_EXP");
#endif

    STDROMANO_CHECK(result == stdromano::StringD("expanded_value"));

    stdromano::env::unset("STDROMANO_TEST_EXP");
}

STDROMANO_TEST_CASE(env_expand_braced_syntax)
{
    stdromano::env::set("STDROMANO_TEST_BRACE", "braced");

#if defined(STDROMANO_WIN)
    const stdromano::StringD result = stdromano::env::expand("%STDROMANO_TEST_BRACE%");
#else
    const stdromano::StringD result = stdromano::env::expand("${STDROMANO_TEST_BRACE}");
#endif

    STDROMANO_CHECK(result == stdromano::StringD("braced"));

    stdromano::env::unset("STDROMANO_TEST_BRACE");
}

STDROMANO_TEST_CASE(env_expand_embedded_in_text)
{
    stdromano::env::set("STDROMANO_TEST_EMB", "world");

#if defined(STDROMANO_WIN)
    const stdromano::StringD result = stdromano::env::expand("hello %STDROMANO_TEST_EMB% end");
#else
    const stdromano::StringD result = stdromano::env::expand("hello ${STDROMANO_TEST_EMB} end");
#endif

    STDROMANO_CHECK(result == stdromano::StringD("hello world end"));

    stdromano::env::unset("STDROMANO_TEST_EMB");
}

STDROMANO_TEST_CASE(env_expand_multiple_variables)
{
    stdromano::env::set("STDROMANO_TEST_A", "foo");
    stdromano::env::set("STDROMANO_TEST_B", "bar");

#if defined(STDROMANO_WIN)
    const stdromano::StringD result = stdromano::env::expand("%STDROMANO_TEST_A%-%STDROMANO_TEST_B%");
#else
    const stdromano::StringD result = stdromano::env::expand("${STDROMANO_TEST_A}-${STDROMANO_TEST_B}");
#endif

    STDROMANO_CHECK(result == stdromano::StringD("foo-bar"));

    stdromano::env::unset("STDROMANO_TEST_A");
    stdromano::env::unset("STDROMANO_TEST_B");
}

STDROMANO_TEST_CASE(env_expand_no_variables)
{
    const stdromano::StringD result = stdromano::env::expand("plain text no vars");
    STDROMANO_CHECK(result == stdromano::StringD("plain text no vars"));
}

STDROMANO_TEST_CASE(env_expand_nullptr)
{
    const stdromano::StringD result = stdromano::env::expand(static_cast<const char*>(nullptr));
    STDROMANO_CHECK(result.empty());
}

STDROMANO_TEST_CASE(env_expand_stringd_overload)
{
    stdromano::env::set("STDROMANO_TEST_EXP_SD", "ok");

#if defined(STDROMANO_WIN)
    const stdromano::StringD input("%STDROMANO_TEST_EXP_SD%");
#else
    const stdromano::StringD input("$STDROMANO_TEST_EXP_SD");
#endif

    const stdromano::StringD result = stdromano::env::expand(input);
    STDROMANO_CHECK(result == stdromano::StringD("ok"));

    stdromano::env::unset("STDROMANO_TEST_EXP_SD");
}

STDROMANO_TEST_CASE(env_expand_undefined_variable)
{
    stdromano::env::unset("STDROMANO_TEST_UNDEF_VAR");

#if defined(STDROMANO_WIN)
    const stdromano::StringD result = stdromano::env::expand("%STDROMANO_TEST_UNDEF_VAR%");
    STDROMANO_CHECK(result == stdromano::StringD("%STDROMANO_TEST_UNDEF_VAR%"));
#else
    const stdromano::StringD result = stdromano::env::expand("${STDROMANO_TEST_UNDEF_VAR}");
    STDROMANO_CHECK(result == stdromano::StringD("${STDROMANO_TEST_UNDEF_VAR}"));
#endif
}

STDROMANO_TEST_CASE(env_append_prepend_ordering)
{
    stdromano::env::set("STDROMANO_TEST_ORD", "middle");
    stdromano::env::prepend("STDROMANO_TEST_ORD", "first", ',');
    stdromano::env::append("STDROMANO_TEST_ORD", "last", ',');

    const stdromano::StringD val = stdromano::env::get("STDROMANO_TEST_ORD");
    STDROMANO_CHECK(val == stdromano::StringD("first,middle,last"));

    stdromano::env::unset("STDROMANO_TEST_ORD");
}

STDROMANO_TEST_CASE(env_lifecycle)
{
    const char* name = "STDROMANO_TEST_LIFE";

    STDROMANO_CHECK(!stdromano::env::has(name));

    stdromano::env::set(name, "alive");
    STDROMANO_CHECK(stdromano::env::has(name));
    STDROMANO_CHECK(stdromano::env::get(name) == stdromano::StringD("alive"));

    stdromano::env::set(name, "updated");
    STDROMANO_CHECK(stdromano::env::get(name) == stdromano::StringD("updated"));

    stdromano::env::unset(name);
    STDROMANO_CHECK(!stdromano::env::has(name));
    STDROMANO_CHECK(stdromano::env::get(name).empty());
}

STDROMANO_TEST_CASE(env_long_value)
{
    stdromano::StringD long_val;

    for(int i = 0; i < 1000; i++)
        long_val.push_back('A');

    STDROMANO_CHECK(stdromano::env::set("STDROMANO_TEST_LONG", long_val.c_str()));

    const stdromano::StringD got = stdromano::env::get("STDROMANO_TEST_LONG");
    STDROMANO_CHECK(got.size() == 1000);
    STDROMANO_CHECK(got == long_val);

    stdromano::env::unset("STDROMANO_TEST_LONG");
}

STDROMANO_TEST_CASE(env_special_characters)
{
    const char* val = "hello=world&foo bar!@#";

    STDROMANO_CHECK(stdromano::env::set("STDROMANO_TEST_SPEC", val));

    const stdromano::StringD got = stdromano::env::get("STDROMANO_TEST_SPEC");
    STDROMANO_CHECK(got == stdromano::StringD(val));

    stdromano::env::unset("STDROMANO_TEST_SPEC");
}

STDROMANO_TEST_CASE(fuzz_set_get_round_trips)
{
    const auto report = stdromano::fuzz::run_property(fixtures::options("env_round_trip", 300), [](stdromano::fuzz::Source& source) {
        const char* name = "STDROMANO_TEST_FUZZ_ENV";

        const stdromano::StringD value = source.string(200, "abcXYZ0123456789_-./:= ");

        STDROMANO_FUZZ_CHECK(stdromano::env::set(name, value.c_str()));
        STDROMANO_FUZZ_CHECK(stdromano::env::has(name) || value.empty());

        const stdromano::StringD got = stdromano::env::get(name);
        STDROMANO_FUZZ_CHECK(got == value);

        stdromano::env::unset(name);
        STDROMANO_FUZZ_CHECK(!stdromano::env::has(name));

        return true;
    });

    TESTS_REQUIRE_PROPERTY(report);
}

STDROMANO_TEST_CASE(fuzz_append_and_prepend_build_a_list)
{
    const auto report = stdromano::fuzz::run_property(fixtures::options("env_list", 200), [](stdromano::fuzz::Source& source) {
        const char* name = "STDROMANO_TEST_FUZZ_LIST";
        const char separator = source.pick({',', ';', ':'});

        stdromano::env::unset(name);

        std::deque<std::string> expected;

        const std::size_t count = source.range<std::size_t>(1, 12);

        for(std::size_t i = 0; i < count; ++i)
        {
            const std::string item = "item" + std::to_string(i);

            if(source.boolean())
            {
                STDROMANO_FUZZ_CHECK(stdromano::env::append(name, item.c_str(), separator));
                expected.push_back(item);
            }
            else
            {
                STDROMANO_FUZZ_CHECK(stdromano::env::prepend(name, item.c_str(), separator));
                expected.push_front(item);
            }
        }

        std::string joined;

        for(const std::string& item : expected)
        {
            if(!joined.empty())
                joined.push_back(separator);

            joined += item;
        }

        const stdromano::StringD got = stdromano::env::get(name);
        stdromano::env::unset(name);

        STDROMANO_FUZZ_CHECK_EQ(std::string(got.c_str(), got.size()), joined);

        return true;
    });

    TESTS_REQUIRE_PROPERTY(report);
}

STDROMANO_TEST_MAIN()
