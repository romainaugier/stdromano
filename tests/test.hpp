// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - Present Romain Augier
// All rights reserved.

#if !defined(__STDROMANO_TEST)
#define __STDROMANO_TEST

// stdromano's own tests, on top of the library harness (stdromano/test.hpp). The old macro names
// are kept so the tests did not have to be rewritten; new tests can use the STDROMANO_ ones.
//
// Unlike before, a failed ASSERT ends the current case instead of the process: the other cases
// still run, and run_all() returns 1, so main has to return it for ctest to see the failure

#include "stdromano/string.hpp"
#include "stdromano/test.hpp"
#include "stdromano/vector.hpp"

#include "spdlog/spdlog.h"

#include <functional>

#define TEST_CASE(name) void name()

#define ASSERT(condition) STDROMANO_REQUIRE(condition)

#define ASSERT_EQUAL(expected, actual) STDROMANO_REQUIRE_EQ(expected, actual)

#define ASSERT_THROWS(expression, exception_type) STDROMANO_REQUIRE_THROWS(expression, exception_type)

class TestRunner : public stdromano::test::TestRunner
{
public:
    explicit TestRunner(const char* name = nullptr) : stdromano::test::TestRunner(name)
    {
        spdlog::set_level(spdlog::level::trace);
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
