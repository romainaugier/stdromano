// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - Present Romain Augier
// All rights reserved.

#include "stdromano/filesystem.h"
#include "stdromano/logger.h"
#include "test.h"

TEST_CASE(TestPathExists)
{
    ASSERT_EQUAL(true, stdromano::fs_path_exists(__FILE__));
    ASSERT_EQUAL(false, stdromano::fs_path_exists(stdromano::StringD("{}n", __FILE__)));
}

TEST_CASE(TestParentDir)
{
    stdromano::log_info(stdromano::fs_parent_dir(__FILE__));
}

TEST_CASE(TestFilename)
{
    stdromano::log_info(stdromano::fs_filename(__FILE__));
}

TEST_CASE(TestExpandExecutable)
{
    stdromano::log_info(stdromano::expand_from_executable_dir("test/expand/file.c"));
}

TEST_CASE(TestLoadFileContent)
{
    stdromano::String<> content = stdromano::load_file_content(__FILE__, "r");

    ASSERT(!content.empty());
}

TEST_CASE(TestListDir)
{
    const stdromano::StringD directory_path = stdromano::fs_parent_dir(__FILE__);
    stdromano::log_debug("Listing directory: {}", directory_path);

    stdromano::ListDirIterator it;

    while(fs_list_dir(it, directory_path, stdromano::ListDirFlags_ListAll))
    {
        stdromano::log_debug(it.get_current_path());
        stdromano::log_debug(stdromano::fs_filename(it.get_current_path()));
        stdromano::log_debug("Is File: {} | Is Dir: {}", it.is_file(), it.is_directory());
    }
}

TEST_CASE(TestFileDialog)
{
    const stdromano::String<> file_path =
                   stdromano::open_file_dialog(stdromano::FileDialogMode_OpenFile,
                                               "Open A File",
                                               stdromano::expand_from_executable_dir(""),
                                               "*.cpp|*.h|*.txt");

    stdromano::log_debug("Chosen file: {}", file_path.empty() ? "None" : file_path.c_str());


    const stdromano::String<> dir_path =
                   stdromano::open_file_dialog(stdromano::FileDialogMode_OpenDir,
                                               "Select A Directory",
                                               stdromano::expand_from_executable_dir(""),
                                               "");

    stdromano::log_debug("Chosen directory: {}", dir_path.empty() ? "None" : dir_path.c_str());
}

TEST_CASE(TestWalk)
{
    const stdromano::StringD directory_path = stdromano::fs_parent_dir(__FILE__);

    stdromano::log_debug("Walk: {}", directory_path);

    for(stdromano::WalkIterator it(directory_path, stdromano::ListDirFlags_ListAll); 
        it != stdromano::WalkIterator();
        ++it) 
    {
        stdromano::log_debug(it->get_current_path());
    }
}

TEST_CASE(TestCurrentDir)
{
    const stdromano::StringD cwd = stdromano::fs_current_dir();

    stdromano::log_debug("CWD: {}", cwd);
}

int main()
{
    stdromano::set_log_level(stdromano::LogLevel::Debug);

    TestRunner runner;

    runner.add_test("PathExists", TestPathExists);
    runner.add_test("ParentDir", TestParentDir);
    runner.add_test("Filename", TestFilename);
    runner.add_test("Expand Executable", TestExpandExecutable);
    runner.add_test("Load File Content", TestLoadFileContent);
    runner.add_test("List Dir", TestListDir);
    // runner.add_test("File Dialog", TestFileDialog);
    runner.add_test("CurrentDir", TestCurrentDir);
    runner.add_test("Walk", TestWalk);

    runner.run_all();

    return 0;
}