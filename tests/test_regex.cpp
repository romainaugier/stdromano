// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - Present Romain Augier
// All rights reserved.

#include "stdromano/regex.hpp"
#include "stdromano/hashmap.hpp"

#include "test.hpp"

#include "spdlog/spdlog.h"

/* Helper: assert a match succeeded and the full-match string equals expected */
#define ASSERT_MATCH(re, input, expected_str)                                  \
    do {                                                                       \
        auto _m = (re).match(stdromano::StringD(input));                       \
        ASSERT(_m.matched());                                                  \
        ASSERT(_m.str() == stdromano::StringD(expected_str));                   \
    } while(0)

#define ASSERT_NO_MATCH(re, input)                                             \
    do {                                                                       \
        auto _m = (re).match(stdromano::StringD(input));                       \
        ASSERT(!_m.matched());                                                 \
    } while(0)

#define ASSERT_SEARCH(re, input, expected_str)                                 \
    do {                                                                       \
        auto _m = (re).search(stdromano::StringD(input));                      \
        ASSERT(_m.matched());                                                  \
        ASSERT(_m.str() == stdromano::StringD(expected_str));                   \
    } while(0)

#define ASSERT_NO_SEARCH(re, input)                                            \
    do {                                                                       \
        auto _m = (re).search(stdromano::StringD(input));                      \
        ASSERT(!_m.matched());                                                 \
    } while(0)

#define ASSERT_GROUP(match_obj, group_idx, expected_str)                        \
    do {                                                                       \
        ASSERT((match_obj).group(group_idx).matched());                        \
        ASSERT((match_obj).group_str(group_idx) ==                             \
               stdromano::StringD(expected_str));                               \
    } while(0)

/* ================================================================== */
/* 1. Basic quantifiers                                               */
/* ================================================================== */

TEST_CASE(test_star_quantifier_digits)
{
    stdromano::Regex re("[0-9]*", stdromano::RegexFlags_DebugCompilation);
    ASSERT(re.valid());
    ASSERT(re.match("123456789").matched());
    ASSERT(re.match("12345abcde").matched());   /* greedy: matches leading digits */
    ASSERT(re.match("abcde12345").matched());   /* * allows zero digits */
}

TEST_CASE(test_plus_quantifier_digits)
{
    stdromano::Regex re("[0-9]+", stdromano::RegexFlags_DebugCompilation);
    ASSERT(re.valid());
    ASSERT(re.match("123456789").matched());
    ASSERT(re.match("12345abcde").matched());
    ASSERT(re.match("1abcde").matched());
    ASSERT(re.match("12abcde").matched());
    ASSERT_NO_MATCH(re, "abcde12345");          /* no leading digit */
}

/* ================================================================== */
/* 2. Alternation                                                     */
/* ================================================================== */

TEST_CASE(test_alternation_star_and_literal)
{
    stdromano::Regex re("a*b|cd", stdromano::RegexFlags_DebugCompilation);
    ASSERT(re.match("aaaaaacd").matched());
    ASSERT(re.match("abd").matched());
    ASSERT(re.match("bd").matched());
    ASSERT(re.match("cd").matched());
    ASSERT_NO_MATCH(re, "aaaacacd");
}

TEST_CASE(test_alternation_words)
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

TEST_CASE(test_optional_with_group)
{
    stdromano::Regex re("a?([b-e])+");
    ASSERT(re.match("abcdebcde").matched());
    ASSERT(re.match("bcdebcde").matched());
    ASSERT_NO_MATCH(re, "rbcdebcde");
}

TEST_CASE(test_optional_colour)
{
    stdromano::Regex re("colou?r");
    ASSERT_MATCH(re, "color", "color");
    ASSERT_MATCH(re, "colour", "colour");
    ASSERT_NO_MATCH(re, "colouur");
}

/* ================================================================== */
/* 4. Dot (any character)                                             */
/* ================================================================== */

TEST_CASE(test_dot_single)
{
    stdromano::Regex re("a.b", stdromano::RegexFlags_DebugCompilation);
    ASSERT_MATCH(re, "axb", "axb");
    ASSERT_MATCH(re, "a1b", "a1b");
    ASSERT_MATCH(re, "a_b", "a_b");
    ASSERT_NO_MATCH(re, "ab");
}

TEST_CASE(test_dot_triple)
{
    stdromano::Regex re("...");
    ASSERT_MATCH(re, "abc", "abc");
    ASSERT_MATCH(re, "123", "123");
    ASSERT_NO_MATCH(re, "ab");
}

/* ================================================================== */
/* 5. Escape sequences                                                */
/* ================================================================== */

TEST_CASE(test_escape_digit)
{
    stdromano::Regex re("\\d+");
    ASSERT_MATCH(re, "42", "42");
    ASSERT_MATCH(re, "0", "0");
    ASSERT_NO_MATCH(re, "abc");
}

TEST_CASE(test_escape_word)
{
    stdromano::Regex re("\\w+");
    ASSERT_MATCH(re, "hello_world", "hello_world");
    ASSERT_MATCH(re, "test123", "test123");
    ASSERT_NO_MATCH(re, " ");
}

TEST_CASE(test_escape_whitespace)
{
    stdromano::Regex re("a\\s+b");
    ASSERT_MATCH(re, "a   b", "a   b");
    ASSERT_MATCH(re, "a b", "a b");
    ASSERT_NO_MATCH(re, "ab");
}

TEST_CASE(test_escape_dot_literal)
{
    stdromano::Regex re("a\\.b");
    ASSERT_MATCH(re, "a.b", "a.b");
    ASSERT_NO_MATCH(re, "axb");
}

TEST_CASE(test_escape_star_literal)
{
    stdromano::Regex re("a\\*b");
    ASSERT_MATCH(re, "a*b", "a*b");
    ASSERT_NO_MATCH(re, "aab");
}

/* ================================================================== */
/* 6. Character classes                                               */
/* ================================================================== */

TEST_CASE(test_char_class_multi_range)
{
    stdromano::Regex re("[a-zA-Z]+", stdromano::RegexFlags_DebugCompilation);
    ASSERT_MATCH(re, "Hello", "Hello");
    ASSERT_MATCH(re, "WORLD", "WORLD");
    ASSERT_NO_MATCH(re, "123");
}

TEST_CASE(test_char_class_mixed)
{
    stdromano::Regex re("[a-z0-9]+");
    ASSERT_MATCH(re, "abc123", "abc123");
    ASSERT_NO_MATCH(re, "ABC");
}

TEST_CASE(test_char_class_single)
{
    stdromano::Regex re("[x]");
    ASSERT_MATCH(re, "x", "x");
    ASSERT_NO_MATCH(re, "y");
}

/* ================================================================== */
/* 7. Negated character classes                                       */
/* ================================================================== */

TEST_CASE(test_negated_class_digits)
{
    stdromano::Regex re("[^0-9]+", stdromano::RegexFlags_DebugCompilation);
    ASSERT_MATCH(re, "hello", "hello");
    ASSERT_NO_MATCH(re, "123");
}

TEST_CASE(test_negated_class_lowercase)
{
    stdromano::Regex re("[^a-z]+");
    ASSERT_MATCH(re, "123", "123");
    ASSERT_MATCH(re, "ABC", "ABC");
    ASSERT_NO_MATCH(re, "hello");
}

/* ================================================================== */
/* 8. Groups and capture                                              */
/* ================================================================== */

TEST_CASE(test_group_email_like)
{
    stdromano::Regex re("(\\w+)@(\\w+)");
    auto m = re.match(stdromano::StringD("user@host"));
    ASSERT(m.matched());
    ASSERT_GROUP(m, 0, "user@host");
    ASSERT_GROUP(m, 1, "user");
    ASSERT_GROUP(m, 2, "host");
}

TEST_CASE(test_group_alpha_digits)
{
    stdromano::Regex re("([a-z]+)([0-9]+)");
    auto m = re.match(stdromano::StringD("abc123"));
    ASSERT(m.matched());
    ASSERT_GROUP(m, 0, "abc123");
    ASSERT_GROUP(m, 1, "abc");
    ASSERT_GROUP(m, 2, "123");
}

TEST_CASE(test_group_nested_disabled)
{
    // TODO: add backtracking to support greedy patterns
#if 0
    stdromano::Regex re("((\\w+)_(\\w+))", stdromano::RegexFlags_DebugCompilation);
    auto m = re.match(stdromano::StringD("hello_world"));
    ASSERT(m.matched());
    ASSERT_GROUP(m, 0, "hello_world");
    ASSERT_GROUP(m, 1, "hello_world");
    ASSERT_GROUP(m, 2, "hello");
    ASSERT_GROUP(m, 3, "world");
#endif // 0
}

TEST_CASE(test_group_with_alternation)
{
    stdromano::Regex re("(cat|dog)s");
    auto m = re.match(stdromano::StringD("cats"));
    ASSERT(m.matched());
    ASSERT_GROUP(m, 1, "cat");

    auto m2 = re.match(stdromano::StringD("dogs"));
    ASSERT(m2.matched());
    ASSERT_GROUP(m2, 1, "dog");

    ASSERT_NO_MATCH(re, "birds");
}

TEST_CASE(test_group_with_quantifier_disabled)
{
    // TODO: review the regex vm to exit when appropriate
#if 0
    stdromano::Regex re("(ab)+", stdromano::RegexFlags_DebugCompilation);
    auto m = re.match(stdromano::StringD("ababab"));
    ASSERT(m.matched());
    ASSERT_GROUP(m, 0, "ababab");
    ASSERT(m.group(1).matched());
#endif // 0
}

/* ================================================================== */
/* 9. RegexMatch API                                                  */
/* ================================================================== */

TEST_CASE(test_match_api_basic)
{
    stdromano::Regex re("(\\w+)");
    auto m = re.match(stdromano::StringD("hello"));
    ASSERT(m.matched());
    ASSERT(static_cast<bool>(m));
    ASSERT(m.start() == 0);
    ASSERT(m.end() == 5);
    ASSERT(m.str() == stdromano::StringD("hello"));
    ASSERT(m.group_count() >= 2); /* group 0 + group 1 */
}

TEST_CASE(test_match_api_no_match)
{
    stdromano::Regex re("xyz");
    auto m = re.match(stdromano::StringD("abc"));
    ASSERT(!m.matched());
    ASSERT(!static_cast<bool>(m));
}

TEST_CASE(test_match_api_out_of_range_group)
{
    stdromano::Regex re("hello");
    auto m = re.match(stdromano::StringD("hello"));
    ASSERT(m.matched());
    auto g = m.group(99);
    ASSERT(!g.matched());
    ASSERT(m.group_str(99) == stdromano::StringD());
}

/* ================================================================== */
/* 10. search()                                                       */
/* ================================================================== */

TEST_CASE(test_search_digits)
{
    stdromano::Regex re("\\d+");
    ASSERT_SEARCH(re, "abc123def", "123");
}

TEST_CASE(test_search_with_groups)
{
    stdromano::Regex re("(\\w+)@(\\w+)");
    auto m = re.search(stdromano::StringD("contact: user@host please"));
    ASSERT(m.matched());
    ASSERT_GROUP(m, 0, "user@host");
    ASSERT_GROUP(m, 1, "user");
    ASSERT_GROUP(m, 2, "host");
    ASSERT(m.start() == 9);
    ASSERT(m.end() == 18);
}

TEST_CASE(test_search_no_match)
{
    stdromano::Regex re("xyz");
    ASSERT_NO_SEARCH(re, "abcdef");
}

TEST_CASE(test_search_first_occurrence)
{
    stdromano::Regex re("[0-9]+");
    auto m = re.search(stdromano::StringD("aaa111bbb222"));
    ASSERT(m.matched());
    ASSERT(m.str() == stdromano::StringD("111"));
    ASSERT(m.start() == 3);
}

/* ================================================================== */
/* 11. match_iter() and match_all()                                   */
/* ================================================================== */

TEST_CASE(test_match_all_digits)
{
    stdromano::Regex re("[0-9]+");
    auto matches = re.match_all(stdromano::StringD("abc123def456ghi789"));

    ASSERT(matches.size() == 3);
    ASSERT(matches[0].str() == stdromano::StringD("123"));
    ASSERT(matches[1].str() == stdromano::StringD("456"));
    ASSERT(matches[2].str() == stdromano::StringD("789"));
}

TEST_CASE(test_match_all_words)
{
    stdromano::Regex re("\\w+");
    auto matches = re.match_all(stdromano::StringD("hello world foo"));

    ASSERT(matches.size() == 3);
    ASSERT(matches[0].str() == stdromano::StringD("hello"));
    ASSERT(matches[1].str() == stdromano::StringD("world"));
    ASSERT(matches[2].str() == stdromano::StringD("foo"));
}

TEST_CASE(test_match_iter_with_groups)
{
    stdromano::Regex re("(\\w+)=(\\w+)");
    stdromano::StringD input("key1=val1 key2=val2 key3=val3");

    stdromano::Vector<stdromano::StringD> keys;
    stdromano::Vector<stdromano::StringD> vals;

    re.match_iter(input, [&](const stdromano::RegexMatch& m) {
        keys.push_back(m.group_str(1));
        vals.push_back(m.group_str(2));
    });

    ASSERT(keys.size() == 3);
    ASSERT(keys[0] == stdromano::StringD("key1"));
    ASSERT(keys[1] == stdromano::StringD("key2"));
    ASSERT(keys[2] == stdromano::StringD("key3"));
    ASSERT(vals[0] == stdromano::StringD("val1"));
    ASSERT(vals[1] == stdromano::StringD("val2"));
    ASSERT(vals[2] == stdromano::StringD("val3"));
}

TEST_CASE(test_match_all_empty_result)
{
    stdromano::Regex re("[0-9]+");
    auto matches = re.match_all(stdromano::StringD("no digits here"));
    ASSERT(matches.size() == 0);
}

TEST_CASE(test_match_all_positions)
{
    stdromano::Regex re("[a-z]+");
    auto matches = re.match_all(stdromano::StringD("123abc456def"));

    ASSERT(matches.size() == 2);
    ASSERT(matches[0].start() == 3);
    ASSERT(matches[0].end() == 6);
    ASSERT(matches[1].start() == 9);
    ASSERT(matches[1].end() == 12);
}

/* ================================================================== */
/* 12. replace_iter / replace_all                                     */
/* ================================================================== */

TEST_CASE(test_replace_all_simple)
{
    stdromano::StringD s("${ENV_VAR}");
    stdromano::Regex re("${([A-Z0-9_]+)}");
    ASSERT(re.replace_all(s, "ENV_VAR") == "ENV_VAR");
}

TEST_CASE(test_replace_iter_with_map)
{
    stdromano::Regex re("${([A-Z0-9_]+)}");

    stdromano::HashMap<stdromano::StringD, stdromano::StringD> vars = {
        { "VAR1", "value1" },
        { "VAR2", "value2" },
        { "VAR3", "value3" },
    };

    auto replace_func = [&](const stdromano::RegexMatch& m) {
        if(m.group(1).matched())
        {
            const auto it = vars.find(m.group_str(1));

            if(it != vars.end())
                return it->second;
        }

        return stdromano::StringD();
    };

    stdromano::StringD res = re.replace_iter("${VAR2} string with ${VAR1} and ${VAR3}", replace_func);
    ASSERT(res == "value2 string with value1 and value3");
}

/* ================================================================== */
/* 13. Edge cases                                                     */
/* ================================================================== */

TEST_CASE(test_edge_empty_pattern)
{
    stdromano::Regex re("");
    ASSERT(re.valid());
}

TEST_CASE(test_edge_single_char)
{
    stdromano::Regex re("a");
    ASSERT_MATCH(re, "a", "a");
    ASSERT_NO_MATCH(re, "b");
}

TEST_CASE(test_edge_long_alternation)
{
    stdromano::Regex re("a|b|c|d|e");
    ASSERT_MATCH(re, "a", "a");
    ASSERT_MATCH(re, "c", "c");
    ASSERT_MATCH(re, "e", "e");
    ASSERT_NO_MATCH(re, "f");
}

TEST_CASE(test_edge_quantifier_on_group)
{
    stdromano::Regex re("(ab)*c", stdromano::RegexFlags_DebugCompilation);
    ASSERT_MATCH(re, "ababc", "ababc");
    ASSERT_MATCH(re, "abc", "abc");
    ASSERT_MATCH(re, "c", "c");
    // TODO
    // ASSERT_MATCH(re, "ac", "c");
}

TEST_CASE(test_edge_email_like_pattern)
{
    stdromano::Regex re("\\w+\\.\\w+@\\w+\\.\\w+");
    ASSERT_MATCH(re, "john.doe@example.com", "john.doe@example.com");
    ASSERT_NO_MATCH(re, "johndoe@example");
}

TEST_CASE(test_edge_multiple_groups_and_quantifiers)
{
    stdromano::Regex re("([a-z]+)_([0-9]+)\\.(txt|log)");
    auto m = re.match(stdromano::StringD("report_2025.txt"));
    ASSERT(m.matched());
    ASSERT_GROUP(m, 1, "report");
    ASSERT_GROUP(m, 2, "2025");
    ASSERT_GROUP(m, 3, "txt");

    auto m2 = re.match(stdromano::StringD("error_42.log"));
    ASSERT(m2.matched());
    ASSERT_GROUP(m2, 1, "error");
    ASSERT_GROUP(m2, 2, "42");
    ASSERT_GROUP(m2, 3, "log");
}

int main()
{
    spdlog::set_level(spdlog::level::debug);
    spdlog::info("Starting Regex tests");

    TestRunner runner("regex");
    runner.add_test("Star Quantifier Digits", test_star_quantifier_digits);
    runner.add_test("Plus Quantifier Digits", test_plus_quantifier_digits);
    runner.add_test("Alternation Star And Literal", test_alternation_star_and_literal);
    runner.add_test("Alternation Words", test_alternation_words);
    runner.add_test("Optional With Group", test_optional_with_group);
    runner.add_test("Optional Colour", test_optional_colour);
    runner.add_test("Dot Single", test_dot_single);
    runner.add_test("Dot Triple", test_dot_triple);
    runner.add_test("Escape Digit", test_escape_digit);
    runner.add_test("Escape Word", test_escape_word);
    runner.add_test("Escape Whitespace", test_escape_whitespace);
    runner.add_test("Escape Dot Literal", test_escape_dot_literal);
    runner.add_test("Escape Star Literal", test_escape_star_literal);
    runner.add_test("Char Class Multi Range", test_char_class_multi_range);
    runner.add_test("Char Class Mixed", test_char_class_mixed);
    runner.add_test("Char Class Single", test_char_class_single);
    runner.add_test("Negated Class Digits", test_negated_class_digits);
    runner.add_test("Negated Class Lowercase", test_negated_class_lowercase);
    runner.add_test("Group Email Like", test_group_email_like);
    runner.add_test("Group Alpha Digits", test_group_alpha_digits);
    runner.add_test("Group Nested Disabled", test_group_nested_disabled);
    runner.add_test("Group With Alternation", test_group_with_alternation);
    runner.add_test("Group With Quantifier Disabled", test_group_with_quantifier_disabled);
    runner.add_test("Match Api Basic", test_match_api_basic);
    runner.add_test("Match Api No Match", test_match_api_no_match);
    runner.add_test("Match Api Out Of Range Group", test_match_api_out_of_range_group);
    runner.add_test("Search Digits", test_search_digits);
    runner.add_test("Search With Groups", test_search_with_groups);
    runner.add_test("Search No Match", test_search_no_match);
    runner.add_test("Search First Occurrence", test_search_first_occurrence);
    runner.add_test("Match All Digits", test_match_all_digits);
    runner.add_test("Match All Words", test_match_all_words);
    runner.add_test("Match Iter With Groups", test_match_iter_with_groups);
    runner.add_test("Match All Empty Result", test_match_all_empty_result);
    runner.add_test("Match All Positions", test_match_all_positions);
    runner.add_test("Replace All Simple", test_replace_all_simple);
    runner.add_test("Replace Iter With Map", test_replace_iter_with_map);
    runner.add_test("Edge Empty Pattern", test_edge_empty_pattern);
    runner.add_test("Edge Single Char", test_edge_single_char);
    runner.add_test("Edge Long Alternation", test_edge_long_alternation);
    runner.add_test("Edge Quantifier On Group", test_edge_quantifier_on_group);
    runner.add_test("Edge Email Like Pattern", test_edge_email_like_pattern);
    runner.add_test("Edge Multiple Groups And Quantifiers", test_edge_multiple_groups_and_quantifiers);
    runner.run_all();

    spdlog::info("Finished Regex tests");

    return 0;
}
