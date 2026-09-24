// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - Present Romain Augier
// All rights reserved.

#include "stdromano/vector.hpp"

#include "fixtures.hpp"

#include <string>
#include <vector>

using namespace stdromano;
using fixtures::Tracked;

static std::int32_t offset(const std::size_t index) noexcept
{
    return static_cast<std::int32_t>(index);
}

template <typename T>
static bool same_content(const Vector<T>& vec, const std::vector<T>& reference)
{
    if(vec.size() != reference.size())
        return false;

    for(std::size_t i = 0; i < reference.size(); ++i)
        if(!(vec[i] == reference[i]))
            return false;

    return true;
}

static Vector<Tracked> make_tracked(std::initializer_list<const char*> values)
{
    Vector<Tracked> vec;

    for(const char* value : values)
        vec.emplace_back(value);

    return vec;
}

STDROMANO_TEST_CASE(default_construction_is_empty)
{
    const std::int64_t live = Tracked::live();

    {
        Vector<Tracked> vec;

        STDROMANO_CHECK_EQ(vec.size(), 0u);
        STDROMANO_CHECK_EQ(vec.capacity(), 0u);
        STDROMANO_CHECK(vec.empty());
        STDROMANO_CHECK(vec.data() == nullptr);
    }

    STDROMANO_CHECK_EQ(Tracked::live(), live);
}

STDROMANO_TEST_CASE(count_construction_copies_the_value)
{
    const std::int64_t live = Tracked::live();

    {
        Vector<Tracked> vec(5, Tracked("test"));

        STDROMANO_REQUIRE_EQ(vec.size(), 5u);
        STDROMANO_CHECK_EQ(Tracked::live(), live + 5);

        for(const Tracked& item : vec)
            STDROMANO_CHECK_EQ(item.value(), "test");
    }

    STDROMANO_CHECK_EQ(Tracked::live(), live);
}

STDROMANO_TEST_CASE(zero_count_construction_allocates)
{
    Vector<Tracked> vec(0, Tracked("x"));

    STDROMANO_CHECK_EQ(vec.size(), 0u);
    STDROMANO_CHECK_GT(vec.capacity(), 0u);
}

STDROMANO_TEST_CASE(initializer_list_and_iterator_construction)
{
    const std::int64_t live = Tracked::live();

    {
        Vector<Tracked> vec({Tracked("test1"), Tracked("test2")});

        STDROMANO_REQUIRE_EQ(vec.size(), 2u);
        STDROMANO_CHECK_EQ(vec[0].value(), "test1");
        STDROMANO_CHECK_EQ(vec[1].value(), "test2");
        STDROMANO_CHECK_EQ(Tracked::live(), live + 2);

        Vector<Tracked> vec2(vec.begin(), vec.end());

        STDROMANO_REQUIRE_EQ(vec2.size(), 2u);
        STDROMANO_CHECK(vec2[0] == vec[0]);
        STDROMANO_CHECK(vec2[1] == vec[1]);
        STDROMANO_CHECK_EQ(Tracked::live(), live + 4);
    }

    STDROMANO_CHECK_EQ(Tracked::live(), live);
}

STDROMANO_TEST_CASE(push_back_and_pop_back)
{
    Vector<Tracked> vec;

    vec.push_back(Tracked("first"));
    STDROMANO_REQUIRE_EQ(vec.size(), 1u);
    STDROMANO_CHECK_EQ(vec[0].value(), "first");

    const Tracked second("second");
    vec.push_back(second);
    STDROMANO_REQUIRE_EQ(vec.size(), 2u);
    STDROMANO_CHECK_EQ(vec[1].value(), "second");

    const Tracked popped = vec.pop_back();
    STDROMANO_CHECK_EQ(popped.value(), "second");
    STDROMANO_REQUIRE_EQ(vec.size(), 1u);
    STDROMANO_CHECK_EQ(vec[0].value(), "first");
}

STDROMANO_TEST_CASE(emplace_back)
{
    Vector<Tracked> vec;

    vec.emplace_back("emplace1");
    vec.emplace_back("emplace2");

    STDROMANO_REQUIRE_EQ(vec.size(), 2u);
    STDROMANO_CHECK_EQ(vec[0].value(), "emplace1");
    STDROMANO_CHECK_EQ(vec[1].value(), "emplace2");
}

STDROMANO_TEST_CASE(copy_and_move)
{
    const std::int64_t live = Tracked::live();

    {
        Vector<Tracked> vec1 = make_tracked({"test1", "test2"});

        Vector<Tracked> vec2(vec1);
        STDROMANO_REQUIRE_EQ(vec2.size(), vec1.size());
        STDROMANO_CHECK(vec2[0] == vec1[0]);
        STDROMANO_CHECK(vec2[1] == vec1[1]);

        Vector<Tracked> vec3(std::move(vec1));
        STDROMANO_CHECK_EQ(vec3.size(), 2u);
        STDROMANO_CHECK_EQ(vec1.size(), 0u);
        STDROMANO_CHECK(vec1.data() == nullptr);

        Vector<Tracked> vec4;
        vec4 = vec2;
        STDROMANO_REQUIRE_EQ(vec4.size(), vec2.size());
        STDROMANO_CHECK(vec4[0] == vec2[0]);

        Vector<Tracked> vec5 = make_tracked({"overwritten"});
        vec5 = std::move(vec2);
        STDROMANO_CHECK_EQ(vec5.size(), 2u);
        STDROMANO_CHECK_EQ(vec2.size(), 0u);
    }

    STDROMANO_CHECK_EQ(Tracked::live(), live);
}

STDROMANO_TEST_CASE(copy_and_move_empty)
{
    Vector<Tracked> empty;

    Vector<Tracked> copy(empty);
    STDROMANO_CHECK_EQ(copy.size(), 0u);
    STDROMANO_CHECK(copy.data() == nullptr);

    Vector<Tracked> moved(std::move(empty));
    STDROMANO_CHECK_EQ(moved.size(), 0u);

    Vector<Tracked> nonempty = make_tracked({"x"});
    nonempty = copy;
    STDROMANO_CHECK_EQ(nonempty.size(), 0u);
}

STDROMANO_TEST_CASE(self_assignment)
{
    Vector<Tracked> vec = make_tracked({"a", "b"});
    Vector<Tracked>& alias = vec;

    vec = alias;
    STDROMANO_REQUIRE_EQ(vec.size(), 2u);
    STDROMANO_CHECK_EQ(vec[0].value(), "a");

    vec = std::move(alias);
    STDROMANO_REQUIRE_EQ(vec.size(), 2u);
    STDROMANO_CHECK_EQ(vec[1].value(), "b");
}

STDROMANO_TEST_CASE(element_access)
{
    Vector<Tracked> vec = make_tracked({"first", "middle", "last"});
    const Vector<Tracked>& cvec = vec;

    STDROMANO_CHECK_EQ(vec[0].value(), "first");
    STDROMANO_CHECK_EQ(vec.at(1)->value(), "middle");
    STDROMANO_CHECK_EQ(vec.front().value(), "first");
    STDROMANO_CHECK_EQ(vec.back().value(), "last");
    STDROMANO_CHECK_EQ(cvec.front().value(), "first");
    STDROMANO_CHECK_EQ(cvec.back().value(), "last");
    STDROMANO_CHECK_EQ(cvec.at(2)->value(), "last");
}

STDROMANO_TEST_CASE(data_pointer)
{
    Vector<Tracked> vec;
    STDROMANO_CHECK(vec.data() == nullptr);

    vec.emplace_back("x");
    STDROMANO_REQUIRE(vec.data() != nullptr);
    STDROMANO_CHECK_EQ(vec.data()[0].value(), "x");

    const Vector<Tracked>& cvec = vec;
    STDROMANO_CHECK(cvec.data() == vec.data());
}

STDROMANO_TEST_CASE(empty_and_clear)
{
    const std::int64_t live = Tracked::live();

    Vector<Tracked> vec;
    STDROMANO_CHECK(vec.empty());
    vec.clear();
    STDROMANO_CHECK_EQ(vec.size(), 0u);

    for(int i = 0; i < 10; ++i)
        vec.emplace_back(std::to_string(i));

    STDROMANO_CHECK(!vec.empty());

    const std::size_t capacity = vec.capacity();

    vec.clear();
    STDROMANO_CHECK(vec.empty());
    STDROMANO_CHECK_EQ(vec.capacity(), capacity);
    STDROMANO_CHECK_EQ(Tracked::live(), live);
}

STDROMANO_TEST_CASE(capacity_growth_keeps_the_elements)
{
    Vector<Tracked> vec;

    for(int i = 0; i < 1000; ++i)
    {
        vec.emplace_back(std::to_string(i));
        STDROMANO_REQUIRE_GE(vec.capacity(), vec.size());
    }

    STDROMANO_REQUIRE_EQ(vec.size(), 1000u);

    for(int i = 0; i < 1000; ++i)
        STDROMANO_REQUIRE_EQ(vec[i].value(), std::to_string(i));
}

STDROMANO_TEST_CASE(resize_and_reserve)
{
    Vector<Tracked> vec;

    vec.reserve(50);
    STDROMANO_CHECK_GE(vec.capacity(), 50u);
    STDROMANO_CHECK_EQ(vec.size(), 0u);

    const std::size_t capacity = vec.capacity();

    vec.resize(capacity - 1);
    STDROMANO_CHECK_EQ(vec.capacity(), capacity);

    vec.resize(capacity + 100);
    STDROMANO_CHECK_GE(vec.capacity(), capacity + 100);
}

STDROMANO_TEST_CASE(insert_single)
{
    Vector<Tracked> vec = make_tracked({"a", "c"});

    vec.insert(Tracked("b"), 1);
    vec.insert(Tracked("z"), 0);
    vec.insert(Tracked("end"), vec.size());

    STDROMANO_CHECK(same_content(vec, std::vector<Tracked>{"z", "a", "b", "c", "end"}));
}

STDROMANO_TEST_CASE(insert_count)
{
    const std::int64_t live = Tracked::live();

    {
        Vector<Tracked> vec = make_tracked({"a", "d"});
        vec.insert(vec.begin() + 1, 2, Tracked("x"));
        STDROMANO_CHECK(same_content(vec, std::vector<Tracked>{"a", "x", "x", "d"}));

        Vector<Tracked> tail = make_tracked({"a", "b", "c", "d"});
        tail.insert(tail.begin() + 1, 2, Tracked("x"));
        STDROMANO_CHECK(same_content(tail, std::vector<Tracked>{"a", "x", "x", "b", "c", "d"}));
    }

    STDROMANO_CHECK_EQ(Tracked::live(), live);
}

STDROMANO_TEST_CASE(insert_range)
{
    const std::int64_t live = Tracked::live();

    {
        const Vector<Tracked> src = make_tracked({"x", "y"});

        Vector<Tracked> middle = make_tracked({"a", "b"});
        middle.insert(middle.begin() + 1, src.begin(), src.end());
        STDROMANO_CHECK(same_content(middle, std::vector<Tracked>{"a", "x", "y", "b"}));

        Vector<Tracked> end = make_tracked({"a"});
        end.insert(end.end(), src.begin(), src.end());
        STDROMANO_CHECK(same_content(end, std::vector<Tracked>{"a", "x", "y"}));

        Vector<Tracked> long_tail = make_tracked({"a", "b", "c", "d"});
        long_tail.insert(long_tail.begin(), src.begin(), src.end());
        STDROMANO_CHECK(same_content(long_tail, std::vector<Tracked>{"x", "y", "a", "b", "c", "d"}));

        Vector<Tracked> list = make_tracked({"a", "d"});
        list.insert(list.cbegin() + 1, {Tracked("b"), Tracked("c")});
        STDROMANO_CHECK(same_content(list, std::vector<Tracked>{"a", "b", "c", "d"}));
    }

    STDROMANO_CHECK_EQ(Tracked::live(), live);
}

STDROMANO_TEST_CASE(erase)
{
    const std::int64_t live = Tracked::live();

    {
        Vector<Tracked> single = make_tracked({"a", "b", "c"});
        single.erase(single.cbegin() + 1);
        STDROMANO_CHECK(same_content(single, std::vector<Tracked>{"a", "c"}));

        Vector<Tracked> range = make_tracked({"a", "b", "c", "d"});
        range.erase(range.cbegin() + 1, range.cbegin() + 3);
        STDROMANO_CHECK(same_content(range, std::vector<Tracked>{"a", "d"}));

        Vector<Tracked> empty_range = make_tracked({"a", "b"});
        empty_range.erase(empty_range.cbegin() + 1, empty_range.cbegin() + 1);
        STDROMANO_CHECK_EQ(empty_range.size(), 2u);
    }

    STDROMANO_CHECK_EQ(Tracked::live(), live);
}

STDROMANO_TEST_CASE(find_and_cfind)
{
    Vector<Tracked> vec = make_tracked({"a", "b", "c"});

    auto found = vec.find([](const Tracked& o) { return o.value() == "b"; });
    STDROMANO_REQUIRE(found != vec.end());
    STDROMANO_CHECK_EQ((*found).value(), "b");

    STDROMANO_CHECK(vec.find([](const Tracked& o) { return o.value() == "z"; }) == vec.end());
    STDROMANO_CHECK(vec.find(Tracked("c")) == vec.begin() + 2);

    Vector<Tracked> empty;
    STDROMANO_CHECK(empty.find([](const Tracked&) { return true; }) == empty.end());

    const Vector<Tracked>& cvec = vec;

    auto cfound = cvec.cfind([](const Tracked& o) { return o.value() == "a"; });
    STDROMANO_REQUIRE(cfound != cvec.cend());
    STDROMANO_CHECK_EQ((*cfound).value(), "a");

    STDROMANO_CHECK(cvec.cfind([](const Tracked& o) { return o.value() == "z"; }) == cvec.cend());
}

STDROMANO_TEST_CASE(iterators)
{
    Vector<Tracked> vec = make_tracked({"a", "b", "c"});

    std::size_t count = 0;

    for(auto it = vec.begin(); it != vec.end(); ++it)
        ++count;

    STDROMANO_CHECK_EQ(count, 3u);

    auto it = vec.begin();
    auto it2 = it + 2;

    STDROMANO_CHECK_EQ((*it).value(), "a");
    STDROMANO_CHECK_EQ((*it2).value(), "c");
    STDROMANO_CHECK_EQ(it2 - it, 2);
    STDROMANO_CHECK(it < it2);
    STDROMANO_CHECK(it2 > it);
    STDROMANO_CHECK(it <= it);
    STDROMANO_CHECK(it >= it);

    --it2;
    STDROMANO_CHECK_EQ((*it2).value(), "b");
    STDROMANO_CHECK_EQ(it[1].value(), "b");

    const Vector<Tracked>& cvec = vec;

    count = 0;

    for(auto cit = cvec.cbegin(); cit != cvec.cend(); ++cit)
        ++count;

    STDROMANO_CHECK_EQ(count, 3u);

    Vector<Tracked> empty;

    for(const Tracked& item : empty)
        STDROMANO_FAIL(StringD::make_fmt("empty vector yielded {}", item.value()).c_str());
}

STDROMANO_TEST_CASE(memory_usage)
{
    Vector<Tracked> vec;
    STDROMANO_CHECK_EQ(vec.memory_usage(), 0u);

    vec.emplace_back("a");
    STDROMANO_CHECK_EQ(vec.memory_usage(), sizeof(Tracked));

    vec.emplace_back("b");
    STDROMANO_CHECK_EQ(vec.memory_usage(), sizeof(Tracked) * 2);
}

STDROMANO_TEST_CASE(fuzz_against_std_vector)
{
    const std::int64_t live = Tracked::live();

    const auto report = fuzz::run_property(fixtures::options("vector_vs_std_vector", 500), [](fuzz::Source& source) {
        Vector<Tracked> vec;
        std::vector<Tracked> reference;

        const std::size_t steps = source.range<std::size_t>(1, 200);

        for(std::size_t step = 0; step < steps; ++step)
        {
            const std::string value = std::to_string(source.range<int>(0, 1000));

            switch(source.index(11))
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
                    if(!reference.empty())
                    {
                        const Tracked popped = vec.pop_back();
                        STDROMANO_FUZZ_CHECK_EQ(popped.value(), reference.back().value());
                        reference.pop_back();
                    }
                    break;
                case 3:
                {
                    const std::size_t position = source.range<std::size_t>(0, reference.size());
                    vec.insert(Tracked(value), position);
                    reference.insert(reference.begin() + position, Tracked(value));
                    break;
                }
                case 4:
                {
                    const std::size_t position = source.range<std::size_t>(0, reference.size());
                    const std::size_t count = source.range<std::size_t>(0, 8);
                    vec.insert(vec.begin() + offset(position), count, Tracked(value));
                    reference.insert(reference.begin() + position, count, Tracked(value));
                    break;
                }
                case 5:
                {
                    std::vector<Tracked> items;
                    const std::size_t count = source.range<std::size_t>(0, 8);

                    for(std::size_t i = 0; i < count; ++i)
                        items.emplace_back(value + "_" + std::to_string(i));

                    const std::size_t position = source.range<std::size_t>(0, reference.size());
                    vec.insert(vec.begin() + offset(position), items.begin(), items.end());
                    reference.insert(reference.begin() + position, items.begin(), items.end());
                    break;
                }
                case 6:
                    if(!reference.empty())
                    {
                        const std::size_t position = source.index(reference.size());
                        vec.erase(vec.cbegin() + offset(position));
                        reference.erase(reference.begin() + position);
                    }
                    break;
                case 7:
                {
                    const std::size_t first = source.range<std::size_t>(0, reference.size());
                    const std::size_t last = source.range<std::size_t>(first, reference.size());

                    if(first < reference.size())
                    {
                        vec.erase(vec.cbegin() + offset(first), vec.cbegin() + offset(last));
                        reference.erase(reference.begin() + first, reference.begin() + last);
                    }
                    break;
                }
                case 8:
                    if(source.one_in(8))
                    {
                        vec.clear();
                        reference.clear();
                    }
                    break;
                case 9:
                {
                    Vector<Tracked> copy(vec);
                    vec = std::move(copy);
                    break;
                }
                default:
                    vec.reserve(source.range<std::size_t>(0, 64));
                    break;
            }

            STDROMANO_FUZZ_CHECK_EQ(vec.size(), reference.size());
        }

        STDROMANO_FUZZ_CHECK(same_content(vec, reference));

        return true;
    });

    TESTS_REQUIRE_PROPERTY(report);
    STDROMANO_CHECK_EQ(Tracked::live(), live);
}

STDROMANO_TEST_CASE(fuzz_trivial_type_against_std_vector)
{
    const auto report = fuzz::run_property(fixtures::options("vector_int_vs_std_vector", 500), [](fuzz::Source& source) {
        Vector<std::int64_t> vec;
        std::vector<std::int64_t> reference;

        const std::size_t steps = source.range<std::size_t>(1, 300);

        for(std::size_t step = 0; step < steps; ++step)
        {
            const std::int64_t value = source.integer<std::int64_t>();

            switch(source.index(5))
            {
                case 0:
                    vec.push_back(value);
                    reference.push_back(value);
                    break;
                case 1:
                {
                    const std::size_t position = source.range<std::size_t>(0, reference.size());
                    const std::size_t count = source.range<std::size_t>(0, 16);
                    vec.insert(vec.begin() + offset(position), count, value);
                    reference.insert(reference.begin() + position, count, value);
                    break;
                }
                case 2:
                {
                    const std::size_t position = source.range<std::size_t>(0, reference.size());
                    vec.insert(value, position);
                    reference.insert(reference.begin() + position, value);
                    break;
                }
                case 3:
                    if(!reference.empty())
                    {
                        const std::size_t position = source.index(reference.size());
                        vec.erase(vec.cbegin() + offset(position));
                        reference.erase(reference.begin() + position);
                    }
                    break;
                default:
                    if(!reference.empty())
                    {
                        STDROMANO_FUZZ_CHECK_EQ(vec.pop_back(), reference.back());
                        reference.pop_back();
                    }
                    break;
            }
        }

        STDROMANO_FUZZ_CHECK(same_content(vec, reference));

        return true;
    });

    TESTS_REQUIRE_PROPERTY(report);
}

STDROMANO_TEST_MAIN()
