// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - Present Romain Augier
// All rights reserved.

#include "stdromano/logger.h"
#include "stdromano/memory.h"
#include "stdromano/string.h"
#include "test.h"

TEST_CASE(test_memory_arena)
{
    stdromano::Arena arena(4096);
    int* ptr = arena.emplace<int>(12);
    ASSERT_EQUAL(12, *ptr);

    stdromano::String<> my_string_ref = stdromano::String<>::make_ref("this is a string ref");
    stdromano::String<> my_string_alloced("this is a string{}", " alloced");

    stdromano::String<>* my_string_ref_ptr = arena.emplace<stdromano::String<>>(my_string_ref);
    stdromano::String<>* my_string_alloced_ptr =
                   arena.emplace<stdromano::String<>>(my_string_alloced);
    stdromano::String<>* my_string_alloced_another_ptr =
                   arena.emplace<stdromano::String<>>(my_string_alloced);
    stdromano::String<>* my_string_emplaced_ptr =
                   arena.emplace<stdromano::String<>>("this is a string emplaced");

    ASSERT(std::strcmp(my_string_ref_ptr->c_str(), "this is a string ref") == 0);
    ASSERT(std::strcmp(my_string_alloced_ptr->c_str(), "this is a string alloced") == 0);
    ASSERT(std::strcmp(my_string_alloced_another_ptr->c_str(), "this is a string alloced") == 0);
    ASSERT(std::strcmp(my_string_emplaced_ptr->c_str(), "this is a string emplaced") == 0);

    arena.clear();

    /*
        GCC optimizes so well that the dtor is bypassed and the following assertions are false in
       release mode
    */
#if !defined(STDROMANO_GCC) && !DEBUG_BUILD
    ASSERT(my_string_ref_ptr->empty());
    ASSERT(my_string_alloced_ptr->empty());
    ASSERT(my_string_alloced_another_ptr->empty());
    ASSERT(my_string_emplaced_ptr->empty());
#endif /* !defined(STDROMANO_GCC) && !DEBUG_BUILD */
}

int main()
{
    stdromano::set_log_level(stdromano::LogLevel::Debug);

    TestRunner runner;

    runner.add_test("Memory Arena", test_memory_arena);

    runner.run_all();

    return 0;
}