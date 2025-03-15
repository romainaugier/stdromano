// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - Present Romain Augier
// All rights reserved.

#include "stdromano/filesystem.h"
#include "stdromano/logger.h"
#include "test.h"

TEST_CASE(test_expand_executable) { stdromano::log_info(stdromano::expand_from_executable_dir("test/expand/file.c")); }

TEST_CASE(test_load_file_content)
{
    stdromano::String<> content = std::move(stdromano::load_file_content(__FILE__));

    ASSERT(!content.empty());

    stdromano::log_info(content);
}

int main()
{
    TestRunner runner;

    runner.add_test("Expand Executable", test_expand_executable);
    runner.add_test("Load File Content", test_load_file_content);

    runner.run_all();

    return 0;
}