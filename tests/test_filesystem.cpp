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

TEST_CASE(test_list_dir)
{
    const stdromano::String<> directory_path = stdromano::fs_parent_dir(
        stdromano::String<>::make_ref(__FILE__, std::strlen(__FILE__)));
    stdromano::log_debug("Listing directory: {}", directory_path);

    stdromano::ListDirIterator it;

    while(fs_list_dir(it, directory_path, stdromano::ListDirFlags_ListAll))
    {
        stdromano::log_debug(it.get_current_path());
        stdromano::log_debug(stdromano::fs_filename(it.get_current_path()));
        stdromano::log_debug("Is File: {} | Is Dir: {}", it.is_file(), it.is_directory());
    }
}

TEST_CASE(test_file_dialog)
{
    const stdromano::String<> file_path = stdromano::open_file_dialog(stdromano::FileDialogMode_OpenFile,
                                                                      "Open A File",
                                                                      stdromano::fs_parent_dir(__FILE__),
                                                                      "*.cpp|*.h|*.txt");

    stdromano::log_debug("Chosen file: {}", file_path);
}

int main()
{
    stdromano::set_log_level(stdromano::LogLevel::Debug);

    TestRunner runner;

    runner.add_test("Expand Executable", test_expand_executable);
    runner.add_test("Load File Content", test_load_file_content);
    runner.add_test("List Dir", test_list_dir);
    runner.add_test("File Dialog", test_file_dialog);

    runner.run_all();

    return 0;
}