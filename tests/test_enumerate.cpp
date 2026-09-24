// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - Present Romain Augier
// All rights reserved.

#include "stdromano/enumerate.hpp"
#include "stdromano/vector.hpp"

#include "fixtures.hpp"

#include <string>
#include <vector>

using namespace stdromano;
using fixtures::Tracked;

STDROMANO_TEST_CASE(indices_follow_the_elements)
{
    Vector<Tracked> vec;

    for(int i = 0; i < 100; ++i)
        vec.emplace_back(std::to_string(i));

    std::size_t expected = 0;

    for(const auto& [i, item] : enumerate(vec))
    {
        STDROMANO_CHECK_EQ(i, expected);
        STDROMANO_CHECK_EQ(item.value(), std::to_string(i));
        ++expected;
    }

    STDROMANO_CHECK_EQ(expected, vec.size());
}

STDROMANO_TEST_CASE(elements_are_references)
{
    std::vector<int> values = {1, 2, 3};

    for(auto [i, value] : enumerate(values))
        value = static_cast<int>(i) * 10;

    STDROMANO_CHECK_EQ(values[0], 0);
    STDROMANO_CHECK_EQ(values[1], 10);
    STDROMANO_CHECK_EQ(values[2], 20);
}

STDROMANO_TEST_CASE(const_container)
{
    const std::vector<int> values = {5, 6, 7};

    int sum = 0;
    std::size_t index_sum = 0;

    for(const auto& [i, value] : enumerate(values))
    {
        sum += value;
        index_sum += i;
    }

    STDROMANO_CHECK_EQ(sum, 18);
    STDROMANO_CHECK_EQ(index_sum, 3u);
}

STDROMANO_TEST_CASE(empty_container_yields_nothing)
{
    Vector<Tracked> vec;

    for(const auto& [i, item] : enumerate(vec))
        STDROMANO_FAIL(StringD::make_fmt("empty container yielded {} at {}", item.value(), i).c_str());
}

STDROMANO_TEST_CASE(fuzz_matches_manual_indexing)
{
    const auto report = fuzz::run_property(fixtures::options("enumerate_indices", 200), [](fuzz::Source& source) {
        const std::size_t size = source.size(256);

        std::vector<std::uint32_t> values(size);

        for(auto& value : values)
            value = source.integer<std::uint32_t>();

        std::size_t count = 0;

        for(const auto& [i, value] : enumerate(values))
        {
            STDROMANO_FUZZ_CHECK_EQ(i, count);
            STDROMANO_FUZZ_CHECK_EQ(static_cast<std::uint32_t>(value), values[count]);
            ++count;
        }

        STDROMANO_FUZZ_CHECK_EQ(count, size);

        return true;
    });

    TESTS_REQUIRE_PROPERTY(report);
}

STDROMANO_TEST_MAIN()
