// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - Present Romain Augier
// All rights reserved.

#include "stdromano/string.hpp"
#include "test.hpp"

using namespace stdromano;

String<> create_large_string(size_t size)
{
    String<> result;

    for(size_t i = 0; i < size; ++i)
    {
        result.push_back(static_cast<char>('A' + (i % 26)));
    }

    return result;
}

TEST_CASE(TestConstruction)
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

TEST_CASE(TestMakeRef)
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

TEST_CASE(TestComparison)
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
}

TEST_CASE(TestPushBack)
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

TEST_CASE(TestAppendPrepend)
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
}

TEST_CASE(TestCaseConversion)
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

TEST_CASE(TestStrip)
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

TEST_CASE(TestStartsWithEndsWith)
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

TEST_CASE(TestFind)
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

TEST_CASE(TestSplit)
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

TEST_CASE(TestReferenceStringRestrictions)
{
    const char* raw = "Immutable";
    String<> ref = String<>::make_ref(raw, std::strlen(raw));

    ASSERT(ref.is_ref());
    ASSERT_EQUAL(0, std::strcmp(ref.c_str(), "Immutable"));
    ASSERT_EQUAL(std::strlen(raw), ref.size());
}

TEST_CASE(TestLargeString)
{
    String<> large_str = create_large_string(1000);
    ASSERT_EQUAL(1000, large_str.size());

    for(size_t i = 0; i < large_str.size(); ++i)
    {
        ASSERT_EQUAL('A' + (i % 26), large_str.data()[i]);
    }

    ASSERT_EQUAL(0, large_str.data()[large_str.size()]);
}

TEST_CASE(TestCopyConstructor)
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

TEST_CASE(TestMoveConstructor)
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

TEST_CASE(TestCopyAssignment)
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

TEST_CASE(TestMoveAssignment)
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

TEST_CASE(TestZeroedString)
{
    const String<> zeroed = String<>::make_zeroed(2048);
    ASSERT_EQUAL(2048, zeroed.capacity());
}

TEST_CASE(TestReplace)
{
    const String<> s = "this,string,is,sep,by,commas";

    const String<> r_without_commas = s.replace(',', ' ');

    ASSERT_EQUAL(-1, r_without_commas.find(","));
}

TEST_CASE(TestZFill)
{
    const String<> s = "1";

    const String<> zfilled = s.zfill(8);

    ASSERT_EQUAL(0, std::strcmp(zfilled.c_str(), "00000001"));
}

TEST_CASE(TestLongLongConversion)
{
    const String<> s = "1";

    ASSERT_EQUAL(1, s.to_long_long());
}

TEST_CASE(TestDoubleConversion)
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

TEST_CASE(TestSubStr)
{
    const StringD s = "Hello World!";
    const StringD w = StringD::make_ref("World!");
    const StringD w2 = StringD::make_ref("Wor");

    ASSERT_EQUAL(0, std::strncmp(w.data(), s.substr(6).data(), w.size()));
    ASSERT_EQUAL(0, std::strncmp(w2.data(), s.substr(6, 3).data(), w2.size()));
}

TEST_CASE(TestClear)
{
    StringD s = "Hello World!";

    s.clear();
    ASSERT_EQUAL(0, s.size());
}

TEST_CASE(TestErase)
{
    StringD s = "Hello World!";

    s.erase(5);

    ASSERT_EQUAL(0, std::strncmp(s.c_str(), "Hello", 5));
}

TEST_CASE(TestShrinkToFit)
{
    StringD s = "Hello World!";

    const std::size_t old_size = s.size();

    s.shrink_to_fit();

    ASSERT_EQUAL(old_size, s.size());

    s.shrink_to_fit(5);

    ASSERT_EQUAL(0, std::strncmp(s.c_str(), "Hello", 5));
}

TEST_CASE(TestInsertion)
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

TEST_CASE(TestIsDigit)
{
    String<> digits = "0123456789";
    String<> no_digits = "abcdef";
    String<> mixed = "0123abcd";

    ASSERT_EQUAL(true, digits.is_digit());
    ASSERT_EQUAL(false, no_digits.is_digit());
    ASSERT_EQUAL(false, mixed.is_digit());
}

int main()
{
    TestRunner runner;

    runner.add_test("Construction", TestConstruction);
    runner.add_test("MakeRef", TestMakeRef);
    runner.add_test("Comparison", TestComparison);
    runner.add_test("PushBack", TestPushBack);
    runner.add_test("AppendPrepend", TestAppendPrepend);
    runner.add_test("CaseConversion", TestCaseConversion);
    runner.add_test("Strip", TestStrip);
    runner.add_test("StartsWithEndsWith", TestStartsWithEndsWith);
    runner.add_test("Find", TestFind);
    runner.add_test("Split", TestSplit);
    runner.add_test("ReferenceStringRestrictions", TestReferenceStringRestrictions);
    runner.add_test("LargeString", TestLargeString);
    runner.add_test("CopyConstructor", TestCopyConstructor);
    runner.add_test("MoveConstructor", TestMoveConstructor);
    runner.add_test("CopyAssignment", TestCopyAssignment);
    runner.add_test("MoveAssignment", TestMoveAssignment);
    runner.add_test("ZeroedString", TestZeroedString);
    runner.add_test("Replace", TestReplace);
    runner.add_test("ZFill", TestZFill);
    runner.add_test("LongLongConversion", TestLongLongConversion);
    runner.add_test("DoubleConversion", TestDoubleConversion);
    runner.add_test("SubStr", TestSubStr);
    runner.add_test("Clear", TestClear);
    runner.add_test("Erase", TestErase);
    runner.add_test("ShrinkToFit", TestShrinkToFit);
    runner.add_test("Insertion", TestInsertion);
    runner.add_test("IsDigit", TestIsDigit);

    runner.run_all();

    return 0;
}