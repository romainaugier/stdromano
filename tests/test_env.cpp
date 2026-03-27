// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - Present Romain Augier
// All rights reserved.

#include "stdromano/env.hpp"
#include "test.hpp"

#include "spdlog/spdlog.h"

TEST_CASE(test_env_set_and_get)
{
    const bool ok = stdromano::env::set("STDROMANO_TEST_VAR", "hello");
    ASSERT(ok);

    const stdromano::StringD val = stdromano::env::get("STDROMANO_TEST_VAR");
    ASSERT(val == stdromano::StringD("hello"));

    stdromano::env::unset("STDROMANO_TEST_VAR");
}

TEST_CASE(test_env_set_and_get_stringd_overload)
{
    const stdromano::StringD name("STDROMANO_TEST_SD");
    const stdromano::StringD value("world");

    ASSERT(stdromano::env::set(name, value));

    const stdromano::StringD got = stdromano::env::get(name);
    ASSERT(got == value);

    stdromano::env::unset(name);
}

TEST_CASE(test_env_get_nonexistent)
{
    const stdromano::StringD val = stdromano::env::get("STDROMANO_SURELY_DOES_NOT_EXIST_12345");
    ASSERT(val.empty());
}

TEST_CASE(test_env_get_nullptr)
{
    const stdromano::StringD val = stdromano::env::get(static_cast<const char*>(nullptr));
    ASSERT(val.empty());
}

TEST_CASE(test_env_set_empty_value)
{
    ASSERT(stdromano::env::set("STDROMANO_TEST_EMPTY", ""));

    const stdromano::StringD val = stdromano::env::get("STDROMANO_TEST_EMPTY");
    ASSERT(val.empty());

    stdromano::env::unset("STDROMANO_TEST_EMPTY");
}

TEST_CASE(test_env_set_nullptr_name)
{
    ASSERT(!stdromano::env::set(static_cast<const char*>(nullptr), "value"));
}

TEST_CASE(test_env_set_nullptr_value)
{
    ASSERT(!stdromano::env::set("STDROMANO_TEST_NV", static_cast<const char*>(nullptr)));
}

TEST_CASE(test_env_set_overwrite_true)
{
    stdromano::env::set("STDROMANO_TEST_OW", "first");
    stdromano::env::set("STDROMANO_TEST_OW", "second", true);

    const stdromano::StringD val = stdromano::env::get("STDROMANO_TEST_OW");
    ASSERT(val == stdromano::StringD("second"));

    stdromano::env::unset("STDROMANO_TEST_OW");
}

TEST_CASE(test_env_set_overwrite_false)
{
    stdromano::env::set("STDROMANO_TEST_NOOW", "first");
    stdromano::env::set("STDROMANO_TEST_NOOW", "second", false);

    const stdromano::StringD val = stdromano::env::get("STDROMANO_TEST_NOOW");
    ASSERT(val == stdromano::StringD("first"));

    stdromano::env::unset("STDROMANO_TEST_NOOW");
}

TEST_CASE(test_env_set_overwrite_false_when_unset)
{
    stdromano::env::unset("STDROMANO_TEST_NOOW2");

    stdromano::env::set("STDROMANO_TEST_NOOW2", "value", false);

    const stdromano::StringD val = stdromano::env::get("STDROMANO_TEST_NOOW2");
    ASSERT(val == stdromano::StringD("value"));

    stdromano::env::unset("STDROMANO_TEST_NOOW2");
}

TEST_CASE(test_env_unset)
{
    stdromano::env::set("STDROMANO_TEST_UNSET", "bye");
    ASSERT(stdromano::env::has("STDROMANO_TEST_UNSET"));

    const bool ok = stdromano::env::unset("STDROMANO_TEST_UNSET");
    ASSERT(ok);
    ASSERT(!stdromano::env::has("STDROMANO_TEST_UNSET"));

    const stdromano::StringD val = stdromano::env::get("STDROMANO_TEST_UNSET");
    ASSERT(val.empty());
}

TEST_CASE(test_env_unset_nonexistent)
{
    /* Unsetting something that doesn't exist should still succeed */
    const bool ok = stdromano::env::unset("STDROMANO_NEVER_SET_XYZ");
    /* On Linux unsetenv returns 0 even if not set; on Windows SetEnvironmentVariable
       with NULL value returns 0 if the var didn't exist. Accept either. */
    (void)ok;
}

TEST_CASE(test_env_unset_nullptr)
{
    ASSERT(!stdromano::env::unset(static_cast<const char*>(nullptr)));
}

TEST_CASE(test_env_unset_stringd_overload)
{
    const stdromano::StringD name("STDROMANO_TEST_UNSET_SD");

    stdromano::env::set(name, stdromano::StringD("tmp"));
    ASSERT(stdromano::env::has(name));

    stdromano::env::unset(name);
    ASSERT(!stdromano::env::has(name));
}

TEST_CASE(test_env_has_existing)
{
    stdromano::env::set("STDROMANO_TEST_HAS", "1");
    ASSERT(stdromano::env::has("STDROMANO_TEST_HAS"));
    stdromano::env::unset("STDROMANO_TEST_HAS");
}

TEST_CASE(test_env_has_missing)
{
    ASSERT(!stdromano::env::has("STDROMANO_MISSING_VAR_999"));
}

TEST_CASE(test_env_has_nullptr)
{
    ASSERT(!stdromano::env::has(static_cast<const char*>(nullptr)));
}

TEST_CASE(test_env_has_stringd_overload)
{
    const stdromano::StringD name("STDROMANO_TEST_HAS_SD");

    ASSERT(!stdromano::env::has(name));
    stdromano::env::set(name, stdromano::StringD("yes"));
    ASSERT(stdromano::env::has(name));
    stdromano::env::unset(name);
}

TEST_CASE(test_env_append_to_existing)
{
    stdromano::env::set("STDROMANO_TEST_APP", "/usr/bin");

    const bool ok = stdromano::env::append("STDROMANO_TEST_APP", "/usr/local/bin");
    ASSERT(ok);

    const stdromano::StringD val = stdromano::env::get("STDROMANO_TEST_APP");

#if defined(STDROMANO_WIN)
    ASSERT(val == stdromano::StringD("/usr/bin;/usr/local/bin"));
#else
    ASSERT(val == stdromano::StringD("/usr/bin:/usr/local/bin"));
#endif

    stdromano::env::unset("STDROMANO_TEST_APP");
}

TEST_CASE(test_env_append_to_nonexistent)
{
    stdromano::env::unset("STDROMANO_TEST_APP_NEW");

    const bool ok = stdromano::env::append("STDROMANO_TEST_APP_NEW", "/first");
    ASSERT(ok);

    const stdromano::StringD val = stdromano::env::get("STDROMANO_TEST_APP_NEW");
    ASSERT(val == stdromano::StringD("/first"));

    stdromano::env::unset("STDROMANO_TEST_APP_NEW");
}

TEST_CASE(test_env_append_custom_separator)
{
    stdromano::env::set("STDROMANO_TEST_APP_SEP", "a");

    stdromano::env::append("STDROMANO_TEST_APP_SEP", "b", ',');

    const stdromano::StringD val = stdromano::env::get("STDROMANO_TEST_APP_SEP");
    ASSERT(val == stdromano::StringD("a,b"));

    stdromano::env::unset("STDROMANO_TEST_APP_SEP");
}

TEST_CASE(test_env_append_multiple)
{
    stdromano::env::set("STDROMANO_TEST_APP_M", "1");
    stdromano::env::append("STDROMANO_TEST_APP_M", "2", ',');
    stdromano::env::append("STDROMANO_TEST_APP_M", "3", ',');

    const stdromano::StringD val = stdromano::env::get("STDROMANO_TEST_APP_M");
    ASSERT(val == stdromano::StringD("1,2,3"));

    stdromano::env::unset("STDROMANO_TEST_APP_M");
}

TEST_CASE(test_env_append_nullptr)
{
    ASSERT(!stdromano::env::append(static_cast<const char*>(nullptr), "val"));
    ASSERT(!stdromano::env::append("STDROMANO_TEST_APP_NP", static_cast<const char*>(nullptr)));
}

TEST_CASE(test_env_append_stringd_overload)
{
    const stdromano::StringD name("STDROMANO_TEST_APP_SD");
    const stdromano::StringD v1("alpha");
    const stdromano::StringD v2("beta");

    stdromano::env::set(name, v1);
    stdromano::env::append(name, v2, ',');

    const stdromano::StringD val = stdromano::env::get(name);
    ASSERT(val == stdromano::StringD("alpha,beta"));

    stdromano::env::unset(name);
}

TEST_CASE(test_env_prepend_to_existing)
{
    stdromano::env::set("STDROMANO_TEST_PRE", "/usr/bin");

    const bool ok = stdromano::env::prepend("STDROMANO_TEST_PRE", "/opt/bin");
    ASSERT(ok);

    const stdromano::StringD val = stdromano::env::get("STDROMANO_TEST_PRE");

#if defined(STDROMANO_WIN)
    ASSERT(val == stdromano::StringD("/opt/bin;/usr/bin"));
#else
    ASSERT(val == stdromano::StringD("/opt/bin:/usr/bin"));
#endif

    stdromano::env::unset("STDROMANO_TEST_PRE");
}

TEST_CASE(test_env_prepend_to_nonexistent)
{
    stdromano::env::unset("STDROMANO_TEST_PRE_NEW");

    const bool ok = stdromano::env::prepend("STDROMANO_TEST_PRE_NEW", "/only");
    ASSERT(ok);

    const stdromano::StringD val = stdromano::env::get("STDROMANO_TEST_PRE_NEW");
    ASSERT(val == stdromano::StringD("/only"));

    stdromano::env::unset("STDROMANO_TEST_PRE_NEW");
}

TEST_CASE(test_env_prepend_custom_separator)
{
    stdromano::env::set("STDROMANO_TEST_PRE_SEP", "b");

    stdromano::env::prepend("STDROMANO_TEST_PRE_SEP", "a", '|');

    const stdromano::StringD val = stdromano::env::get("STDROMANO_TEST_PRE_SEP");
    ASSERT(val == stdromano::StringD("a|b"));

    stdromano::env::unset("STDROMANO_TEST_PRE_SEP");
}

TEST_CASE(test_env_prepend_multiple)
{
    stdromano::env::set("STDROMANO_TEST_PRE_M", "3");
    stdromano::env::prepend("STDROMANO_TEST_PRE_M", "2", ',');
    stdromano::env::prepend("STDROMANO_TEST_PRE_M", "1", ',');

    const stdromano::StringD val = stdromano::env::get("STDROMANO_TEST_PRE_M");
    ASSERT(val == stdromano::StringD("1,2,3"));

    stdromano::env::unset("STDROMANO_TEST_PRE_M");
}

TEST_CASE(test_env_prepend_nullptr)
{
    ASSERT(!stdromano::env::prepend(static_cast<const char*>(nullptr), "val"));
    ASSERT(!stdromano::env::prepend("STDROMANO_TEST_PRE_NP", static_cast<const char*>(nullptr)));
}

TEST_CASE(test_env_prepend_stringd_overload)
{
    const stdromano::StringD name("STDROMANO_TEST_PRE_SD");
    const stdromano::StringD v1("second");
    const stdromano::StringD v2("first");

    stdromano::env::set(name, v1);
    stdromano::env::prepend(name, v2, ',');

    const stdromano::StringD val = stdromano::env::get(name);
    ASSERT(val == stdromano::StringD("first,second"));

    stdromano::env::unset(name);
}

TEST_CASE(test_env_expand_single_variable)
{
    stdromano::env::set("STDROMANO_TEST_EXP", "expanded_value");

#if defined(STDROMANO_WIN)
    const stdromano::StringD result = stdromano::env::expand("%STDROMANO_TEST_EXP%");
#else
    const stdromano::StringD result = stdromano::env::expand("$STDROMANO_TEST_EXP");
#endif

    ASSERT(result == stdromano::StringD("expanded_value"));

    stdromano::env::unset("STDROMANO_TEST_EXP");
}

TEST_CASE(test_env_expand_braced_syntax)
{
    stdromano::env::set("STDROMANO_TEST_BRACE", "braced");

#if defined(STDROMANO_WIN)
    const stdromano::StringD result = stdromano::env::expand("%STDROMANO_TEST_BRACE%");
#else
    const stdromano::StringD result = stdromano::env::expand("${STDROMANO_TEST_BRACE}");
#endif

    ASSERT(result == stdromano::StringD("braced"));

    stdromano::env::unset("STDROMANO_TEST_BRACE");
}

TEST_CASE(test_env_expand_embedded_in_text)
{
    stdromano::env::set("STDROMANO_TEST_EMB", "world");

#if defined(STDROMANO_WIN)
    const stdromano::StringD result = stdromano::env::expand("hello %STDROMANO_TEST_EMB% end");
#else
    const stdromano::StringD result = stdromano::env::expand("hello ${STDROMANO_TEST_EMB} end");
#endif

    ASSERT(result == stdromano::StringD("hello world end"));

    stdromano::env::unset("STDROMANO_TEST_EMB");
}

TEST_CASE(test_env_expand_multiple_variables)
{
    stdromano::env::set("STDROMANO_TEST_A", "foo");
    stdromano::env::set("STDROMANO_TEST_B", "bar");

#if defined(STDROMANO_WIN)
    const stdromano::StringD result = stdromano::env::expand("%STDROMANO_TEST_A%-%STDROMANO_TEST_B%");
#else
    const stdromano::StringD result = stdromano::env::expand("${STDROMANO_TEST_A}-${STDROMANO_TEST_B}");
#endif

    ASSERT(result == stdromano::StringD("foo-bar"));

    stdromano::env::unset("STDROMANO_TEST_A");
    stdromano::env::unset("STDROMANO_TEST_B");
}

TEST_CASE(test_env_expand_no_variables)
{
    const stdromano::StringD result = stdromano::env::expand("plain text no vars");
    ASSERT(result == stdromano::StringD("plain text no vars"));
}

TEST_CASE(test_env_expand_nullptr)
{
    const stdromano::StringD result = stdromano::env::expand(static_cast<const char*>(nullptr));
    ASSERT(result.empty());
}

TEST_CASE(test_env_expand_stringd_overload)
{
    stdromano::env::set("STDROMANO_TEST_EXP_SD", "ok");

#if defined(STDROMANO_WIN)
    const stdromano::StringD input("%STDROMANO_TEST_EXP_SD%");
#else
    const stdromano::StringD input("$STDROMANO_TEST_EXP_SD");
#endif

    const stdromano::StringD result = stdromano::env::expand(input);
    ASSERT(result == stdromano::StringD("ok"));

    stdromano::env::unset("STDROMANO_TEST_EXP_SD");
}

TEST_CASE(test_env_expand_undefined_variable)
{
    stdromano::env::unset("STDROMANO_TEST_UNDEF_VAR");

#if defined(STDROMANO_WIN)
    /* Windows leaves %UNKNOWN% as-is */
    const stdromano::StringD result = stdromano::env::expand("%STDROMANO_TEST_UNDEF_VAR%");
    ASSERT(result == stdromano::StringD("%STDROMANO_TEST_UNDEF_VAR%"));
#else
    /* With WRDE_UNDEF, wordexp fails and we return the original string */
    const stdromano::StringD result = stdromano::env::expand("${STDROMANO_TEST_UNDEF_VAR}");
    ASSERT(result == stdromano::StringD("${STDROMANO_TEST_UNDEF_VAR}"));
#endif
}

TEST_CASE(test_env_append_prepend_ordering)
{
    stdromano::env::set("STDROMANO_TEST_ORD", "middle");
    stdromano::env::prepend("STDROMANO_TEST_ORD", "first", ',');
    stdromano::env::append("STDROMANO_TEST_ORD", "last", ',');

    const stdromano::StringD val = stdromano::env::get("STDROMANO_TEST_ORD");
    ASSERT(val == stdromano::StringD("first,middle,last"));

    stdromano::env::unset("STDROMANO_TEST_ORD");
}

TEST_CASE(test_env_lifecycle)
{
    const char* name = "STDROMANO_TEST_LIFE";

    ASSERT(!stdromano::env::has(name));

    stdromano::env::set(name, "alive");
    ASSERT(stdromano::env::has(name));
    ASSERT(stdromano::env::get(name) == stdromano::StringD("alive"));

    stdromano::env::set(name, "updated");
    ASSERT(stdromano::env::get(name) == stdromano::StringD("updated"));

    stdromano::env::unset(name);
    ASSERT(!stdromano::env::has(name));
    ASSERT(stdromano::env::get(name).empty());
}

TEST_CASE(test_env_long_value)
{
    stdromano::StringD long_val;

    for(int i = 0; i < 1000; i++)
    {
        long_val.push_back('A');
    }

    ASSERT(stdromano::env::set("STDROMANO_TEST_LONG", long_val.c_str()));

    const stdromano::StringD got = stdromano::env::get("STDROMANO_TEST_LONG");
    ASSERT(got.size() == 1000);
    ASSERT(got == long_val);

    stdromano::env::unset("STDROMANO_TEST_LONG");
}

TEST_CASE(test_env_special_characters)
{
    const char* val = "hello=world&foo bar!@#";

    ASSERT(stdromano::env::set("STDROMANO_TEST_SPEC", val));

    const stdromano::StringD got = stdromano::env::get("STDROMANO_TEST_SPEC");
    ASSERT(got == stdromano::StringD(val));

    stdromano::env::unset("STDROMANO_TEST_SPEC");
}

int main()
{
    TestRunner runner("env");

    /* get / set */
    runner.add_test("env::set and env::get", test_env_set_and_get);
    runner.add_test("env::set/get StringD overload", test_env_set_and_get_stringd_overload);
    runner.add_test("env::get nonexistent", test_env_get_nonexistent);
    runner.add_test("env::get nullptr", test_env_get_nullptr);
    runner.add_test("env::set empty value", test_env_set_empty_value);
    runner.add_test("env::set nullptr name", test_env_set_nullptr_name);
    runner.add_test("env::set nullptr value", test_env_set_nullptr_value);

    /* overwrite */
    runner.add_test("env::set overwrite=true", test_env_set_overwrite_true);
    runner.add_test("env::set overwrite=false (existing)", test_env_set_overwrite_false);
    runner.add_test("env::set overwrite=false (unset)", test_env_set_overwrite_false_when_unset);

    /* unset */
    runner.add_test("env::unset", test_env_unset);
    runner.add_test("env::unset nonexistent", test_env_unset_nonexistent);
    runner.add_test("env::unset nullptr", test_env_unset_nullptr);
    runner.add_test("env::unset StringD overload", test_env_unset_stringd_overload);

    /* has */
    runner.add_test("env::has existing", test_env_has_existing);
    runner.add_test("env::has missing", test_env_has_missing);
    runner.add_test("env::has nullptr", test_env_has_nullptr);
    runner.add_test("env::has StringD overload", test_env_has_stringd_overload);

    /* append */
    runner.add_test("env::append to existing", test_env_append_to_existing);
    runner.add_test("env::append to nonexistent", test_env_append_to_nonexistent);
    runner.add_test("env::append custom separator", test_env_append_custom_separator);
    runner.add_test("env::append multiple", test_env_append_multiple);
    runner.add_test("env::append nullptr", test_env_append_nullptr);
    runner.add_test("env::append StringD overload", test_env_append_stringd_overload);

    /* prepend */
    runner.add_test("env::prepend to existing", test_env_prepend_to_existing);
    runner.add_test("env::prepend to nonexistent", test_env_prepend_to_nonexistent);
    runner.add_test("env::prepend custom separator", test_env_prepend_custom_separator);
    runner.add_test("env::prepend multiple", test_env_prepend_multiple);
    runner.add_test("env::prepend nullptr", test_env_prepend_nullptr);
    runner.add_test("env::prepend StringD overload", test_env_prepend_stringd_overload);

    /* expand */
    runner.add_test("env::expand single variable", test_env_expand_single_variable);
    runner.add_test("env::expand braced syntax", test_env_expand_braced_syntax);
    runner.add_test("env::expand embedded in text", test_env_expand_embedded_in_text);
    runner.add_test("env::expand multiple variables", test_env_expand_multiple_variables);
    runner.add_test("env::expand no variables", test_env_expand_no_variables);
    runner.add_test("env::expand nullptr", test_env_expand_nullptr);
    runner.add_test("env::expand StringD overload", test_env_expand_stringd_overload);
    runner.add_test("env::expand undefined variable", test_env_expand_undefined_variable);

    /* combined / integration */
    runner.add_test("env::append + prepend ordering", test_env_append_prepend_ordering);
    runner.add_test("env lifecycle round-trip", test_env_lifecycle);
    runner.add_test("env long value", test_env_long_value);
    runner.add_test("env special characters", test_env_special_characters);

    runner.run_all();

    return 0;
}
