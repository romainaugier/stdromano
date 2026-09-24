// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - Present Romain Augier
// All rights reserved.

#include "stdromano/stackvector.hpp"

#include "fixtures.hpp"

#include <string>
#include <vector>

using namespace stdromano;
using fixtures::Tracked;

template <typename T, std::size_t N>
static bool same_content(const StackVector<T, N>& vec, const std::vector<T>& reference)
{
    if(vec.size() != reference.size())
        return false;

    for(std::size_t i = 0; i < reference.size(); ++i)
        if(!(vec[i] == reference[i]))
            return false;

    return true;
}

STDROMANO_TEST_CASE(construction_and_destruction)
{
    const std::int64_t live = Tracked::live();

    {
        StackVector<Tracked, 4> vec;
        STDROMANO_CHECK_EQ(vec.size(), 0u);
        STDROMANO_CHECK_EQ(vec.capacity(), 4u);
        STDROMANO_CHECK(vec.empty());
    }

    {
        StackVector<Tracked, 5> vec(5, Tracked("test"));
        STDROMANO_CHECK_EQ(vec.size(), 5u);
        STDROMANO_CHECK_EQ(Tracked::live(), live + 5);
    }

    {
        StackVector<Tracked, 2> heap(6, Tracked("heap"));
        STDROMANO_CHECK_EQ(heap.size(), 6u);
        STDROMANO_CHECK_GE(heap.capacity(), 6u);

        StackVector<Tracked, 2> list = {Tracked("a"), Tracked("b"), Tracked("c")};
        STDROMANO_CHECK(same_content(list, std::vector<Tracked>{"a", "b", "c"}));
    }

    STDROMANO_CHECK_EQ(Tracked::live(), live);
}

STDROMANO_TEST_CASE(push_back_and_pop_back)
{
    StackVector<Tracked, 4> vec;

    vec.push_back(Tracked("first"));
    const Tracked second("second");
    vec.push_back(second);

    STDROMANO_REQUIRE_EQ(vec.size(), 2u);
    STDROMANO_CHECK_EQ(vec[0].value(), "first");
    STDROMANO_CHECK_EQ(vec[1].value(), "second");

    vec.pop_back();
    STDROMANO_REQUIRE_EQ(vec.size(), 1u);
    STDROMANO_CHECK_EQ(vec.back().value(), "first");

    vec.pop_back();
    vec.pop_back();
    STDROMANO_CHECK(vec.empty());
}

STDROMANO_TEST_CASE(emplace_back_returns_the_element)
{
    StackVector<Tracked, 2> vec;

    for(int i = 0; i < 10; ++i)
    {
        Tracked& item = vec.emplace_back(std::to_string(i));
        STDROMANO_CHECK_EQ(item.value(), std::to_string(i));
        STDROMANO_CHECK(&item == &vec.back());
    }
}

STDROMANO_TEST_CASE(copy_and_move_on_stack_and_heap)
{
    const std::int64_t live = Tracked::live();

    for(const std::size_t count : {std::size_t(2), std::size_t(9)})
    {
        StackVector<Tracked, 4> vec1;

        for(std::size_t i = 0; i < count; ++i)
            vec1.emplace_back(std::to_string(i));

        StackVector<Tracked, 4> vec2(vec1);
        STDROMANO_REQUIRE_EQ(vec2.size(), count);
        STDROMANO_CHECK(vec2[count - 1] == vec1[count - 1]);

        StackVector<Tracked, 4> vec3(std::move(vec1));
        STDROMANO_CHECK_EQ(vec3.size(), count);
        STDROMANO_CHECK_EQ(vec1.size(), 0u);
        STDROMANO_CHECK_EQ(vec1.capacity(), 4u);

        StackVector<Tracked, 4> vec4;
        vec4.emplace_back("overwritten");
        vec4 = vec2;
        STDROMANO_REQUIRE_EQ(vec4.size(), count);
        STDROMANO_CHECK(vec4[0] == vec2[0]);

        StackVector<Tracked, 4> vec5(12, Tracked("heap"));
        vec5 = std::move(vec2);
        STDROMANO_REQUIRE_EQ(vec5.size(), count);
        STDROMANO_CHECK_EQ(vec5[0].value(), "0");
        STDROMANO_CHECK_EQ(vec2.size(), 0u);
    }

    STDROMANO_CHECK_EQ(Tracked::live(), live);
}

STDROMANO_TEST_CASE(element_access)
{
    StackVector<Tracked, 4> vec;
    vec.emplace_back("first");
    vec.emplace_back("second");

    const StackVector<Tracked, 4>& cvec = vec;

    STDROMANO_CHECK_EQ(vec[0].value(), "first");
    STDROMANO_CHECK_EQ(vec.at(1).value(), "second");
    STDROMANO_CHECK_EQ(cvec.front().value(), "first");
    STDROMANO_CHECK_EQ(cvec.back().value(), "second");
    STDROMANO_CHECK(cvec.data() == vec.data());
    STDROMANO_CHECK_EQ(static_cast<std::size_t>(cvec.end() - cvec.begin()), 2u);
}

STDROMANO_TEST_CASE(capacity_grows_past_the_stack)
{
    StackVector<Tracked, 4> vec;

    for(int i = 0; i < 100; ++i)
        vec.emplace_back(std::to_string(i));

    STDROMANO_REQUIRE_EQ(vec.size(), 100u);
    STDROMANO_CHECK_GE(vec.capacity(), vec.size());

    for(int i = 0; i < 100; ++i)
        STDROMANO_REQUIRE_EQ(vec[i].value(), std::to_string(i));
}

STDROMANO_TEST_CASE(reserve_resize_and_clear)
{
    const std::int64_t live = Tracked::live();

    {
        StackVector<Tracked, 4> vec;

        vec.reserve(2);
        STDROMANO_CHECK_EQ(vec.capacity(), 4u);

        vec.reserve(32);
        STDROMANO_CHECK_GE(vec.capacity(), 32u);

        vec.resize(10, Tracked("x"));
        STDROMANO_REQUIRE_EQ(vec.size(), 10u);
        STDROMANO_CHECK_EQ(vec[9].value(), "x");

        vec.resize(3);
        STDROMANO_CHECK_EQ(vec.size(), 3u);

        vec.clear();
        STDROMANO_CHECK(vec.empty());
    }

    STDROMANO_CHECK_EQ(Tracked::live(), live);
}

STDROMANO_TEST_CASE(fuzz_against_std_vector)
{
    const std::int64_t live = Tracked::live();

    const auto report = fuzz::run_property(fixtures::options("stackvector_vs_std_vector", 500), [](fuzz::Source& source) {
        StackVector<Tracked, 4> vec;
        std::vector<Tracked> reference;

        const std::size_t steps = source.range<std::size_t>(1, 150);

        for(std::size_t step = 0; step < steps; ++step)
        {
            const std::string value = std::to_string(source.range<int>(0, 1000));

            switch(source.index(8))
            {
                case 0:
                    vec.push_back(Tracked(value));
                    reference.push_back(Tracked(value));
                    break;
                case 1:
                    vec.emplace_back(value);
                    reference.emplace_back(value);
                    break;
                case 2:
                    vec.pop_back();

                    if(!reference.empty())
                        reference.pop_back();
                    break;
                case 3:
                {
                    const std::size_t count = source.range<std::size_t>(0, 12);
                    vec.resize(count, Tracked(value));
                    reference.resize(count, Tracked(value));
                    break;
                }
                case 4:
                    vec.reserve(source.range<std::size_t>(0, 24));
                    break;
                case 5:
                {
                    StackVector<Tracked, 4> copy(vec);
                    vec = std::move(copy);
                    break;
                }
                case 6:
                {
                    StackVector<Tracked, 4> other;
                    other = vec;
                    StackVector<Tracked, 4> moved(std::move(other));
                    vec = moved;
                    break;
                }
                default:
                    if(source.one_in(6))
                    {
                        vec.clear();
                        reference.clear();
                    }
                    break;
            }

            STDROMANO_FUZZ_CHECK_EQ(vec.size(), reference.size());
            STDROMANO_FUZZ_CHECK(vec.capacity() >= vec.size());
        }

        STDROMANO_FUZZ_CHECK(same_content(vec, reference));

        return true;
    });

    TESTS_REQUIRE_PROPERTY(report);
    STDROMANO_CHECK_EQ(Tracked::live(), live);
}

STDROMANO_TEST_MAIN()
