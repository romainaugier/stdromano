// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - Present Romain Augier
// All rights reserved.

#include "stdromano/hashset.hpp"

#include "fixtures.hpp"

#include <numeric>
#include <random>
#include <set>
#include <string>
#include <unordered_set>
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
        return std::hash<int>()(key.id) ^ (std::hash<std::string>()(key.name) << 1);
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
    return fixtures::is_debug_build() ? 10000 : 100000;
}

template <typename K, typename H>
static bool matches_reference(const HashSet<K, H>& set, const std::unordered_set<K>& reference)
{
    if(set.size() != reference.size())
        return false;

    for(const K& key : reference)
        if(!set.contains(key))
            return false;

    std::size_t iterated = 0;

    for(const K& key : set)
    {
        if(reference.count(key) != 1)
            return false;

        ++iterated;
    }

    return iterated == reference.size();
}

STDROMANO_TEST_CASE(basic_operations)
{
    HashSet<int> set;

    STDROMANO_CHECK_EQ(set.size(), 0u);
    STDROMANO_CHECK(set.empty());

    const auto first = set.insert(1);
    STDROMANO_CHECK(first.second);
    STDROMANO_CHECK_EQ(*first.first, 1);
    STDROMANO_CHECK_EQ(set.size(), 1u);

    const auto duplicate = set.insert(1);
    STDROMANO_CHECK(!duplicate.second);
    STDROMANO_CHECK_EQ(*duplicate.first, 1);
    STDROMANO_CHECK_EQ(set.size(), 1u);

    const auto it = set.find(1);
    STDROMANO_REQUIRE(it != set.end());
    STDROMANO_CHECK_EQ(*it, 1);

    STDROMANO_CHECK(set.find(2) == set.end());
    STDROMANO_CHECK(!set.contains(2));
    STDROMANO_CHECK_EQ(set.count(1), 1u);
    STDROMANO_CHECK_EQ(set.count(2), 0u);

    STDROMANO_CHECK_EQ(set.erase(1), 1u);
    STDROMANO_CHECK(set.empty());
    STDROMANO_CHECK_EQ(set.erase(1), 0u);

    const HashSet<int> list({1, 2, 3, 4});
    STDROMANO_CHECK_EQ(list.size(), 4u);

    for(int key = 1; key <= 4; ++key)
        STDROMANO_CHECK(list.contains(key));
}

STDROMANO_TEST_CASE(complex_key)
{
    HashSet<ComplexKey, ComplexKeyHash> set;

    const ComplexKey key1{1, "one"};
    const ComplexKey key2{2, "two"};

    STDROMANO_CHECK(set.insert(key1).second);
    STDROMANO_CHECK(set.insert(key2).second);

    const auto duplicate = set.insert(ComplexKey{1, "one"});
    STDROMANO_CHECK(!duplicate.second);
    STDROMANO_CHECK(*duplicate.first == key1);
    STDROMANO_CHECK_EQ(set.size(), 2u);

    STDROMANO_CHECK(set.contains(key1));
    STDROMANO_CHECK(set.contains(key2));
    STDROMANO_CHECK(!set.contains(ComplexKey{3, "three"}));

    STDROMANO_CHECK_EQ(set.erase(key1), 1u);
    STDROMANO_CHECK(!set.contains(key1));
    STDROMANO_CHECK(set.contains(key2));
}

STDROMANO_TEST_CASE(iterators_visit_every_key_once)
{
    HashSet<int> set;
    std::set<int> reference;

    for(int i = 0; i < 10; ++i)
    {
        set.insert(i);
        reference.insert(i);
    }

    std::multiset<int> seen;

    for(auto it = set.begin(); it != set.end(); ++it)
        seen.insert(*it);

    const HashSet<int>& const_set = set;

    for(auto it = const_set.cbegin(); it != const_set.cend(); ++it)
        seen.insert(*it);

    for(const int key : reference)
        STDROMANO_CHECK_EQ(seen.count(key), 2u);

    STDROMANO_CHECK_EQ(seen.size(), 20u);
}

STDROMANO_TEST_CASE(load_factor_and_rehashing)
{
    HashSet<int> set(2);
    const std::size_t initial_capacity = set.capacity();
    STDROMANO_CHECK_GE(initial_capacity, 2u);

    for(int i = 0; i < 100; ++i)
    {
        set.insert(i);
        STDROMANO_REQUIRE_LE(set.load_factor(), 1.0f);
    }

    STDROMANO_CHECK_EQ(set.size(), 100u);
    STDROMANO_CHECK_GT(set.capacity(), initial_capacity);

    for(int i = 0; i < 100; ++i)
        STDROMANO_REQUIRE(set.contains(i));
}

STDROMANO_TEST_CASE(collisions)
{
    HashSet<int, CollisionHash> set;

    set.insert(1);
    set.insert(10);
    set.insert(20);

    STDROMANO_CHECK_EQ(set.size(), 3u);
    STDROMANO_CHECK(set.contains(1));
    STDROMANO_CHECK(set.contains(10));
    STDROMANO_CHECK(set.contains(20));
    STDROMANO_CHECK(!set.contains(5));

    STDROMANO_CHECK_EQ(set.erase(10), 1u);
    STDROMANO_CHECK(set.contains(1));
    STDROMANO_CHECK(!set.contains(10));
    STDROMANO_CHECK(set.contains(20));
}

STDROMANO_TEST_CASE(clear_and_reserve)
{
    HashSet<int> set;

    set.reserve(100);
    const std::size_t capacity = set.capacity();
    STDROMANO_CHECK_GE(capacity, 100u);

    for(int i = 0; i < 50; ++i)
        set.insert(i);

    set.clear();
    STDROMANO_CHECK(set.empty());
    STDROMANO_CHECK_EQ(set.capacity(), capacity);

    set.insert(100);
    STDROMANO_CHECK_EQ(set.size(), 1u);
    STDROMANO_CHECK(set.contains(100));
}

STDROMANO_TEST_CASE(edge_cases)
{
    HashSet<std::string> set;

    STDROMANO_CHECK(set.insert("").second);
    STDROMANO_CHECK(!set.insert("").second);
    STDROMANO_CHECK(set.insert("test").second);
    STDROMANO_CHECK(!set.insert("test").second);
    STDROMANO_CHECK_EQ(set.size(), 2u);

    STDROMANO_CHECK_EQ(set.erase("non-existent"), 0u);
    STDROMANO_CHECK(set.find("non-existent") == set.end());

    STDROMANO_CHECK_EQ(set.erase(""), 1u);
    STDROMANO_CHECK(!set.contains(""));
    STDROMANO_CHECK_EQ(set.size(), 1u);
}

STDROMANO_TEST_CASE(erase_by_iterator)
{
    HashSet<int> set({1, 2, 3});

    set.erase(set.find(2));
    STDROMANO_CHECK_EQ(set.size(), 2u);
    STDROMANO_CHECK(!set.contains(2));

    set.erase(set.end());
    STDROMANO_CHECK_EQ(set.size(), 2u);
}

STDROMANO_TEST_CASE(copy_and_move)
{
    HashSet<std::string> set;

    for(int i = 0; i < 100; ++i)
        set.insert(std::to_string(i));

    HashSet<std::string> copy(set);
    copy.erase("0");
    STDROMANO_CHECK(set.contains("0"));
    STDROMANO_CHECK_EQ(copy.size(), 99u);

    HashSet<std::string> moved(std::move(copy));
    STDROMANO_CHECK_EQ(moved.size(), 99u);
    STDROMANO_CHECK_EQ(copy.size(), 0u);
    STDROMANO_CHECK(!copy.contains("1"));

    copy.insert("reused");
    STDROMANO_CHECK(copy.contains("reused"));

    HashSet<std::string> assigned;
    assigned = set;
    STDROMANO_CHECK_EQ(assigned.size(), 100u);

    HashSet<std::string> move_assigned;
    move_assigned = std::move(assigned);
    STDROMANO_CHECK_EQ(move_assigned.size(), 100u);
}

STDROMANO_TEST_CASE(stress)
{
    std::mt19937 rng(0x5E7);

    const int count = stress_size();

    std::vector<std::int64_t> keys(count);
    std::iota(keys.begin(), keys.end(), 0);
    std::shuffle(keys.begin(), keys.end(), rng);

    HashSet<std::int64_t> set;

    for(const std::int64_t key : keys)
        set.insert(key);

    STDROMANO_REQUIRE_EQ(set.size(), static_cast<std::size_t>(count));

    for(const std::int64_t key : keys)
    {
        const auto result = set.insert(key);
        STDROMANO_REQUIRE(!result.second);
        STDROMANO_REQUIRE_EQ(*result.first, key);
    }

    std::shuffle(keys.begin(), keys.end(), rng);

    std::size_t erased = 0;

    for(int i = 0; i < count / 2; ++i)
        erased += set.erase(keys[i]);

    STDROMANO_CHECK_EQ(erased, static_cast<std::size_t>(count / 2));

    std::size_t found = 0;

    for(const std::int64_t key : keys)
        found += set.count(key);

    STDROMANO_CHECK_EQ(found, static_cast<std::size_t>(count - count / 2));
    STDROMANO_CHECK_EQ(set.size(), found);
}

STDROMANO_TEST_CASE(fuzz_against_unordered_set)
{
    const auto report = fuzz::run_property(fixtures::options("hashset_vs_unordered_set", 400), [](fuzz::Source& source) {
        HashSet<std::int64_t> set;
        std::unordered_set<std::int64_t> reference;

        const std::size_t steps = source.range<std::size_t>(1, 400);
        const std::int64_t key_space = source.pick<std::int64_t>({8, 128, 1 << 20});

        for(std::size_t i = 0; i < steps; ++i)
        {
            const std::int64_t key = source.range<std::int64_t>(-key_space, key_space);

            switch(source.index(5))
            {
                case 0:
                case 1:
                {
                    const auto inserted = set.insert(key);
                    const auto expected = reference.insert(key);

                    STDROMANO_FUZZ_CHECK_EQ(inserted.second, expected.second);
                    STDROMANO_FUZZ_CHECK_EQ(*inserted.first, key);
                    break;
                }
                case 2:
                    STDROMANO_FUZZ_CHECK_EQ(set.erase(key), reference.erase(key));
                    break;
                case 3:
                    STDROMANO_FUZZ_CHECK_EQ(set.contains(key), reference.count(key) == 1);
                    break;
                default:
                    if(source.one_in(20))
                    {
                        set.clear();
                        reference.clear();
                    }
                    break;
            }

            STDROMANO_FUZZ_CHECK_EQ(set.size(), reference.size());
        }

        STDROMANO_FUZZ_CHECK(matches_reference(set, reference));

        return true;
    });

    TESTS_REQUIRE_PROPERTY(report);
}

STDROMANO_TEST_MAIN()
