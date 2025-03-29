


#include "stdromano/hashset.h"

#define STDROMANO_ENABLE_PROFILING
#include "stdromano/profiling.h"

#include "test.h"

#include <numeric>
#include <random>
#include <set>
#include <string>
#include <vector>

struct ComplexKey
{
    int id;
    std::string name;

    bool operator==(const ComplexKey& other) const { return id == other.id && name == other.name; }

    bool operator<(const ComplexKey& other) const
    {
        if(id != other.id)
            return id < other.id;
        return name < other.name;
    }
};

struct ComplexKeyHash
{
    size_t operator()(const ComplexKey& key) const
    {
        size_t h1 = std::hash<int>()(key.id);
        size_t h2 = std::hash<std::string>()(key.name);
        return h1 ^ (h2 << 1);
    }
};

TEST_CASE(test_set_basic_operations)
{
    stdromano::HashSet<int> my_set;

    ASSERT_EQUAL(0u, my_set.size());
    ASSERT(my_set.empty());

    auto result1 = my_set.insert(1);
    ASSERT(result1.second);
    ASSERT_EQUAL(1, *result1.first);
    ASSERT_EQUAL(1u, my_set.size());
    ASSERT(!my_set.empty());

    auto result2 = my_set.insert(1);
    ASSERT(!result2.second);
    ASSERT_EQUAL(1, *result2.first);
    ASSERT_EQUAL(1u, my_set.size());

    auto it = my_set.find(1);
    ASSERT(it != my_set.end());
    ASSERT_EQUAL(1, *it);

    ASSERT(my_set.find(2) == my_set.end());
    ASSERT(!my_set.contains(2));
    ASSERT(my_set.contains(1));

    size_t erased_count = my_set.erase(1);
    ASSERT_EQUAL(1u, erased_count);
    ASSERT_EQUAL(0u, my_set.size());
    ASSERT(my_set.empty());
    ASSERT(!my_set.contains(1));


    erased_count = my_set.erase(1);
    ASSERT_EQUAL(0u, erased_count);
}

TEST_CASE(test_set_complex_key)
{
    stdromano::HashSet<ComplexKey, ComplexKeyHash> set;

    ComplexKey key1{1, "one"};
    ComplexKey key2{2, "two"};
    ComplexKey key1_dup{1, "one"};

    auto res1 = set.insert(key1);
    ASSERT(res1.second);
    ASSERT_EQUAL(key1, *res1.first);

    auto res2 = set.insert(key2);
    ASSERT(res2.second);
    ASSERT_EQUAL(key2, *res2.first);

    ASSERT_EQUAL(2u, set.size());

    auto res_dup = set.insert(key1_dup);
    ASSERT(!res_dup.second);
    ASSERT_EQUAL(key1, *res_dup.first);
    ASSERT_EQUAL(2u, set.size());

    auto it1 = set.find(key1);
    ASSERT(it1 != set.end());
    ASSERT_EQUAL(key1, *it1);

    auto it2 = set.find(key2);
    ASSERT(it2 != set.end());
    ASSERT_EQUAL(key2, *it2);

    ASSERT(set.contains(key1));
    ASSERT(set.contains(key2));
    ASSERT(!set.contains({3, "three"}));

    ASSERT_EQUAL(1u, set.erase(key1));
    ASSERT_EQUAL(1u, set.size());
    ASSERT(!set.contains(key1));
    ASSERT(set.contains(key2));
}

TEST_CASE(test_set_iterator)
{
    stdromano::HashSet<int> set;
    const int TEST_SIZE = 10;
    std::set<int> reference_set;

    for(int i = 0; i < TEST_SIZE; ++i)
    {
        set.insert(i);
        reference_set.insert(i);
    }
    ASSERT_EQUAL(static_cast<size_t>(TEST_SIZE), set.size());

    size_t count = 0;
    for(auto it = set.begin(); it != set.end(); ++it)
    {

        ASSERT(reference_set.count(*it) == 1);
        ++count;
    }
    ASSERT_EQUAL(static_cast<size_t>(TEST_SIZE), count);


    count = 0;
    for(const auto& key : set)
    {
        ASSERT(reference_set.count(key) == 1);
        count++;
    }
    ASSERT_EQUAL(static_cast<size_t>(TEST_SIZE), count);


    const stdromano::HashSet<int>& const_set = set;
    count = 0;
    for(auto it = const_set.cbegin(); it != const_set.cend(); ++it)
    {
        ASSERT(reference_set.count(*it) == 1);
        ++count;
    }
    ASSERT_EQUAL(static_cast<size_t>(TEST_SIZE), count);
}

TEST_CASE(test_set_load_factor_and_rehashing)
{

    stdromano::HashSet<int> set(2);
    size_t initial_capacity = set.capacity();
    ASSERT(initial_capacity >= 2);

    const int INSERT_COUNT = 100;
    for(int i = 0; i < INSERT_COUNT; ++i)
    {
        set.insert(i);

        ASSERT(set.load_factor() <= 1.0f);
    }

    ASSERT_EQUAL(static_cast<size_t>(INSERT_COUNT), set.size());
    ASSERT(set.capacity() > initial_capacity);


    for(int i = 0; i < INSERT_COUNT; ++i)
    {
        ASSERT(set.contains(i));
    }
}

TEST_CASE(test_set_collisions)
{

    struct CollisionHash
    {
        size_t operator()(int) const { return 1; }
    };

    stdromano::HashSet<int, CollisionHash> set;

    set.insert(1);
    set.insert(10);
    set.insert(20);

    ASSERT_EQUAL(3u, set.size());
    ASSERT(set.contains(1));
    ASSERT(set.contains(10));
    ASSERT(set.contains(20));
    ASSERT(!set.contains(5));

    ASSERT_EQUAL(1u, set.erase(10));
    ASSERT_EQUAL(2u, set.size());
    ASSERT(set.contains(1));
    ASSERT(!set.contains(10));
    ASSERT(set.contains(20));
}

TEST_CASE(test_set_clear_and_reserve)
{
    stdromano::HashSet<int> set;

    set.reserve(100);
    size_t capacity_after_reserve = set.capacity();
    ASSERT(capacity_after_reserve >= 100);

    for(int i = 0; i < 50; ++i)
    {
        set.insert(i);
    }
    ASSERT_EQUAL(50u, set.size());

    set.clear();
    ASSERT_EQUAL(0u, set.size());
    ASSERT(set.empty());

    ASSERT_EQUAL(capacity_after_reserve, set.capacity());

    set.insert(100);
    ASSERT_EQUAL(1u, set.size());
    ASSERT(set.contains(100));
}

TEST_CASE(test_set_edge_cases)
{
    stdromano::HashSet<std::string> set;

    auto res_empty = set.insert("");
    ASSERT(res_empty.second);
    ASSERT_EQUAL(1u, set.size());
    ASSERT(set.contains(""));

    res_empty = set.insert("");
    ASSERT(!res_empty.second);
    ASSERT_EQUAL(1u, set.size());

    auto res_test = set.insert("test");
    ASSERT(res_test.second);
    ASSERT_EQUAL(2u, set.size());

    res_test = set.insert("test");
    ASSERT(!res_test.second);
    ASSERT_EQUAL(2u, set.size());

    size_t size_before = set.size();
    ASSERT_EQUAL(0u, set.erase("non-existent"));
    ASSERT_EQUAL(size_before, set.size());

    ASSERT(set.find("non-existent") == set.end());

    ASSERT_EQUAL(1u, set.erase(""));
    ASSERT(!set.contains(""));
    ASSERT_EQUAL(1u, set.size());
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

TEST_CASE(test_set_stress)
{
    stdromano::HashSet<int64_t> set;

#if defined(DEBUG_BUILD)
    const int TEST_SIZE = 10000;
#else
    const int TEST_SIZE = 100000;
#endif

    std::vector<std::int64_t> keys = get_random_shuffle_range_ints(TEST_SIZE);

    SCOPED_PROFILE_START(stdromano::ProfileUnit::MilliSeconds, set_insert);
    for(int i = 0; i < TEST_SIZE; ++i)
    {
        set.insert(keys[i]);
    }
    SCOPED_PROFILE_STOP(set_insert);

    ASSERT_EQUAL(static_cast<size_t>(TEST_SIZE), set.size());
    float load_factor = set.load_factor();
    ASSERT(load_factor > 0.0f && load_factor <= 1.0f);

    size_t count = 0;
    SCOPED_PROFILE_START(stdromano::ProfileUnit::MilliSeconds, set_iterate);
    for(auto it = set.begin(); it != set.end(); ++it)
    {

        ASSERT(set.contains(*it));
        ++count;
    }
    SCOPED_PROFILE_STOP(set_iterate);
    ASSERT_EQUAL(count, set.size());

    std::shuffle(keys.begin(), keys.end(), generator);
    SCOPED_PROFILE_START(stdromano::ProfileUnit::MilliSeconds, set_erase);
    size_t erase_count = 0;
    for(int i = 0; i < TEST_SIZE / 2; i++)
    {
        erase_count += set.erase(keys[i]);
    }
    SCOPED_PROFILE_STOP(set_erase);
    ASSERT_EQUAL(static_cast<size_t>(TEST_SIZE / 2), erase_count);
    ASSERT_EQUAL(static_cast<size_t>(TEST_SIZE - TEST_SIZE / 2), set.size());

    std::shuffle(keys.begin(), keys.end(), generator);
    size_t num_found = 0;
    size_t num_not_found = 0;
    SCOPED_PROFILE_START(stdromano::ProfileUnit::MilliSeconds, set_find);
    for(int i = 0; i < TEST_SIZE; ++i)
    {
        if(set.contains(keys[i]))
        {
            num_found++;
        }
        else
        {
            num_not_found++;
        }
    }
    SCOPED_PROFILE_STOP(set_find);

    ASSERT_EQUAL(num_found, set.size());
    ASSERT_EQUAL(num_found, static_cast<size_t>(TEST_SIZE - TEST_SIZE / 2));
    ASSERT_EQUAL(num_not_found, static_cast<size_t>(TEST_SIZE / 2));
}

TEST_CASE(test_set_stress_lookup_and_failed_insert)
{
    stdromano::HashSet<int64_t> set;

#if defined(DEBUG_BUILD)
    const int TEST_SIZE = 10000;
#else
    const int TEST_SIZE = 100000;
#endif

    std::vector<int64_t> keys = get_random_shuffle_range_ints(TEST_SIZE);

    for(int i = 0; i < TEST_SIZE; ++i)
    {
        set.insert(keys[i]);
    }
    ASSERT_EQUAL(static_cast<size_t>(TEST_SIZE), set.size());


    SCOPED_PROFILE_START(stdromano::ProfileUnit::MilliSeconds, set_find_existing);
    for(int i = 0; i < TEST_SIZE; ++i)
    {
        ASSERT(set.find(keys[i]) != set.end());
    }
    SCOPED_PROFILE_STOP(set_find_existing);

    SCOPED_PROFILE_START(stdromano::ProfileUnit::MilliSeconds, set_insert_existing);
    size_t failed_insert_count = 0;
    for(int i = 0; i < TEST_SIZE; ++i)
    {
        auto result = set.insert(keys[i]);
        ASSERT(!result.second);
        ASSERT_EQUAL(keys[i], *result.first);
        if(!result.second)
        {
            failed_insert_count++;
        }
    }
    SCOPED_PROFILE_STOP(set_insert_existing);
    ASSERT_EQUAL(static_cast<size_t>(TEST_SIZE), failed_insert_count);
    ASSERT_EQUAL(static_cast<size_t>(TEST_SIZE), set.size());
}

int main()
{
    TestRunner runner;

    runner.add_test("HashSet Basic Operations", test_set_basic_operations);
    runner.add_test("HashSet Complex Key", test_set_complex_key);
    runner.add_test("HashSet Iterator", test_set_iterator);
    runner.add_test("HashSet Load Factor and Rehashing", test_set_load_factor_and_rehashing);
    runner.add_test("HashSet Collisions", test_set_collisions);
    runner.add_test("HashSet Clear and Reserve", test_set_clear_and_reserve);
    runner.add_test("HashSet Edge Cases", test_set_edge_cases);
    runner.add_test("HashSet Stress Test", test_set_stress);
    runner.add_test("HashSet Stress Lookup/Failed Insert", test_set_stress_lookup_and_failed_insert);

    runner.run_all();

    return 0;
}