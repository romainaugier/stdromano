// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - Present Romain Augier
// All rights reserved.

#if !defined(__STDROMANO_TEST)
#define __STDROMANO_TEST

#include "stdromano/string.hpp"
#include "stdromano/vector.hpp"

#include "spdlog/spdlog.h"

#include <functional>

#define TEST_CASE(name) void name()

#define ASSERT(condition)                                                                          \
    do                                                                                             \
    {                                                                                              \
        if(!(condition))                                                                           \
        {                                                                                          \
            spdlog::error("Assertion failed: {}\nFile: {}\nLine: {}",                              \
                          #condition,                                                              \
                          __FILE__,                                                                \
                          __LINE__);                                                               \
            std::abort();                                                                          \
        }                                                                                          \
    } while(0)

#define ASSERT_THROWS(expression, exception_type)                                                  \
    do                                                                                             \
    {                                                                                              \
        bool caught = false;                                                                       \
        try                                                                                        \
        {                                                                                          \
            expression;                                                                            \
        }                                                                                          \
        catch(const exception_type&)                                                               \
        {                                                                                          \
            caught = true;                                                                         \
        }                                                                                          \
        catch(...)                                                                                 \
        {                                                                                          \
            throw std::runtime_error("Wrong exception type caught");                               \
        }                                                                                          \
        if(!caught)                                                                                \
        {                                                                                          \
            throw std::runtime_error("Expected exception not thrown");                             \
        }                                                                                          \
    } while(0)

#define ASSERT_EQUAL(expected, actual)                                                             \
do                                                                                                 \
    {                                                                                              \
        if(!((expected) == (actual)))                                                              \
        {                                                                                          \
            spdlog::error("Assertion failed\nExpected: {}\nActual: "                                 \
                          "{}\nFile: {}\nLine: {}",                                                \
                          #expected,                                                               \
                          #actual,                                                                 \
                          __FILE__,                                                                \
                          __LINE__);                                                               \
            std::abort();                                                                          \
        }                                                                                          \
    } while(0)

class TestRunner
{
    struct TestCase
    {
        const char* name;
        std::function<void()> func;
    };

    stdromano::Vector<TestCase> tests;

    int passed = 0;
    int failed = 0;

    const char* name = nullptr;

public:
    TestRunner(const char* name = nullptr) : name(name)
    {
        spdlog::set_level(spdlog::level::trace);
    }

    void add_test(const char* test_name, std::function<void()> test)
    {
        tests.push_back({test_name, test});
    }

    void run_all()
    {
        if(this->name != nullptr)
            spdlog::info("Starting {} test", this->name);

        for(const auto& test : this->tests)
        {
            spdlog::info("Running {} ...", test.name);

            try
            {
                test.func();
                spdlog::info("PASSED");
                ++passed;
            }
            catch(const std::exception& e)
            {
                spdlog::error("FAILED: {}", e.what());
                ++failed;
            }
        }

        spdlog::info("Test Summary:\nPassed: {}\nFailed: {}\nTotal: {}",
                     passed,
                     failed,
                     tests.size());

        if(this->name != nullptr)
            spdlog::info("Finished {} test", this->name);
    }
};

class TestObject
{
  private:
    stdromano::StringD* data = nullptr;
    size_t ref_count;
    static size_t total_instances;

  public:
    TestObject(const stdromano::StringD& str = "")
        : ref_count(0)
    {
        this->data = new stdromano::StringD(str);
        ++TestObject::total_instances;
    }

    TestObject(const TestObject& other)
        : ref_count(other.ref_count)
    {
        this->data = new stdromano::StringD(*other.data);
        ++TestObject::total_instances;
    }

    TestObject(TestObject&& other) noexcept
        : data(other.data),
          ref_count(other.ref_count)
    {
        other.data = nullptr;
    }

    TestObject& operator=(const TestObject& other)
    {
        if(this != &other)
        {
            delete this->data;

            this->data = new stdromano::StringD(*other.data);
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
        if(this->data == nullptr || other.data == nullptr)
            return this->data == other.data;

        return *this->data == *other.data;
    }

    bool operator!=(const TestObject& other) const
    {
        return !(*this == other);
    }

    stdromano::StringD get_data() const
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
