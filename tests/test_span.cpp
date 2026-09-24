// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - Present Romain Augier
// All rights reserved.

#include "stdromano/span.hpp"
#include "stdromano/vector.hpp"
#include "stdromano/stackvector.hpp"

#include "fixtures.hpp"

#include <cstring>

#include <array>
#include <string>
#include <vector>
#include <type_traits>

STDROMANO_TEST_CASE(default_constructor)
{
    stdromano::Span<int> span;
    STDROMANO_CHECK_EQ(span.size(), 0u);
    STDROMANO_CHECK_EQ(span.data(), nullptr);
    STDROMANO_CHECK(span.empty());
}

STDROMANO_TEST_CASE(pointer_and_size_constructor)
{
    int arr[] = {1, 2, 3, 4, 5};
    stdromano::Span<int> span(arr, 5);
    STDROMANO_CHECK_EQ(span.size(), 5u);
    STDROMANO_CHECK_EQ(arr, span.data());
    STDROMANO_CHECK_EQ(span[0], 1);
    STDROMANO_CHECK_EQ(span[4], 5);
}

STDROMANO_TEST_CASE(pointer_range_constructor)
{
    int arr[] = {10, 20, 30};
    stdromano::Span<int> span(arr, arr + 3);
    STDROMANO_CHECK_EQ(span.size(), 3u);
    STDROMANO_CHECK_EQ(arr, span.data());
    STDROMANO_CHECK_EQ(span.front(), 10);
    STDROMANO_CHECK_EQ(span.back(), 30);
}

STDROMANO_TEST_CASE(raw_array_constructor)
{
    int arr[] = {7, 8, 9};
    stdromano::Span<int> span(arr);
    STDROMANO_CHECK_EQ(span.size(), 3u);
    STDROMANO_CHECK_EQ(arr, span.data());
    STDROMANO_CHECK_EQ(span[0], 7);
    STDROMANO_CHECK_EQ(span[2], 9);
}

STDROMANO_TEST_CASE(std_array_constructor_mutable)
{
    std::array<int, 4> arr = {1, 2, 3, 4};
    stdromano::Span<int> span(arr);
    STDROMANO_CHECK_EQ(span.size(), 4u);
    STDROMANO_CHECK_EQ(arr.data(), span.data());
    STDROMANO_CHECK_EQ(span.front(), 1);
    STDROMANO_CHECK_EQ(span.back(), 4);

    span[0] = 42;
    STDROMANO_CHECK_EQ(arr[0], 42);
}

STDROMANO_TEST_CASE(std_array_constructor_const)
{
    const std::array<int, 3> arr = {5, 6, 7};
    stdromano::Span<const int> span(arr);
    STDROMANO_CHECK_EQ(span.size(), 3u);
    STDROMANO_CHECK_EQ(arr.data(), span.data());
    STDROMANO_CHECK_EQ(span[0], 5);
    STDROMANO_CHECK_EQ(span[2], 7);
}

STDROMANO_TEST_CASE(container_constructor_std_vector)
{
    std::vector<int> vec = {10, 20, 30, 40};
    stdromano::Span<int> span(vec);
    STDROMANO_CHECK_EQ(span.size(), 4u);
    STDROMANO_CHECK_EQ(vec.data(), span.data());
    STDROMANO_CHECK_EQ(span[0], 10);
    STDROMANO_CHECK_EQ(span[3], 40);

    span[1] = 99;
    STDROMANO_CHECK_EQ(vec[1], 99);
}

STDROMANO_TEST_CASE(container_constructor_const_std_vector)
{
    const std::vector<int> vec = {1, 2, 3};
    stdromano::Span<const int> span(vec);
    STDROMANO_CHECK_EQ(span.size(), 3u);
    STDROMANO_CHECK_EQ(vec.data(), span.data());
    STDROMANO_CHECK_EQ(span[0], 1);
    STDROMANO_CHECK_EQ(span[2], 3);
}

STDROMANO_TEST_CASE(container_constructor_std_string)
{
    std::string str = "hello";
    stdromano::Span<char> span(str);
    STDROMANO_CHECK_EQ(span.size(), 5u);
    STDROMANO_CHECK_EQ(str.data(), span.data());
    STDROMANO_CHECK_EQ(span[0], 'h');
    STDROMANO_CHECK_EQ(span[4], 'o');

    span[0] = 'H';
    STDROMANO_CHECK_EQ(str[0], 'H');
}

STDROMANO_TEST_CASE(container_constructor_stdromano_vector)
{
    stdromano::Vector<int> vec;
    vec.push_back(1);
    vec.push_back(2);
    vec.push_back(3);

    stdromano::Span<int> span(vec);
    STDROMANO_CHECK_EQ(span.size(), 3u);
    STDROMANO_CHECK_EQ(vec.data(), span.data());
    STDROMANO_CHECK_EQ(span[0], 1);
    STDROMANO_CHECK_EQ(span[2], 3);
}

STDROMANO_TEST_CASE(container_constructor_stdromano_stackvector)
{
    stdromano::StackVector<int, 8> vec;
    vec.push_back(10);
    vec.push_back(20);

    stdromano::Span<int> span(vec);
    STDROMANO_CHECK_EQ(span.size(), 2u);
    STDROMANO_CHECK_EQ(vec.data(), span.data());
    STDROMANO_CHECK_EQ(span[0], 10);
    STDROMANO_CHECK_EQ(span[1], 20);
}

STDROMANO_TEST_CASE(copy_and_assignment)
{
    int arr[] = {1, 2, 3};
    stdromano::Span<int> span1(arr, 3);

    stdromano::Span<int> span2(span1);
    STDROMANO_CHECK_EQ(span1.size(), span2.size());
    STDROMANO_CHECK_EQ(span1.data(), span2.data());

    stdromano::Span<int> span3;
    span3 = span1;
    STDROMANO_CHECK_EQ(span1.size(), span3.size());
    STDROMANO_CHECK_EQ(span1.data(), span3.data());
}

STDROMANO_TEST_CASE(const_conversion)
{
    int arr[] = {1, 2, 3};
    stdromano::Span<int> mutable_span(arr, 3);
    stdromano::Span<const int> const_span(mutable_span);

    STDROMANO_CHECK_EQ(mutable_span.size(), const_span.size());
    STDROMANO_CHECK_EQ(mutable_span.data(), const_span.data());
    STDROMANO_CHECK_EQ(const_span[0], 1);
}

STDROMANO_TEST_CASE(deduction_guides)
{
    int arr[] = {1, 2, 3};
    stdromano::Span span1(arr);
    static_assert(std::is_same_v<decltype(span1), stdromano::Span<int>>);
    STDROMANO_CHECK_EQ(span1.size(), 3u);

    std::array<int, 2> stdarr = {4, 5};
    stdromano::Span span2(stdarr);
    static_assert(std::is_same_v<decltype(span2), stdromano::Span<int>>);
    STDROMANO_CHECK_EQ(span2.size(), 2u);

    const std::array<int, 2> const_stdarr = {6, 7};
    stdromano::Span span3(const_stdarr);
    static_assert(std::is_same_v<decltype(span3), stdromano::Span<const int>>);
    STDROMANO_CHECK_EQ(span3.size(), 2u);

    int* ptr = arr;
    stdromano::Span span4(ptr, 3);
    static_assert(std::is_same_v<decltype(span4), stdromano::Span<int>>);
    STDROMANO_CHECK_EQ(span4.size(), 3u);

    stdromano::Span span5(arr, arr + 3);
    static_assert(std::is_same_v<decltype(span5), stdromano::Span<int>>);
    STDROMANO_CHECK_EQ(span5.size(), 3u);

    std::vector<int> vec = {8, 9, 10};
    stdromano::Span span6(vec);
    static_assert(std::is_same_v<decltype(span6), stdromano::Span<int>>);
    STDROMANO_CHECK_EQ(span6.size(), 3u);

    const std::vector<int> cvec = {11, 12};
    stdromano::Span span7(cvec);
    static_assert(std::is_same_v<decltype(span7), stdromano::Span<const int>>);
    STDROMANO_CHECK_EQ(span7.size(), 2u);
}

STDROMANO_TEST_CASE(element_access)
{
    int arr[] = {1, 2, 3, 4, 5};
    stdromano::Span<int> span(arr, 5);

    STDROMANO_CHECK_EQ(span[0], 1);
    STDROMANO_CHECK_EQ(span[4], 5);

    STDROMANO_CHECK_EQ(arr, span.at(0));
    STDROMANO_CHECK_EQ(arr + 4, span.at(4));

    STDROMANO_CHECK_EQ(span.front(), 1);
    STDROMANO_CHECK_EQ(span.back(), 5);

    STDROMANO_CHECK_EQ(arr, span.data());

    STDROMANO_CHECK(!span.empty());

    span[0] = 100;
    STDROMANO_CHECK_EQ(arr[0], 100);
}

STDROMANO_TEST_CASE(const_span_element_access)
{
    int arr[] = {1, 2, 3};
    stdromano::Span<const int> span(arr, 3);

    STDROMANO_CHECK_EQ(span[0], 1);
    STDROMANO_CHECK_EQ(span[2], 3);
    STDROMANO_CHECK_EQ(span.front(), 1);
    STDROMANO_CHECK_EQ(span.back(), 3);
    STDROMANO_CHECK_EQ(arr, span.data());
    STDROMANO_CHECK(!span.empty());
}

STDROMANO_TEST_CASE(iterators)
{
    int arr[] = {1, 2, 3, 4};
    stdromano::Span<int> span(arr, 4);

    STDROMANO_CHECK_EQ(arr, span.begin());
    STDROMANO_CHECK_EQ(arr + 4, span.end());

    int sum = 0;
    for (int v : span)
        sum += v;
    STDROMANO_CHECK_EQ(sum, 10);

    STDROMANO_CHECK_EQ(arr, span.cbegin());
    STDROMANO_CHECK_EQ(arr + 4, span.cend());

    STDROMANO_CHECK_EQ(*span.rbegin(), 4);
    STDROMANO_CHECK_EQ(*(span.rend() - 1), 1);

    STDROMANO_CHECK_EQ(*span.crbegin(), 4);
    STDROMANO_CHECK_EQ(*(span.crend() - 1), 1);
}

STDROMANO_TEST_CASE(const_iterators)
{
    int arr[] = {1, 2, 3};
    stdromano::Span<const int> span(arr, 3);

    STDROMANO_CHECK_EQ(arr, span.begin());
    STDROMANO_CHECK_EQ(arr + 3, span.end());

    int sum = 0;
    for (int v : span)
        sum += v;
    STDROMANO_CHECK_EQ(sum, 6);
}

STDROMANO_TEST_CASE(first_last_subspan)
{
    int arr[] = {1, 2, 3, 4, 5};
    stdromano::Span<int> span(arr, 5);

    auto first2 = span.first(2);
    STDROMANO_CHECK_EQ(first2.size(), 2u);
    STDROMANO_CHECK_EQ(first2[0], 1);
    STDROMANO_CHECK_EQ(first2[1], 2);

    auto last2 = span.last(2);
    STDROMANO_CHECK_EQ(last2.size(), 2u);
    STDROMANO_CHECK_EQ(last2[0], 4);
    STDROMANO_CHECK_EQ(last2[1], 5);

    auto sub = span.subspan(1);
    STDROMANO_CHECK_EQ(sub.size(), 4u);
    STDROMANO_CHECK_EQ(sub[0], 2);
    STDROMANO_CHECK_EQ(sub[3], 5);

    auto sub2 = span.subspan(1, 3);
    STDROMANO_CHECK_EQ(sub2.size(), 3u);
    STDROMANO_CHECK_EQ(sub2[0], 2);
    STDROMANO_CHECK_EQ(sub2[2], 4);

    auto sub3 = span.subspan(2, stdromano::Span<int>::npos);
    STDROMANO_CHECK_EQ(sub3.size(), 3u);
    STDROMANO_CHECK_EQ(sub3[0], 3);
}

STDROMANO_TEST_CASE(empty_subviews)
{
    int arr[] = {1, 2, 3};
    stdromano::Span<int> span(arr, 3);

    auto first0 = span.first(0);
    STDROMANO_CHECK_EQ(first0.size(), 0u);
    STDROMANO_CHECK(first0.empty());

    auto last0 = span.last(0);
    STDROMANO_CHECK_EQ(last0.size(), 0u);
    STDROMANO_CHECK(last0.empty());

    auto sub = span.subspan(3);
    STDROMANO_CHECK_EQ(sub.size(), 0u);
    STDROMANO_CHECK(sub.empty());
}

STDROMANO_TEST_CASE(size_bytes)
{
    int arr[] = {1, 2, 3};
    stdromano::Span<int> span(arr, 3);
    STDROMANO_CHECK_EQ(3 * sizeof(int), span.size_bytes());

    char carr[] = {'a', 'b'};
    stdromano::Span<char> cspan(carr, 2);
    STDROMANO_CHECK_EQ(2 * sizeof(char), cspan.size_bytes());

    stdromano::Span<int> empty;
    STDROMANO_CHECK_EQ(empty.size_bytes(), 0u);
}

STDROMANO_TEST_CASE(as_bytes_mutable)
{
    int arr[] = {0x01020304, 0x05060708};
    stdromano::Span<int> span(arr, 2);
    auto bytes = span.as_bytes();

    static_assert(std::is_same_v<decltype(bytes), stdromano::Span<unsigned char>>);
    STDROMANO_CHECK_EQ(span.size_bytes(), bytes.size());
    STDROMANO_CHECK_EQ(reinterpret_cast<unsigned char*>(arr), bytes.data());

    if (bytes.size() >= 4) {
        STDROMANO_CHECK_EQ(static_cast<unsigned char>(0x04), bytes[0]);
        STDROMANO_CHECK_EQ(static_cast<unsigned char>(0x03), bytes[1]);
        STDROMANO_CHECK_EQ(static_cast<unsigned char>(0x02), bytes[2]);
        STDROMANO_CHECK_EQ(static_cast<unsigned char>(0x01), bytes[3]);
    }
}

STDROMANO_TEST_CASE(as_bytes_const)
{
    const int arr[] = {0x11223344};
    stdromano::Span<const int> span(arr, 1);
    auto bytes = span.as_bytes();

    static_assert(std::is_same_v<decltype(bytes), stdromano::Span<const unsigned char>>);
    STDROMANO_CHECK_EQ(span.size_bytes(), bytes.size());
    STDROMANO_CHECK_EQ(reinterpret_cast<const unsigned char*>(arr), bytes.data());

    if (bytes.size() >= 4) {
        STDROMANO_CHECK_EQ(static_cast<unsigned char>(0x44), bytes[0]);
        STDROMANO_CHECK_EQ(static_cast<unsigned char>(0x33), bytes[1]);
        STDROMANO_CHECK_EQ(static_cast<unsigned char>(0x22), bytes[2]);
        STDROMANO_CHECK_EQ(static_cast<unsigned char>(0x11), bytes[3]);
    }
}

STDROMANO_TEST_CASE(make_span_pointer_size)
{
    int arr[] = {1, 2, 3};
    auto span = stdromano::make_span(arr, 3);
    static_assert(std::is_same_v<decltype(span), stdromano::Span<int>>);
    STDROMANO_CHECK_EQ(span.size(), 3u);
    STDROMANO_CHECK_EQ(arr, span.data());
}

STDROMANO_TEST_CASE(make_span_container)
{
    std::vector<int> vec = {10, 20, 30};
    auto span = stdromano::make_span(vec);
    static_assert(std::is_same_v<decltype(span), stdromano::Span<int>>);
    STDROMANO_CHECK_EQ(span.size(), 3u);
    STDROMANO_CHECK_EQ(vec.data(), span.data());
}

STDROMANO_TEST_CASE(make_cspan_const_container)
{
    const std::vector<int> vec = {1, 2, 3};
    auto span = stdromano::make_cspan(vec);
    static_assert(std::is_same_v<decltype(span), stdromano::Span<const int>>);
    STDROMANO_CHECK_EQ(span.size(), 3u);
    STDROMANO_CHECK_EQ(vec.data(), span.data());
}

STDROMANO_TEST_CASE(empty_span_operations)
{
    stdromano::Span<int> span;

    STDROMANO_CHECK_EQ(span.size(), 0u);
    STDROMANO_CHECK_EQ(span.data(), nullptr);
    STDROMANO_CHECK(span.empty());
    STDROMANO_CHECK_EQ(span.size_bytes(), 0u);

    STDROMANO_CHECK_EQ(span.begin(), nullptr);
    STDROMANO_CHECK_EQ(span.end(), nullptr);
    STDROMANO_CHECK_EQ(span.cbegin(), nullptr);
    STDROMANO_CHECK_EQ(span.cend(), nullptr);

    STDROMANO_CHECK(span.rbegin() == span.rend());
    STDROMANO_CHECK(span.crbegin() == span.crend());

    auto first = span.first(0);
    STDROMANO_CHECK(first.empty());
    auto last = span.last(0);
    STDROMANO_CHECK(last.empty());
    auto sub = span.subspan(0);
    STDROMANO_CHECK(sub.empty());
}

STDROMANO_TEST_CASE(compile_time_constraints)
{
    static_assert(std::is_constructible_v<stdromano::Span<const int>, stdromano::Span<int>>);
    static_assert(!std::is_constructible_v<stdromano::Span<int>, stdromano::Span<const int>>);

    static_assert(!std::is_constructible_v<stdromano::Span<int>, const std::vector<int>&>);
    static_assert(std::is_constructible_v<stdromano::Span<const int>, const std::vector<int>&>);

    static_assert(std::is_constructible_v<stdromano::Span<int>, std::vector<int>&>);
    static_assert(std::is_constructible_v<stdromano::Span<const int>, std::vector<int>&>);

    static_assert(!std::is_constructible_v<stdromano::Span<int>, std::vector<int>&&>);
}

STDROMANO_TEST_CASE(fuzz_subviews_match_the_underlying_range)
{
    const auto report = stdromano::fuzz::run_property(fixtures::options("span_subviews", 1000), [](stdromano::fuzz::Source& source) {
        std::vector<std::int32_t> values(source.size(128));

        for(auto& value : values)
            value = source.integer<std::int32_t>();

        const stdromano::Span<const std::int32_t> span(values);

        STDROMANO_FUZZ_CHECK_EQ(span.size(), values.size());
        STDROMANO_FUZZ_CHECK_EQ(span.size_bytes(), values.size() * sizeof(std::int32_t));

        const std::size_t offset = source.range<std::size_t>(0, values.size());
        const std::size_t count = source.range<std::size_t>(0, values.size() - offset);

        const auto sub = span.subspan(offset, count);
        const auto tail = span.subspan(offset);
        const auto first = span.first(count);
        const auto last = span.last(count);

        STDROMANO_FUZZ_CHECK_EQ(sub.size(), count);
        STDROMANO_FUZZ_CHECK_EQ(tail.size(), values.size() - offset);

        for(std::size_t i = 0; i < count; ++i)
        {
            STDROMANO_FUZZ_CHECK_EQ(sub[i], values[offset + i]);
            STDROMANO_FUZZ_CHECK_EQ(first[i], values[i]);
            STDROMANO_FUZZ_CHECK_EQ(last[i], values[values.size() - count + i]);
        }

        std::size_t index = 0;

        for(const std::int32_t value : tail)
            STDROMANO_FUZZ_CHECK_EQ(value, values[offset + index++]);

        const auto bytes = span.as_bytes();
        STDROMANO_FUZZ_CHECK_EQ(bytes.size(), span.size_bytes());
        STDROMANO_FUZZ_CHECK(values.empty() || std::memcmp(bytes.data(), values.data(), bytes.size()) == 0);

        return true;
    });

    TESTS_REQUIRE_PROPERTY(report);
}

STDROMANO_TEST_MAIN()
