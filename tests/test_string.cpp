// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - Present Romain Augier
// All rights reserved.

#include "stdromano/string.hpp"
#include "stdromano/simd.hpp"
#include "test.hpp"

#define STDROMANO_ENABLE_PROFILING
#include "stdromano/profiling.hpp"

#include "spdlog/spdlog.h"

using namespace stdromano;

String<> create_large_string(std::size_t size)
{
    String<> result;

    for(std::size_t i = 0; i < size; ++i)
        result.push_back(static_cast<char>('A' + (i % 26)));

    return result;
}

TEST_CASE(test_construction)
{
    String<> empty_str;
    ASSERT_EQUAL(0, empty_str.size());
    ASSERT(empty_str.empty());
    ASSERT_EQUAL(0, std::strcmp(empty_str.c_str(), ""));

    String<> str("Test");
    ASSERT_EQUAL(4, str.size());
    ASSERT(!str.empty());
    ASSERT_EQUAL(0, std::strcmp(str.c_str(), "Test"));

    String<> fmt_str("{} World", "Hello");
    ASSERT_EQUAL(0, std::strcmp(fmt_str.c_str(), "Hello World"));
}

TEST_CASE(test_make_ref)
{
    const char* raw = "Reference";
    String<> ref = String<>::make_ref(raw, std::strlen(raw));
    ASSERT(ref.is_ref());
    ASSERT_EQUAL(0, std::strcmp(ref.c_str(), "Reference"));
    ASSERT_EQUAL(std::strlen(raw), ref.size());

    String<> str("Hello");
    String<> ref_from_str = String<>::make_ref(str);
    ASSERT(ref_from_str.is_ref());
    ASSERT_EQUAL(0, std::strcmp(ref_from_str.c_str(), "Hello"));
    ASSERT_EQUAL(str.size(), ref_from_str.size());
}

TEST_CASE(test_comparison)
{
    String<> str1("Hello");
    String<> str2("Hello");
    String<> str3("World");

    ASSERT(str1 == str2);
    ASSERT(!(str1 == str3));
    ASSERT(!(str1 != str2));
    ASSERT(str1 != str3);

    String<> empty_str;
    ASSERT(!(empty_str == str1));
    ASSERT(empty_str == empty_str);

    StringD large = create_large_string(4096);
    StringD large_copy = large.copy();

    StringD large2 = large.copy();
    large2[2048] = ' ';

    for(std::uint32_t mode = VectorizationMode_Scalar; mode < VectorizationMode_Max; ++mode)
    {
        simd_force_vectorization_mode(mode);

        spdlog::debug("strcmp vectorization mode: {}", simd_get_vectorization_mode_as_string());

        for(std::size_t i = 0; i < 10; ++i)
        {
            SCOPED_PROFILE_START(ProfileUnit::Cycles, large_str_cmp_impl);
            ASSERT(large == large_copy);
            ASSERT(large != large2);
            SCOPED_PROFILE_STOP(large_str_cmp_impl);

            SCOPED_PROFILE_START(ProfileUnit::Cycles, large_str_cmp_memcmp);
            ASSERT(std::memcmp(large.c_str(), large_copy.c_str(), large.size()) == 0);
            ASSERT(std::memcmp(large.c_str(), large2.c_str(), large.size()) != 0);
            SCOPED_PROFILE_STOP(large_str_cmp_memcmp);
        }
    }
}

TEST_CASE(test_pushback)
{
    String<> str;
    str.push_back('A');
    ASSERT_EQUAL(1, str.size());
    ASSERT_EQUAL(0, std::strcmp(str.c_str(), "A"));

    for(size_t i = 0; i < 10; ++i)
    {
        str.push_back('B');
    }

    ASSERT_EQUAL(11, str.size());
    ASSERT_EQUAL(0, std::strcmp(str.c_str(), "ABBBBBBBBBB"));
}

TEST_CASE(test_append_prepend)
{
    String<> str("Middle");

    str.appendc("End");
    ASSERT_EQUAL(0, std::strcmp(str.c_str(), "MiddleEnd"));

    str.prependc("Start");
    ASSERT_EQUAL(0, std::strcmp(str.c_str(), "StartMiddleEnd"));

    String<> other("More");
    str.appends(other);
    ASSERT_EQUAL(0, std::strcmp(str.c_str(), "StartMiddleEndMore"));

    String<> prefix("Pre");
    str.prepends(prefix);
    ASSERT_EQUAL(0, std::strcmp(str.c_str(), "PreStartMiddleEndMore"));

    str.appendf(" {}", "Formatted");
    ASSERT_EQUAL(0, std::strcmp(str.c_str(), "PreStartMiddleEndMore Formatted"));

    str.prependf("Before ");
    ASSERT_EQUAL(0, std::strcmp(str.c_str(), "Before PreStartMiddleEndMore Formatted"));

    String<> another = String<>("Middle").prependc("Start").appendf("{}", "End");
    ASSERT_EQUAL(0, std::strcmp(another.c_str(), "StartMiddleEnd"));

}

TEST_CASE(test_case_conversion)
{
    String<> str("Hello World");

    String<> upper = str.upper();
    ASSERT_EQUAL(0, std::strcmp(upper.c_str(), "HELLO WORLD"));

    String<> lower = str.lower();
    ASSERT_EQUAL(0, std::strcmp(lower.c_str(), "hello world"));

    String<> cap = str.capitalize();
    ASSERT_EQUAL(0, std::strcmp(cap.c_str(), "Hello world"));

    String<> empty_str;
    ASSERT_EQUAL(0, std::strcmp(empty_str.upper().c_str(), ""));
    ASSERT_EQUAL(0, std::strcmp(empty_str.lower().c_str(), ""));
    ASSERT_EQUAL(0, std::strcmp(empty_str.capitalize().c_str(), ""));
}

TEST_CASE(test_strip)
{
    String<> str("  Hello World  ");

    String<> stripped = str.strip();
    ASSERT_EQUAL(0, std::strncmp(stripped.c_str(), "Hello World", stripped.size()));

    String<> lstripped = str.lstrip();
    ASSERT_EQUAL(0, std::strncmp(lstripped.c_str(), "Hello World  ", stripped.size()));

    String<> rstripped = str.rstrip();
    ASSERT_EQUAL(0, std::strncmp(rstripped.c_str(), "  Hello World", stripped.size()));

    String<> empty_str;
    ASSERT_EQUAL(0, std::strcmp(empty_str.strip().c_str(), ""));

    String<> space_str("    ");
    String<> space_strip = space_str.strip();
    ASSERT_EQUAL(0, std::strncmp(space_strip.c_str(), "", space_strip.size()));

    String<> str_custom("###Hello###");
    String<> stripped_custom = str_custom.strip('#');
    ASSERT_EQUAL(0, std::strncmp(stripped_custom.c_str(), "Hello", stripped_custom.size()));

    String<> str_custom_ref = String<>::make_ref(str_custom);
    String<> stripped_custom_ref = str_custom.strip('#');
    ASSERT_EQUAL(0, std::strncmp(stripped_custom_ref.c_str(), "Hello", stripped_custom_ref.size()));

    String<> str_hello_ref = String<>::make_ref("Hello", 5);
    String<> stripped_hello_ref = str_hello_ref.strip('#');
    ASSERT_EQUAL(0, std::strncmp(stripped_hello_ref.c_str(), "Hello", stripped_hello_ref.size()));
}

TEST_CASE(test_startswith_endswith)
{
    String<> str("Hello World");

    ASSERT(str.startswith("Hello"));
    ASSERT(!str.startswith("World"));

    ASSERT(str.endswith("World"));
    ASSERT(!str.endswith("Hello"));

    String<> empty_str;
    ASSERT(!empty_str.startswith("a"));
    ASSERT(!empty_str.endswith("a"));
    ASSERT(str.startswith(""));
    ASSERT(str.endswith(""));

    ASSERT(!str.startswith("Hello World Long"));
    ASSERT(!str.endswith("Long Hello World"));
}

TEST_CASE(test_fin)
{
    String<> str("Hello Hello World");

    ASSERT_EQUAL(0, str.find("Hello"));
    ASSERT_EQUAL(12, str.find("World"));
    ASSERT_EQUAL(-1, str.find("Missing"));

    ASSERT_EQUAL(0, str.find(""));
    ASSERT_EQUAL(-1, str.find("TooLongForTheString"));
    String<> empty_str;
    ASSERT_EQUAL(-1, empty_str.find("a"));
}

TEST_CASE(test_split)
{
    String<> str("Hello,World,Test");
    String<> sep(",");
    String<> split;
    String<>::split_iterator it = 0;

    ASSERT(str.split(sep, it, split));
    ASSERT_EQUAL(0, std::strncmp(split.c_str(), "Hello", split.size()));
    ASSERT_EQUAL(6, it);

    ASSERT(str.split(sep, it, split));
    ASSERT_EQUAL(0, std::strncmp(split.c_str(), "World", split.size()));
    ASSERT_EQUAL(12, it);

    ASSERT(str.split(sep, it, split));
    ASSERT_EQUAL(0, std::strncmp(split.c_str(), "Test", split.size()));
    ASSERT_EQUAL(str.size(), it);

    ASSERT(!str.split(sep, it, split));
    ASSERT_EQUAL(0, std::strncmp(split.c_str(), "", split.size()));
    ASSERT_EQUAL(str.size(), it);

    String<> empty;
    it = 0;
    ASSERT(!empty.split(sep, it, split));
    ASSERT_EQUAL(0, std::strcmp(split.c_str(), ""));
    ASSERT_EQUAL(0, it);

    String<> str2("Hello");
    it = 0;
    ASSERT(!str2.split(String<>(), it, split));
    ASSERT_EQUAL(0, std::strcmp(split.c_str(), "Hello"));
    ASSERT_EQUAL(str2.size(), it);

    String<> str3("Hello");
    it = 0;
    ASSERT(str3.split(String<>(";"), it, split));
    ASSERT_EQUAL(0, std::strcmp(split.c_str(), "Hello"));
    ASSERT_EQUAL(str3.size(), it);

    String<> lrsplit;
    String<> lsplit = str.lsplit(",", &lrsplit);
    ASSERT_EQUAL(0, std::strncmp(lsplit.c_str(), "Hello", lsplit.size()));
    ASSERT_EQUAL(0, std::strncmp(lrsplit.c_str(), "World,Test", lrsplit.size()));

    String<> rlsplit;
    String<> rsplit = str.rsplit(",", &rlsplit);
    ASSERT_EQUAL(0, std::strncmp(rsplit.c_str(), "Test", rsplit.size()));
    ASSERT_EQUAL(0, std::strncmp(rlsplit.c_str(), "Hello,World", rlsplit.size()));
}

TEST_CASE(test_ref_string_restrictions)
{
    const char* raw = "Immutable";
    String<> ref = String<>::make_ref(raw, std::strlen(raw));

    ASSERT(ref.is_ref());
    ASSERT_EQUAL(0, std::strcmp(ref.c_str(), "Immutable"));
    ASSERT_EQUAL(std::strlen(raw), ref.size());
}

TEST_CASE(test_large_string)
{
    String<> large_str = create_large_string(1000);
    ASSERT_EQUAL(1000, large_str.size());

    for(size_t i = 0; i < large_str.size(); ++i)
    {
        ASSERT_EQUAL(static_cast<char>('A' + (i % 26)), large_str.data()[i]);
    }

    ASSERT_EQUAL(0, large_str.data()[large_str.size()]);
}

TEST_CASE(test_copy_constructor)
{
    String<> local("Hello");
    String<> local_copy(local);
    ASSERT_EQUAL(0, std::strcmp(local_copy.c_str(), "Hello"));
    ASSERT_EQUAL(local.size(), local_copy.size());
    ASSERT(!local_copy.is_ref());
    ASSERT(local_copy.capacity() == local.capacity());
    ASSERT(local == local_copy);

    String<> heap = create_large_string(100);
    String<> heap_copy(heap);
    ASSERT_EQUAL(0, std::strncmp(heap_copy.c_str(), heap.c_str(), heap.size()));
    ASSERT_EQUAL(heap.size(), heap_copy.size());
    ASSERT(!heap_copy.is_ref());
    ASSERT(heap_copy.capacity() == heap.capacity());
    ASSERT(heap == heap_copy);

    const char* raw = "Reference";
    String<> ref = String<>::make_ref(raw, std::strlen(raw));
    String<> ref_copy(ref);
    ASSERT_EQUAL(0, std::strcmp(ref_copy.c_str(), "Reference"));
    ASSERT(ref_copy.is_ref());
    ASSERT_EQUAL(ref.size(), ref_copy.size());
    ASSERT(ref == ref_copy);
}

TEST_CASE(test_move_constructor)
{
    String<> local("Hello");
    String<> local_moved(std::move(local));
    ASSERT_EQUAL(0, std::strcmp(local_moved.c_str(), "Hello"));
    ASSERT(local.empty());
    ASSERT_EQUAL(0, local.size());
    ASSERT(!local_moved.is_ref());

    String<> heap = create_large_string(100);
    String<> heap_moved(std::move(heap));
    ASSERT_EQUAL(100, heap_moved.size());
    ASSERT(heap.empty());
    ASSERT_EQUAL(0, heap.size());
    ASSERT(!heap_moved.is_ref());

    const char* raw = "Reference";
    String<> ref = String<>::make_ref(raw, std::strlen(raw));
    String<> ref_moved(std::move(ref));
    ASSERT_EQUAL(0, std::strcmp(ref_moved.c_str(), "Reference"));
    ASSERT(ref_moved.is_ref());
    ASSERT(ref.empty());
    ASSERT_EQUAL(0, ref.size());
}

TEST_CASE(test_copy_assignment)
{
    String<> local("Hello");
    String<> target;
    target = local;
    ASSERT_EQUAL(0, std::strcmp(target.c_str(), "Hello"));
    ASSERT_EQUAL(local.size(), target.size());
    ASSERT(!target.is_ref());
    ASSERT(local == target);

    String<> heap = create_large_string(100);
    String<> target_heap = create_large_string(50);
    target_heap = heap;
    ASSERT_EQUAL(0, std::strncmp(target_heap.c_str(), heap.c_str(), heap.size()));
    ASSERT_EQUAL(heap.size(), target_heap.size());
    ASSERT(!target_heap.is_ref());
    ASSERT(heap == target_heap);

    const char* raw = "Reference";
    String<> ref = String<>::make_ref(raw, std::strlen(raw));
    String<> target_ref = create_large_string(100);
    target_ref = ref;
    ASSERT_EQUAL(0, std::strcmp(target_ref.c_str(), "Reference"));
    ASSERT(target_ref.is_ref());
    ASSERT_EQUAL(ref.size(), target_ref.size());
    ASSERT(ref == target_ref);

    String<> self("Self");
    self = self;
    ASSERT_EQUAL(0, std::strcmp(self.c_str(), "Self"));
}

TEST_CASE(test_move_assignment)
{
    String<> local("Hello");
    String<> target;
    target = std::move(local);
    ASSERT_EQUAL(0, std::strcmp(target.c_str(), "Hello"));
    ASSERT(local.empty());
    ASSERT_EQUAL(0, local.size());
    ASSERT(!target.is_ref());

    String<> heap = create_large_string(100);
    String<> target_heap = create_large_string(50);
    target_heap = std::move(heap);
    ASSERT_EQUAL(100, target_heap.size());
    ASSERT(heap.empty());
    ASSERT_EQUAL(0, heap.size());
    ASSERT(!target_heap.is_ref());

    const char* raw = "Reference";
    String<> ref = String<>::make_ref(raw, std::strlen(raw));
    String<> target_ref = create_large_string(100);
    target_ref = std::move(ref);
    ASSERT_EQUAL(0, std::strcmp(target_ref.c_str(), "Reference"));
    ASSERT(target_ref.is_ref());
    ASSERT(ref.empty());
    ASSERT_EQUAL(0, ref.size());

    String<> self("Self");
    self = std::move(self);
    ASSERT_EQUAL(0, std::strcmp(self.c_str(), "Self"));
}

TEST_CASE(test_zeroed_string)
{
    const String<> zeroed = String<>::make_zeroed(2048);
    ASSERT_EQUAL(2048, zeroed.capacity());
}

TEST_CASE(test_replace)
{
    const String<> s = "this,string,is,sep,by,commas";

    const String<> r_without_commas = s.replace(',', ' ');

    ASSERT_EQUAL(-1, r_without_commas.find(","));
}

TEST_CASE(test_zfill)
{
    const String<> s = "1";

    const String<> zfilled = s.zfill(8);

    ASSERT_EQUAL(0, std::strcmp(zfilled.c_str(), "00000001"));
}

TEST_CASE(test_longlong_conversion)
{
    const String<> s = "1";

    ASSERT_EQUAL(1, s.to_long_long());
}

TEST_CASE(test_double_conversion)
{
    const String<> s = "1.0";

    ASSERT_EQUAL(1.0, s.to_double());
}

TEST_CASE(TestBoolConversion)
{
    const String<> zero = "0";
    const String<> one = "1";
    const String<> _true = "true";
    const String<> _True = "True";
    const String<> _TRUE = "TRUE";
    const String<> _false = "false";
    const String<> _False = "False";
    const String<> _FALSE = "FALSE";

    ASSERT_EQUAL(false, zero.to_bool());
    ASSERT_EQUAL(true, one.to_bool());
    ASSERT_EQUAL(true, _true.to_bool());
    ASSERT_EQUAL(true, _True.to_bool());
    ASSERT_EQUAL(true, _TRUE.to_bool());
    ASSERT_EQUAL(false, _false.to_bool());
    ASSERT_EQUAL(false, _False.to_bool());
    ASSERT_EQUAL(false, _FALSE.to_bool());
}

TEST_CASE(test_substr)
{
    const StringD s = "Hello World!";
    const StringD w = StringD::make_ref("World!");
    const StringD w2 = StringD::make_ref("Wor");

    ASSERT_EQUAL(0, std::strncmp(w.data(), s.substr(6).data(), w.size()));
    ASSERT_EQUAL(0, std::strncmp(w2.data(), s.substr(6, 3).data(), w2.size()));
}

TEST_CASE(test_clear)
{
    StringD s = "Hello World!";

    s.clear();
    ASSERT_EQUAL(0, s.size());
}

TEST_CASE(test_erase)
{
    StringD s = "Hello World!";

    s.erase(5);

    ASSERT_EQUAL(0, std::strncmp(s.c_str(), "Hello", 5));
}

TEST_CASE(test_shrinktofit)
{
    StringD s = "Hello World!";

    const std::size_t old_size = s.size();

    s.shrink_to_fit();

    ASSERT_EQUAL(old_size, s.size());

    s.shrink_to_fit(5);

    ASSERT_EQUAL(0, std::strncmp(s.c_str(), "Hello", 5));
}

TEST_CASE(test_insertion)
{
    String<> str = "Hello World";

    str.insertc(5, " Beautiful");
    ASSERT_EQUAL(0, std::strcmp(str.c_str(), "Hello Beautiful World"));

    String<> insert = " Amazing";
    str.inserts(15, insert);
    ASSERT_EQUAL(0, std::strcmp(str.c_str(), "Hello Beautiful Amazing World"));

    str.insertf(0, "{}: ", 42);
    ASSERT_EQUAL(0, std::strcmp(str.c_str(), "42: Hello Beautiful Amazing World"));
}

TEST_CASE(test_is_digit)
{
    String<> digits = "0123456789";
    String<> no_digits = "abcdef";
    String<> mixed = "0123abcd";

    ASSERT_EQUAL(true, digits.is_digit());
    ASSERT_EQUAL(false, no_digits.is_digit());
    ASSERT_EQUAL(false, mixed.is_digit());
}

TEST_CASE(test_to_string)
{
    ASSERT_EQUAL(0, std::strcmp("1", stdromano::to_string(1).c_str()));
    ASSERT_EQUAL(0, std::strcmp("123456789", stdromano::to_string(123456789).c_str()));
    ASSERT_EQUAL(0, std::strcmp("1.5", stdromano::to_string(1.5).c_str()));
}

struct UTF8TestCase {
    const char* data;
    bool expected_valid;
    const char* description;
};

std::vector<UTF8TestCase> get_utf8_test_cases() {
    return {
        {
            "Hello, world! This is a test string with UTF-8 characters like é, ü, and €.",
            true,
            "Euro symbol"
        },
        {
            "This has an invalid sequence: \xC3" "A",
            false,
            "Invalid 0xC3 without a following continuation byte"
        },
        {
            "This has an overlong sequence: \xC0\xAF",
            false,
            "Invalid overlong encoding of '/'"
        },
        {
            "Start with a continuation byte: \x80",
            false,
            "Invalid continuation byte not preceded by a lead byte"
        }
    };
}

TEST_CASE(test_utf8_validation)
{
    for(const auto& test : get_utf8_test_cases())
    {
        if(validate_utf8(reinterpret_cast<const char*>(test.data),
                         std::strlen(test.data)) != test.expected_valid)
        {
            spdlog::error("Invalid UTF-8 validation: expected {} for case {}",
                          test.expected_valid,
                          test.description);
            STDROMANO_ASSERT(0, "Invalid UTF-8 validation");
        }
    }
}

TEST_CASE(test_utf8_iterator)
{
    /* 1. ASCII-only string */
    {
        StringD s("Hello");
        ASSERT(s.u8length() == 5);

        auto it = s.u8begin();
        ASSERT(*it == U'H'); ++it;
        ASSERT(*it == U'e'); ++it;
        ASSERT(*it == U'l'); ++it;
        ASSERT(*it == U'l'); ++it;
        ASSERT(*it == U'o'); ++it;
        ASSERT(it == s.u8end());
    }

    /* 2. Empty string */
    {
        StringD s;
        ASSERT(s.u8length() == 0);
        ASSERT(s.u8begin() == s.u8end());
    }

    /* 3. 2-byte sequences (Latin accents) — "café" = 4 codepoints, 5 bytes */
    {
        StringD s("caf\xC3\xA9"); /* café */
        ASSERT(s.size() == 5);
        ASSERT(s.u8length() == 4);

        auto it = s.u8begin();
        ASSERT(*it == U'c'); ++it;
        ASSERT(*it == U'a'); ++it;
        ASSERT(*it == U'f'); ++it;
        ASSERT(*it == U'é'); ++it;
        ASSERT(it == s.u8end());
    }

    /* 4. 3-byte sequences (CJK / Euro sign) — "€" = U+20AC */
    {
        StringD s("\xE2\x82\xAC"); /* € */
        ASSERT(s.size() == 3);
        ASSERT(s.u8length() == 1);
        ASSERT(*s.u8begin() == U'\u20AC');
    }

    /* 5. 4-byte sequences (emoji) — U+1F600 😀 */
    {
        StringD s("\xF0\x9F\x98\x80"); /* 😀 */
        ASSERT(s.size() == 4);
        ASSERT(s.u8length() == 1);
        ASSERT(*s.u8begin() == U'\U0001F600');
    }

    /* 6. Mixed byte lengths — "Héllo 🌍" */
    {
        /* H(1) é(2) l(1) l(1) o(1) ' '(1) 🌍(4) = 7 codepoints, 11 bytes */
        StringD s("H\xC3\xA9llo \xF0\x9F\x8C\x8D");
        ASSERT(s.size() == 11);
        ASSERT(s.u8length() == 7);

        const char32_t expected[] = { U'H', U'é', U'l', U'l', U'o', U' ', U'\U0001F30D' };

        std::size_t idx = 0;

        for(auto it = s.u8begin(); it != s.u8end(); ++it, ++idx)
        {
            ASSERT(*it == expected[idx]);
        }

        ASSERT(idx == 7);
    }

    /* 7. byte_length() per codepoint */
    {
        StringD s("A\xC3\xA9\xE2\x82\xAC\xF0\x9F\x98\x80"); /* A é € 😀 */
        auto it = s.u8begin();
        ASSERT(it.byte_length() == 1); ++it; /* A */
        ASSERT(it.byte_length() == 2); ++it; /* é */
        ASSERT(it.byte_length() == 3); ++it; /* € */
        ASSERT(it.byte_length() == 4); ++it; /* 😀 */
        ASSERT(it == s.u8end());
    }

    /* 8. base() returns correct raw pointer offset */
    {
        StringD s("ab\xC3\xA9"); /* a b é */
        auto it = s.u8begin();
        ASSERT(it.base() == s.data());     ++it;
        ASSERT(it.base() == s.data() + 1); ++it;
        ASSERT(it.base() == s.data() + 2); ++it;
        ASSERT(it.base() == s.data() + 4); /* past the 2-byte é */
        ASSERT(it == s.u8end());
    }

    /* 9. Post-increment */
    {
        StringD s("ab");
        auto it = s.u8begin();
        auto prev = it++;
        ASSERT(*prev == U'a');
        ASSERT(*it == U'b');
    }

    /* 10. Backward iteration (--) */
    {
        StringD s("A\xC3\xA9\xE2\x82\xAC"); /* A é € = 3 codepoints */
        auto it = s.u8end();

        --it; ASSERT(*it == U'\u20AC'); /* € */
        --it; ASSERT(*it == U'é');
        --it; ASSERT(*it == U'A');
        ASSERT(it == s.u8begin());
    }

    /* 11. Post-decrement */
    {
        StringD s("ab");
        auto it = s.u8end();
        auto prev = it--;
        ASSERT(prev == s.u8end());
        ASSERT(*it == U'b');
    }

    /* 12. Comparison operators */
    {
        StringD s("abc");
        auto a = s.u8begin();
        auto b = s.u8begin();
        ++b;

        ASSERT(a < b);
        ASSERT(b > a);
        ASSERT(a <= b);
        ASSERT(b >= a);
        ASSERT(a <= a);
        ASSERT(a >= a);
        ASSERT(!(a == b));
        ASSERT(a != b);
    }

    /* 13. Range-for via a small wrapper (verifies begin/end contract) */
    {
        StringD s("Hi\xF0\x9F\x91\x8B"); /* Hi👋 */
        std::size_t count = 0;
        char32_t last = 0;

        /* Manual loop since u8begin/u8end aren't called begin/end */
        for(auto it = s.u8begin(); it != s.u8end(); ++it)
        {
            last = *it;
            ++count;
        }

        ASSERT(count == 3);
        ASSERT(last == U'\U0001F44B'); /* 👋 */
    }

    /* 14. Works with local (SSO) and heap strings */
    {
        /* Small string — should stay local */
        String<7> small("abc");
        ASSERT(small.u8length() == 3);

        /* Large string — should heap-allocate */
        String<7> big("abcdefghijklmnop");
        ASSERT(big.u8length() == 16);
    }
}

int main()
{
    TestRunner runner;

    runner.add_test("Construction", test_construction);
    runner.add_test("MakeRef", test_make_ref);
    runner.add_test("Comparison", test_comparison);
    runner.add_test("PushBack", test_pushback);
    runner.add_test("AppendPrepend", test_append_prepend);
    runner.add_test("CaseConversion", test_case_conversion);
    runner.add_test("Strip", test_strip);
    runner.add_test("StartsWithEndsWith", test_startswith_endswith);
    runner.add_test("Find", test_fin);
    runner.add_test("Split", test_split);
    runner.add_test("ReferenceStringRestrictions", test_ref_string_restrictions);
    runner.add_test("LargeString", test_large_string);
    runner.add_test("CopyConstructor", test_copy_constructor);
    runner.add_test("MoveConstructor", test_move_constructor);
    runner.add_test("CopyAssignment", test_copy_assignment);
    runner.add_test("MoveAssignment", test_move_assignment);
    runner.add_test("ZeroedString", test_zeroed_string);
    runner.add_test("Replace", test_replace);
    runner.add_test("ZFill", test_zfill);
    runner.add_test("LongLongConversion", test_longlong_conversion);
    runner.add_test("DoubleConversion", test_double_conversion);
    runner.add_test("SubStr", test_substr);
    runner.add_test("Clear", test_clear);
    runner.add_test("Erase", test_erase);
    runner.add_test("ShrinkToFit", test_shrinktofit);
    runner.add_test("Insertion", test_insertion);
    runner.add_test("IsDigit", test_is_digit);
    runner.add_test("ToString", test_to_string);
    runner.add_test("UTF-8 Validation", test_utf8_validation);
    runner.add_test("UTF-8 Iterator", test_utf8_iterator);

    runner.run_all();

    return 0;
}
