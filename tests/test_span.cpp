// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - Present Romain Augier
// All rights reserved.

#include "stdromano/span.hpp"
#include "stdromano/vector.hpp"
#include "stdromano/stackvector.hpp"

#include "test.hpp"

#include <array>
#include <string>
#include <vector>
#include <type_traits>

INIT_TEST_OBJECT;

TEST_CASE(test_default_constructor)
{
    stdromano::Span<int> span;
    ASSERT_EQUAL(0u, span.size());
    ASSERT_EQUAL(nullptr, span.data());
    ASSERT(span.empty());
}

TEST_CASE(test_pointer_and_size_constructor)
{
    int arr[] = {1, 2, 3, 4, 5};
    stdromano::Span<int> span(arr, 5);
    ASSERT_EQUAL(5u, span.size());
    ASSERT_EQUAL(arr, span.data());
    ASSERT_EQUAL(1, span[0]);
    ASSERT_EQUAL(5, span[4]);
}

TEST_CASE(test_pointer_range_constructor)
{
    int arr[] = {10, 20, 30};
    stdromano::Span<int> span(arr, arr + 3);
    ASSERT_EQUAL(3u, span.size());
    ASSERT_EQUAL(arr, span.data());
    ASSERT_EQUAL(10, span.front());
    ASSERT_EQUAL(30, span.back());
}

TEST_CASE(test_raw_array_constructor)
{
    int arr[] = {7, 8, 9};
    stdromano::Span<int> span(arr);
    ASSERT_EQUAL(3u, span.size());
    ASSERT_EQUAL(arr, span.data());
    ASSERT_EQUAL(7, span[0]);
    ASSERT_EQUAL(9, span[2]);
}

TEST_CASE(test_std_array_constructor_mutable)
{
    std::array<int, 4> arr = {1, 2, 3, 4};
    stdromano::Span<int> span(arr);
    ASSERT_EQUAL(4u, span.size());
    ASSERT_EQUAL(arr.data(), span.data());
    ASSERT_EQUAL(1, span.front());
    ASSERT_EQUAL(4, span.back());

    // Modify through span
    span[0] = 42;
    ASSERT_EQUAL(42, arr[0]);
}

TEST_CASE(test_std_array_constructor_const)
{
    const std::array<int, 3> arr = {5, 6, 7};
    stdromano::Span<const int> span(arr);
    ASSERT_EQUAL(3u, span.size());
    ASSERT_EQUAL(arr.data(), span.data());
    ASSERT_EQUAL(5, span[0]);
    ASSERT_EQUAL(7, span[2]);
}

TEST_CASE(test_container_constructor_std_vector)
{
    std::vector<int> vec = {10, 20, 30, 40};
    stdromano::Span<int> span(vec);
    ASSERT_EQUAL(4u, span.size());
    ASSERT_EQUAL(vec.data(), span.data());
    ASSERT_EQUAL(10, span[0]);
    ASSERT_EQUAL(40, span[3]);

    // Modify through span
    span[1] = 99;
    ASSERT_EQUAL(99, vec[1]);
}

TEST_CASE(test_container_constructor_const_std_vector)
{
    const std::vector<int> vec = {1, 2, 3};
    stdromano::Span<const int> span(vec);
    ASSERT_EQUAL(3u, span.size());
    ASSERT_EQUAL(vec.data(), span.data());
    ASSERT_EQUAL(1, span[0]);
    ASSERT_EQUAL(3, span[2]);
}

TEST_CASE(test_container_constructor_std_string)
{
    std::string str = "hello";
    stdromano::Span<char> span(str);
    ASSERT_EQUAL(5u, span.size());
    ASSERT_EQUAL(str.data(), span.data());
    ASSERT_EQUAL('h', span[0]);
    ASSERT_EQUAL('o', span[4]);

    // Modify through span
    span[0] = 'H';
    ASSERT_EQUAL('H', str[0]);
}

TEST_CASE(test_container_constructor_stdromano_vector)
{
    stdromano::Vector<int> vec;
    vec.push_back(1);
    vec.push_back(2);
    vec.push_back(3);

    stdromano::Span<int> span(vec);
    ASSERT_EQUAL(3u, span.size());
    ASSERT_EQUAL(vec.data(), span.data());
    ASSERT_EQUAL(1, span[0]);
    ASSERT_EQUAL(3, span[2]);
}

TEST_CASE(test_container_constructor_stdromano_stackvector)
{
    stdromano::StackVector<int, 8> vec;
    vec.push_back(10);
    vec.push_back(20);

    stdromano::Span<int> span(vec);
    ASSERT_EQUAL(2u, span.size());
    ASSERT_EQUAL(vec.data(), span.data());
    ASSERT_EQUAL(10, span[0]);
    ASSERT_EQUAL(20, span[1]);
}

TEST_CASE(test_copy_and_assignment)
{
    int arr[] = {1, 2, 3};
    stdromano::Span<int> span1(arr, 3);

    // Copy constructor
    stdromano::Span<int> span2(span1);
    ASSERT_EQUAL(span1.size(), span2.size());
    ASSERT_EQUAL(span1.data(), span2.data());

    // Copy assignment
    stdromano::Span<int> span3;
    span3 = span1;
    ASSERT_EQUAL(span1.size(), span3.size());
    ASSERT_EQUAL(span1.data(), span3.data());
}

TEST_CASE(test_const_conversion)
{
    int arr[] = {1, 2, 3};
    stdromano::Span<int> mutable_span(arr, 3);
    stdromano::Span<const int> const_span(mutable_span);

    ASSERT_EQUAL(mutable_span.size(), const_span.size());
    ASSERT_EQUAL(mutable_span.data(), const_span.data());
    ASSERT_EQUAL(1, const_span[0]);
}

TEST_CASE(test_deduction_guides)
{
    // Raw array
    int arr[] = {1, 2, 3};
    stdromano::Span span1(arr);
    static_assert(std::is_same_v<decltype(span1), stdromano::Span<int>>);
    ASSERT_EQUAL(3u, span1.size());

    // std::array mutable
    std::array<int, 2> stdarr = {4, 5};
    stdromano::Span span2(stdarr);
    static_assert(std::is_same_v<decltype(span2), stdromano::Span<int>>);
    ASSERT_EQUAL(2u, span2.size());

    // std::array const
    const std::array<int, 2> const_stdarr = {6, 7};
    stdromano::Span span3(const_stdarr);
    static_assert(std::is_same_v<decltype(span3), stdromano::Span<const int>>);
    ASSERT_EQUAL(2u, span3.size());

    // Pointer + size
    int* ptr = arr;
    stdromano::Span span4(ptr, 3);
    static_assert(std::is_same_v<decltype(span4), stdromano::Span<int>>);
    ASSERT_EQUAL(3u, span4.size());

    // Pointer range
    stdromano::Span span5(arr, arr + 3);
    static_assert(std::is_same_v<decltype(span5), stdromano::Span<int>>);
    ASSERT_EQUAL(3u, span5.size());

    // Container
    std::vector<int> vec = {8, 9, 10};
    stdromano::Span span6(vec);
    static_assert(std::is_same_v<decltype(span6), stdromano::Span<int>>);
    ASSERT_EQUAL(3u, span6.size());

    // Const container
    const std::vector<int> cvec = {11, 12};
    stdromano::Span span7(cvec);
    static_assert(std::is_same_v<decltype(span7), stdromano::Span<const int>>);
    ASSERT_EQUAL(2u, span7.size());
}

TEST_CASE(test_element_access)
{
    int arr[] = {1, 2, 3, 4, 5};
    stdromano::Span<int> span(arr, 5);

    // operator[]
    ASSERT_EQUAL(1, span[0]);
    ASSERT_EQUAL(5, span[4]);

    // at()
    ASSERT_EQUAL(arr, span.at(0));
    ASSERT_EQUAL(arr + 4, span.at(4));

    // front/back
    ASSERT_EQUAL(1, span.front());
    ASSERT_EQUAL(5, span.back());

    // data()
    ASSERT_EQUAL(arr, span.data());

    // empty()
    ASSERT(!span.empty());

    // Modifying through span
    span[0] = 100;
    ASSERT_EQUAL(100, arr[0]);
}

TEST_CASE(test_const_span_element_access)
{
    int arr[] = {1, 2, 3};
    stdromano::Span<const int> span(arr, 3);

    ASSERT_EQUAL(1, span[0]);
    ASSERT_EQUAL(3, span[2]);
    ASSERT_EQUAL(1, span.front());
    ASSERT_EQUAL(3, span.back());
    ASSERT_EQUAL(arr, span.data());
    ASSERT(!span.empty());
}

TEST_CASE(test_iterators)
{
    int arr[] = {1, 2, 3, 4};
    stdromano::Span<int> span(arr, 4);

    // begin/end
    ASSERT_EQUAL(arr, span.begin());
    ASSERT_EQUAL(arr + 4, span.end());

    // Range-based for
    int sum = 0;
    for (int v : span)
        sum += v;
    ASSERT_EQUAL(10, sum);

    // cbegin/cend
    ASSERT_EQUAL(arr, span.cbegin());
    ASSERT_EQUAL(arr + 4, span.cend());

    // rbegin/rend
    ASSERT_EQUAL(4, *span.rbegin());
    ASSERT_EQUAL(1, *(span.rend() - 1));

    // crbegin/crend
    ASSERT_EQUAL(4, *span.crbegin());
    ASSERT_EQUAL(1, *(span.crend() - 1));
}

TEST_CASE(test_const_iterators)
{
    int arr[] = {1, 2, 3};
    stdromano::Span<const int> span(arr, 3);

    ASSERT_EQUAL(arr, span.begin());
    ASSERT_EQUAL(arr + 3, span.end());

    int sum = 0;
    for (int v : span)
        sum += v;
    ASSERT_EQUAL(6, sum);
}

TEST_CASE(test_first_last_subspan)
{
    int arr[] = {1, 2, 3, 4, 5};
    stdromano::Span<int> span(arr, 5);

    // first
    auto first2 = span.first(2);
    ASSERT_EQUAL(2u, first2.size());
    ASSERT_EQUAL(1, first2[0]);
    ASSERT_EQUAL(2, first2[1]);

    // last
    auto last2 = span.last(2);
    ASSERT_EQUAL(2u, last2.size());
    ASSERT_EQUAL(4, last2[0]);
    ASSERT_EQUAL(5, last2[1]);

    // subspan with offset
    auto sub = span.subspan(1);
    ASSERT_EQUAL(4u, sub.size());
    ASSERT_EQUAL(2, sub[0]);
    ASSERT_EQUAL(5, sub[3]);

    // subspan with offset and count
    auto sub2 = span.subspan(1, 3);
    ASSERT_EQUAL(3u, sub2.size());
    ASSERT_EQUAL(2, sub2[0]);
    ASSERT_EQUAL(4, sub2[2]);

    // subspan with npos default (all remaining)
    auto sub3 = span.subspan(2, stdromano::Span<int>::npos);
    ASSERT_EQUAL(3u, sub3.size());
    ASSERT_EQUAL(3, sub3[0]);
}

TEST_CASE(test_empty_subviews)
{
    int arr[] = {1, 2, 3};
    stdromano::Span<int> span(arr, 3);

    auto first0 = span.first(0);
    ASSERT_EQUAL(0u, first0.size());
    ASSERT(first0.empty());

    auto last0 = span.last(0);
    ASSERT_EQUAL(0u, last0.size());
    ASSERT(last0.empty());

    auto sub = span.subspan(3);
    ASSERT_EQUAL(0u, sub.size());
    ASSERT(sub.empty());
}

TEST_CASE(test_size_bytes)
{
    int arr[] = {1, 2, 3};
    stdromano::Span<int> span(arr, 3);
    ASSERT_EQUAL(3 * sizeof(int), span.size_bytes());

    char carr[] = {'a', 'b'};
    stdromano::Span<char> cspan(carr, 2);
    ASSERT_EQUAL(2 * sizeof(char), cspan.size_bytes());

    stdromano::Span<int> empty;
    ASSERT_EQUAL(0u, empty.size_bytes());
}

TEST_CASE(test_as_bytes_mutable)
{
    int arr[] = {0x01020304, 0x05060708};
    stdromano::Span<int> span(arr, 2);
    auto bytes = span.as_bytes();

    static_assert(std::is_same_v<decltype(bytes), stdromano::Span<unsigned char>>);
    ASSERT_EQUAL(span.size_bytes(), bytes.size());
    ASSERT_EQUAL(reinterpret_cast<unsigned char*>(arr), bytes.data());

    // Check a known byte (platform-dependent, but typically little-endian)
    if (bytes.size() >= 4) {
        ASSERT_EQUAL(static_cast<unsigned char>(0x04), bytes[0]);
        ASSERT_EQUAL(static_cast<unsigned char>(0x03), bytes[1]);
        ASSERT_EQUAL(static_cast<unsigned char>(0x02), bytes[2]);
        ASSERT_EQUAL(static_cast<unsigned char>(0x01), bytes[3]);
    }
}

TEST_CASE(test_as_bytes_const)
{
    const int arr[] = {0x11223344};
    stdromano::Span<const int> span(arr, 1);
    auto bytes = span.as_bytes();

    static_assert(std::is_same_v<decltype(bytes), stdromano::Span<const unsigned char>>);
    ASSERT_EQUAL(span.size_bytes(), bytes.size());
    ASSERT_EQUAL(reinterpret_cast<const unsigned char*>(arr), bytes.data());

    if (bytes.size() >= 4) {
        ASSERT_EQUAL(static_cast<unsigned char>(0x44), bytes[0]);
        ASSERT_EQUAL(static_cast<unsigned char>(0x33), bytes[1]);
        ASSERT_EQUAL(static_cast<unsigned char>(0x22), bytes[2]);
        ASSERT_EQUAL(static_cast<unsigned char>(0x11), bytes[3]);
    }
}

TEST_CASE(test_make_span_pointer_size)
{
    int arr[] = {1, 2, 3};
    auto span = stdromano::make_span(arr, 3);
    static_assert(std::is_same_v<decltype(span), stdromano::Span<int>>);
    ASSERT_EQUAL(3u, span.size());
    ASSERT_EQUAL(arr, span.data());
}

TEST_CASE(test_make_span_container)
{
    std::vector<int> vec = {10, 20, 30};
    auto span = stdromano::make_span(vec);
    static_assert(std::is_same_v<decltype(span), stdromano::Span<int>>);
    ASSERT_EQUAL(3u, span.size());
    ASSERT_EQUAL(vec.data(), span.data());
}

TEST_CASE(test_make_cspan_const_container)
{
    const std::vector<int> vec = {1, 2, 3};
    auto span = stdromano::make_cspan(vec);
    static_assert(std::is_same_v<decltype(span), stdromano::Span<const int>>);
    ASSERT_EQUAL(3u, span.size());
    ASSERT_EQUAL(vec.data(), span.data());
}

TEST_CASE(test_empty_span_operations)
{
    stdromano::Span<int> span;

    ASSERT_EQUAL(0u, span.size());
    ASSERT_EQUAL(nullptr, span.data());
    ASSERT(span.empty());
    ASSERT_EQUAL(0u, span.size_bytes());

    ASSERT_EQUAL(nullptr, span.begin());
    ASSERT_EQUAL(nullptr, span.end());
    ASSERT_EQUAL(nullptr, span.cbegin());
    ASSERT_EQUAL(nullptr, span.cend());

    // rbegin/rend should also be valid on empty span
    ASSERT(span.rbegin() == span.rend());
    ASSERT(span.crbegin() == span.crend());

    // subviews of empty
    auto first = span.first(0);
    ASSERT(first.empty());
    auto last = span.last(0);
    ASSERT(last.empty());
    auto sub = span.subspan(0);
    ASSERT(sub.empty());
}

TEST_CASE(test_compile_time_constraints)
{
    // Span<const T> is constructible from Span<T>
    static_assert(std::is_constructible_v<stdromano::Span<const int>, stdromano::Span<int>>);
    // Span<T> is NOT constructible from Span<const T>
    static_assert(!std::is_constructible_v<stdromano::Span<int>, stdromano::Span<const int>>);

    // Mutable Span from const container should be disabled
    static_assert(!std::is_constructible_v<stdromano::Span<int>, const std::vector<int>&>);
    // Const Span from const container is allowed
    static_assert(std::is_constructible_v<stdromano::Span<const int>, const std::vector<int>&>);

    // Mutable Span from mutable container allowed
    static_assert(std::is_constructible_v<stdromano::Span<int>, std::vector<int>&>);
    // Const Span from mutable container allowed
    static_assert(std::is_constructible_v<stdromano::Span<const int>, std::vector<int>&>);

    // rvalue deleted
    static_assert(!std::is_constructible_v<stdromano::Span<int>, std::vector<int>&&>);
}

int main()
{
    TestRunner runner("span");

    runner.add_test("Default Constructor", test_default_constructor);
    runner.add_test("Pointer and Size Constructor", test_pointer_and_size_constructor);
    runner.add_test("Pointer Range Constructor", test_pointer_range_constructor);
    runner.add_test("Raw Array Constructor", test_raw_array_constructor);
    runner.add_test("std::array Constructor (mutable)", test_std_array_constructor_mutable);
    runner.add_test("std::array Constructor (const)", test_std_array_constructor_const);
    runner.add_test("Container Constructor std::vector", test_container_constructor_std_vector);
    runner.add_test("Container Constructor const std::vector", test_container_constructor_const_std_vector);
    runner.add_test("Container Constructor std::string", test_container_constructor_std_string);
    runner.add_test("Container Constructor stdromano::Vector", test_container_constructor_stdromano_vector);
    runner.add_test("Container Constructor stdromano::StackVector", test_container_constructor_stdromano_stackvector);
    runner.add_test("Copy and Assignment", test_copy_and_assignment);
    runner.add_test("Const Conversion", test_const_conversion);
    runner.add_test("Deduction Guides", test_deduction_guides);
    runner.add_test("Element Access", test_element_access);
    runner.add_test("Const Span Element Access", test_const_span_element_access);
    runner.add_test("Iterators", test_iterators);
    runner.add_test("Const Iterators", test_const_iterators);
    runner.add_test("First, Last, Subspan", test_first_last_subspan);
    runner.add_test("Empty Subviews", test_empty_subviews);
    runner.add_test("size_bytes", test_size_bytes);
    runner.add_test("as_bytes (mutable)", test_as_bytes_mutable);
    runner.add_test("as_bytes (const)", test_as_bytes_const);
    runner.add_test("make_span pointer+size", test_make_span_pointer_size);
    runner.add_test("make_span container", test_make_span_container);
    runner.add_test("make_cspan const container", test_make_cspan_const_container);
    runner.add_test("Empty Span Operations", test_empty_span_operations);
    runner.add_test("Compile-time Constraints", test_compile_time_constraints);

    runner.run_all();

    return 0;
}