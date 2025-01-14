// SPDX-License-Identifier: BSD-3-Clause 
// Copyright (c) 2025 - Present Romain Augier 
// All rights reserved. 

#include "stdromano/vector.h"
#include "test.h"

#include <string>

INIT_TEST_OBJECT;

TEST_CASE(test_constructor_and_destructor) 
{
    // Default constructor
    {
        stdromano::Vector<TestObject> vec;
        ASSERT_EQUAL(0u, vec.size());
        ASSERT_EQUAL(0u, TestObject::get_total_instances());
    }

    // Constructor with size
    {
        stdromano::Vector<TestObject> vec(5, TestObject("test"));
        ASSERT_EQUAL(5u, vec.size());
        ASSERT_EQUAL(5u, TestObject::get_total_instances());
    }
}

TEST_CASE(test_push_back_and_pop_back) 
{
    stdromano::Vector<TestObject> vec;
    
    // Push back
    vec.push_back(TestObject("first"));
    ASSERT_EQUAL(1u, vec.size());
    ASSERT_EQUAL("first", vec[0].get_data());

    vec.push_back(TestObject("second"));
    ASSERT_EQUAL(2u, vec.size());
    ASSERT_EQUAL("second", vec[1].get_data());

    // Pop back
    vec.pop_back();
    ASSERT_EQUAL(1u, vec.size());
    ASSERT_EQUAL("first", vec[0].get_data());
}

TEST_CASE(test_copy_and_move) 
{
    stdromano::Vector<TestObject> vec1;
    vec1.push_back(TestObject("test1"));
    vec1.push_back(TestObject("test2"));

    // Test copy constructor
    stdromano::Vector<TestObject> vec2(vec1);
    ASSERT_EQUAL(vec1.size(), vec2.size());
    ASSERT_EQUAL(vec1[0].get_data(), vec2[0].get_data());

    // Test move constructor
    stdromano::Vector<TestObject> vec3(std::move(vec1));
    ASSERT_EQUAL(2u, vec3.size());
    ASSERT_EQUAL(0u, vec1.size()); // vec1 should be empty after move

    // Test copy assignment
    stdromano::Vector<TestObject> vec4;
    vec4 = vec2;
    ASSERT_EQUAL(vec2.size(), vec4.size());
    ASSERT_EQUAL(vec2[0].get_data(), vec4[0].get_data());

    // Test move assignment
    stdromano::Vector<TestObject> vec5;
    vec5 = std::move(vec2);
    ASSERT_EQUAL(2u, vec5.size());
    ASSERT_EQUAL(0u, vec2.size()); // vec2 should be empty after move
}

TEST_CASE(test_element_access) 
{
    stdromano::Vector<TestObject> vec;
    vec.push_back(TestObject("first"));
    vec.push_back(TestObject("second"));

    // Test operator[]
    ASSERT_EQUAL("first", vec[0].get_data());
    ASSERT_EQUAL("second", vec[1].get_data());

    // Test at() with valid index
    ASSERT_EQUAL("first", vec.at(0)->get_data());

    // Test at() with invalid index
    // ASSERT_EQUAL(vec.at(2), nullptr);
}

TEST_CASE(test_capacity) 
{
    stdromano::Vector<TestObject> vec;
    
    // Test initial capacity
    ASSERT_EQUAL(0u, vec.size());
    
    // Test capacity growth
    for (int i = 0; i < 100; ++i) {
        vec.push_back(TestObject(std::to_string(i)));
    }
    
    ASSERT_EQUAL(100u, vec.size());
    ASSERT(vec.capacity() >= vec.size());
}

int main() 
{
    TestRunner runner;
    
    runner.add_test("Constructor and Destructor", test_constructor_and_destructor);
    runner.add_test("Push Back and Pop Back", test_push_back_and_pop_back);
    runner.add_test("Copy and Move", test_copy_and_move);
    runner.add_test("Element Access", test_element_access);
    runner.add_test("Capacity", test_capacity);
    
    runner.run_all();

    return 0;
}