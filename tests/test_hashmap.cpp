// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - Present Romain Augier
// All rights reserved.

#include "stdromano/hashmap.hpp"

#include "fixtures.hpp"

#include <numeric>
#include <random>
#include <string>
#include <unordered_map>
#include <vector>

using namespace stdromano;

struct ComplexKey
{
    int id;
    std::string name;

    bool operator==(const ComplexKey& other) const
    {
        return this->id == other.id && this->name == other.name;
    }
};

struct ComplexKeyHash
{
    std::size_t operator()(const ComplexKey& key) const
    {
        return std::hash<int>()(key.id) ^ std::hash<std::string>()(key.name);
    }
};

struct CollisionHash
{
    std::size_t operator()(int) const
    {
        return 1;
    }
};

static int stress_size()
{
    return fixtures::is_debug_build() ? 10000 : 1000000;
}

static std::vector<std::int64_t> shuffled_range(const std::size_t count, std::mt19937& rng)
{
    std::vector<std::int64_t> values(count);
    std::iota(values.begin(), values.end(), 0);
    std::shuffle(values.begin(), values.end(), rng);
    return values;
}

template <typename K, typename V, typename H, typename R>
static bool matches_reference(const HashMap<K, V, H>& map, const R& reference)
{
    if(map.size() != reference.size())
        return false;

    for(const auto& entry : reference)
    {
        const auto it = map.find(entry.first);

        if(it == map.cend() || !(it->second == entry.second))
            return false;
    }

    std::size_t iterated = 0;

    for(auto it = map.cbegin(); it != map.cend(); ++it)
    {
        const auto ref = reference.find(it->first);

        if(ref == reference.end() || !(ref->second == it->second))
            return false;

        ++iterated;
    }

    return iterated == reference.size();
}

STDROMANO_TEST_CASE(basic_operations)
{
    HashMap<int, std::string> map;

    STDROMANO_CHECK_EQ(map.size(), 0u);
    STDROMANO_CHECK(map.empty());

    map.insert(std::pair<int, std::string>(1, "one"));
    STDROMANO_CHECK_EQ(map.size(), 1u);
    STDROMANO_CHECK(!map.empty());

    auto it = map.find(1);
    STDROMANO_REQUIRE(it != map.end());
    STDROMANO_CHECK_EQ(it->second, "one");
    STDROMANO_CHECK(map.find(2) == map.end());

    map.insert(std::pair<int, std::string>(1, "uno"));
    STDROMANO_CHECK_EQ(map.size(), 1u);
    STDROMANO_CHECK_EQ(map.find(1)->second, "one");

    map.erase(1);
    STDROMANO_CHECK_EQ(map.size(), 0u);
    STDROMANO_CHECK(map.empty());
}

STDROMANO_TEST_CASE(operator_bracket)
{
    HashMap<std::string, int> map;

    map["test"] = 42;
    STDROMANO_CHECK_EQ(map.size(), 1u);
    STDROMANO_CHECK_EQ(map["test"], 42);

    map["test"] = 24;
    STDROMANO_CHECK_EQ(map.size(), 1u);
    STDROMANO_CHECK_EQ(map["test"], 24);

    int& value = map["new"];
    STDROMANO_CHECK_EQ(map.size(), 2u);
    STDROMANO_CHECK_EQ(value, 0);
    value = 100;
    STDROMANO_CHECK_EQ(map["new"], 100);
}

STDROMANO_TEST_CASE(complex_key)
{
    HashMap<ComplexKey, double, ComplexKeyHash> map;

    const ComplexKey key1{1, "one"};
    const ComplexKey key2{2, "two"};

    map.insert(std::pair<ComplexKey, double>(key1, 1.1));
    map.insert(std::pair<ComplexKey, double>(key2, 2.2));

    STDROMANO_REQUIRE_EQ(map.size(), 2u);
    STDROMANO_CHECK_EQ(map.find(key1)->second, 1.1);
    STDROMANO_CHECK_EQ(map.find(key2)->second, 2.2);
    STDROMANO_CHECK(!map.contains(ComplexKey{1, "two"}));
}

STDROMANO_TEST_CASE(iterators_visit_every_entry_once)
{
    HashMap<int, int> map;

    for(int i = 0; i < 10; ++i)
        map.insert(std::pair<int, int>(i, i * i));

    std::vector<int> seen(10, 0);

    for(auto it = map.begin(); it != map.end(); ++it)
    {
        STDROMANO_CHECK_EQ(it->second, it->first * it->first);
        seen[it->first]++;
    }

    const HashMap<int, int>& const_map = map;

    for(auto it = const_map.cbegin(); it != const_map.cend(); ++it)
        seen[it->first]++;

    for(const int count : seen)
        STDROMANO_CHECK_EQ(count, 2);

    HashMap<int, int> empty;
    STDROMANO_CHECK(empty.begin() == empty.end());
}

STDROMANO_TEST_CASE(load_factor_stays_bounded)
{
    HashMap<int, int> map(2);

    for(int i = 0; i < 100; ++i)
    {
        map.insert(std::pair<int, int>(i, i));
        STDROMANO_REQUIRE_LE(map.load_factor(), 1.0f);
    }

    for(int i = 0; i < 100; ++i)
    {
        const auto it = map.find(i);
        STDROMANO_REQUIRE(it != map.end());
        STDROMANO_CHECK_EQ(it->second, i);
    }
}

STDROMANO_TEST_CASE(collisions)
{
    HashMap<int, std::string, CollisionHash> map;

    map.insert(std::pair<int, std::string>(1, "one"));
    map.insert(std::pair<int, std::string>(2, "two"));
    map.insert(std::pair<int, std::string>(3, "three"));

    STDROMANO_REQUIRE_EQ(map.size(), 3u);
    STDROMANO_CHECK_EQ(map.find(1)->second, "one");
    STDROMANO_CHECK_EQ(map.find(2)->second, "two");
    STDROMANO_CHECK_EQ(map.find(3)->second, "three");

    map.erase(2);
    STDROMANO_CHECK(!map.contains(2));
    STDROMANO_CHECK_EQ(map.find(3)->second, "three");
}

STDROMANO_TEST_CASE(clear_keeps_the_capacity)
{
    HashMap<int, int> map;

    map.reserve(100);
    const std::size_t capacity = map.capacity();
    STDROMANO_CHECK_GE(capacity, 100u);

    for(int i = 0; i < 50; ++i)
        map.insert(std::pair<int, int>(i, i));

    map.clear();
    STDROMANO_CHECK_EQ(map.size(), 0u);
    STDROMANO_CHECK(map.empty());
    STDROMANO_CHECK_EQ(map.capacity(), capacity);
    STDROMANO_CHECK(!map.contains(10));
}

STDROMANO_TEST_CASE(edge_cases)
{
    HashMap<std::string, int> map;

    map.insert(std::pair<std::string, int>("", 0));
    STDROMANO_CHECK_EQ(map[""], 0);

    map.insert(std::pair<std::string, int>("test", 1));
    map["test"] = 2;
    STDROMANO_CHECK_EQ(map["test"], 2);

    const std::size_t size = map.size();
    map.erase("non-existent");
    STDROMANO_CHECK_EQ(map.size(), size);
    STDROMANO_CHECK(map.find("non-existent") == map.end());
}

STDROMANO_TEST_CASE(initializer_list)
{
    const HashMap<std::int64_t, std::string> map = {
        {0, "zero"},
        {1, "one"},
        {2, "two"},
        {3, "three"},
    };

    STDROMANO_CHECK_EQ(map.size(), 4u);

    for(std::int64_t key = 0; key < 4; ++key)
        STDROMANO_CHECK(map.contains(key));

    STDROMANO_CHECK(!map.contains(6));
}

STDROMANO_TEST_CASE(copy_and_move)
{
    HashMap<std::string, int> map;

    for(int i = 0; i < 200; ++i)
        map[std::to_string(i)] = i;

    HashMap<std::string, int> copy(map);
    STDROMANO_CHECK_EQ(copy.size(), map.size());
    copy["0"] = -1;
    STDROMANO_CHECK_EQ(map["0"], 0);

    HashMap<std::string, int> moved(std::move(copy));
    STDROMANO_CHECK_EQ(moved.size(), 200u);
    STDROMANO_CHECK_EQ(moved["0"], -1);

    HashMap<std::string, int> assigned;
    assigned["x"] = 1;
    assigned = map;
    STDROMANO_CHECK_EQ(assigned.size(), 200u);
    STDROMANO_CHECK(!assigned.contains("x"));

    HashMap<std::string, int> move_assigned;
    move_assigned = std::move(assigned);
    STDROMANO_CHECK_EQ(move_assigned.size(), 200u);
    STDROMANO_CHECK_EQ(move_assigned["199"], 199);
}

STDROMANO_TEST_CASE(moved_from_map_is_reusable)
{
    HashMap<int, int> map;

    for(int i = 0; i < 64; ++i)
        map[i] = i;

    HashMap<int, int> other(std::move(map));

    STDROMANO_CHECK_EQ(map.size(), 0u);
    STDROMANO_CHECK(!map.contains(3));

    for(int i = 0; i < 64; ++i)
        map[i] = -i;

    STDROMANO_CHECK_EQ(map.size(), 64u);
    STDROMANO_CHECK_EQ(map[10], -10);
    STDROMANO_CHECK_EQ(other[10], 10);
}

STDROMANO_TEST_CASE(emplace_returns_the_inserted_element)
{
    for(int trial = 0; trial < 200; ++trial)
    {
        HashMap<int, int> map;

        for(int i = 0; i < 256; ++i)
        {
            const auto result = map.emplace(i, i + 1000);

            STDROMANO_REQUIRE(result.second);
            STDROMANO_REQUIRE_EQ(result.first->first, i);
            STDROMANO_REQUIRE_EQ(result.first->second, i + 1000);
        }

        const auto again = map.emplace(7, 0);
        STDROMANO_CHECK(!again.second);
        STDROMANO_CHECK_EQ(again.first->second, 1007);
    }
}

STDROMANO_TEST_CASE(operator_bracket_after_displacement)
{
    for(int trial = 0; trial < 200; ++trial)
    {
        HashMap<int, int> map;

        for(int i = 0; i < 256; ++i)
            map[i] = i * 7 + 1;

        STDROMANO_REQUIRE_EQ(map.size(), 256u);

        for(int i = 0; i < 256; ++i)
        {
            const auto it = map.find(i);
            STDROMANO_REQUIRE(it != map.end());
            STDROMANO_REQUIRE_EQ(it->second, i * 7 + 1);
        }
    }
}

STDROMANO_TEST_CASE(stress)
{
    std::mt19937 rng(0x5EED);

    const int count = stress_size();
    std::vector<std::int64_t> keys = shuffled_range(count, rng);

    HashMap<std::int64_t, std::int64_t> map;

    for(const std::int64_t key : keys)
        map.insert(std::pair<std::int64_t, std::int64_t>(key, key));

    STDROMANO_REQUIRE_EQ(map.size(), static_cast<std::size_t>(count));

    const float load_factor = map.load_factor();
    STDROMANO_CHECK(load_factor > 0.0f && load_factor <= 1.0f);

    std::size_t iterated = 0;

    for(auto it = map.begin(); it != map.end(); ++it)
        ++iterated;

    STDROMANO_CHECK_EQ(iterated, map.size());

    std::shuffle(keys.begin(), keys.end(), rng);

    for(int i = 0; i < count / 2; ++i)
        map.erase(keys[i]);

    std::size_t found = 0;

    for(const std::int64_t key : keys)
        if(map.find(key) != map.end())
            ++found;

    STDROMANO_CHECK_EQ(found, static_cast<std::size_t>(count - count / 2));
    STDROMANO_CHECK_EQ(map.size(), found);
}

STDROMANO_TEST_CASE(fuzz_int_keys_against_unordered_map)
{
    const auto report = fuzz::run_property(fixtures::options("hashmap_int_vs_unordered_map", 400), [](fuzz::Source& source) {
        HashMap<int, int> map;
        std::unordered_map<int, int> reference;

        const std::size_t steps = source.range<std::size_t>(1, 400);
        const int key_space = source.pick({8, 96, 4096});

        for(std::size_t i = 0; i < steps; ++i)
        {
            const int key = source.range<int>(0, key_space);
            const int value = static_cast<int>(i) * 7 + 1;

            switch(source.index(7))
            {
                case 0:
                    map[key] = value;
                    reference[key] = value;
                    break;
                case 1:
                {
                    const auto inserted = map.emplace(key, value);
                    const auto expected = reference.emplace(key, value);

                    STDROMANO_FUZZ_CHECK_EQ(inserted.second, expected.second);
                    STDROMANO_FUZZ_CHECK_EQ(inserted.first->first, key);
                    STDROMANO_FUZZ_CHECK_EQ(inserted.first->second, expected.first->second);
                    break;
                }
                case 2:
                    map.insert(std::pair<int, int>(key, value));
                    reference.insert(std::pair<int, int>(key, value));
                    break;
                case 3:
                    map.erase(key);
                    reference.erase(key);
                    break;
                case 4:
                {
                    const auto it = map.find(key);
                    const bool found = it != map.end();

                    STDROMANO_FUZZ_CHECK_EQ(found, reference.count(key) == 1);

                    if(found)
                        STDROMANO_FUZZ_CHECK_EQ(it->second, reference[key]);
                    break;
                }
                case 5:
                    if(source.one_in(4))
                        map.reserve(source.range<std::size_t>(0, 2048));
                    break;
                default:
                    if(source.one_in(20))
                    {
                        map.clear();
                        reference.clear();
                    }
                    break;
            }

            STDROMANO_FUZZ_CHECK_EQ(map.size(), reference.size());
        }

        STDROMANO_FUZZ_CHECK(matches_reference(map, reference));

        const HashMap<int, int> copy(map);
        STDROMANO_FUZZ_CHECK(matches_reference(copy, reference));

        return true;
    });

    TESTS_REQUIRE_PROPERTY(report);
}

STDROMANO_TEST_CASE(fuzz_string_keys_with_collisions)
{
    struct WeakHash
    {
        std::size_t operator()(const std::string& key) const
        {
            return key.size() % 3;
        }
    };

    const auto report = fuzz::run_property(fixtures::options("hashmap_weak_hash", 200), [](fuzz::Source& source) {
        HashMap<std::string, std::string, WeakHash> map;
        std::unordered_map<std::string, std::string> reference;

        const std::size_t steps = source.range<std::size_t>(1, 120);

        for(std::size_t i = 0; i < steps; ++i)
        {
            const StringD raw = source.string(6, "abc");
            const std::string key(raw.c_str(), raw.size());
            const std::string value = std::to_string(i);

            switch(source.index(3))
            {
                case 0:
                    map[key] = value;
                    reference[key] = value;
                    break;
                case 1:
                    map.erase(key);
                    reference.erase(key);
                    break;
                default:
                    STDROMANO_FUZZ_CHECK_EQ(map.contains(key), reference.count(key) == 1);
                    break;
            }
        }

        STDROMANO_FUZZ_CHECK(matches_reference(map, reference));

        return true;
    });

    TESTS_REQUIRE_PROPERTY(report);
}

STDROMANO_TEST_MAIN()
