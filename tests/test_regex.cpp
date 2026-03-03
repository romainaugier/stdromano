// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - Present Romain Augier
// All rights reserved.

#include "stdromano/regex.hpp"
#include "stdromano/logger.hpp"

#include <cassert>

/* Helper: assert a match succeeded and the full-match string equals expected */
#define ASSERT_MATCH(re, input, expected_str)                                  \
    do {                                                                       \
        auto _m = (re).match(stdromano::StringD(input));                       \
        assert(_m.matched());                                                  \
        assert(_m.str() == stdromano::StringD(expected_str));                   \
    } while(0)

#define ASSERT_NO_MATCH(re, input)                                             \
    do {                                                                       \
        auto _m = (re).match(stdromano::StringD(input));                       \
        assert(!_m.matched());                                                 \
    } while(0)

#define ASSERT_SEARCH(re, input, expected_str)                                 \
    do {                                                                       \
        auto _m = (re).search(stdromano::StringD(input));                      \
        assert(_m.matched());                                                  \
        assert(_m.str() == stdromano::StringD(expected_str));                   \
    } while(0)

#define ASSERT_NO_SEARCH(re, input)                                            \
    do {                                                                       \
        auto _m = (re).search(stdromano::StringD(input));                      \
        assert(!_m.matched());                                                 \
    } while(0)

#define ASSERT_GROUP(match_obj, group_idx, expected_str)                        \
    do {                                                                       \
        assert((match_obj).group(group_idx).matched());                          \
        assert((match_obj).group_str(group_idx) ==                             \
               stdromano::StringD(expected_str));                               \
    } while(0)

int main(int argc, char** argv)
{
    stdromano::set_log_level(stdromano::LogLevel::Debug);
    stdromano::log_info("Starting Regex tests");

    /* ================================================================== */
    /* 1. Basic quantifiers (ported from original tests)                  */
    /* ================================================================== */
    stdromano::log_info("--- Basic quantifiers ---");
    {
        stdromano::Regex re("[0-9]*", stdromano::RegexFlags_DebugCompilation);
        assert(re.valid());
        assert(re.match("123456789").matched());
        assert(re.match("12345abcde").matched());  /* greedy: matches leading digits */
        assert(re.match("abcde12345").matched());  /* * allows zero digits */
    }

    {
        stdromano::Regex re("[0-9]+", stdromano::RegexFlags_DebugCompilation);
        assert(re.valid());
        assert(re.match("123456789").matched());
        assert(re.match("12345abcde").matched());
        assert(re.match("1abcde").matched());
        assert(re.match("12abcde").matched());
        ASSERT_NO_MATCH(re, "abcde12345");         /* no leading digit */
    }

    /* ================================================================== */
    /* 2. Alternation                                                     */
    /* ================================================================== */
    stdromano::log_info("--- Alternation ---");
    {
        stdromano::Regex re("a*b|cd", stdromano::RegexFlags_DebugCompilation);
        assert(re.match("aaaaaacd").matched());
        assert(re.match("abd").matched());
        assert(re.match("bd").matched());
        assert(re.match("cd").matched());
        ASSERT_NO_MATCH(re, "aaaacacd");
    }

    {
        stdromano::Regex re("cat|dog|bird");
        ASSERT_MATCH(re, "cat", "cat");
        ASSERT_MATCH(re, "dog", "dog");
        ASSERT_MATCH(re, "bird", "bird");
        ASSERT_NO_MATCH(re, "fish");
    }

    /* ================================================================== */
    /* 3. Optional quantifier (?)                                         */
    /* ================================================================== */
    stdromano::log_info("--- Optional quantifier ---");
    {
        stdromano::Regex re("a?([b-e])+");
        assert(re.match("abcdebcde").matched());
        assert(re.match("bcdebcde").matched());
        ASSERT_NO_MATCH(re, "rbcdebcde");
    }

    {
        stdromano::Regex re("colou?r");
        ASSERT_MATCH(re, "color", "color");
        ASSERT_MATCH(re, "colour", "colour");
        ASSERT_NO_MATCH(re, "colouur");
    }

    /* ================================================================== */
    /* 4. Dot (any character)                                             */
    /* ================================================================== */
    stdromano::log_info("--- Dot (any char) ---");
    {
        stdromano::Regex re("a.b", stdromano::RegexFlags_DebugCompilation);
        ASSERT_MATCH(re, "axb", "axb");
        ASSERT_MATCH(re, "a1b", "a1b");
        ASSERT_MATCH(re, "a_b", "a_b");
        ASSERT_NO_MATCH(re, "ab");
    }

    {
        stdromano::Regex re("...");
        ASSERT_MATCH(re, "abc", "abc");
        ASSERT_MATCH(re, "123", "123");
        ASSERT_NO_MATCH(re, "ab");
    }

    /* ================================================================== */
    /* 5. Escape sequences                                                */
    /* ================================================================== */
    stdromano::log_info("--- Escape sequences ---");
    {
        stdromano::Regex re("\\d+");
        ASSERT_MATCH(re, "42", "42");
        ASSERT_MATCH(re, "0", "0");
        ASSERT_NO_MATCH(re, "abc");
    }

    {
        stdromano::Regex re("\\w+");
        ASSERT_MATCH(re, "hello_world", "hello_world");
        ASSERT_MATCH(re, "test123", "test123");
        ASSERT_NO_MATCH(re, " ");
    }

    {
        stdromano::Regex re("a\\s+b");
        ASSERT_MATCH(re, "a   b", "a   b");
        ASSERT_MATCH(re, "a b", "a b");
        ASSERT_NO_MATCH(re, "ab");
    }

    {
        /* Escaped special characters */
        stdromano::Regex re("a\\.b");
        ASSERT_MATCH(re, "a.b", "a.b");
        ASSERT_NO_MATCH(re, "axb");
    }

    {
        stdromano::Regex re("a\\*b");
        ASSERT_MATCH(re, "a*b", "a*b");
        ASSERT_NO_MATCH(re, "aab");
    }

    /* ================================================================== */
    /* 6. Character classes                                               */
    /* ================================================================== */
    stdromano::log_info("--- Character classes ---");
    {
        /* Multi-range class */
        stdromano::Regex re("[a-zA-Z]+", stdromano::RegexFlags_DebugCompilation);
        ASSERT_MATCH(re, "Hello", "Hello");
        ASSERT_MATCH(re, "WORLD", "WORLD");
        ASSERT_NO_MATCH(re, "123");
    }

    {
        /* Mixed ranges and literals */
        stdromano::Regex re("[a-z0-9]+");
        ASSERT_MATCH(re, "abc123", "abc123");
        ASSERT_NO_MATCH(re, "ABC");
    }

    {
        /* Single character in class */
        stdromano::Regex re("[x]");
        ASSERT_MATCH(re, "x", "x");
        ASSERT_NO_MATCH(re, "y");
    }

    /* ================================================================== */
    /* 7. Negated character classes                                        */
    /* ================================================================== */
    stdromano::log_info("--- Negated character classes ---");
    {
        stdromano::Regex re("[^0-9]+", stdromano::RegexFlags_DebugCompilation);
        ASSERT_MATCH(re, "hello", "hello");
        ASSERT_NO_MATCH(re, "123");
    }

    {
        stdromano::Regex re("[^a-z]+");
        ASSERT_MATCH(re, "123", "123");
        ASSERT_MATCH(re, "ABC", "ABC");
        ASSERT_NO_MATCH(re, "hello");
    }

    /* ================================================================== */
    /* 8. Groups and capture                                              */
    /* ================================================================== */
    stdromano::log_info("--- Groups and capture ---");
    {
        stdromano::Regex re("(\\w+)@(\\w+)");
        auto m = re.match(stdromano::StringD("user@host"));
        assert(m.matched());
        ASSERT_GROUP(m, 0, "user@host");    /* full match */
        ASSERT_GROUP(m, 1, "user");         /* group 1 */
        ASSERT_GROUP(m, 2, "host");         /* group 2 */
    }

    {
        stdromano::Regex re("([a-z]+)([0-9]+)");
        auto m = re.match(stdromano::StringD("abc123"));
        assert(m.matched());
        ASSERT_GROUP(m, 0, "abc123");
        ASSERT_GROUP(m, 1, "abc");
        ASSERT_GROUP(m, 2, "123");
    }

    {
        // TODO: add backtracking to support greedy patterns
        /* Nested groups */
#if 0
        stdromano::Regex re("((\\w+)_(\\w+))", stdromano::RegexFlags_DebugCompilation);
        auto m = re.match(stdromano::StringD("hello_world"));
        assert(m.matched());
        ASSERT_GROUP(m, 0, "hello_world");   /* full match */
        ASSERT_GROUP(m, 1, "hello_world");   /* outer group */
        ASSERT_GROUP(m, 2, "hello");         /* first inner */
        ASSERT_GROUP(m, 3, "world");         /* second inner */
#endif // 0
    }

    {
        /* Group with alternation */
        stdromano::Regex re("(cat|dog)s");
        auto m = re.match(stdromano::StringD("cats"));
        assert(m.matched());
        ASSERT_GROUP(m, 1, "cat");

        auto m2 = re.match(stdromano::StringD("dogs"));
        assert(m2.matched());
        ASSERT_GROUP(m2, 1, "dog");

        ASSERT_NO_MATCH(re, "birds");
    }

    {
        // TODO: review the regex vm to exit when appropriate
#if 0
        /* Group with quantifier */
        stdromano::Regex re("(ab)+", stdromano::RegexFlags_DebugCompilation);
        auto m = re.match(stdromano::StringD("ababab"));
        assert(m.matched());
        ASSERT_GROUP(m, 0, "ababab");
        /* Group 1 captures the last iteration */
        assert(m.group(1).matched());
#endif // 0
    }

    /* ================================================================== */
    /* 9. RegexMatch API                                                  */
    /* ================================================================== */
    stdromano::log_info("--- RegexMatch API ---");
    {
        stdromano::Regex re("(\\w+)");
        auto m = re.match(stdromano::StringD("hello"));
        assert(m.matched());
        assert(static_cast<bool>(m));
        assert(m.start() == 0);
        assert(m.end() == 5);
        assert(m.str() == stdromano::StringD("hello"));
        assert(m.group_count() >= 2); /* group 0 + group 1 */
    }

    {
        /* Non-matching RegexMatch */
        stdromano::Regex re("xyz");
        auto m = re.match(stdromano::StringD("abc"));
        assert(!m.matched());
        assert(!static_cast<bool>(m));
    }

    {
        /* Out of range group access returns empty */
        stdromano::Regex re("hello");
        auto m = re.match(stdromano::StringD("hello"));
        assert(m.matched());
        auto g = m.group(99);
        assert(!g.matched());
        assert(m.group_str(99) == stdromano::StringD());
    }

    /* ================================================================== */
    /* 10. search()                                                       */
    /* ================================================================== */
    stdromano::log_info("--- search() ---");
    {
        stdromano::Regex re("\\d+");
        ASSERT_SEARCH(re, "abc123def", "123");
    }

    {
        stdromano::Regex re("(\\w+)@(\\w+)");
        auto m = re.search(stdromano::StringD("contact: user@host please"));
        assert(m.matched());
        ASSERT_GROUP(m, 0, "user@host");
        ASSERT_GROUP(m, 1, "user");
        ASSERT_GROUP(m, 2, "host");
        assert(m.start() == 9);
        assert(m.end() == 18);
    }

    {
        stdromano::Regex re("xyz");
        ASSERT_NO_SEARCH(re, "abcdef");
    }

    {
        /* Search finds the first occurrence */
        stdromano::Regex re("[0-9]+");
        auto m = re.search(stdromano::StringD("aaa111bbb222"));
        assert(m.matched());
        assert(m.str() == stdromano::StringD("111"));
        assert(m.start() == 3);
    }

    /* ================================================================== */
    /* 11. match_iter() and match_all()                                   */
    /* ================================================================== */
    stdromano::log_info("--- match_iter / match_all ---");
    {
        stdromano::Regex re("[0-9]+");
        auto matches = re.match_all(stdromano::StringD("abc123def456ghi789"));

        assert(matches.size() == 3);
        assert(matches[0].str() == stdromano::StringD("123"));
        assert(matches[1].str() == stdromano::StringD("456"));
        assert(matches[2].str() == stdromano::StringD("789"));
    }

    {
        stdromano::Regex re("\\w+");
        auto matches = re.match_all(stdromano::StringD("hello world foo"));

        assert(matches.size() == 3);
        assert(matches[0].str() == stdromano::StringD("hello"));
        assert(matches[1].str() == stdromano::StringD("world"));
        assert(matches[2].str() == stdromano::StringD("foo"));
    }

    {
        /* match_iter with groups */
        stdromano::Regex re("(\\w+)=(\\w+)");
        stdromano::StringD input("key1=val1 key2=val2 key3=val3");

        stdromano::Vector<stdromano::StringD> keys;
        stdromano::Vector<stdromano::StringD> vals;

        re.match_iter(input, [&](const stdromano::RegexMatch& m) {
            keys.push_back(m.group_str(1));
            vals.push_back(m.group_str(2));
        });

        assert(keys.size() == 3);
        assert(keys[0] == stdromano::StringD("key1"));
        assert(keys[1] == stdromano::StringD("key2"));
        assert(keys[2] == stdromano::StringD("key3"));
        assert(vals[0] == stdromano::StringD("val1"));
        assert(vals[1] == stdromano::StringD("val2"));
        assert(vals[2] == stdromano::StringD("val3"));
    }

    {
        /* match_all with no matches */
        stdromano::Regex re("[0-9]+");
        auto matches = re.match_all(stdromano::StringD("no digits here"));
        assert(matches.size() == 0);
    }

    {
        /* match_all positions are correct */
        stdromano::Regex re("[a-z]+");
        auto matches = re.match_all(stdromano::StringD("123abc456def"));

        assert(matches.size() == 2);
        assert(matches[0].start() == 3);
        assert(matches[0].end() == 6);
        assert(matches[1].start() == 9);
        assert(matches[1].end() == 12);
    }

    /* ================================================================== */
    /* 12. match_iter() and match_all()                                   */
    /* ================================================================== */
    stdromano::log_info("--- replace_iter / replace_all ---");

    {
        stdromano::StringD s("${ENV_VAR}");

        stdromano::Regex re("${([A-Z_]+)}");

        assert(re.replace_all(s, "ENV_VAR") == "ENV_VAR");
    }

    /* ================================================================== */
    /* 13. Edge cases                                                     */
    /* ================================================================== */
    stdromano::log_info("--- Edge cases ---");
    {
        /* Empty pattern matches empty string */
        stdromano::Regex re("");
        assert(re.valid());
    }

    {
        /* Single character */
        stdromano::Regex re("a");
        ASSERT_MATCH(re, "a", "a");
        ASSERT_NO_MATCH(re, "b");
    }

    {
        /* Long alternation chain */
        stdromano::Regex re("a|b|c|d|e");
        ASSERT_MATCH(re, "a", "a");
        ASSERT_MATCH(re, "c", "c");
        ASSERT_MATCH(re, "e", "e");
        ASSERT_NO_MATCH(re, "f");
    }

    {
        /* Quantifier on group */
        stdromano::Regex re("(ab)*c", stdromano::RegexFlags_DebugCompilation);
        ASSERT_MATCH(re, "ababc", "ababc");
        ASSERT_MATCH(re, "abc", "abc");
        ASSERT_MATCH(re, "c", "c");
        // TODO
        // ASSERT_MATCH(re, "ac", "c");
    }

    {
        /* Complex pattern: simplified email-like */
        stdromano::Regex re("\\w+\\.\\w+@\\w+\\.\\w+");
        ASSERT_MATCH(re, "john.doe@example.com", "john.doe@example.com");
        ASSERT_NO_MATCH(re, "johndoe@example");
    }

    {
        /* Pattern with multiple groups and quantifiers */
        stdromano::Regex re("([a-z]+)_([0-9]+)\\.(txt|log)");
        auto m = re.match(stdromano::StringD("report_2025.txt"));
        assert(m.matched());
        ASSERT_GROUP(m, 1, "report");
        ASSERT_GROUP(m, 2, "2025");
        ASSERT_GROUP(m, 3, "txt");

        auto m2 = re.match(stdromano::StringD("error_42.log"));
        assert(m2.matched());
        ASSERT_GROUP(m2, 1, "error");
        ASSERT_GROUP(m2, 2, "42");
        ASSERT_GROUP(m2, 3, "log");
    }

    stdromano::log_info("All regex tests passed!");

    return 0;
}
