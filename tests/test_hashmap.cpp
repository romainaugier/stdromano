// SPDX-License-Identifier: BSD-3-Clause 
// Copyright (c) 2025 - Present Romain Augier 
// All rights reserved. 

#include "stdromano/hashmap.h"

#include "test.h"

#include <string>
#include <random>

// Custom type to test with complex keys
struct ComplexKey 
{
    int id;
    std::string name;

    bool operator==(const ComplexKey& other) const 
    {
        return id == other.id && name == other.name;
    }
};

// Custom hash function for ComplexKey
struct ComplexKeyHash 
{
    size_t operator()(const ComplexKey& key) const 
    {
        return std::hash<int>()(key.id) ^ std::hash<std::string>()(key.name);
    }
};

TEST_CASE(test_basic_operations) 
{
    stdromano::HashMap<int, std::string> map;
    
    // Test initial state
    ASSERT_EQUAL(0u, map.size());
    ASSERT(map.empty());
    
    // Test insertion
    map.insert(1, "one");
    ASSERT_EQUAL(1u, map.size());
    ASSERT(!map.empty());
    
    // Test find
    auto it = map.find(1);
    ASSERT(it != map.end());
    ASSERT_EQUAL("one", it.value());
    
    // Test non-existent key
    ASSERT(map.find(2) == map.end());
    
    // Test erase
    map.erase(1);
    ASSERT_EQUAL(0u, map.size());
    ASSERT(map.empty());
}

TEST_CASE(test_operator_bracket) 
{
    stdromano::HashMap<std::string, int> map;
    
    // Test operator[] insertion
    map["test"] = 42;
    ASSERT_EQUAL(1u, map.size());
    ASSERT_EQUAL(42, map["test"]);
    
    // Test operator[] update
    map["test"] = 24;
    ASSERT_EQUAL(1u, map.size());
    ASSERT_EQUAL(24, map["test"]);
    
    // Test operator[] auto-insertion
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
    
    map.insert(key1, 1.1);
    map.insert(key2, 2.2);
    
    ASSERT_EQUAL(2u, map.size());
    ASSERT_EQUAL(1.1, map.find(key1).value());
    ASSERT_EQUAL(2.2, map.find(key2).value());
}

TEST_CASE(test_iterator) 
{
    stdromano::HashMap<int, int> map;
    const int TEST_SIZE = 10;
    
    // Insert elements
    for(int i = 0; i < TEST_SIZE; ++i)
    {
        map.insert(i, i * i);
    }
    
    // Test iterator traversal
    size_t count = 0;

    for(auto it = map.begin(); it != map.end(); ++it) 
    {
        ASSERT_EQUAL(it.key() * it.key(), it.value());
        ++count;
    }

    ASSERT_EQUAL(TEST_SIZE, count);
    
    // Test const iterator
    const stdromano::HashMap<int, int>& const_map = map;

    count = 0;

    for(auto it = const_map.cbegin(); it != const_map.cend(); ++it) 
    {
        ASSERT_EQUAL(it.key() * it.key(), it.value());
        ++count;
    }

    ASSERT_EQUAL(TEST_SIZE, count);
}

TEST_CASE(test_load_factor_and_rehashing) 
{
    stdromano::HashMap<int, int> map(2); // Start with small capacity
    
    // Insert elements to trigger rehashing
    for(int i = 0; i < 100; ++i) 
    {
        map.insert(i, i);
        ASSERT(map.load_factor() <= 1.0f);
    }
    
    // Verify all elements are still accessible
    for(int i = 0; i < 100; ++i) 
    {
        auto it = map.find(i);
        ASSERT(it != map.end());
        ASSERT_EQUAL(i, it.value());
    }
}

TEST_CASE(test_collisions) 
{
    struct CollisionHash 
    {
        size_t operator()(int) const { return 1; } // Always returns same hash
    };
    
    stdromano::HashMap<int, std::string, CollisionHash> map;
    
    // Insert elements that will all collide
    map.insert(1, "one");
    map.insert(2, "two");
    map.insert(3, "three");
    
    ASSERT_EQUAL(3u, map.size());
    ASSERT_EQUAL("one", map.find(1).value());
    ASSERT_EQUAL("two", map.find(2).value());
    ASSERT_EQUAL("three", map.find(3).value());
}

TEST_CASE(test_clear_and_reserve) 
{
    stdromano::HashMap<int, int> map;
    
    // Test reserve
    map.reserve(100);
    size_t capacity = map.capacity();
    ASSERT(capacity >= 100);
    
    // Insert elements
    for(int i = 0; i < 50; ++i) 
    {
        map.insert(i, i);
    }
    
    // Test clear
    map.clear();
    ASSERT_EQUAL(0u, map.size());
    ASSERT(map.empty());
    ASSERT_EQUAL(capacity, map.capacity()); // Capacity should remain unchanged after clear
}

TEST_CASE(test_edge_cases) 
{
    stdromano::HashMap<std::string, int> map;
    
    // Test empty string key
    map.insert("", 0);
    ASSERT_EQUAL(0, map[""]);
    
    // Test duplicate insertions
    map.insert("test", 1);
    map.insert("test", 2);
    ASSERT_EQUAL(2, map["test"]); // Should update value
    
    // Test erase of non-existent key
    size_t size_before = map.size();
    map.erase("non-existent");
    ASSERT_EQUAL(size_before, map.size());
    
    // Test find with non-existent key
    ASSERT(map.find("non-existent") == map.end());
}

TEST_CASE(test_stress) 
{
    stdromano::HashMap<int, int> map;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(1, 1000000);
    
    // Insert many random elements
    const int TEST_SIZE = 10000;

    for(int i = 0; i < TEST_SIZE; ++i) 
    {
        int key = dis(gen);
        map.insert(key, i);
    }
    
    // Verify load factor
    float load_factor = map.load_factor();
    ASSERT(load_factor > 0.0f && load_factor <= 1.0f);
    
    // Test iteration stability
    size_t count = 0;

    for(auto it = map.begin(); it != map.end(); ++it) 
    {
        ++count;
    }
    ASSERT_EQUAL(count, map.size());
}

int main() 
{
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
    
    runner.run_all();

    return 0;
}