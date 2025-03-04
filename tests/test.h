// SPDX-License-Identifier: BSD-3-Clause 
// Copyright (c) 2025 - Present Romain Augier 
// All rights reserved. 

#if !defined(__STDROMANO_TEST)
#define __STDROMANO_TEST

#include <functional>
#include <vector>
#include <string>

#define TEST_CASE(name) void name()

#define ASSERT(condition) \
    do \
    { \
        if(!(condition)) \
        { \
            std::printf("Assertion failed: %s\nFile: %s\nLine: %d\n", #condition, __FILE__, __LINE__); \
            std::abort(); \
        } \
    } \
    while (0)

#define ASSERT_THROWS(expression, exception_type) \
    do \
    { \
        bool caught = false; \
        try \
        { \
            expression; \
        } \
        catch (const exception_type&) \
        { \
            caught = true; \
        } \
        catch (...) \
        { \
            throw std::runtime_error("Wrong exception type caught"); \
        } \
        if (!caught) \
        { \
            throw std::runtime_error("Expected exception not thrown"); \
        } \
    } \
    while (0)

#define ASSERT_EQUAL(expected, actual) \
    do \
    { \
        if(!((expected) == (actual))) \
        { \
            std::printf("Assertion failed\nExpected: %sActual: %s\nFile: %s\nLine: %d\n", #expected, #actual, __FILE__, __LINE__); \
            std::abort(); \
        } \
    } \
    while (0)

class TestRunner 
{
    struct TestCase 
    {
        const char* name;
        std::function<void()> func;
    };

    std::vector<TestCase> tests;
    int passed = 0;
    int failed = 0;

public:
    void add_test(const char* name, std::function<void()> test) 
    {
        tests.push_back({name, test});
    }

    void run_all() 
    {
        for(const auto& test : tests) 
        {
            std::printf("Running %s ...\n", test.name);

            try 
            {
                test.func();
                std::printf("PASSED\n");
                ++passed;
            } 
            catch (const std::exception& e) 
            {
                std::printf("FAILED: %s\n", e.what());
                ++failed;
            }
        }

        std::printf("Test Summary:\nPassed: %d\nFailed: %d\nTotal: %zu\n", passed, failed, tests.size());
    }
};

class TestObject 
{
private:
    std::string* data;
    size_t ref_count;
    static size_t total_instances;

public:
    TestObject(const std::string& str = "") : ref_count(0) 
    {
        this->data = new std::string(str);
        ++TestObject::total_instances;
    }

    TestObject(const TestObject& other) : ref_count(other.ref_count) 
    {
        this->data = new std::string(*other.data);
        ++TestObject::total_instances;
    }

    TestObject(TestObject&& other) noexcept : data(other.data), ref_count(other.ref_count) 
    {
        other.data = nullptr;
    }

    TestObject& operator=(const TestObject& other) 
    {
        if(this != &other) 
        {
            delete this->data;
            this->data = new std::string(*other.data);
            this->ref_count = other.ref_count;
        }

        return *this;
    }

    TestObject& operator=(TestObject&& other) noexcept 
    {
        if(this != &other) 
        {
            delete this->data;
            this->data = other.data;
            this->ref_count = other.ref_count;
            other.data = nullptr;
        }

        return *this;
    }

    ~TestObject() 
    {
        if(this->data != nullptr) 
        {
            delete this->data;
            --this->total_instances;
        }
    }

    bool operator==(const TestObject& other) const 
    {
        if(this->data == nullptr || other.data == nullptr) return this->data == other.data;
        return *this->data == *other.data;
    }

    bool operator!=(const TestObject& other) const 
    {
        return !(*this == other);
    }

    std::string get_data() const 
    {
        return this->data ? *this->data : "";
    }

    static size_t get_total_instances() 
    {
        return TestObject::total_instances;
    }
};

#define INIT_TEST_OBJECT size_t TestObject::total_instances = 0;

#endif // !defined(__STDROMANO_TEST)