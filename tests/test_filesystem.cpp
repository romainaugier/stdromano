// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - Present Romain Augier
// All rights reserved.

#include "stdromano/filesystem.h"
#include "stdromano/logger.h"
#include "test.h"

TEST_CASE(test_expand_executable) { stdromano::log_info(stdromano::expand_from_executable_dir("test/expand/file.c")); }

int main()
{
    TestRunner runner;

    runner.add_test("Expand Executable", test_expand_executable);

    runner.run_all();

    return 0;
}