// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - Present Romain Augier
// All rights reserved.

#include "stdromano/filesystem.hpp"

#include "test.hpp"

TEST_CASE(test_path_exists)
{
    ASSERT_EQUAL(true, stdromano::fs::path_exists(__FILE__));
    ASSERT_EQUAL(false, stdromano::fs::path_exists(stdromano::StringD("{}n", __FILE__)));
}

TEST_CASE(test_parent_dir)
{
    spdlog::info(stdromano::fs::parent_dir(__FILE__));
}

TEST_CASE(test_filename)
{
    spdlog::info(stdromano::fs::filename(__FILE__));
}

TEST_CASE(test_expand_executable)
{
    spdlog::info(stdromano::fs::expand_from_executable_dir("test/expand/file.c").unwrap());
}

TEST_CASE(test_expand_library)
{
    spdlog::info(stdromano::fs::expand_from_lib_dir("test/expand/file.c").unwrap());
}

TEST_CASE(test_load_file_content)
{
    auto content = stdromano::fs::load_file_content(__FILE__, "r");

    ASSERT(!content.has_error());
}

TEST_CASE(test_list_dir)
{
    const stdromano::StringD directory_path = stdromano::fs::parent_dir(__FILE__).copy();
    spdlog::debug("Listing directory: {}", directory_path);

    stdromano::fs::ListDirIterator it;

    while(stdromano::fs::list_dir(it, directory_path, stdromano::fs::ListDirFlags_ListAll))
    {
        spdlog::debug(it.get_current_path());
        spdlog::debug(stdromano::fs::filename(it.get_current_path()));
        spdlog::debug("Is File: {} | Is Dir: {}", it.is_file(), it.is_directory());
    }
}

TEST_CASE(test_file_dialog)
{
    const stdromano::String<> file_path = stdromano::fs::open_file_dialog(stdromano::fs::FileDialogMode_OpenFile,
                                                                          "Open A File",
                                                                          stdromano::fs::expand_from_executable_dir("").unwrap(),
                                                                          "*.cpp|*.h|*.txt");

    spdlog::debug("Chosen file: {}", file_path.empty() ? "None" : file_path.c_str());


    const stdromano::String<> dir_path = stdromano::fs::open_file_dialog(stdromano::fs::FileDialogMode_OpenDir,
                                                                         "Select A Directory",
                                                                         stdromano::fs::expand_from_executable_dir("").unwrap(),
                                                                         "");

    spdlog::debug("Chosen directory: {}", dir_path.empty() ? "None" : dir_path.c_str());
}

TEST_CASE(test_walk)
{
    const stdromano::StringD directory_path = stdromano::fs::parent_dir(__FILE__).copy();

    spdlog::debug("Walk: {}", directory_path);

    for(stdromano::fs::WalkIterator it(directory_path, stdromano::fs::ListDirFlags_ListAll);
        it != stdromano::fs::WalkIterator();
        ++it)
    {
        spdlog::debug(it->get_current_path());
    }
}

TEST_CASE(test_current_dir)
{
    const stdromano::StringD cwd = stdromano::fs::current_dir();

    spdlog::debug("CWD: {}", cwd);
}

TEST_CASE(test_tmp_dir)
{
    const stdromano::StringD tmp = stdromano::fs::tmp_dir().unwrap();

    spdlog::debug("TMP: {}", tmp);
}

TEST_CASE(test_home_dir)
{
    const stdromano::StringD home = stdromano::fs::home_dir().unwrap();

    spdlog::debug("HOME: {}", home);
}

int main()
{
    TestRunner runner("filesystem");

    runner.add_test("PathExists", test_path_exists);
    runner.add_test("ParentDir", test_parent_dir);
    runner.add_test("Filename", test_filename);
    runner.add_test("Expand Executable", test_expand_executable);
    runner.add_test("Expand Library", test_expand_library);
    runner.add_test("Load File Content", test_load_file_content);
    runner.add_test("List Dir", test_list_dir);
    // runner.add_test("File Dialog", TestFileDialog);
    runner.add_test("CurrentDir", test_current_dir);
    runner.add_test("TmpDir", test_tmp_dir);
    runner.add_test("HomeDir", test_home_dir);
    runner.add_test("Walk", test_walk);

    runner.run_all();

    return 0;
}
