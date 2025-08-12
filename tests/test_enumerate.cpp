// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - Present Romain Augier
// All rights reserved.

#include "stdromano/vector.hpp"
#include "stdromano/logger.hpp"
#include "stdromano/enumerate.hpp"

#include "test.hpp"

INIT_TEST_OBJECT;

TEST_CASE(test_enumerate)
{
    stdromano::Vector<TestObject> vec;

    ASSERT_EQUAL(0u, vec.size());

    for(int i = 0; i < 100; ++i)
    {
        vec.push_back(TestObject(std::to_string(i)));
    }

    for(const auto& [i, obj] : stdromano::enumerate(vec))    
    {
        stdromano::log_debug("{} : {}", i, obj.get_data());
    }
}

int main()
{
    stdromano::set_log_level(stdromano::LogLevel::Debug);

    TestRunner runner;

    runner.add_test("Enumerate", test_enumerate);

    runner.run_all();

    return 0;
}