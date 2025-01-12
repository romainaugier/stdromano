// SPDX-License-Identifier: BSD-3-Clause 
// Copyright (c) 2025 - Present Romain Augier 
// All rights reserved. 

#include "stdromano/vector.h"

#include <cassert>
#include <string>
#include <iostream>

// Test helper to print test names
#define RUN_TEST(name) \
    std::printf("Running " #name "... \n"); \
    name(); \
    std::printf("PASSED\n");

class TestObject 
{
public:
    static int constructor_count;
    static int destructor_count;
    static int move_count;
    static int copy_count;
    
    int value;
    
    TestObject(int v = 0) : value(v) { ++constructor_count; }
    ~TestObject() { ++destructor_count; }
    TestObject(const TestObject& other) : value(other.value) { ++copy_count; }
    TestObject(TestObject&& other) noexcept : value(other.value) 
    { 
        other.value = 0;
        ++move_count;
    }
    
    TestObject& operator=(const TestObject& other) 
    {
        value = other.value;
        ++copy_count;
        return *this;
    }
    
    TestObject& operator=(TestObject&& other) noexcept 
    {
        value = other.value;
        other.value = 0;
        ++move_count;
        return *this;
    }
    
    bool operator==(const TestObject& other) const 
    {
        return value == other.value;
    }
    
    static void reset_counters() 
    {
        constructor_count = 0;
        destructor_count = 0;
        move_count = 0;
        copy_count = 0;
    }
};

int TestObject::constructor_count = 0;
int TestObject::destructor_count = 0;
int TestObject::move_count = 0;
int TestObject::copy_count = 0;

void test_construction() 
{
    // Default construction
    stdromano::Vector<TestObject> v1;
    assert(v1.size() == 0);
    assert(v1.capacity() == 128); // Default capacity
    
    // Initial capacity construction
    stdromano::Vector<TestObject> v2(256);
    assert(v2.capacity() == 256);
    
    // Copy construction
    v1.push_back(TestObject(1));
    v1.push_back(TestObject(2));
    stdromano::Vector<TestObject> v3(v1);
    assert(v3.size() == v1.size());
    assert(v3[0].value == 1);
    assert(v3[1].value == 2);
    
    // Move construction
    stdromano::Vector<TestObject> v4(std::move(v1));
    assert(v4.size() == 2);
    assert(v1.size() == 0); // v1 should be empty after move
}

void test_element_access() 
{
    stdromano::Vector<int> v;
    v.push_back(1);
    v.push_back(2);
    v.push_back(3);
    
    // operator[]
    assert(v[0] == 1);
    assert(v[1] == 2);
    assert(v[2] == 3);
    
    // at()
    assert(*v.at(0) == 1);
    assert(*v.at(1) == 2);
    assert(*v.at(2) == 3);
    
    // front() and back()
    assert(v.front() == 1);
    assert(v.back() == 3);
    
    // data()
    const int* data = v.data();
    assert(data[0] == 1);
    assert(data[1] == 2);
    assert(data[2] == 3);
}

void test_modifiers() 
{
    TestObject::reset_counters();
    stdromano::Vector<TestObject> v;
    
    // push_back
    v.push_back(TestObject(1));
    assert(v.size() == 1);
    assert(v[0].value == 1);
    
    // emplace_back
    auto& ref = v.emplace_back(2);
    assert(v.size() == 2);
    assert(ref.value == 2);
    
    // insert
    v.insert(TestObject(3), 1);
    assert(v.size() == 3);
    assert(v[1].value == 3);
    
    // emplace_at
    auto& ref2 = v.emplace_at(1, 4);
    assert(v.size() == 4);
    assert(ref2.value == 4);
    
    // remove
    v.remove(1);
    assert(v.size() == 3);
    assert(v[1].value == 3);
    
    // pop
    TestObject popped = v.pop();
    assert(v.size() == 2);
    assert(popped.value == 3);
}

void test_iterators() 
{
    stdromano::Vector<int> v;

    for(int i = 0; i < 5; ++i) 
    {
        v.push_back(i);
    }
    
    // Forward iteration
    int sum = 0;
    for(auto it = v.begin(); it != v.end(); ++it) {
        sum += *it;
    }
    assert(sum == 10);
    
    // Const iteration
    sum = 0;
    for(auto it = v.cbegin(); it != v.cend(); ++it) {
        sum += *it;
    }
    assert(sum == 10);
}

void test_capacity() 
{
    stdromano::Vector<int> v;
    
    // reserve
    v.reserve(200);
    assert(v.capacity() >= 200);
    
    // shrink_to_fit
    for(int i = 0; i < 10; ++i) 
    {
        v.push_back(i);
    }

    size_t old_capacity = v.capacity();
    v.shrink_to_fit();
    assert(v.capacity() == v.size());
    assert(v.capacity() < old_capacity);
    
    // Growth
    while(v.size() < v.capacity()) 
    {
        v.push_back(42);
    }

    size_t cap_before_growth = v.capacity();
    v.push_back(42);
    assert(v.capacity() > cap_before_growth);
}

void test_operations() 
{
    stdromano::Vector<int> v;

    for(int i = 5; i >= 0; --i) 
    {
        v.push_back(i);
    }
    
    // find
    auto it = v.find(3);
    assert(it != v.end());
    assert(*it == 3);
    
    // sort
    v.sort([](const void* a, const void* b) 
    {
        return *(const int*)a - *(const int*)b;
    });

    for(size_t i = 1; i < v.size(); ++i) 
    {
        assert(v[i] >= v[i-1]);
    }
}

int main() 
{
    RUN_TEST(test_construction);
    RUN_TEST(test_element_access);
    RUN_TEST(test_modifiers);
    RUN_TEST(test_iterators);
    RUN_TEST(test_capacity);
    RUN_TEST(test_operations);
    
    std::printf("All tests passed\n");

    return 0;
}