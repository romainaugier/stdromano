// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - Present Romain Augier
// All rights reserved.

#include "stdromano/string.hpp"

#include "fixtures.hpp"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <string>
#include <vector>

using namespace stdromano;

static String<> create_large_string(const std::size_t size)
{
    String<> result;

    for(std::size_t i = 0; i < size; ++i)
        result.push_back(static_cast<char>('A' + (i % 26)));

    return result;
}

static std::string to_std(const StringD& str)
{
    return std::string(str.data(), str.size());
}

static StringD from_std(const std::string& str)
{
    return StringD::make_from_c_str(str.c_str(), str.size());
}

static bool reference_validate_utf8(const std::uint8_t* data, const std::size_t size)
{
    std::size_t i = 0;

    while(i < size)
    {
        const std::uint8_t lead = data[i];

        std::size_t length = 0;
        std::uint32_t code_point = 0;

        if(lead < 0x80)
        {
            ++i;
            continue;
        }
        else if(lead >= 0xC2 && lead <= 0xDF)
        {
            length = 2;
            code_point = lead & 0x1F;
        }
        else if(lead >= 0xE0 && lead <= 0xEF)
        {
            length = 3;
            code_point = lead & 0x0F;
        }
        else if(lead >= 0xF0 && lead <= 0xF4)
        {
            length = 4;
            code_point = lead & 0x07;
        }
        else
        {
            return false;
        }

        if(i + length > size)
            return false;

        for(std::size_t j = 1; j < length; ++j)
        {
            if((data[i + j] & 0xC0) != 0x80)
                return false;

            code_point = (code_point << 6) | (data[i + j] & 0x3F);
        }

        if((length == 3 && code_point < 0x800) || (length == 4 && code_point < 0x10000))
            return false;

        if(code_point > 0x10FFFF || (code_point >= 0xD800 && code_point <= 0xDFFF))
            return false;

        i += length;
    }

    return true;
}

static void encode_utf8(const char32_t code_point, std::string& out)
{
    if(code_point < 0x80)
    {
        out.push_back(static_cast<char>(code_point));
    }
    else if(code_point < 0x800)
    {
        out.push_back(static_cast<char>(0xC0 | (code_point >> 6)));
        out.push_back(static_cast<char>(0x80 | (code_point & 0x3F)));
    }
    else if(code_point < 0x10000)
    {
        out.push_back(static_cast<char>(0xE0 | (code_point >> 12)));
        out.push_back(static_cast<char>(0x80 | ((code_point >> 6) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | (code_point & 0x3F)));
    }
    else
    {
        out.push_back(static_cast<char>(0xF0 | (code_point >> 18)));
        out.push_back(static_cast<char>(0x80 | ((code_point >> 12) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | ((code_point >> 6) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | (code_point & 0x3F)));
    }
}

static char32_t valid_code_point(fuzz::Source& source)
{
    switch(source.index(4))
    {
        case 0:
            return static_cast<char32_t>(source.range<std::uint32_t>(0x01, 0x7F));
        case 1:
            return static_cast<char32_t>(source.range<std::uint32_t>(0x80, 0x7FF));
        case 2:
        {
            const std::uint32_t value = source.range<std::uint32_t>(0x800, 0xFFFF - 0x800);
            return static_cast<char32_t>(value >= 0xD800 ? value + 0x800 : value);
        }
        default:
            return static_cast<char32_t>(source.range<std::uint32_t>(0x10000, 0x10FFFF));
    }
}

static std::string random_utf8_bytes(fuzz::Source& source, const std::size_t max_size)
{
    std::string out;

    const std::size_t count = source.size(max_size);

    for(std::size_t i = 0; i < count; ++i)
    {
        switch(source.index(3))
        {
            case 0:
                encode_utf8(valid_code_point(source), out);
                break;
            case 1:
                out.push_back(static_cast<char>(source.pick<int>({0x80, 0xBF, 0xC0, 0xC1, 0xC2, 0xDF, 0xE0,
                                                                  0xED, 0xEF, 0xF0, 0xF4, 0xF5, 0xFF})));
                break;
            default:
                out.push_back(static_cast<char>(source.range<int>(0x20, 0x7E)));
                break;
        }
    }

    return out;
}

STDROMANO_TEST_CASE(construction)
{
    const String<> empty;
    STDROMANO_CHECK_EQ(empty.size(), 0u);
    STDROMANO_CHECK(empty.empty());
    STDROMANO_CHECK_EQ(std::strcmp(empty.c_str(), ""), 0);

    const String<> str("Test");
    STDROMANO_CHECK_EQ(str.size(), 4u);
    STDROMANO_CHECK(!str.empty());
    STDROMANO_CHECK_EQ(std::strcmp(str.c_str(), "Test"), 0);

    const String<> fmt_str("{} World", "Hello");
    STDROMANO_CHECK_EQ(std::strcmp(fmt_str.c_str(), "Hello World"), 0);

    const String<> partial = String<>::make_from_c_str("Hello World", 5);
    STDROMANO_CHECK_EQ(to_std(partial), "Hello");
}

STDROMANO_TEST_CASE(make_ref)
{
    const char* raw = "Reference";
    const String<> ref = String<>::make_ref(raw, std::strlen(raw));
    STDROMANO_CHECK(ref.is_ref());
    STDROMANO_CHECK(ref.data() == raw);
    STDROMANO_CHECK_EQ(ref.size(), std::strlen(raw));

    const String<> str("Hello");
    const String<> ref_from_str = String<>::make_ref(str);
    STDROMANO_CHECK(ref_from_str.is_ref());
    STDROMANO_CHECK_EQ(to_std(ref_from_str), "Hello");
}

STDROMANO_TEST_CASE(equality_across_vectorization_modes)
{
    const StringD large = create_large_string(4096);
    const StringD large_copy = large.copy();

    StringD large_changed = large.copy();
    large_changed[2048] = ' ';

    fixtures::for_each_vectorization_mode([&](std::uint32_t) {
        STDROMANO_CHECK(String<>("Hello") == String<>("Hello"));
        STDROMANO_CHECK(String<>("Hello") != String<>("World"));
        STDROMANO_CHECK(String<>() == String<>());
        STDROMANO_CHECK(String<>() != String<>("Hello"));
        STDROMANO_CHECK(large == large_copy);
        STDROMANO_CHECK(large != large_changed);
    });
}

STDROMANO_TEST_CASE(push_back)
{
    String<> str;
    str.push_back('A');
    STDROMANO_CHECK_EQ(to_std(str), "A");

    for(std::size_t i = 0; i < 10; ++i)
        str.push_back('B');

    STDROMANO_CHECK_EQ(to_std(str), "ABBBBBBBBBB");
    STDROMANO_CHECK_EQ(str.data()[str.size()], '\0');
}

STDROMANO_TEST_CASE(append_and_prepend)
{
    String<> str("Middle");

    str.appendc("End");
    STDROMANO_CHECK_EQ(to_std(str), "MiddleEnd");

    str.prependc("Start");
    STDROMANO_CHECK_EQ(to_std(str), "StartMiddleEnd");

    str.appends(String<>("More"));
    STDROMANO_CHECK_EQ(to_std(str), "StartMiddleEndMore");

    str.prepends(String<>("Pre"));
    STDROMANO_CHECK_EQ(to_std(str), "PreStartMiddleEndMore");

    str.appendf(" {}", "Formatted");
    STDROMANO_CHECK_EQ(to_std(str), "PreStartMiddleEndMore Formatted");

    str.prependf("Before ");
    STDROMANO_CHECK_EQ(to_std(str), "Before PreStartMiddleEndMore Formatted");

    const String<> chained = String<>("Middle").prependc("Start").appendf("{}", "End");
    STDROMANO_CHECK_EQ(to_std(chained), "StartMiddleEnd");
}

STDROMANO_TEST_CASE(case_conversion)
{
    const String<> str("Hello World");

    STDROMANO_CHECK_EQ(to_std(str.upper()), "HELLO WORLD");
    STDROMANO_CHECK_EQ(to_std(str.lower()), "hello world");
    STDROMANO_CHECK_EQ(to_std(str.capitalize()), "Hello world");

    const String<> empty;
    STDROMANO_CHECK(empty.upper().empty());
    STDROMANO_CHECK(empty.lower().empty());
    STDROMANO_CHECK(empty.capitalize().empty());
}

STDROMANO_TEST_CASE(strip)
{
    const String<> str("  Hello World  ");

    STDROMANO_CHECK_EQ(to_std(str.strip()), "Hello World");
    STDROMANO_CHECK_EQ(to_std(str.lstrip()), "Hello World  ");
    STDROMANO_CHECK_EQ(to_std(str.rstrip()), "  Hello World");

    STDROMANO_CHECK(String<>().strip().empty());
    STDROMANO_CHECK(String<>("    ").strip().empty());

    const String<> custom("###Hello###");
    STDROMANO_CHECK_EQ(to_std(custom.strip('#')), "Hello");
    STDROMANO_CHECK_EQ(to_std(String<>::make_ref(custom).strip('#')), "Hello");
    STDROMANO_CHECK_EQ(to_std(String<>::make_ref("Hello", 5).strip('#')), "Hello");
}

STDROMANO_TEST_CASE(startswith_and_endswith)
{
    const String<> str("Hello World");

    STDROMANO_CHECK(str.startswith("Hello"));
    STDROMANO_CHECK(!str.startswith("World"));
    STDROMANO_CHECK(str.endswith("World"));
    STDROMANO_CHECK(!str.endswith("Hello"));

    STDROMANO_CHECK(!String<>().startswith("a"));
    STDROMANO_CHECK(!String<>().endswith("a"));
    STDROMANO_CHECK(str.startswith(""));
    STDROMANO_CHECK(str.endswith(""));

    STDROMANO_CHECK(!str.startswith("Hello World Long"));
    STDROMANO_CHECK(!str.endswith("Long Hello World"));
}

STDROMANO_TEST_CASE(find)
{
    const String<> str("Hello Hello World");

    STDROMANO_CHECK_EQ(str.find("Hello"), 0);
    STDROMANO_CHECK_EQ(str.find("World"), 12);
    STDROMANO_CHECK_EQ(str.find("Missing"), -1);
    STDROMANO_CHECK_EQ(str.find(""), 0);
    STDROMANO_CHECK_EQ(str.find("TooLongForTheString"), -1);
    STDROMANO_CHECK_EQ(String<>().find("a"), -1);

    const String<> prefix = str.substr(0, 8);
    STDROMANO_CHECK_EQ(prefix.find("World"), -1);
    STDROMANO_CHECK_EQ(prefix.find("Hel"), 0);
}

STDROMANO_TEST_CASE(split)
{
    const String<> str("Hello,World,Test");
    const String<> sep(",");
    String<> part;
    String<>::split_iterator it = 0;

    STDROMANO_REQUIRE(str.split(sep, it, part));
    STDROMANO_CHECK_EQ(to_std(part), "Hello");
    STDROMANO_CHECK_EQ(it, 6u);

    STDROMANO_REQUIRE(str.split(sep, it, part));
    STDROMANO_CHECK_EQ(to_std(part), "World");
    STDROMANO_CHECK_EQ(it, 12u);

    STDROMANO_REQUIRE(str.split(sep, it, part));
    STDROMANO_CHECK_EQ(to_std(part), "Test");
    STDROMANO_CHECK_EQ(it, str.size());

    STDROMANO_CHECK(!str.split(sep, it, part));
    STDROMANO_CHECK(part.empty());

    it = 0;
    STDROMANO_CHECK(!String<>().split(sep, it, part));
    STDROMANO_CHECK(part.empty());
    STDROMANO_CHECK_EQ(it, 0u);

    const String<> hello("Hello");

    it = 0;
    STDROMANO_CHECK(!hello.split(String<>(), it, part));
    STDROMANO_CHECK_EQ(to_std(part), "Hello");
    STDROMANO_CHECK_EQ(it, hello.size());

    it = 0;
    STDROMANO_CHECK(hello.split(String<>(";"), it, part));
    STDROMANO_CHECK_EQ(to_std(part), "Hello");
    STDROMANO_CHECK_EQ(it, hello.size());

    String<> right;
    STDROMANO_CHECK_EQ(to_std(str.lsplit(",", &right)), "Hello");
    STDROMANO_CHECK_EQ(to_std(right), "World,Test");

    String<> left;
    STDROMANO_CHECK_EQ(to_std(str.rsplit(",", &left)), "Test");
    STDROMANO_CHECK_EQ(to_std(left), "Hello,World");
}

STDROMANO_TEST_CASE(large_string)
{
    const String<> large = create_large_string(1000);
    STDROMANO_REQUIRE_EQ(large.size(), 1000u);

    for(std::size_t i = 0; i < large.size(); ++i)
        STDROMANO_REQUIRE_EQ(large.data()[i], static_cast<char>('A' + (i % 26)));

    STDROMANO_CHECK_EQ(large.data()[large.size()], '\0');
}

STDROMANO_TEST_CASE(copy_construction)
{
    const String<> local("Hello");
    const String<> local_copy(local);
    STDROMANO_CHECK_EQ(to_std(local_copy), "Hello");
    STDROMANO_CHECK(!local_copy.is_ref());
    STDROMANO_CHECK_EQ(local_copy.capacity(), local.capacity());
    STDROMANO_CHECK(local == local_copy);

    const String<> heap = create_large_string(100);
    const String<> heap_copy(heap);
    STDROMANO_CHECK(heap == heap_copy);
    STDROMANO_CHECK(heap.data() != heap_copy.data());
    STDROMANO_CHECK(!heap_copy.is_ref());

    const String<> ref = String<>::make_ref("Reference", 9);
    const String<> ref_copy(ref);
    STDROMANO_CHECK(ref_copy.is_ref());
    STDROMANO_CHECK(ref == ref_copy);
}

STDROMANO_TEST_CASE(move_construction)
{
    String<> local("Hello");
    const String<> local_moved(std::move(local));
    STDROMANO_CHECK_EQ(to_std(local_moved), "Hello");
    STDROMANO_CHECK(local.empty());

    String<> heap = create_large_string(100);
    const String<> heap_moved(std::move(heap));
    STDROMANO_CHECK_EQ(heap_moved.size(), 100u);
    STDROMANO_CHECK(heap.empty());

    String<> ref = String<>::make_ref("Reference", 9);
    const String<> ref_moved(std::move(ref));
    STDROMANO_CHECK(ref_moved.is_ref());
    STDROMANO_CHECK(ref.empty());
}

STDROMANO_TEST_CASE(copy_assignment)
{
    const String<> local("Hello");
    String<> target;
    target = local;
    STDROMANO_CHECK(target == local);
    STDROMANO_CHECK(!target.is_ref());

    const String<> heap = create_large_string(100);
    String<> target_heap = create_large_string(50);
    target_heap = heap;
    STDROMANO_CHECK(target_heap == heap);

    const String<> ref = String<>::make_ref("Reference", 9);
    String<> target_ref = create_large_string(100);
    target_ref = ref;
    STDROMANO_CHECK(target_ref.is_ref());
    STDROMANO_CHECK(target_ref == ref);

    String<> self("Self");
    const String<>& alias = self;
    self = alias;
    STDROMANO_CHECK_EQ(to_std(self), "Self");
}

STDROMANO_TEST_CASE(move_assignment)
{
    String<> local("Hello");
    String<> target;
    target = std::move(local);
    STDROMANO_CHECK_EQ(to_std(target), "Hello");
    STDROMANO_CHECK(local.empty());

    String<> heap = create_large_string(100);
    String<> target_heap = create_large_string(50);
    target_heap = std::move(heap);
    STDROMANO_CHECK_EQ(target_heap.size(), 100u);
    STDROMANO_CHECK(heap.empty());

    String<> ref = String<>::make_ref("Reference", 9);
    String<> target_ref = create_large_string(100);
    target_ref = std::move(ref);
    STDROMANO_CHECK(target_ref.is_ref());
    STDROMANO_CHECK(ref.empty());

    String<> self("Self");
    String<>& alias = self;
    self = std::move(alias);
    STDROMANO_CHECK_EQ(to_std(self), "Self");
}

STDROMANO_TEST_CASE(zeroed_string)
{
    const String<> zeroed = String<>::make_zeroed(2048);
    STDROMANO_CHECK_EQ(zeroed.capacity(), 2048u);
}

STDROMANO_TEST_CASE(replace_and_zfill)
{
    const String<> csv = "this,string,is,sep,by,commas";
    STDROMANO_CHECK_EQ(to_std(csv.replace(',', ' ')), "this string is sep by commas");

    STDROMANO_CHECK_EQ(to_std(String<>("1").zfill(8)), "00000001");
    STDROMANO_CHECK_EQ(to_std(String<>("123456789").zfill(4)), "123456789");
}

STDROMANO_TEST_CASE(numeric_conversions)
{
    STDROMANO_CHECK_EQ(String<>("1").to_long_long(), 1);
    STDROMANO_CHECK_EQ(String<>("-42").to_long_long(), -42);
    STDROMANO_CHECK_EQ(String<>("1.0").to_double(), 1.0);
    STDROMANO_CHECK_EQ(String<>("-2.5").to_double(), -2.5);

    for(const char* value : {"1", "true", "True", "TRUE"})
        STDROMANO_CHECK_MSG(String<>(value).to_bool(), value);

    for(const char* value : {"0", "false", "False", "FALSE"})
        STDROMANO_CHECK_MSG(!String<>(value).to_bool(), value);
}

STDROMANO_TEST_CASE(substr_clear_erase_and_shrink)
{
    const StringD s = "Hello World!";

    STDROMANO_CHECK_EQ(to_std(s.substr(6)), "World!");
    STDROMANO_CHECK_EQ(to_std(s.substr(6, 3)), "Wor");
    STDROMANO_CHECK_EQ(to_std(s.substr(6, 100)), "World!");
    STDROMANO_CHECK(s.substr(s.size()).empty());

    StringD cleared = "Hello World!";
    cleared.clear();
    STDROMANO_CHECK(cleared.empty());

    StringD erased = "Hello World!";
    erased.erase(5);
    STDROMANO_CHECK_EQ(to_std(erased), "Hello");

    StringD shrunk = "Hello World!";
    shrunk.shrink_to_fit();
    STDROMANO_CHECK_EQ(to_std(shrunk), "Hello World!");
    shrunk.shrink_to_fit(5);
    STDROMANO_CHECK_EQ(to_std(shrunk), "Hello");
}

STDROMANO_TEST_CASE(insertion)
{
    String<> str = "Hello World";

    str.insertc(5, " Beautiful");
    STDROMANO_CHECK_EQ(to_std(str), "Hello Beautiful World");

    str.inserts(15, String<>(" Amazing"));
    STDROMANO_CHECK_EQ(to_std(str), "Hello Beautiful Amazing World");

    str.insertf(0, "{}: ", 42);
    STDROMANO_CHECK_EQ(to_std(str), "42: Hello Beautiful Amazing World");
}

STDROMANO_TEST_CASE(is_digit)
{
    STDROMANO_CHECK(String<>("0123456789").is_digit());
    STDROMANO_CHECK(!String<>("abcdef").is_digit());
    STDROMANO_CHECK(!String<>("0123abcd").is_digit());
}

STDROMANO_TEST_CASE(to_string_and_join)
{
    STDROMANO_CHECK_EQ(to_std(to_string(1)), "1");
    STDROMANO_CHECK_EQ(to_std(to_string(123456789)), "123456789");
    STDROMANO_CHECK_EQ(to_std(to_string(1.5)), "1.5");

    const std::vector<int> values = {1, 2, 3};
    const StringD joined = join(values, [](const int& v) { return to_string(v); }, ", ");
    STDROMANO_CHECK_EQ(to_std(joined), "1, 2, 3");
}

STDROMANO_TEST_CASE(three_way_strcmp)
{
    STDROMANO_CHECK_EQ(strcmp(StringD("abc"), StringD("abd")), -1);
    STDROMANO_CHECK_EQ(strcmp(StringD("abd"), StringD("abc")), 1);
    STDROMANO_CHECK_EQ(strcmp(StringD("abc"), StringD("abc")), 0);
    STDROMANO_CHECK_EQ(strcmp(StringD("ab"), StringD("abc")), -1);
    STDROMANO_CHECK_EQ(strcmp(StringD("ABC"), StringD("abc"), false), 0);
    STDROMANO_CHECK_EQ(strcmp(StringD(), StringD()), 0);
}

STDROMANO_TEST_CASE(utf8_validation)
{
    struct Case
    {
        const char* data;
        bool valid;
    };

    const Case cases[] = {
        {"Hello, world! UTF-8 characters like \xC3\xA9, \xC3\xBC, and \xE2\x82\xAC.", true},
        {"An invalid sequence: \xC3" "A", false},
        {"An overlong sequence: \xC0\xAF", false},
        {"A lone continuation byte: \x80", false},
        {"A surrogate: \xED\xA0\x80", false},
        {"Above U+10FFFF: \xF4\x90\x80\x80", false},
        {"Truncated at the end: \xE2\x82", false},
        {"\xF0\x9F\x98\x80", true},
    };

    fixtures::for_each_vectorization_mode([&](std::uint32_t) {
        for(const Case& test : cases)
            STDROMANO_CHECK_MSG(validate_utf8(test.data, std::strlen(test.data)) == test.valid,
                                StringD::make_fmt("mode {}: \"{}\"",
                                                  simd_get_vectorization_mode_as_string(),
                                                  test.data));
    });
}

STDROMANO_TEST_CASE(utf8_iterator)
{
    {
        const StringD s("Hello");
        STDROMANO_CHECK_EQ(s.u8length(), 5u);

        auto it = s.u8begin();

        for(const char32_t expected : {U'H', U'e', U'l', U'l', U'o'})
        {
            STDROMANO_CHECK(*it == expected);
            ++it;
        }

        STDROMANO_CHECK(it == s.u8end());
    }

    {
        const StringD s;
        STDROMANO_CHECK_EQ(s.u8length(), 0u);
        STDROMANO_CHECK(s.u8begin() == s.u8end());
    }

    {
        const StringD s("H\xC3\xA9llo \xF0\x9F\x8C\x8D");
        STDROMANO_CHECK_EQ(s.size(), 11u);
        STDROMANO_CHECK_EQ(s.u8length(), 7u);

        const char32_t expected[] = {U'H', U'\u00E9', U'l', U'l', U'o', U' ', U'\U0001F30D'};

        std::size_t index = 0;

        for(auto it = s.u8begin(); it != s.u8end(); ++it, ++index)
            STDROMANO_REQUIRE(*it == expected[index]);

        STDROMANO_CHECK_EQ(index, 7u);
    }

    {
        const StringD s("A\xC3\xA9\xE2\x82\xAC\xF0\x9F\x98\x80");
        auto it = s.u8begin();

        for(const std::size_t length : {std::size_t(1), std::size_t(2), std::size_t(3), std::size_t(4)})
        {
            STDROMANO_CHECK_EQ(it.byte_length(), length);
            ++it;
        }

        STDROMANO_CHECK(it == s.u8end());
    }

    {
        const StringD s("ab\xC3\xA9");
        auto it = s.u8begin();

        STDROMANO_CHECK(it.base() == s.data());
        ++it;
        STDROMANO_CHECK(it.base() == s.data() + 1);
        ++it;
        STDROMANO_CHECK(it.base() == s.data() + 2);
        ++it;
        STDROMANO_CHECK(it.base() == s.data() + 4);
        STDROMANO_CHECK(it == s.u8end());
    }

    {
        const StringD s("ab");
        auto it = s.u8begin();
        const auto previous = it++;
        STDROMANO_CHECK(*previous == U'a');
        STDROMANO_CHECK(*it == U'b');

        auto back = s.u8end();
        const auto end = back--;
        STDROMANO_CHECK(end == s.u8end());
        STDROMANO_CHECK(*back == U'b');
    }

    {
        const StringD s("A\xC3\xA9\xE2\x82\xAC");
        auto it = s.u8end();

        --it;
        STDROMANO_CHECK(*it == U'\u20AC');
        --it;
        STDROMANO_CHECK(*it == U'\u00E9');
        --it;
        STDROMANO_CHECK(*it == U'A');
        STDROMANO_CHECK(it == s.u8begin());
    }

    {
        const StringD s("abc");
        auto a = s.u8begin();
        auto b = s.u8begin();
        ++b;

        STDROMANO_CHECK(a < b);
        STDROMANO_CHECK(b > a);
        STDROMANO_CHECK(a <= b);
        STDROMANO_CHECK(b >= a);
        STDROMANO_CHECK(a <= a);
        STDROMANO_CHECK(a >= a);
        STDROMANO_CHECK(a != b);
    }

    {
        const String<7> small("abc");
        STDROMANO_CHECK_EQ(small.u8length(), 3u);

        const String<7> big("abcdefghijklmnop");
        STDROMANO_CHECK_EQ(big.u8length(), 16u);
    }
}

STDROMANO_TEST_CASE(fuzz_equality_matches_memcmp)
{
    const auto report = fuzz::run_property(fixtures::options("string_equality", 500), [](fuzz::Source& source) {
        const std::string lhs = to_std(source.string(300, "abAB01 "));
        std::string rhs = lhs;

        if(!rhs.empty() && source.boolean())
            rhs[source.index(rhs.size())] ^= 0x20;

        if(source.one_in(4))
            rhs.push_back('x');

        const StringD a = from_std(lhs);
        const StringD b = from_std(rhs);

        bool ok = true;

        fixtures::for_each_vectorization_mode([&](std::uint32_t) {
            ok &= (a == b) == (lhs == rhs);
            ok &= (a != b) == (lhs != rhs);
        });

        return ok;
    });

    TESTS_REQUIRE_PROPERTY(report);
}

STDROMANO_TEST_CASE(fuzz_case_conversion_matches_ctype)
{
    const auto report = fuzz::run_property(fixtures::options("string_case", 500), [](fuzz::Source& source) {
        const std::string input = to_std(source.string(200));

        std::string lower = input;
        std::string upper = input;

        for(char& c : lower)
            if(c >= 'A' && c <= 'Z')
                c = static_cast<char>(c - 'A' + 'a');

        for(char& c : upper)
            if(c >= 'a' && c <= 'z')
                c = static_cast<char>(c - 'a' + 'A');

        const StringD str = from_std(input);

        bool ok = to_std(str.upper()) == upper;

        fixtures::for_each_vectorization_mode([&](std::uint32_t) { ok &= to_std(str.lower()) == lower; });

        return ok;
    });

    TESTS_REQUIRE_PROPERTY(report);
}

STDROMANO_TEST_CASE(fuzz_search_matches_std_string)
{
    const auto report = fuzz::run_property(fixtures::options("string_search", 1000), [](fuzz::Source& source) {
        const std::string haystack = to_std(source.string(64, "ab,"));
        const std::string needle = to_std(source.string(4, "ab,"));

        const StringD str = from_std(haystack);
        const StringD sub = from_std(needle);

        const std::size_t expected = haystack.find(needle);
        STDROMANO_FUZZ_CHECK_EQ(str.find(sub), expected == std::string::npos ? -1 : static_cast<int>(expected));

        const bool starts = haystack.compare(0, needle.size(), needle) == 0 && needle.size() <= haystack.size();
        const bool ends = needle.size() <= haystack.size() &&
                          haystack.compare(haystack.size() - needle.size(), needle.size(), needle) == 0;

        STDROMANO_FUZZ_CHECK_EQ(str.startswith(sub), starts);
        STDROMANO_FUZZ_CHECK_EQ(str.endswith(sub), ends);

        const std::size_t cut = source.range<std::size_t>(0, haystack.size());
        const StringD prefix = str.substr(0, cut);
        const std::size_t prefix_expected = haystack.substr(0, cut).find(needle);

        STDROMANO_FUZZ_CHECK_EQ(prefix.find(sub),
                                prefix_expected == std::string::npos ? -1 : static_cast<int>(prefix_expected));

        return true;
    });

    TESTS_REQUIRE_PROPERTY(report);
}

STDROMANO_TEST_CASE(fuzz_split_round_trips)
{
    const auto report = fuzz::run_property(fixtures::options("string_split", 1000), [](fuzz::Source& source) {
        const std::string input = to_std(source.string(64, "ab,;"));
        const std::string separator = source.pick<const char*>({",", ";", ",;", "ab"});

        const StringD str = from_std(input);
        const StringD sep = from_std(separator);

        std::vector<std::string> expected;
        std::size_t start = 0;

        while(start < input.size())
        {
            const std::size_t found = input.find(separator, start);

            if(found == std::string::npos)
            {
                expected.push_back(input.substr(start));
                break;
            }

            expected.push_back(input.substr(start, found - start));
            start = found + separator.size();
        }

        std::vector<std::string> parts;
        StringD part;
        StringD::split_iterator it = 0;

        while(str.split(sep, it, part))
        {
            parts.push_back(to_std(part));
            STDROMANO_FUZZ_CHECK(parts.size() <= input.size() + 1);
        }

        STDROMANO_FUZZ_CHECK(parts == expected);

        const std::size_t first = input.find(separator);
        const std::size_t last = input.rfind(separator);

        StringD right;
        const StringD left = str.lsplit(sep, &right);
        StringD before;
        const StringD after = str.rsplit(sep, &before);

        if(first == std::string::npos)
        {
            STDROMANO_FUZZ_CHECK_EQ(to_std(left), input);
            STDROMANO_FUZZ_CHECK(right.empty());
            STDROMANO_FUZZ_CHECK_EQ(to_std(after), input);
            STDROMANO_FUZZ_CHECK(before.empty());
        }
        else
        {
            STDROMANO_FUZZ_CHECK_EQ(to_std(left), input.substr(0, first));
            STDROMANO_FUZZ_CHECK_EQ(to_std(right), input.substr(first + separator.size()));
            STDROMANO_FUZZ_CHECK_EQ(to_std(after), input.substr(last + separator.size()));
            STDROMANO_FUZZ_CHECK_EQ(to_std(before), input.substr(0, last));
        }

        const StringD view = str.substr(0, source.range<std::size_t>(0, input.size()));
        std::size_t view_parts = 0;
        StringD::split_iterator view_it = 0;

        while(view.split(sep, view_it, part))
        {
            STDROMANO_FUZZ_CHECK(part.data() + part.size() <= view.data() + view.size());
            ++view_parts;
        }

        STDROMANO_FUZZ_CHECK(view_parts <= view.size() + 1);

        return true;
    });

    TESTS_REQUIRE_PROPERTY(report);
}

STDROMANO_TEST_CASE(fuzz_strip_matches_manual_trim)
{
    const auto report = fuzz::run_property(fixtures::options("string_strip", 500), [](fuzz::Source& source) {
        const std::string input = to_std(source.string(32, " #a"));
        const char c = source.pick({' ', '#'});

        const std::size_t first = input.find_first_not_of(c);
        const std::size_t last = input.find_last_not_of(c);

        const std::string lstripped = first == std::string::npos ? "" : input.substr(first);
        const std::string rstripped = last == std::string::npos ? "" : input.substr(0, last + 1);
        const std::string stripped = first == std::string::npos ? "" : input.substr(first, last - first + 1);

        const StringD str = from_std(input);

        STDROMANO_FUZZ_CHECK_EQ(to_std(str.lstrip(c)), lstripped);
        STDROMANO_FUZZ_CHECK_EQ(to_std(str.rstrip(c)), rstripped);
        STDROMANO_FUZZ_CHECK_EQ(to_std(str.strip(c)), stripped);

        return true;
    });

    TESTS_REQUIRE_PROPERTY(report);
}

STDROMANO_TEST_CASE(fuzz_edits_match_std_string)
{
    const auto report = fuzz::run_property(fixtures::options("string_edits", 500), [](fuzz::Source& source) {
        StringD str;
        std::string reference;

        const std::size_t steps = source.range<std::size_t>(1, 60);

        for(std::size_t step = 0; step < steps; ++step)
        {
            const std::string chunk = to_std(source.string(40, "xyz0123"));

            switch(source.index(7))
            {
                case 0:
                    str.push_back('p');
                    reference.push_back('p');
                    break;
                case 1:
                    str.appendc(chunk.c_str(), chunk.size());
                    reference += chunk;
                    break;
                case 2:
                    str.prepends(from_std(chunk));
                    reference.insert(0, chunk);
                    break;
                case 3:
                {
                    const std::size_t position = source.range<std::size_t>(0, reference.size());
                    str.inserts(position, from_std(chunk));
                    reference.insert(position, chunk);
                    break;
                }
                case 4:
                {
                    const std::size_t start = source.range<std::size_t>(0, reference.size());
                    const std::size_t length = source.range<std::size_t>(0, reference.size() - start);
                    str.erase(start, length);
                    reference.erase(start, length);
                    break;
                }
                case 5:
                {
                    StringD copy = str;
                    str = std::move(copy);
                    break;
                }
                default:
                    str.appendf("{}", step);
                    reference += std::to_string(step);
                    break;
            }

            STDROMANO_FUZZ_CHECK_EQ(str.size(), reference.size());
            STDROMANO_FUZZ_CHECK_EQ(str.c_str()[str.size()], '\0');
        }

        STDROMANO_FUZZ_CHECK_EQ(to_std(str), reference);

        return true;
    });

    TESTS_REQUIRE_PROPERTY(report);
}

STDROMANO_TEST_CASE(fuzz_integer_conversions_round_trip)
{
    const auto report = fuzz::run_property(fixtures::options("string_integers", 1000), [](fuzz::Source& source) {
        const long long value = source.integer<long long>();
        const StringD str = to_string(value);

        STDROMANO_FUZZ_CHECK_EQ(to_std(str), std::to_string(value));
        STDROMANO_FUZZ_CHECK_EQ(str.to_long_long(), value);

        return true;
    });

    TESTS_REQUIRE_PROPERTY(report);
}

STDROMANO_TEST_CASE(fuzz_three_way_strcmp)
{
    const auto report = fuzz::run_property(fixtures::options("string_strcmp", 1000), [](fuzz::Source& source) {
        const std::string lhs = to_std(source.string(8, "aAbB"));
        const std::string rhs = to_std(source.string(8, "aAbB"));

        const int expected = lhs < rhs ? -1 : (lhs == rhs ? 0 : 1);

        STDROMANO_FUZZ_CHECK_EQ(strcmp(from_std(lhs), from_std(rhs)), expected);

        std::string lhs_lower = lhs;
        std::string rhs_lower = rhs;
        std::transform(lhs_lower.begin(), lhs_lower.end(), lhs_lower.begin(), ::tolower);
        std::transform(rhs_lower.begin(), rhs_lower.end(), rhs_lower.begin(), ::tolower);

        const int expected_ci = lhs_lower < rhs_lower ? -1 : (lhs_lower == rhs_lower ? 0 : 1);

        STDROMANO_FUZZ_CHECK_EQ(strcmp(from_std(lhs), from_std(rhs), false), expected_ci);

        return true;
    });

    TESTS_REQUIRE_PROPERTY(report);
}

STDROMANO_TEST_CASE(fuzz_utf8_validation_matches_reference)
{
    const auto report = fuzz::run_property(fixtures::options("utf8_validation", 2000), [](fuzz::Source& source) {
        const std::string bytes = random_utf8_bytes(source, 80);
        const bool expected = reference_validate_utf8(reinterpret_cast<const std::uint8_t*>(bytes.data()),
                                                      bytes.size());

        bool ok = true;

        fixtures::for_each_vectorization_mode([&](std::uint32_t) {
            ok &= validate_utf8(bytes.data(), bytes.size()) == expected;
        });

        return ok;
    });

    TESTS_REQUIRE_PROPERTY(report);
}

STDROMANO_TEST_CASE(fuzz_utf8_iterator_decodes_valid_input)
{
    const auto report = fuzz::run_property(fixtures::options("utf8_iterator", 1000), [](fuzz::Source& source) {
        std::vector<char32_t> code_points;
        std::string bytes;

        const std::size_t count = source.size(64);

        for(std::size_t i = 0; i < count; ++i)
        {
            code_points.push_back(valid_code_point(source));
            encode_utf8(code_points.back(), bytes);
        }

        const StringD str = from_std(bytes);

        STDROMANO_FUZZ_CHECK_EQ(str.u8length(), code_points.size());

        std::size_t index = 0;

        for(auto it = str.u8begin(); it != str.u8end(); ++it, ++index)
            STDROMANO_FUZZ_CHECK_EQ(static_cast<std::uint32_t>(*it), static_cast<std::uint32_t>(code_points[index]));

        STDROMANO_FUZZ_CHECK_EQ(index, code_points.size());

        auto it = str.u8end();

        for(std::size_t i = code_points.size(); i > 0; --i)
        {
            --it;
            STDROMANO_FUZZ_CHECK_EQ(static_cast<std::uint32_t>(*it), static_cast<std::uint32_t>(code_points[i - 1]));
        }

        STDROMANO_FUZZ_CHECK(it == str.u8begin());

        return true;
    });

    TESTS_REQUIRE_PROPERTY(report);
}

STDROMANO_TEST_MAIN()
