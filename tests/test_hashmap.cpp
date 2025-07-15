// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - Present Romain Augier
// All rights reserved.

#include "stdromano/hashmap.h"

#define STDROMANO_ENABLE_PROFILING
#include "stdromano/profiling.h"

#include "test.h"

#include <numeric>
#include <random>
#include <string>
#include <vector>

struct ComplexKey
{
    int id;
    std::string name;

    bool operator==(const ComplexKey& other) const
    {
        return id == other.id && name == other.name;
    }
};

struct ComplexKeyHash
{
    size_t operator()(const ComplexKey& key) const
    {
        return std::hash<int>()(key.id) ^ std::hash<std::string>()(key.name);
    }
};

TEST_CASE(test_basic_operations)
{
    stdromano::HashMap<int, std::string> my_map;

    ASSERT_EQUAL(0u, my_map.size());
    ASSERT(my_map.empty());

    my_map.insert(std::pair<int, std::string>(1, "one"));
    ASSERT_EQUAL(1u, my_map.size());
    ASSERT(!my_map.empty());

    auto it = my_map.find(1);
    ASSERT(it != my_map.end());
    ASSERT_EQUAL("one", it->second);

    ASSERT(my_map.find(2) == my_map.end());

    my_map.erase(1);
    ASSERT_EQUAL(0u, my_map.size());
    ASSERT(my_map.empty());
}

TEST_CASE(test_operator_bracket)
{
    stdromano::HashMap<std::string, int> map;

    map["test"] = 42;
    ASSERT_EQUAL(1u, map.size());
    ASSERT_EQUAL(42, map["test"]);

    map["test"] = 24;
    ASSERT_EQUAL(1u, map.size());
    ASSERT_EQUAL(24, map["test"]);

    int& value = map["new"];
    ASSERT_EQUAL(2u, map.size());
    value = 100;
    ASSERT_EQUAL(100, map["new"]);
}

TEST_CASE(test_complex_key)
{
    stdromano::HashMap<ComplexKey, double, ComplexKeyHash> map;

    ComplexKey key1{1, "one"};
    ComplexKey key2{2, "two"};

    map.insert(std::pair<ComplexKey, double>(key1, 1.1));
    map.insert(std::pair<ComplexKey, double>(key2, 2.2));

    ASSERT_EQUAL(2u, map.size());
    ASSERT_EQUAL(1.1, map.find(key1)->second);
    ASSERT_EQUAL(2.2, map.find(key2)->second);
}

TEST_CASE(test_iterator)
{
    stdromano::HashMap<int, int> map;
    const int TEST_SIZE = 10;

    for(int i = 0; i < TEST_SIZE; ++i)
    {
        map.insert(std::pair<int, int>(i, i * i));
    }

    size_t count = 0;

    for(auto it = map.begin(); it != map.end(); ++it)
    {
        ASSERT_EQUAL(it->first * it->first, it->second);
        ++count;
    }

    ASSERT_EQUAL(TEST_SIZE, count);

    const stdromano::HashMap<int, int>& const_map = map;

    count = 0;

    for(auto it = const_map.cbegin(); it != const_map.cend(); ++it)
    {
        ASSERT_EQUAL(it->first * it->first, it->second);
        ++count;
    }

    ASSERT_EQUAL(TEST_SIZE, count);
}

TEST_CASE(test_load_factor_and_rehashing)
{
    stdromano::HashMap<int, int> map(2);

    for(int i = 0; i < 100; ++i)
    {
        map.insert(std::pair<int, int>(i, i));
        ASSERT(map.load_factor() <= 1.0f);
    }

    for(int i = 0; i < 100; ++i)
    {
        auto it = map.find(i);
        ASSERT(it != map.end());
        ASSERT_EQUAL(i, it->second);
    }
}

TEST_CASE(test_collisions)
{
    struct CollisionHash
    {
        size_t operator()(int) const
        {
            return 1;
        }
    };

    stdromano::HashMap<int, std::string, CollisionHash> map;

    map.insert(std::pair<int, std::string>(1, "one"));
    map.insert(std::pair<int, std::string>(2, "two"));
    map.insert(std::pair<int, std::string>(3, "three"));

    ASSERT_EQUAL(3u, map.size());
    ASSERT_EQUAL("one", map.find(1)->second);
    ASSERT_EQUAL("two", map.find(2)->second);
    ASSERT_EQUAL("three", map.find(3)->second);
}

TEST_CASE(test_clear_and_reserve)
{
    stdromano::HashMap<int, int> map;

    map.reserve(100);
    size_t capacity = map.capacity();
    ASSERT(capacity >= 100);

    for(int i = 0; i < 50; ++i)
    {
        map.insert(std::pair<int, int>(i, i));
    }

    map.clear();
    ASSERT_EQUAL(0u, map.size());
    ASSERT(map.empty());
    ASSERT_EQUAL(capacity, map.capacity());
}

TEST_CASE(test_edge_cases)
{
    stdromano::HashMap<std::string, int> map;

    map.insert(std::pair<std::string, int>("", 0));
    ASSERT_EQUAL(0, map[""]);

    map.insert(std::pair<std::string, int>("test", 1));

    map.insert(std::pair<std::string, int>("test", 2));
    ASSERT_EQUAL(2, map["test"]);

    size_t size_before = map.size();
    map.erase("non-existent");
    ASSERT_EQUAL(size_before, map.size());

    ASSERT(map.find("non-existent") == map.end());
}

static std::random_device rd;
static std::mt19937 generator(rd());

std::vector<std::int64_t> get_random_shuffle_range_ints(std::size_t nb_ints)
{
    std::vector<std::int64_t> random_shuffle_ints(nb_ints);
    std::iota(random_shuffle_ints.begin(), random_shuffle_ints.end(), 0);
    std::shuffle(random_shuffle_ints.begin(), random_shuffle_ints.end(), generator);

    return random_shuffle_ints;
}

TEST_CASE(test_stress)
{
    stdromano::HashMap<int64_t, int64_t> map;

#if defined(DEBUG_BUILD)
    const int TEST_SIZE = 10000;
#else
    const int TEST_SIZE = 1000000;
#endif // DEBUG_BUILD

    std::vector<std::int64_t> keys = get_random_shuffle_range_ints(TEST_SIZE);

    SCOPED_PROFILE_START(stdromano::ProfileUnit::MilliSeconds, map_insert);

    for(int i = 0; i < TEST_SIZE; ++i)
    {
        map.insert(std::pair<int64_t, int64_t>(keys[i], 1));
    }

    SCOPED_PROFILE_STOP(map_insert);

    float load_factor = map.load_factor();
    ASSERT(load_factor > 0.0f && load_factor <= 1.0f);

    size_t count = 0;

    SCOPED_PROFILE_START(stdromano::ProfileUnit::MilliSeconds, map_iterate);

    for(auto it = map.begin(); it != map.end(); ++it)
    {
        ++count;
    }

    SCOPED_PROFILE_STOP(map_iterate);

    ASSERT_EQUAL(count, map.size());

    std::shuffle(keys.begin(), keys.end(), generator);

    SCOPED_PROFILE_START(stdromano::ProfileUnit::MilliSeconds, map_erase);

    for(int i = 0; i < TEST_SIZE / 2; i++)
    {
        map.erase(keys[i]);
    }

    SCOPED_PROFILE_STOP(map_erase);

    std::shuffle(keys.begin(), keys.end(), generator);

    size_t num_found = 0;

    SCOPED_PROFILE_START(stdromano::ProfileUnit::MilliSeconds, map_find);

    for(int i = 0; i < TEST_SIZE; ++i)
    {
        if(map.find(keys[i]) != map.end())
        {
            num_found++;
        }
    }

    SCOPED_PROFILE_STOP(map_find);

    ASSERT_EQUAL(num_found, TEST_SIZE / 2);
}

TEST_CASE(test_stress_emplace_vs_find_existing)
{
    stdromano::HashMap<int64_t, std::string> map;

#if defined(DEBUG_BUILD)
    const int TEST_SIZE = 10000;
#else
    const int TEST_SIZE = 1000000;
#endif // DEBUG_BUILD

    std::vector<std::int64_t> keys = get_random_shuffle_range_ints(TEST_SIZE);

    for(int i = 0; i < TEST_SIZE; ++i)
    {
        map.insert(std::pair<int64_t, std::string>(keys[i], std::string(1000, 'a')));
    }

    SCOPED_PROFILE_START(stdromano::ProfileUnit::MilliSeconds, find_time);
    for(int i = 0; i < TEST_SIZE; ++i)
    {
        auto it = map.find(keys[i]);
        ASSERT(it != map.end());
    }
    SCOPED_PROFILE_STOP(find_time);
}

TEST_CASE(test_initializer_list)
{
    stdromano::HashMap<std::int64_t, std::string> map = {
        { 0, "zero" },
        { 1, "one" },
        { 2, "two" },
        { 3, "three" },
    };

    ASSERT(map.contains(0));
    ASSERT(map.contains(1));
    ASSERT(map.contains(2));
    ASSERT(map.contains(3));
    ASSERT(!map.contains(6));
}

int main()
{
    stdromano::set_log_level(stdromano::LogLevel::Debug);

    TestRunner runner;

    runner.add_test("Basic Operations", test_basic_operations);
    runner.add_test("Operator []", test_operator_bracket);
    runner.add_test("Complex Key", test_complex_key);
    runner.add_test("Iterator", test_iterator);
    runner.add_test("Load Factor and Rehashing", test_load_factor_and_rehashing);
    runner.add_test("Collisions", test_collisions);
    runner.add_test("Clear and Reserve", test_clear_and_reserve);
    runner.add_test("Edge Cases", test_edge_cases);
    runner.add_test("Stress Test", test_stress);
    runner.add_test("Stress Emplace vs Find Existing", test_stress_emplace_vs_find_existing);
    runner.add_test("Test Initializer list construction", test_initializer_list);

    runner.run_all();

    return 0;
}