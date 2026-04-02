// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - Present Romain Augier
// All rights reserved.

#include "stdromano/filesystem.hpp"
#include "test.hpp"

/* path_exists */

TEST_CASE(test_path_exists_file)
{
    ASSERT_EQUAL(true, stdromano::fs::path_exists(__FILE__));
}

TEST_CASE(test_path_exists_nonexistent)
{
    ASSERT_EQUAL(false, stdromano::fs::path_exists(stdromano::StringD("{}n", __FILE__)));
}

TEST_CASE(test_path_exists_directory)
{
    const stdromano::StringD dir = stdromano::fs::parent_dir(__FILE__).copy();
    ASSERT_EQUAL(true, stdromano::fs::path_exists(dir));
}

TEST_CASE(test_path_exists_empty)
{
    ASSERT_EQUAL(false, stdromano::fs::path_exists(stdromano::String<>("")));
}

/* parent_dir */

TEST_CASE(test_parent_dir_file)
{
    const stdromano::String<> parent = stdromano::fs::parent_dir(__FILE__);
    ASSERT(!parent.empty());
    ASSERT_EQUAL(true, stdromano::fs::path_exists(parent));
}

TEST_CASE(test_parent_dir_nested)
{
    const stdromano::String<> parent = stdromano::fs::parent_dir(__FILE__);
    const stdromano::String<> grandparent = stdromano::fs::parent_dir(parent);
    ASSERT(!grandparent.empty());
    ASSERT_EQUAL(true, stdromano::fs::path_exists(grandparent));
}

/* filename */

TEST_CASE(test_filename_from_path)
{
    const stdromano::String<> name = stdromano::fs::filename(__FILE__);
    ASSERT(!name.empty());
    spdlog::debug("Filename: {}", name);
}

TEST_CASE(test_filename_no_directory)
{
    const stdromano::String<> name = stdromano::fs::filename("just_a_file.txt");
    ASSERT(!name.empty());
}

/* filesize */

TEST_CASE(test_filesize)
{
    const std::size_t size = stdromano::fs::filesize(__FILE__).unwrap();
    spdlog::debug("Size: {}", size);

    const stdromano::StringD content = stdromano::fs::load_file_content(__FILE__).unwrap();

    ASSERT(size == content.size());

    spdlog::debug("Content size: {}", content.size());
}

/* relative_to */

TEST_CASE(test_relative_to)
{
    const stdromano::StringD parent = stdromano::fs::parent_dir(__FILE__);
    const stdromano::StringD rel = stdromano::fs::relative_to(__FILE__, parent).unwrap();

    spdlog::debug("Relative to: {}, {}", parent, rel);
}

/* current_dir */

TEST_CASE(test_current_dir)
{
    const stdromano::StringD cwd = stdromano::fs::current_dir();
    ASSERT(!cwd.empty());
    ASSERT_EQUAL(true, stdromano::fs::path_exists(cwd));
    spdlog::debug("CWD: {}", cwd);
}

/* tmp_dir */

TEST_CASE(test_tmp_dir)
{
    auto result = stdromano::fs::tmp_dir();
    ASSERT(!result.has_error());

    const stdromano::StringD tmp = result.unwrap();
    ASSERT(!tmp.empty());
    ASSERT_EQUAL(true, stdromano::fs::path_exists(tmp));
    spdlog::debug("TMP: {}", tmp);
}

/* home_dir */

TEST_CASE(test_home_dir)
{
    auto result = stdromano::fs::home_dir();
    ASSERT(!result.has_error());

    const stdromano::StringD home = result.unwrap();
    ASSERT(!home.empty());
    ASSERT_EQUAL(true, stdromano::fs::path_exists(home));
    spdlog::debug("HOME: {}", home);
}

TEST_CASE(test_home_dir_use_env)
{
    auto result = stdromano::fs::home_dir(true);
    ASSERT(!result.has_error());

    const stdromano::StringD home = result.unwrap();
    ASSERT(!home.empty());
    ASSERT_EQUAL(true, stdromano::fs::path_exists(home));
    spdlog::debug("HOME (env): {}", home);
}

/* makedir / removedir */

TEST_CASE(test_makedir_removedir)
{
    const stdromano::StringD tmp = stdromano::fs::tmp_dir().unwrap();
    const stdromano::StringD test_dir = stdromano::StringD("{}/stdromano_test_mkdir", tmp);

    // Cleanup in case previous run left it behind
    stdromano::fs::removedir(test_dir, true);

    // Create directory
    auto mk_result = stdromano::fs::makedir(test_dir);
    ASSERT(!mk_result.has_error());
    ASSERT_EQUAL(true, stdromano::fs::path_exists(test_dir));

    // Remove directory
    auto rm_result = stdromano::fs::removedir(test_dir, false);
    ASSERT(!rm_result.has_error());
    ASSERT_EQUAL(false, stdromano::fs::path_exists(test_dir));
}

TEST_CASE(test_removedir_nonexistent)
{
    const stdromano::StringD bogus("/tmp/stdromano_nonexistent_dir_xyz");

    // Should be a no-op, not an error
    auto result = stdromano::fs::removedir(bogus);
    ASSERT(!result.has_error());
}

TEST_CASE(test_removedir_recursive)
{
    const stdromano::StringD tmp = stdromano::fs::tmp_dir().unwrap();
    const stdromano::StringD root = stdromano::StringD("{}/stdromano_test_recursive", tmp);
    const stdromano::StringD child = stdromano::StringD("{}/child", root);

    stdromano::fs::removedir(root, true);

    // Create parent and child
    ASSERT(!stdromano::fs::makedir(root).has_error());
    ASSERT(!stdromano::fs::makedir(child).has_error());
    ASSERT_EQUAL(true, stdromano::fs::path_exists(child));

    // Write a file inside the child dir
    const stdromano::StringD file_in_child = stdromano::StringD("{}/dummy.txt", child);
    const char* data = "hello";
    ASSERT(!stdromano::fs::write_file_content(data, 5, file_in_child).has_error());
    ASSERT_EQUAL(true, stdromano::fs::path_exists(file_in_child));

    // Recursive remove should delete everything
    auto result = stdromano::fs::removedir(root, true);
    ASSERT(!result.has_error());
    ASSERT_EQUAL(false, stdromano::fs::path_exists(root));
}

/* removefile */

TEST_CASE(test_removefile)
{
    const stdromano::StringD tmp = stdromano::fs::tmp_dir().unwrap();
    const stdromano::StringD file_path = stdromano::StringD("{}/stdromano_test_removefile.txt", tmp);

    const char* data = "to be removed";
    ASSERT(!stdromano::fs::write_file_content(data, 13, file_path).has_error());
    ASSERT_EQUAL(true, stdromano::fs::path_exists(file_path));

    auto result = stdromano::fs::removefile(file_path);
    ASSERT(!result.has_error());
    ASSERT_EQUAL(false, stdromano::fs::path_exists(file_path));
}

TEST_CASE(test_removefile_nonexistent)
{
    const stdromano::StringD bogus("/tmp/stdromano_no_such_file.txt");

    // Should be a no-op, not an error
    auto result = stdromano::fs::removefile(bogus);
    ASSERT(!result.has_error());
}

/* copyfile */

TEST_CASE(test_copyfile)
{
    const stdromano::StringD tmp = stdromano::fs::tmp_dir().unwrap();
    const stdromano::StringD src = stdromano::StringD("{}/stdromano_copy_src.txt", tmp);
    const stdromano::StringD dst = stdromano::StringD("{}/stdromano_copy_dst.txt", tmp);

    // Cleanup
    stdromano::fs::removefile(src);
    stdromano::fs::removefile(dst);

    const char* data = "copy me";
    ASSERT(!stdromano::fs::write_file_content(data, 7, src).has_error());

    auto result = stdromano::fs::copyfile(src, dst);
    ASSERT(!result.has_error());
    ASSERT_EQUAL(true, stdromano::fs::path_exists(dst));

    // Verify content matches
    auto dst_content = stdromano::fs::load_file_content(dst, "r");
    ASSERT(!dst_content.has_error());

    const stdromano::StringD content = dst_content.unwrap();
    ASSERT_EQUAL(static_cast<std::size_t>(7), content.size());

    // Cleanup
    stdromano::fs::removefile(src);
    stdromano::fs::removefile(dst);
}

/* expand_from_executable_dir / expand_from_lib_dir */

TEST_CASE(test_expand_executable)
{
    auto result = stdromano::fs::expand_from_executable_dir("test/expand/file.c");
    ASSERT(!result.has_error());

    const stdromano::StringD expanded = result.unwrap();
    ASSERT(!expanded.empty());
    spdlog::debug("Expand exe: {}", expanded);
}

TEST_CASE(test_expand_executable_empty)
{
    auto result = stdromano::fs::expand_from_executable_dir("");
    ASSERT(!result.has_error());

    const stdromano::StringD expanded = result.unwrap();
    ASSERT(!expanded.empty());
}

TEST_CASE(test_expand_library)
{
    auto result = stdromano::fs::expand_from_lib_dir("test/expand/file.c");
    ASSERT(!result.has_error());

    const stdromano::StringD expanded = result.unwrap();
    ASSERT(!expanded.empty());
    spdlog::debug("Expand lib: {}", expanded);
}

/* load_file_content */

TEST_CASE(test_load_file_content)
{
    auto content = stdromano::fs::load_file_content(__FILE__, "r");
    ASSERT(!content.has_error());

    const stdromano::StringD text = content.unwrap();
    ASSERT(!text.empty());
}

TEST_CASE(test_load_file_content_binary)
{
    auto content = stdromano::fs::load_file_content(__FILE__, "rb");
    ASSERT(!content.has_error());

    const stdromano::StringD text = content.unwrap();
    ASSERT(!text.empty());
}

TEST_CASE(test_load_file_content_nonexistent)
{
    auto content = stdromano::fs::load_file_content("/no/such/file.txt", "r");
    ASSERT(content.has_error());
}

/* write_file_content */

TEST_CASE(test_write_file_content)
{
    const stdromano::StringD tmp = stdromano::fs::tmp_dir().unwrap();
    const stdromano::StringD file_path = stdromano::StringD("{}/stdromano_test_write.txt", tmp);

    stdromano::fs::removefile(file_path);

    const char* data = "hello world";
    auto result = stdromano::fs::write_file_content(data, 11, file_path);
    ASSERT(!result.has_error());
    ASSERT_EQUAL(true, stdromano::fs::path_exists(file_path));

    // Verify content
    auto content = stdromano::fs::load_file_content(file_path, "r");
    ASSERT(!content.has_error());
    ASSERT_EQUAL(static_cast<std::size_t>(11), content.unwrap().size());

    stdromano::fs::removefile(file_path);
}

TEST_CASE(test_write_file_content_creates_parent_dirs)
{
    const stdromano::StringD tmp = stdromano::fs::tmp_dir().unwrap();
    const stdromano::StringD nested_dir = stdromano::StringD("{}/stdromano_test_nested/sub/dir", tmp);
    const stdromano::StringD file_path = stdromano::StringD("{}/file.txt", nested_dir);
    const stdromano::StringD root = stdromano::StringD("{}/stdromano_test_nested", tmp);

    stdromano::fs::removedir(root, true);

    const char* data = "nested write";
    auto result = stdromano::fs::write_file_content(data, 12, file_path);
    ASSERT(!result.has_error());
    ASSERT_EQUAL(true, stdromano::fs::path_exists(file_path));

    stdromano::fs::removedir(root, true);
}

TEST_CASE(test_write_file_content_append)
{
    const stdromano::StringD tmp = stdromano::fs::tmp_dir().unwrap();
    const stdromano::StringD file_path = stdromano::StringD("{}/stdromano_test_append.txt", tmp);

    stdromano::fs::removefile(file_path);

    const char* data1 = "hello";
    ASSERT(!stdromano::fs::write_file_content(data1, 5, file_path, "w").has_error());

    const char* data2 = " world";
    ASSERT(!stdromano::fs::write_file_content(data2, 6, file_path, "a").has_error());

    auto content = stdromano::fs::load_file_content(file_path, "r");
    ASSERT(!content.has_error());
    ASSERT_EQUAL(static_cast<std::size_t>(11), content.unwrap().size());

    stdromano::fs::removefile(file_path);
}

/* list_dir */

TEST_CASE(test_list_dir_all)
{
    const stdromano::StringD directory_path = stdromano::fs::parent_dir(__FILE__).copy();
    spdlog::debug("Listing directory: {}", directory_path);

    stdromano::fs::ListDirIterator it;
    int count = 0;

    while(stdromano::fs::list_dir(it, directory_path, stdromano::fs::ListDirFlags_ListAll))
    {
        spdlog::debug("{} | file={} dir={}", it.get_current_path(), it.is_file(), it.is_directory());

        // Each entry should be either a file or a directory, not both
        ASSERT(it.is_file() || it.is_directory());
        ASSERT(!(it.is_file() && it.is_directory()));

        count++;
    }

    ASSERT(count > 0);
}

TEST_CASE(test_list_dir_files_only)
{
    const stdromano::StringD directory_path = stdromano::fs::parent_dir(__FILE__).copy();

    stdromano::fs::ListDirIterator it;

    while(stdromano::fs::list_dir(it, directory_path, stdromano::fs::ListDirFlags_ListFiles))
    {
        ASSERT_EQUAL(true, it.is_file());
        ASSERT_EQUAL(false, it.is_directory());
    }
}

TEST_CASE(test_list_dir_dirs_only)
{
    const stdromano::StringD directory_path = stdromano::fs::parent_dir(__FILE__).copy();

    stdromano::fs::ListDirIterator it;

    while(stdromano::fs::list_dir(it, directory_path, stdromano::fs::ListDirFlags_ListDirs))
    {
        ASSERT_EQUAL(false, it.is_file());
        ASSERT_EQUAL(true, it.is_directory());
    }
}

TEST_CASE(test_list_dir_known_contents)
{
    // Create a temp directory with known contents
    const stdromano::StringD tmp = stdromano::fs::tmp_dir().unwrap();
    const stdromano::StringD test_dir = stdromano::StringD("{}/stdromano_test_listdir", tmp);
    const stdromano::StringD sub_dir = stdromano::StringD("{}/subdir", test_dir);
    const stdromano::StringD file1 = stdromano::StringD("{}/file1.txt", test_dir);
    const stdromano::StringD file2 = stdromano::StringD("{}/file2.txt", test_dir);

    stdromano::fs::removedir(test_dir, true);
    stdromano::fs::makedir(test_dir);
    stdromano::fs::makedir(sub_dir);

    const char* data = "x";
    stdromano::fs::write_file_content(data, 1, file1);
    stdromano::fs::write_file_content(data, 1, file2);

    stdromano::fs::ListDirIterator it;
    int file_count = 0;
    int dir_count = 0;

    while(stdromano::fs::list_dir(it, test_dir, stdromano::fs::ListDirFlags_ListAll))
    {
        if(it.is_file()) file_count++;
        if(it.is_directory()) dir_count++;
    }

    ASSERT_EQUAL(2, file_count);
    ASSERT_EQUAL(1, dir_count);

    stdromano::fs::removedir(test_dir, true);
}

/* ListDirIterator move semantics */

TEST_CASE(test_list_dir_iterator_move)
{
    const stdromano::StringD directory_path = stdromano::fs::parent_dir(__FILE__).copy();

    stdromano::fs::ListDirIterator it;
    stdromano::fs::list_dir(it, directory_path, stdromano::fs::ListDirFlags_ListAll);

    // Move construct
    stdromano::fs::ListDirIterator it2(std::move(it));
    ASSERT(!it2.get_current_path().empty());

    // Move assign
    stdromano::fs::ListDirIterator it3;
    it3 = std::move(it2);
    ASSERT(!it3.get_current_path().empty());
}

/* walk */

TEST_CASE(test_walk_all)
{
    const stdromano::StringD directory_path = stdromano::fs::parent_dir(__FILE__).copy();
    spdlog::debug("Walk: {}", directory_path);

    int count = 0;

    for(stdromano::fs::WalkIterator it(directory_path, stdromano::fs::WalkFlags_ListAll);
        it != stdromano::fs::WalkIterator();
        ++it)
    {
        const auto& item = *it;
        ASSERT(!item.get_current_path().empty());
        ASSERT(item.is_file() || item.is_directory());
        ASSERT(!(item.is_file() && item.is_directory()));
        count++;
    }

    ASSERT(count > 0);
}

TEST_CASE(test_walk_files_only)
{
    const stdromano::StringD directory_path = stdromano::fs::parent_dir(__FILE__).copy();

    for(stdromano::fs::WalkIterator it(directory_path, stdromano::fs::WalkFlags_ListFiles);
        it != stdromano::fs::WalkIterator();
        ++it)
    {
        ASSERT_EQUAL(true, it->is_file());
        ASSERT_EQUAL(false, it->is_directory());
    }
}

TEST_CASE(test_walk_dirs_only)
{
    const stdromano::StringD directory_path = stdromano::fs::parent_dir(__FILE__).copy();

    for(stdromano::fs::WalkIterator it(directory_path, stdromano::fs::WalkFlags_ListDirs);
        it != stdromano::fs::WalkIterator();
        ++it)
    {
        ASSERT_EQUAL(false, it->is_file());
        ASSERT_EQUAL(true, it->is_directory());
    }
}

TEST_CASE(test_walk_recursive)
{
    // Create a known hierarchy
    const stdromano::StringD tmp = stdromano::fs::tmp_dir().unwrap();
    const stdromano::StringD root = stdromano::StringD("{}/stdromano_test_walk", tmp);
    const stdromano::StringD child = stdromano::StringD("{}/child", root);
    const stdromano::StringD grandchild = stdromano::StringD("{}/grandchild", child);

    stdromano::fs::removedir(root, true);
    stdromano::fs::makedir(root);
    stdromano::fs::makedir(child);
    stdromano::fs::makedir(grandchild);

    const char* data = "x";
    stdromano::fs::write_file_content(data, 1, stdromano::StringD("{}/a.txt", root));
    stdromano::fs::write_file_content(data, 1, stdromano::StringD("{}/b.txt", child));
    stdromano::fs::write_file_content(data, 1, stdromano::StringD("{}/c.txt", grandchild));

    int file_count = 0;
    int dir_count = 0;

    for(stdromano::fs::WalkIterator it(root, stdromano::fs::WalkFlags_ListAll |
                                             stdromano::fs::WalkFlags_Recursive);
        it != stdromano::fs::WalkIterator();
        ++it)
    {
        spdlog::debug("Walk recursive: {}", it->get_current_path());
        if(it->is_file()) file_count++;
        if(it->is_directory()) dir_count++;
    }

    // Should find files across all levels
    ASSERT_EQUAL(3, file_count);
    // child + grandchild (root itself is not listed as an entry)
    ASSERT_EQUAL(2, dir_count);

    stdromano::fs::removedir(root, true);
}

TEST_CASE(test_walk_non_recursive)
{
    // Create a known hierarchy
    const stdromano::StringD tmp = stdromano::fs::tmp_dir().unwrap();
    const stdromano::StringD root = stdromano::StringD("{}/stdromano_test_walk_nr", tmp);
    const stdromano::StringD child = stdromano::StringD("{}/child", root);

    stdromano::fs::removedir(root, true);
    stdromano::fs::makedir(root);
    stdromano::fs::makedir(child);

    const char* data = "x";
    stdromano::fs::write_file_content(data, 1, stdromano::StringD("{}/a.txt", root));
    stdromano::fs::write_file_content(data, 1, stdromano::StringD("{}/b.txt", child));

    int file_count = 0;

    for(stdromano::fs::WalkIterator it(root, stdromano::fs::WalkFlags_ListFiles);
        it != stdromano::fs::WalkIterator();
        ++it)
    {
        if(it->is_file()) file_count++;
    }

    // Without WalkFlags_Recursive, should only see files in root
    ASSERT_EQUAL(1, file_count);

    stdromano::fs::removedir(root, true);
}

/* WalkIterator end sentinel */

TEST_CASE(test_walk_iterator_end)
{
    stdromano::fs::WalkIterator end;
    ASSERT(end == stdromano::fs::WalkIterator());
}

/* WalkIterator arrow operator */

TEST_CASE(test_walk_iterator_arrow)
{
    const stdromano::StringD directory_path = stdromano::fs::parent_dir(__FILE__).copy();

    stdromano::fs::WalkIterator it(directory_path, stdromano::fs::WalkFlags_ListAll);
    if(it != stdromano::fs::WalkIterator())
    {
        // Test arrow operator
        const stdromano::StringD& path = it->get_current_path();
        ASSERT(!path.empty());

        // Test dereference operator
        const auto& item = *it;
        ASSERT(!item.get_current_path().empty());
    }
}

/* Round-trip write then load */

TEST_CASE(test_write_then_load_roundtrip)
{
    const stdromano::StringD tmp = stdromano::fs::tmp_dir().unwrap();
    const stdromano::StringD file_path = stdromano::StringD("{}/stdromano_roundtrip.txt", tmp);

    stdromano::fs::removefile(file_path);

    const char* expected = "round trip content 12345";
    const std::size_t expected_sz = std::strlen(expected);

    ASSERT(!stdromano::fs::write_file_content(expected, expected_sz, file_path, "w").has_error());

    auto loaded = stdromano::fs::load_file_content(file_path, "r");
    ASSERT(!loaded.has_error());

    const stdromano::StringD content = loaded.unwrap();
    ASSERT_EQUAL(expected_sz, content.size());
    ASSERT_EQUAL(0, std::memcmp(expected, content.c_str(), expected_sz));

    stdromano::fs::removefile(file_path);
}

/* copyfile error case */

TEST_CASE(test_copyfile_nonexistent_src)
{
    const stdromano::StringD tmp = stdromano::fs::tmp_dir().unwrap();
    const stdromano::StringD src("/tmp/stdromano_no_such_src.txt");
    const stdromano::StringD dst = stdromano::StringD("{}/stdromano_copy_fail_dst.txt", tmp);

    auto result = stdromano::fs::copyfile(src, dst);
    ASSERT(result.has_error());
}

int main()
{
    TestRunner runner("filesystem");

    /* path_exists */
    runner.add_test("PathExists_File", test_path_exists_file);
    runner.add_test("PathExists_Nonexistent", test_path_exists_nonexistent);
    runner.add_test("PathExists_Directory", test_path_exists_directory);
    runner.add_test("PathExists_Empty", test_path_exists_empty);

    /* parent_dir */
    runner.add_test("ParentDir_File", test_parent_dir_file);
    runner.add_test("ParentDir_Nested", test_parent_dir_nested);

    /* filename */
    runner.add_test("Filename_FromPath", test_filename_from_path);
    runner.add_test("Filename_NoDirectory", test_filename_no_directory);

    /* filesize */
    runner.add_test("FileSize", test_filesize);

    /* relative_to */
    runner.add_test("RelativeTo", test_relative_to);

    /* current_dir / tmp_dir / home_dir */
    runner.add_test("CurrentDir", test_current_dir);
    runner.add_test("TmpDir", test_tmp_dir);
    runner.add_test("HomeDir", test_home_dir);
    runner.add_test("HomeDir_UseEnv", test_home_dir_use_env);

    /* makedir / removedir */
    runner.add_test("MakeDir_RemoveDir", test_makedir_removedir);
    runner.add_test("RemoveDir_Nonexistent", test_removedir_nonexistent);
    runner.add_test("RemoveDir_Recursive", test_removedir_recursive);

    /* removefile */
    runner.add_test("RemoveFile", test_removefile);
    runner.add_test("RemoveFile_Nonexistent", test_removefile_nonexistent);

    /* copyfile */
    runner.add_test("CopyFile", test_copyfile);
    runner.add_test("CopyFile_NonexistentSrc", test_copyfile_nonexistent_src);

    /* expand */
    runner.add_test("ExpandExecutable", test_expand_executable);
    runner.add_test("ExpandExecutable_Empty", test_expand_executable_empty);
    runner.add_test("ExpandLibrary", test_expand_library);

    /* load / write file content */
    runner.add_test("LoadFileContent", test_load_file_content);
    runner.add_test("LoadFileContent_Binary", test_load_file_content_binary);
    runner.add_test("LoadFileContent_Nonexistent", test_load_file_content_nonexistent);
    runner.add_test("WriteFileContent", test_write_file_content);
    runner.add_test("WriteFileContent_CreatesParentDirs", test_write_file_content_creates_parent_dirs);
    runner.add_test("WriteFileContent_Append", test_write_file_content_append);
    runner.add_test("WriteThenLoad_Roundtrip", test_write_then_load_roundtrip);

    /* list_dir */
    runner.add_test("ListDir_All", test_list_dir_all);
    runner.add_test("ListDir_FilesOnly", test_list_dir_files_only);
    runner.add_test("ListDir_DirsOnly", test_list_dir_dirs_only);
    runner.add_test("ListDir_KnownContents", test_list_dir_known_contents);
    runner.add_test("ListDirIterator_Move", test_list_dir_iterator_move);

    /* walk */
    runner.add_test("Walk_All", test_walk_all);
    runner.add_test("Walk_FilesOnly", test_walk_files_only);
    runner.add_test("Walk_DirsOnly", test_walk_dirs_only);
    runner.add_test("Walk_Recursive", test_walk_recursive);
    runner.add_test("Walk_NonRecursive", test_walk_non_recursive);
    runner.add_test("WalkIterator_End", test_walk_iterator_end);
    runner.add_test("WalkIterator_Arrow", test_walk_iterator_arrow);

    runner.run_all();

    return 0;
}
