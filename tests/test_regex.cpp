// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - Present Romain Augier
// All rights reserved.

#include "stdromano/regex.hpp"
#include "stdromano/hashmap.hpp"

#include "fixtures.hpp"

#include <algorithm>
#include <cstring>
#include <string>

#define REGEX_CHECK_MATCH(re, input, expected_str)                                                  \
    do                                                                                             \
    {                                                                                              \
        const auto regex_match = (re).match(stdromano::StringD(input));                            \
        STDROMANO_CHECK(regex_match.matched());                                                    \
        STDROMANO_CHECK_EQ(regex_match.str(), stdromano::StringD(expected_str));                   \
    } while(0)

#define REGEX_CHECK_NO_MATCH(re, input)                                                            \
    STDROMANO_CHECK(!(re).match(stdromano::StringD(input)).matched())

#define REGEX_CHECK_SEARCH(re, input, expected_str)                                                 \
    do                                                                                             \
    {                                                                                              \
        const auto regex_match = (re).search(stdromano::StringD(input));                           \
        STDROMANO_CHECK(regex_match.matched());                                                    \
        STDROMANO_CHECK_EQ(regex_match.str(), stdromano::StringD(expected_str));                   \
    } while(0)

#define REGEX_CHECK_NO_SEARCH(re, input)                                                           \
    STDROMANO_CHECK(!(re).search(stdromano::StringD(input)).matched())

#define REGEX_CHECK_GROUP(match_obj, group_idx, expected_str)                                      \
    do                                                                                             \
    {                                                                                              \
        STDROMANO_CHECK((match_obj).group(group_idx).matched());                                   \
        STDROMANO_CHECK_EQ((match_obj).group_str(group_idx), stdromano::StringD(expected_str));    \
    } while(0)

STDROMANO_TEST_CASE(star_quantifier_digits)
{
    stdromano::Regex re("[0-9]*", stdromano::RegexFlags_DebugCompilation);
    STDROMANO_CHECK(re.valid());
    STDROMANO_CHECK(re.match("123456789").matched());
    STDROMANO_CHECK(re.match("12345abcde").matched());
    STDROMANO_CHECK(re.match("abcde12345").matched());
}

STDROMANO_TEST_CASE(plus_quantifier_digits)
{
    stdromano::Regex re("[0-9]+", stdromano::RegexFlags_DebugCompilation);
    STDROMANO_CHECK(re.valid());
    STDROMANO_CHECK(re.match("123456789").matched());
    STDROMANO_CHECK(re.match("12345abcde").matched());
    STDROMANO_CHECK(re.match("1abcde").matched());
    STDROMANO_CHECK(re.match("12abcde").matched());
    REGEX_CHECK_NO_MATCH(re, "abcde12345");
}

STDROMANO_TEST_CASE(alternation_star_and_literal)
{
    stdromano::Regex re("a*b|cd", stdromano::RegexFlags_DebugCompilation);
    STDROMANO_CHECK(re.match("aaaaaacd").matched());
    STDROMANO_CHECK(re.match("abd").matched());
    STDROMANO_CHECK(re.match("bd").matched());
    STDROMANO_CHECK(re.match("cd").matched());
    REGEX_CHECK_NO_MATCH(re, "aaaacacd");
}

STDROMANO_TEST_CASE(alternation_words)
{
    stdromano::Regex re("cat|dog|bird");
    REGEX_CHECK_MATCH(re, "cat", "cat");
    REGEX_CHECK_MATCH(re, "dog", "dog");
    REGEX_CHECK_MATCH(re, "bird", "bird");
    REGEX_CHECK_NO_MATCH(re, "fish");
}

STDROMANO_TEST_CASE(optional_with_group)
{
    stdromano::Regex re("a?([b-e])+");
    STDROMANO_CHECK(re.match("abcdebcde").matched());
    STDROMANO_CHECK(re.match("bcdebcde").matched());
    REGEX_CHECK_NO_MATCH(re, "rbcdebcde");
}

STDROMANO_TEST_CASE(optional_colour)
{
    stdromano::Regex re("colou?r");
    REGEX_CHECK_MATCH(re, "color", "color");
    REGEX_CHECK_MATCH(re, "colour", "colour");
    REGEX_CHECK_NO_MATCH(re, "colouur");
}

STDROMANO_TEST_CASE(dot_single)
{
    stdromano::Regex re("a.b", stdromano::RegexFlags_DebugCompilation);
    REGEX_CHECK_MATCH(re, "axb", "axb");
    REGEX_CHECK_MATCH(re, "a1b", "a1b");
    REGEX_CHECK_MATCH(re, "a_b", "a_b");
    REGEX_CHECK_NO_MATCH(re, "ab");
}

STDROMANO_TEST_CASE(dot_triple)
{
    stdromano::Regex re("...");
    REGEX_CHECK_MATCH(re, "abc", "abc");
    REGEX_CHECK_MATCH(re, "123", "123");
    REGEX_CHECK_NO_MATCH(re, "ab");
}

STDROMANO_TEST_CASE(escape_digit)
{
    stdromano::Regex re("\\d+");
    REGEX_CHECK_MATCH(re, "42", "42");
    REGEX_CHECK_MATCH(re, "0", "0");
    REGEX_CHECK_NO_MATCH(re, "abc");
}

STDROMANO_TEST_CASE(escape_word)
{
    stdromano::Regex re("\\w+");
    REGEX_CHECK_MATCH(re, "hello_world", "hello_world");
    REGEX_CHECK_MATCH(re, "test123", "test123");
    REGEX_CHECK_NO_MATCH(re, " ");
}

STDROMANO_TEST_CASE(escape_whitespace)
{
    stdromano::Regex re("a\\s+b");
    REGEX_CHECK_MATCH(re, "a   b", "a   b");
    REGEX_CHECK_MATCH(re, "a b", "a b");
    REGEX_CHECK_NO_MATCH(re, "ab");
}

STDROMANO_TEST_CASE(escape_dot_literal)
{
    stdromano::Regex re("a\\.b");
    REGEX_CHECK_MATCH(re, "a.b", "a.b");
    REGEX_CHECK_NO_MATCH(re, "axb");
}

STDROMANO_TEST_CASE(escape_star_literal)
{
    stdromano::Regex re("a\\*b");
    REGEX_CHECK_MATCH(re, "a*b", "a*b");
    REGEX_CHECK_NO_MATCH(re, "aab");
}

STDROMANO_TEST_CASE(char_class_multi_range)
{
    stdromano::Regex re("[a-zA-Z]+", stdromano::RegexFlags_DebugCompilation);
    REGEX_CHECK_MATCH(re, "Hello", "Hello");
    REGEX_CHECK_MATCH(re, "WORLD", "WORLD");
    REGEX_CHECK_NO_MATCH(re, "123");
}

STDROMANO_TEST_CASE(char_class_mixed)
{
    stdromano::Regex re("[a-z0-9]+");
    REGEX_CHECK_MATCH(re, "abc123", "abc123");
    REGEX_CHECK_NO_MATCH(re, "ABC");
}

STDROMANO_TEST_CASE(char_class_single)
{
    stdromano::Regex re("[x]");
    REGEX_CHECK_MATCH(re, "x", "x");
    REGEX_CHECK_NO_MATCH(re, "y");
}

STDROMANO_TEST_CASE(negated_class_digits)
{
    stdromano::Regex re("[^0-9]+", stdromano::RegexFlags_DebugCompilation);
    REGEX_CHECK_MATCH(re, "hello", "hello");
    REGEX_CHECK_NO_MATCH(re, "123");
}

STDROMANO_TEST_CASE(negated_class_lowercase)
{
    stdromano::Regex re("[^a-z]+");
    REGEX_CHECK_MATCH(re, "123", "123");
    REGEX_CHECK_MATCH(re, "ABC", "ABC");
    REGEX_CHECK_NO_MATCH(re, "hello");
}

STDROMANO_TEST_CASE(group_email_like)
{
    stdromano::Regex re("(\\w+)@(\\w+)");
    auto m = re.match(stdromano::StringD("user@host"));
    STDROMANO_CHECK(m.matched());
    REGEX_CHECK_GROUP(m, 0, "user@host");
    REGEX_CHECK_GROUP(m, 1, "user");
    REGEX_CHECK_GROUP(m, 2, "host");
}

STDROMANO_TEST_CASE(group_alpha_digits)
{
    stdromano::Regex re("([a-z]+)([0-9]+)");
    auto m = re.match(stdromano::StringD("abc123"));
    STDROMANO_CHECK(m.matched());
    REGEX_CHECK_GROUP(m, 0, "abc123");
    REGEX_CHECK_GROUP(m, 1, "abc");
    REGEX_CHECK_GROUP(m, 2, "123");
}

STDROMANO_TEST_CASE(group_nested_disabled)
{
#if 0
    stdromano::Regex re("((\\w+)_(\\w+))", stdromano::RegexFlags_DebugCompilation);
    auto m = re.match(stdromano::StringD("hello_world"));
    STDROMANO_CHECK(m.matched());
    REGEX_CHECK_GROUP(m, 0, "hello_world");
    REGEX_CHECK_GROUP(m, 1, "hello_world");
    REGEX_CHECK_GROUP(m, 2, "hello");
    REGEX_CHECK_GROUP(m, 3, "world");
#endif
}

STDROMANO_TEST_CASE(group_with_alternation)
{
    stdromano::Regex re("(cat|dog)s");
    auto m = re.match(stdromano::StringD("cats"));
    STDROMANO_CHECK(m.matched());
    REGEX_CHECK_GROUP(m, 1, "cat");

    auto m2 = re.match(stdromano::StringD("dogs"));
    STDROMANO_CHECK(m2.matched());
    REGEX_CHECK_GROUP(m2, 1, "dog");

    REGEX_CHECK_NO_MATCH(re, "birds");
}

STDROMANO_TEST_CASE(group_with_quantifier_disabled)
{
#if 0
    stdromano::Regex re("(ab)+", stdromano::RegexFlags_DebugCompilation);
    auto m = re.match(stdromano::StringD("ababab"));
    STDROMANO_CHECK(m.matched());
    REGEX_CHECK_GROUP(m, 0, "ababab");
    STDROMANO_CHECK(m.group(1).matched());
#endif
}

STDROMANO_TEST_CASE(match_api_basic)
{
    stdromano::Regex re("(\\w+)");
    auto m = re.match(stdromano::StringD("hello"));
    STDROMANO_CHECK(m.matched());
    STDROMANO_CHECK(static_cast<bool>(m));
    STDROMANO_CHECK(m.start() == 0);
    STDROMANO_CHECK(m.end() == 5);
    STDROMANO_CHECK(m.str() == stdromano::StringD("hello"));
    STDROMANO_CHECK(m.group_count() >= 2);
}

STDROMANO_TEST_CASE(match_api_no_match)
{
    stdromano::Regex re("xyz");
    auto m = re.match(stdromano::StringD("abc"));
    STDROMANO_CHECK(!m.matched());
    STDROMANO_CHECK(!static_cast<bool>(m));
}

STDROMANO_TEST_CASE(match_api_out_of_range_group)
{
    stdromano::Regex re("hello");
    auto m = re.match(stdromano::StringD("hello"));
    STDROMANO_CHECK(m.matched());
    auto g = m.group(99);
    STDROMANO_CHECK(!g.matched());
    STDROMANO_CHECK(m.group_str(99) == stdromano::StringD());
}

STDROMANO_TEST_CASE(search_digits)
{
    stdromano::Regex re("\\d+");
    REGEX_CHECK_SEARCH(re, "abc123def", "123");
}

STDROMANO_TEST_CASE(search_with_groups)
{
    stdromano::Regex re("(\\w+)@(\\w+)");
    auto m = re.search(stdromano::StringD("contact: user@host please"));
    STDROMANO_CHECK(m.matched());
    REGEX_CHECK_GROUP(m, 0, "user@host");
    REGEX_CHECK_GROUP(m, 1, "user");
    REGEX_CHECK_GROUP(m, 2, "host");
    STDROMANO_CHECK(m.start() == 9);
    STDROMANO_CHECK(m.end() == 18);
}

STDROMANO_TEST_CASE(search_no_match)
{
    stdromano::Regex re("xyz");
    REGEX_CHECK_NO_SEARCH(re, "abcdef");
}

STDROMANO_TEST_CASE(search_first_occurrence)
{
    stdromano::Regex re("[0-9]+");
    auto m = re.search(stdromano::StringD("aaa111bbb222"));
    STDROMANO_CHECK(m.matched());
    STDROMANO_CHECK(m.str() == stdromano::StringD("111"));
    STDROMANO_CHECK(m.start() == 3);
}

STDROMANO_TEST_CASE(match_all_digits)
{
    stdromano::Regex re("[0-9]+");
    auto matches = re.match_all(stdromano::StringD("abc123def456ghi789"));

    STDROMANO_CHECK(matches.size() == 3);
    STDROMANO_CHECK(matches[0].str() == stdromano::StringD("123"));
    STDROMANO_CHECK(matches[1].str() == stdromano::StringD("456"));
    STDROMANO_CHECK(matches[2].str() == stdromano::StringD("789"));
}

STDROMANO_TEST_CASE(match_all_words)
{
    stdromano::Regex re("\\w+");
    auto matches = re.match_all(stdromano::StringD("hello world foo"));

    STDROMANO_CHECK(matches.size() == 3);
    STDROMANO_CHECK(matches[0].str() == stdromano::StringD("hello"));
    STDROMANO_CHECK(matches[1].str() == stdromano::StringD("world"));
    STDROMANO_CHECK(matches[2].str() == stdromano::StringD("foo"));
}

STDROMANO_TEST_CASE(match_iter_with_groups)
{
    stdromano::Regex re("(\\w+)=(\\w+)");
    stdromano::StringD input("key1=val1 key2=val2 key3=val3");

    stdromano::Vector<stdromano::StringD> keys;
    stdromano::Vector<stdromano::StringD> vals;

    re.match_iter(input, [&](const stdromano::RegexMatch& m) {
        keys.push_back(m.group_str(1));
        vals.push_back(m.group_str(2));
    });

    STDROMANO_CHECK(keys.size() == 3);
    STDROMANO_CHECK(keys[0] == stdromano::StringD("key1"));
    STDROMANO_CHECK(keys[1] == stdromano::StringD("key2"));
    STDROMANO_CHECK(keys[2] == stdromano::StringD("key3"));
    STDROMANO_CHECK(vals[0] == stdromano::StringD("val1"));
    STDROMANO_CHECK(vals[1] == stdromano::StringD("val2"));
    STDROMANO_CHECK(vals[2] == stdromano::StringD("val3"));
}

STDROMANO_TEST_CASE(match_all_empty_result)
{
    stdromano::Regex re("[0-9]+");
    auto matches = re.match_all(stdromano::StringD("no digits here"));
    STDROMANO_CHECK(matches.size() == 0);
}

STDROMANO_TEST_CASE(match_all_positions)
{
    stdromano::Regex re("[a-z]+");
    auto matches = re.match_all(stdromano::StringD("123abc456def"));

    STDROMANO_CHECK(matches.size() == 2);
    STDROMANO_CHECK(matches[0].start() == 3);
    STDROMANO_CHECK(matches[0].end() == 6);
    STDROMANO_CHECK(matches[1].start() == 9);
    STDROMANO_CHECK(matches[1].end() == 12);
}

STDROMANO_TEST_CASE(replace_all_simple)
{
    stdromano::StringD s("${ENV_VAR}");
    stdromano::Regex re("${([A-Z0-9_]+)}");
    STDROMANO_CHECK(re.replace_all(s, "ENV_VAR") == "ENV_VAR");
}

STDROMANO_TEST_CASE(replace_iter_with_map)
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
    STDROMANO_CHECK(res == "value2 string with value1 and value3");
}

STDROMANO_TEST_CASE(edge_empty_pattern)
{
    stdromano::Regex re("");
    STDROMANO_CHECK(re.valid());
}

STDROMANO_TEST_CASE(edge_single_char)
{
    stdromano::Regex re("a");
    REGEX_CHECK_MATCH(re, "a", "a");
    REGEX_CHECK_NO_MATCH(re, "b");
}

STDROMANO_TEST_CASE(edge_long_alternation)
{
    stdromano::Regex re("a|b|c|d|e");
    REGEX_CHECK_MATCH(re, "a", "a");
    REGEX_CHECK_MATCH(re, "c", "c");
    REGEX_CHECK_MATCH(re, "e", "e");
    REGEX_CHECK_NO_MATCH(re, "f");
}

STDROMANO_TEST_CASE(edge_quantifier_on_group)
{
    stdromano::Regex re("(ab)*c", stdromano::RegexFlags_DebugCompilation);
    REGEX_CHECK_MATCH(re, "ababc", "ababc");
    REGEX_CHECK_MATCH(re, "abc", "abc");
    REGEX_CHECK_MATCH(re, "c", "c");
}

STDROMANO_TEST_CASE(edge_email_like_pattern)
{
    stdromano::Regex re("\\w+\\.\\w+@\\w+\\.\\w+");
    REGEX_CHECK_MATCH(re, "john.doe@example.com", "john.doe@example.com");
    REGEX_CHECK_NO_MATCH(re, "johndoe@example");
}

STDROMANO_TEST_CASE(edge_multiple_groups_and_quantifiers)
{
    stdromano::Regex re("([a-z]+)_([0-9]+)\\.(txt|log)");
    auto m = re.match(stdromano::StringD("report_2025.txt"));
    STDROMANO_CHECK(m.matched());
    REGEX_CHECK_GROUP(m, 1, "report");
    REGEX_CHECK_GROUP(m, 2, "2025");
    REGEX_CHECK_GROUP(m, 3, "txt");

    auto m2 = re.match(stdromano::StringD("error_42.log"));
    STDROMANO_CHECK(m2.matched());
    REGEX_CHECK_GROUP(m2, 1, "error");
    REGEX_CHECK_GROUP(m2, 2, "42");
    REGEX_CHECK_GROUP(m2, 3, "log");
}

static std::string escape_regex(const std::string& literal)
{
    std::string escaped;

    for(const char c : literal)
    {
        if(std::strchr("\\.^$|?*+()[]{}", c) != nullptr)
            escaped.push_back('\\');

        escaped.push_back(c);
    }

    return escaped;
}

STDROMANO_TEST_CASE(fuzz_literal_search_matches_std_find)
{
    const auto report = stdromano::fuzz::run_property(fixtures::options("regex_literal_search", 500), [](stdromano::fuzz::Source& source) {
        const stdromano::StringD raw_needle = source.string(6, "ab.*+?()[]|\\");
        const stdromano::StringD raw_haystack = source.string(64, "ab.*+?()[]|\\ ");

        const std::string needle(raw_needle.c_str(), raw_needle.size());
        const std::string haystack(raw_haystack.c_str(), raw_haystack.size());

        if(needle.empty())
            return true;

        const std::string pattern = escape_regex(needle);
        const stdromano::Regex re(stdromano::StringD::make_from_c_str(pattern.c_str(), pattern.size()), 0);

        STDROMANO_FUZZ_CHECK(re.valid());

        const auto found = re.search(raw_haystack);
        const std::size_t expected = haystack.find(needle);

        STDROMANO_FUZZ_CHECK_EQ(found.matched(), expected != std::string::npos);

        if(found.matched())
            STDROMANO_FUZZ_CHECK(found.str() == raw_needle);

        return true;
    });

    TESTS_REQUIRE_PROPERTY(report);
}

STDROMANO_TEST_CASE(fuzz_arbitrary_patterns_do_not_crash)
{
    auto options = fixtures::options("regex_arbitrary_patterns", 2000);
    options.max_input_size = 48;
    options.dictionary.tokens = {"(", ")", "[", "]", "*", "+", "?", "|", "\\", ".", "^", "$", "[0-9]", "(a|b)*", "{2}"};
    options.corpus = {{'a', '*', 'b'}, {'(', 'a', '|', 'b', ')', '+'}, {'[', 'a', '-', 'z', ']'}};

    fixtures::QuietLogs quiet;

    const auto report = stdromano::fuzz::run_input(options, [](const std::uint8_t* data, std::size_t size) {
        std::string pattern(reinterpret_cast<const char*>(data), size);
        pattern.erase(std::remove(pattern.begin(), pattern.end(), '\0'), pattern.end());

        const stdromano::Regex re(stdromano::StringD::make_from_c_str(pattern.c_str(), pattern.size()), 0);

        if(re.valid())
        {
            (void)re.match("aaab0123(xyz)|");
            (void)re.search("zzz aab [q] 42");
            (void)re.match_all("ab ab ba");
        }

        return true;
    });

    TESTS_REQUIRE_PROPERTY(report);
}

STDROMANO_TEST_MAIN()
