// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - Present Romain Augier
// All rights reserved.

#include "stdromano/vector.hpp"
#include "test.hpp"

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

    // Constructor with initialization list and with iterator
    {
        stdromano::Vector<TestObject> vec({TestObject("test1"), TestObject("test2")});
        ASSERT_EQUAL(2u, vec.size());
        ASSERT_EQUAL(2u, TestObject::get_total_instances());

        stdromano::Vector<TestObject> vec2(vec.begin(), vec.end());
        ASSERT_EQUAL(2u, vec2.size());
        ASSERT_EQUAL(4u, TestObject::get_total_instances());
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
    for(int i = 0; i < 100; ++i)
    {
        vec.push_back(TestObject(std::to_string(i)));
    }

    ASSERT_EQUAL(100u, vec.size());
    ASSERT(vec.capacity() >= vec.size());
}

TEST_CASE(test_emplace_back)
{
    stdromano::Vector<TestObject> vec;

    vec.emplace_back("emplace1");
    ASSERT_EQUAL(1u, vec.size());
    ASSERT_EQUAL("emplace1", vec[0].get_data());

    vec.emplace_back("emplace2");
    ASSERT_EQUAL(2u, vec.size());
    ASSERT_EQUAL("emplace2", vec[1].get_data());
}

TEST_CASE(test_front_and_back)
{
    stdromano::Vector<TestObject> vec;
    vec.push_back(TestObject("first"));
    vec.push_back(TestObject("middle"));
    vec.push_back(TestObject("last"));

    ASSERT_EQUAL("first", vec.front().get_data());
    ASSERT_EQUAL("last", vec.back().get_data());

    // Test const overloads
    const auto& cvec = vec;
    ASSERT_EQUAL("first", cvec.front().get_data());
    ASSERT_EQUAL("last", cvec.back().get_data());
}

TEST_CASE(test_empty)
{
    stdromano::Vector<TestObject> vec;
    ASSERT(vec.empty());

    vec.push_back(TestObject("x"));
    ASSERT(!vec.empty());

    vec.pop_back();
    ASSERT(vec.empty());
}

TEST_CASE(test_clear)
{
    stdromano::Vector<TestObject> vec;
    for (int i = 0; i < 10; ++i)
        vec.push_back(TestObject(std::to_string(i)));

    ASSERT_EQUAL(10u, vec.size());
    size_t cap_before = vec.capacity();

    vec.clear();
    ASSERT_EQUAL(0u, vec.size());
    // Capacity should remain unchanged after clear
    ASSERT_EQUAL(cap_before, vec.capacity());
}

TEST_CASE(test_clear_empty_vector)
{
    stdromano::Vector<TestObject> vec;
    // Should not crash on empty vector
    vec.clear();
    ASSERT_EQUAL(0u, vec.size());
}

TEST_CASE(test_resize_and_reserve)
{
    stdromano::Vector<TestObject> vec;

    vec.reserve(50);
    ASSERT(vec.capacity() >= 50u);
    ASSERT_EQUAL(0u, vec.size());

    // resize should not shrink
    size_t cap = vec.capacity();
    vec.resize(cap - 1);
    ASSERT_EQUAL(cap, vec.capacity());

    // resize to larger
    vec.resize(cap + 100);
    ASSERT(vec.capacity() >= cap + 100);
}

TEST_CASE(test_insert_single)
{
    stdromano::Vector<TestObject> vec;
    vec.push_back(TestObject("a"));
    vec.push_back(TestObject("c"));

    vec.insert(TestObject("b"), 1);
    ASSERT_EQUAL(3u, vec.size());
    ASSERT_EQUAL("a", vec[0].get_data());
    ASSERT_EQUAL("b", vec[1].get_data());
    ASSERT_EQUAL("c", vec[2].get_data());

    // Insert at front
    vec.insert(TestObject("z"), 0);
    ASSERT_EQUAL(4u, vec.size());
    ASSERT_EQUAL("z", vec[0].get_data());

    // Insert at end
    vec.insert(TestObject("end"), vec.size());
    ASSERT_EQUAL(5u, vec.size());
    ASSERT_EQUAL("end", vec[4].get_data());
}

TEST_CASE(test_insert_count)
{
    stdromano::Vector<TestObject> vec;
    vec.push_back(TestObject("a"));
    vec.push_back(TestObject("d"));

    auto it = vec.insert(vec.cbegin() + 1, 2, TestObject("x"));
    ASSERT_EQUAL(4u, vec.size());
    ASSERT_EQUAL("a", vec[0].get_data());
    ASSERT_EQUAL("x", vec[1].get_data());
    ASSERT_EQUAL("x", vec[2].get_data());
    ASSERT_EQUAL("d", vec[3].get_data());
}

TEST_CASE(test_insert_range)
{
    stdromano::Vector<TestObject> src;
    src.push_back(TestObject("x"));
    src.push_back(TestObject("y"));

    stdromano::Vector<TestObject> dst;
    dst.push_back(TestObject("a"));
    dst.push_back(TestObject("b"));

    dst.insert(dst.begin() + 1, src.begin(), src.end());
    ASSERT_EQUAL(4u, dst.size());
    ASSERT_EQUAL("a", dst[0].get_data());
    ASSERT_EQUAL("x", dst[1].get_data());
    ASSERT_EQUAL("y", dst[2].get_data());
    ASSERT_EQUAL("b", dst[3].get_data());
}

TEST_CASE(test_insert_range_at_end)
{
    stdromano::Vector<TestObject> src;
    src.push_back(TestObject("x"));
    src.push_back(TestObject("y"));

    stdromano::Vector<TestObject> dst;
    dst.push_back(TestObject("a"));

    dst.insert(dst.end(), src.begin(), src.end());
    ASSERT_EQUAL(3u, dst.size());
    ASSERT_EQUAL("a", dst[0].get_data());
    ASSERT_EQUAL("x", dst[1].get_data());
    ASSERT_EQUAL("y", dst[2].get_data());
}

TEST_CASE(test_insert_initializer_list)
{
    stdromano::Vector<TestObject> vec;
    vec.push_back(TestObject("a"));
    vec.push_back(TestObject("d"));

    vec.insert(vec.cbegin() + 1, {TestObject("b"), TestObject("c")});
    ASSERT_EQUAL(4u, vec.size());
    ASSERT_EQUAL("a", vec[0].get_data());
    ASSERT_EQUAL("b", vec[1].get_data());
    ASSERT_EQUAL("c", vec[2].get_data());
    ASSERT_EQUAL("d", vec[3].get_data());
}

TEST_CASE(test_erase_single)
{
    stdromano::Vector<TestObject> vec;
    vec.push_back(TestObject("a"));
    vec.push_back(TestObject("b"));
    vec.push_back(TestObject("c"));

    auto it = vec.erase(vec.cbegin() + 1);
    ASSERT_EQUAL(2u, vec.size());
    ASSERT_EQUAL("a", vec[0].get_data());
    ASSERT_EQUAL("c", vec[1].get_data());
}

TEST_CASE(test_erase_range)
{
    stdromano::Vector<TestObject> vec;
    vec.push_back(TestObject("a"));
    vec.push_back(TestObject("b"));
    vec.push_back(TestObject("c"));
    vec.push_back(TestObject("d"));

    vec.erase(vec.begin() + 1, vec.begin() + 3);
    ASSERT_EQUAL(2u, vec.size());
    ASSERT_EQUAL("a", vec[0].get_data());
    ASSERT_EQUAL("d", vec[1].get_data());
}

TEST_CASE(test_remove)
{
    stdromano::Vector<TestObject> vec;
    vec.push_back(TestObject("a"));
    vec.push_back(TestObject("b"));
    vec.push_back(TestObject("c"));

    vec.remove(0);
    ASSERT_EQUAL(2u, vec.size());
    ASSERT_EQUAL("b", vec[0].get_data());
    ASSERT_EQUAL("c", vec[1].get_data());

    vec.remove(1);
    ASSERT_EQUAL(1u, vec.size());
    ASSERT_EQUAL("b", vec[0].get_data());
}

TEST_CASE(test_find)
{
    stdromano::Vector<TestObject> vec;
    vec.push_back(TestObject("a"));
    vec.push_back(TestObject("b"));
    vec.push_back(TestObject("c"));

    // Find with predicate
    auto it = vec.find([](const TestObject& o) { return o.get_data() == "b"; });
    ASSERT(it != vec.end());
    ASSERT_EQUAL("b", (*it).get_data());

    // Not found
    auto it2 = vec.find([](const TestObject& o) { return o.get_data() == "z"; });
    ASSERT(it2 == vec.end());

    // Find on empty vector
    stdromano::Vector<TestObject> empty_vec;
    auto it3 = empty_vec.find([](const TestObject& o) { return true; });
    ASSERT(it3 == empty_vec.end());
}

TEST_CASE(test_cfind)
{
    stdromano::Vector<TestObject> vec;
    vec.push_back(TestObject("a"));
    vec.push_back(TestObject("b"));

    const auto& cvec = vec;

    auto it = cvec.cfind([](const TestObject& o) { return o.get_data() == "a"; });
    ASSERT(it != cvec.cend());
    ASSERT_EQUAL("a", (*it).get_data());

    auto it2 = cvec.cfind([](const TestObject& o) { return o.get_data() == "z"; });
    ASSERT(it2 == cvec.cend());
}

TEST_CASE(test_iterators)
{
    stdromano::Vector<TestObject> vec;
    vec.push_back(TestObject("a"));
    vec.push_back(TestObject("b"));
    vec.push_back(TestObject("c"));

    // Forward iteration
    size_t count = 0;
    for (auto it = vec.begin(); it != vec.end(); ++it)
        count++;
    ASSERT_EQUAL(3u, count);

    // Range-based for
    count = 0;
    for (const auto& item : vec)
        count++;
    ASSERT_EQUAL(3u, count);

    // Iterator arithmetic
    auto it = vec.begin();
    ASSERT_EQUAL("a", (*it).get_data());
    auto it2 = it + 2;
    ASSERT_EQUAL("c", (*it2).get_data());
    ASSERT_EQUAL(2, it2 - it);

    // Comparison operators
    ASSERT(it < it2);
    ASSERT(it2 > it);
    ASSERT(it <= it);
    ASSERT(it >= it);

    // Decrement
    --it2;
    ASSERT_EQUAL("b", (*it2).get_data());

    // Iterator subscript
    ASSERT_EQUAL("b", it[1].get_data());
}

TEST_CASE(test_const_iterators)
{
    stdromano::Vector<TestObject> vec;
    vec.push_back(TestObject("a"));
    vec.push_back(TestObject("b"));

    const auto& cvec = vec;

    size_t count = 0;
    for (auto it = cvec.begin(); it != cvec.end(); ++it)
        count++;
    ASSERT_EQUAL(2u, count);

    auto cit = cvec.cbegin();
    ASSERT_EQUAL("a", (*cit).get_data());
    ++cit;
    ASSERT_EQUAL("b", (*cit).get_data());
}

TEST_CASE(test_data_pointer)
{
    stdromano::Vector<TestObject> vec;
    ASSERT_EQUAL(nullptr, vec.data());

    vec.push_back(TestObject("x"));
    ASSERT(vec.data() != nullptr);
    ASSERT_EQUAL("x", vec.data()[0].get_data());

    const auto& cvec = vec;
    ASSERT(cvec.data() != nullptr);
}

TEST_CASE(test_memory_usage)
{
    stdromano::Vector<TestObject> vec;
    ASSERT_EQUAL(0u, vec.memory_usage());

    vec.push_back(TestObject("a"));
    ASSERT_EQUAL(sizeof(TestObject) * 1, vec.memory_usage());

    vec.push_back(TestObject("b"));
    ASSERT_EQUAL(sizeof(TestObject) * 2, vec.memory_usage());
}

TEST_CASE(test_growth_under_pressure)
{
    stdromano::Vector<TestObject> vec;
    // Push many elements to trigger multiple growths
    for (int i = 0; i < 1000; ++i)
        vec.push_back(TestObject(std::to_string(i)));

    ASSERT_EQUAL(1000u, vec.size());
    // Verify data integrity after multiple reallocations
    for (int i = 0; i < 1000; ++i)
        ASSERT_EQUAL(std::to_string(i), vec[i].get_data());
}

TEST_CASE(test_self_assignment)
{
    stdromano::Vector<TestObject> vec;
    vec.push_back(TestObject("a"));
    vec.push_back(TestObject("b"));

    vec = vec; // Copy self-assignment
    ASSERT_EQUAL(2u, vec.size());
    ASSERT_EQUAL("a", vec[0].get_data());

    vec = std::move(vec); // Move self-assignment
    ASSERT_EQUAL(2u, vec.size());
    ASSERT_EQUAL("a", vec[0].get_data());
}

TEST_CASE(test_copy_and_move_empty)
{
    stdromano::Vector<TestObject> empty;

    // Copy empty vector
    stdromano::Vector<TestObject> copy(empty);
    ASSERT_EQUAL(0u, copy.size());
    ASSERT_EQUAL(nullptr, copy.data());

    // Move empty vector
    stdromano::Vector<TestObject> moved(std::move(empty));
    ASSERT_EQUAL(0u, moved.size());

    // Assign empty to non-empty
    stdromano::Vector<TestObject> nonempty;
    nonempty.push_back(TestObject("x"));
    nonempty = copy;
    ASSERT_EQUAL(0u, nonempty.size());
}

TEST_CASE(test_constructor_with_zero_count)
{
    stdromano::Vector<TestObject> vec(0, TestObject("x"));
    ASSERT_EQUAL(0u, vec.size());
    ASSERT(vec.capacity() > 0u); // Should allocate MIN_SIZE
}

TEST_CASE(test_pop_back_returns_value)
{
    stdromano::Vector<TestObject> vec;
    vec.push_back(TestObject("val"));

    TestObject popped = vec.pop_back();
    ASSERT_EQUAL("val", popped.get_data());
    ASSERT_EQUAL(0u, vec.size());
}

int main()
{
    TestRunner runner;

    runner.add_test("Constructor and Destructor", test_constructor_and_destructor);
    runner.add_test("Push Back and Pop Back", test_push_back_and_pop_back);
    runner.add_test("Copy and Move", test_copy_and_move);
    runner.add_test("Element Access", test_element_access);
    runner.add_test("Capacity", test_capacity);
    runner.add_test("Emplace Back", test_emplace_back);
    runner.add_test("Front and Back", test_front_and_back);
    runner.add_test("Empty", test_empty);
    runner.add_test("Clear", test_clear);
    runner.add_test("Clear Empty Vector", test_clear_empty_vector);
    runner.add_test("Resize and Reserve", test_resize_and_reserve);
    runner.add_test("Insert Single", test_insert_single);
    runner.add_test("Insert Count", test_insert_count);
    runner.add_test("Insert Range", test_insert_range);
    runner.add_test("Insert Range at End", test_insert_range_at_end);
    runner.add_test("Insert Initializer List", test_insert_initializer_list);
    runner.add_test("Erase Single", test_erase_single);
    runner.add_test("Erase Range", test_erase_range);
    runner.add_test("Remove", test_remove);
    runner.add_test("Find", test_find);
    runner.add_test("CFind", test_cfind);
    runner.add_test("Iterators", test_iterators);
    runner.add_test("Const Iterators", test_const_iterators);
    runner.add_test("Data Pointer", test_data_pointer);
    runner.add_test("Memory Usage", test_memory_usage);
    runner.add_test("Growth Under Pressure", test_growth_under_pressure);
    runner.add_test("Self Assignment", test_self_assignment);
    runner.add_test("Copy and Move Empty", test_copy_and_move_empty);
    runner.add_test("Constructor Zero Count", test_constructor_with_zero_count);
    runner.add_test("Pop Back Returns Value", test_pop_back_returns_value);

    runner.run_all();
    return 0;
}