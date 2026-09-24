// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - Present Romain Augier
// All rights reserved.

#include "stdromano/filesystem.hpp"

#include "fixtures.hpp"

#include <cstring>
#include <string>
#include <vector>

STDROMANO_TEST_CASE(path_exists_file)
{
    STDROMANO_CHECK(stdromano::fs::path_exists(__FILE__));
}

STDROMANO_TEST_CASE(path_exists_nonexistent)
{
    STDROMANO_CHECK(!(stdromano::fs::path_exists(stdromano::StringD("{}n", __FILE__))));
}

STDROMANO_TEST_CASE(path_exists_directory)
{
    const stdromano::StringD dir = stdromano::fs::parent_dir(__FILE__);
    STDROMANO_CHECK(stdromano::fs::path_exists(dir));
}

STDROMANO_TEST_CASE(path_exists_empty)
{
    STDROMANO_CHECK(!(stdromano::fs::path_exists(stdromano::String<>(""))));
}

STDROMANO_TEST_CASE(parent_dir_file)
{
    const stdromano::String<> parent = stdromano::fs::parent_dir(__FILE__);

    STDROMANO_CHECK(!parent.empty());

    STDROMANO_CHECK(stdromano::fs::path_exists(parent));
}

STDROMANO_TEST_CASE(parent_dir_nested)
{
    const stdromano::String<> parent = stdromano::fs::parent_dir(__FILE__);
    const stdromano::String<> grandparent = stdromano::fs::parent_dir(parent);
    STDROMANO_CHECK(!grandparent.empty());
    STDROMANO_CHECK(stdromano::fs::path_exists(grandparent));
}

STDROMANO_TEST_CASE(filename_from_path)
{
    const stdromano::String<> name = stdromano::fs::filename(__FILE__);
    STDROMANO_CHECK(!name.empty());
    spdlog::debug("Filename: {}", name);
}

STDROMANO_TEST_CASE(filename_no_directory)
{
    const stdromano::String<> name = stdromano::fs::filename("just_a_file.txt");
    STDROMANO_CHECK(!name.empty());
}

STDROMANO_TEST_CASE(filesize)
{
    const std::size_t size = stdromano::fs::filesize(__FILE__).unwrap();
    spdlog::debug("Size: {}", size);

    const stdromano::StringD content = stdromano::fs::load_file_content(__FILE__).unwrap();

    STDROMANO_CHECK(size == content.size());

    spdlog::debug("Content size: {}", content.size());
}

STDROMANO_TEST_CASE(relative_to)
{
    const stdromano::StringD parent = stdromano::fs::parent_dir(__FILE__);
    const stdromano::StringD rel = stdromano::fs::relative_to(__FILE__, parent).unwrap();

    spdlog::debug("Relative to: {}, {}", parent, rel);
}

STDROMANO_TEST_CASE(current_dir)
{
    const stdromano::StringD cwd = stdromano::fs::current_dir();
    STDROMANO_CHECK(!cwd.empty());
    STDROMANO_CHECK(stdromano::fs::path_exists(cwd));
    spdlog::debug("CWD: {}", cwd);
}

STDROMANO_TEST_CASE(tmp_dir)
{
    auto result = stdromano::fs::tmp_dir();
    STDROMANO_CHECK(!result.has_error());

    const stdromano::StringD tmp = result.unwrap();
    STDROMANO_CHECK(!tmp.empty());
    STDROMANO_CHECK(stdromano::fs::path_exists(tmp));
    spdlog::debug("TMP: {}", tmp);
}

STDROMANO_TEST_CASE(home_dir)
{
    auto result = stdromano::fs::home_dir();
    STDROMANO_CHECK(!result.has_error());

    const stdromano::StringD home = result.unwrap();
    STDROMANO_CHECK(!home.empty());
    STDROMANO_CHECK(stdromano::fs::path_exists(home));
    spdlog::debug("HOME: {}", home);
}

STDROMANO_TEST_CASE(home_dir_use_env)
{
    auto result = stdromano::fs::home_dir(true);
    STDROMANO_CHECK(!result.has_error());

    const stdromano::StringD home = result.unwrap();
    STDROMANO_CHECK(!home.empty());
    STDROMANO_CHECK(stdromano::fs::path_exists(home));
    spdlog::debug("HOME (env): {}", home);
}

STDROMANO_TEST_CASE(makedir_removedir)
{
    const stdromano::StringD test_dir = stdromano::test::temp_path("stdromano_test_mkdir");

    stdromano::fs::removedir(test_dir, true);

    auto mk_result = stdromano::fs::makedir(test_dir);
    STDROMANO_CHECK(!mk_result.has_error());
    STDROMANO_CHECK(stdromano::fs::path_exists(test_dir));

    auto rm_result = stdromano::fs::removedir(test_dir, false);
    STDROMANO_CHECK(!rm_result.has_error());
    STDROMANO_CHECK(!(stdromano::fs::path_exists(test_dir)));
}

STDROMANO_TEST_CASE(removedir_nonexistent)
{
    const stdromano::StringD bogus("/tmp/stdromano_nonexistent_dir_xyz");

    auto result = stdromano::fs::removedir(bogus);
    STDROMANO_CHECK(!result.has_error());
}

STDROMANO_TEST_CASE(removedir_recursive)
{
    const stdromano::StringD root = stdromano::test::temp_path("stdromano_test_recursive");
    const stdromano::StringD child = stdromano::StringD("{}/child", root);

    stdromano::fs::removedir(root, true);

    STDROMANO_CHECK(!stdromano::fs::makedir(root).has_error());
    STDROMANO_CHECK(!stdromano::fs::makedir(child).has_error());
    STDROMANO_CHECK(stdromano::fs::path_exists(child));

    const stdromano::StringD file_in_child = stdromano::StringD("{}/dummy.txt", child);
    const char* data = "hello";
    STDROMANO_CHECK(!stdromano::fs::write_file_content(data, 5, file_in_child).has_error());
    STDROMANO_CHECK(stdromano::fs::path_exists(file_in_child));

    auto result = stdromano::fs::removedir(root, true);
    STDROMANO_CHECK(!result.has_error());
    STDROMANO_CHECK(!(stdromano::fs::path_exists(root)));
}

STDROMANO_TEST_CASE(copydir)
{
    const stdromano::StringD root = stdromano::test::temp_path("stdromano_test_copy_recursive");
    const stdromano::StringD child = stdromano::StringD("{}/child", root);

    stdromano::fs::removedir(root, true);

    STDROMANO_CHECK(!stdromano::fs::makedir(root).has_error());
    STDROMANO_CHECK(!stdromano::fs::makedir(child).has_error());
    STDROMANO_CHECK(stdromano::fs::path_exists(child));

    const stdromano::StringD file_in_child = stdromano::StringD("{}/dummy.txt", child);
    const char* data = "hello";
    STDROMANO_CHECK(!stdromano::fs::write_file_content(data, 5, file_in_child).has_error());
    STDROMANO_CHECK(stdromano::fs::path_exists(file_in_child));

    const stdromano::StringD root2 = stdromano::test::temp_path("stdromano_test_copy_recursive2");

    if(stdromano::fs::path_exists(root2))
        stdromano::fs::removedir(root2);

    STDROMANO_CHECK(stdromano::fs::copydir(root, root2));

    for(auto it = stdromano::fs::WalkIterator(root2); it != stdromano::fs::WalkIterator(); ++it)
        spdlog::debug("Copied path: {}", it->get_current_path());
}

STDROMANO_TEST_CASE(removefile)
{
    const stdromano::StringD file_path = stdromano::test::temp_path("stdromano_test_removefile.txt");

    const char* data = "to be removed";
    STDROMANO_CHECK(!stdromano::fs::write_file_content(data, 13, file_path).has_error());
    STDROMANO_CHECK(stdromano::fs::path_exists(file_path));

    auto result = stdromano::fs::removefile(file_path);
    STDROMANO_CHECK(!result.has_error());
    STDROMANO_CHECK(!(stdromano::fs::path_exists(file_path)));
}

STDROMANO_TEST_CASE(removefile_nonexistent)
{
    const stdromano::StringD bogus("/tmp/stdromano_no_such_file.txt");

    auto result = stdromano::fs::removefile(bogus);
    STDROMANO_CHECK(!result.has_error());
}

STDROMANO_TEST_CASE(copyfile)
{
    const stdromano::StringD src = stdromano::test::temp_path("stdromano_copy_src.txt");
    const stdromano::StringD dst = stdromano::test::temp_path("stdromano_copy_dst.txt");

    stdromano::fs::removefile(src);
    stdromano::fs::removefile(dst);

    const char* data = "copy me";
    STDROMANO_CHECK(!stdromano::fs::write_file_content(data, 7, src).has_error());

    auto result = stdromano::fs::copyfile(src, dst);
    STDROMANO_CHECK(!result.has_error());
    STDROMANO_CHECK(stdromano::fs::path_exists(dst));

    auto dst_content = stdromano::fs::load_file_content(dst, "r");
    STDROMANO_CHECK(!dst_content.has_error());

    const stdromano::StringD content = dst_content.unwrap();
    STDROMANO_CHECK_EQ(static_cast<std::size_t>(7), content.size());

    stdromano::fs::removefile(src);
    stdromano::fs::removefile(dst);
}

STDROMANO_TEST_CASE(expand_executable)
{
    auto result = stdromano::fs::expand_from_executable_dir("test/expand/file.c");
    STDROMANO_CHECK(!result.has_error());

    const stdromano::StringD expanded = result.unwrap();
    STDROMANO_CHECK(!expanded.empty());
    spdlog::debug("Expand exe: {}", expanded);
}

STDROMANO_TEST_CASE(expand_executable_empty)
{
    auto result = stdromano::fs::expand_from_executable_dir("");
    STDROMANO_CHECK(!result.has_error());

    const stdromano::StringD expanded = result.unwrap();
    STDROMANO_CHECK(!expanded.empty());
}

STDROMANO_TEST_CASE(expand_library)
{
    auto result = stdromano::fs::expand_from_lib_dir("test/expand/file.c");
    STDROMANO_CHECK(!result.has_error());

    const stdromano::StringD expanded = result.unwrap();
    STDROMANO_CHECK(!expanded.empty());
    spdlog::debug("Expand lib: {}", expanded);
}

STDROMANO_TEST_CASE(load_file_content)
{
    auto content = stdromano::fs::load_file_content(__FILE__, "r");
    STDROMANO_CHECK(!content.has_error());

    const stdromano::StringD text = content.unwrap();
    STDROMANO_CHECK(!text.empty());
}

STDROMANO_TEST_CASE(load_file_content_binary)
{
    auto content = stdromano::fs::load_file_content(__FILE__, "rb");
    STDROMANO_CHECK(!content.has_error());

    const stdromano::StringD text = content.unwrap();
    STDROMANO_CHECK(!text.empty());
}

STDROMANO_TEST_CASE(load_file_content_nonexistent)
{
    auto content = stdromano::fs::load_file_content("/no/such/file.txt", "r");
    STDROMANO_CHECK(content.has_error());
}

STDROMANO_TEST_CASE(write_file_content)
{
    const stdromano::StringD file_path = stdromano::test::temp_path("stdromano_test_write.txt");

    stdromano::fs::removefile(file_path);

    const char* data = "hello world";
    auto result = stdromano::fs::write_file_content(data, 11, file_path);
    STDROMANO_CHECK(!result.has_error());
    STDROMANO_CHECK(stdromano::fs::path_exists(file_path));

    auto content = stdromano::fs::load_file_content(file_path, "r");
    STDROMANO_CHECK(!content.has_error());
    STDROMANO_CHECK_EQ(static_cast<std::size_t>(11), content.unwrap().size());

    stdromano::fs::removefile(file_path);
}

STDROMANO_TEST_CASE(write_file_content_creates_parent_dirs)
{
    const stdromano::StringD nested_dir = stdromano::test::temp_path("stdromano_test_nested/sub/dir");
    const stdromano::StringD file_path = stdromano::StringD("{}/file.txt", nested_dir);
    const stdromano::StringD root = stdromano::test::temp_path("stdromano_test_nested");

    stdromano::fs::removedir(root, true);

    const char* data = "nested write";
    auto result = stdromano::fs::write_file_content(data, 12, file_path);
    STDROMANO_CHECK(!result.has_error());
    STDROMANO_CHECK(stdromano::fs::path_exists(file_path));

    stdromano::fs::removedir(root, true);
}

STDROMANO_TEST_CASE(write_file_content_append)
{
    const stdromano::StringD file_path = stdromano::test::temp_path("stdromano_test_append.txt");

    stdromano::fs::removefile(file_path);

    const char* data1 = "hello";
    STDROMANO_CHECK(!stdromano::fs::write_file_content(data1, 5, file_path, "w").has_error());

    const char* data2 = " world";
    STDROMANO_CHECK(!stdromano::fs::write_file_content(data2, 6, file_path, "a").has_error());

    auto content = stdromano::fs::load_file_content(file_path, "r");
    STDROMANO_CHECK(!content.has_error());
    STDROMANO_CHECK_EQ(static_cast<std::size_t>(11), content.unwrap().size());

    stdromano::fs::removefile(file_path);
}

STDROMANO_TEST_CASE(list_dir_all)
{
    const stdromano::StringD directory_path = stdromano::fs::parent_dir(__FILE__).copy();
    spdlog::debug("Listing directory: {}", directory_path);

    stdromano::fs::ListDirIterator it;
    int count = 0;

    while(stdromano::fs::list_dir(it, directory_path, stdromano::fs::ListDirFlags_ListAll))
    {
        spdlog::debug("{} | file={} dir={}", it.get_current_path(), it.is_file(), it.is_directory());

        STDROMANO_CHECK(it.is_file() || it.is_directory());
        STDROMANO_CHECK(!(it.is_file() && it.is_directory()));

        count++;
    }

    STDROMANO_CHECK(count > 0);
}

STDROMANO_TEST_CASE(list_dir_files_only)
{
    const stdromano::StringD directory_path = stdromano::fs::parent_dir(__FILE__).copy();

    stdromano::fs::ListDirIterator it;

    while(stdromano::fs::list_dir(it, directory_path, stdromano::fs::ListDirFlags_ListFiles))
    {
        STDROMANO_CHECK(it.is_file());
        STDROMANO_CHECK(!(it.is_directory()));
    }
}

STDROMANO_TEST_CASE(list_dir_dirs_only)
{
    const stdromano::StringD directory_path = stdromano::fs::parent_dir(__FILE__).copy();

    stdromano::fs::ListDirIterator it;

    while(stdromano::fs::list_dir(it, directory_path, stdromano::fs::ListDirFlags_ListDirs))
    {
        STDROMANO_CHECK(!(it.is_file()));
        STDROMANO_CHECK(it.is_directory());
    }
}

STDROMANO_TEST_CASE(list_dir_known_contents)
{
    const stdromano::StringD test_dir = stdromano::test::temp_path("stdromano_test_listdir");
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

    STDROMANO_CHECK_EQ(file_count, 2);
    STDROMANO_CHECK_EQ(dir_count, 1);

    stdromano::fs::removedir(test_dir, true);
}

STDROMANO_TEST_CASE(list_dir_iterator_move)
{
    const stdromano::StringD directory_path = stdromano::fs::parent_dir(__FILE__).copy();

    stdromano::fs::ListDirIterator it;
    stdromano::fs::list_dir(it, directory_path, stdromano::fs::ListDirFlags_ListAll);

    stdromano::fs::ListDirIterator it2(std::move(it));
    STDROMANO_CHECK(!it2.get_current_path().empty());

    stdromano::fs::ListDirIterator it3;
    it3 = std::move(it2);
    STDROMANO_CHECK(!it3.get_current_path().empty());
}

STDROMANO_TEST_CASE(walk_all)
{
    const stdromano::StringD directory_path = stdromano::fs::parent_dir(__FILE__).copy();
    spdlog::debug("Walk: {}", directory_path);

    int count = 0;

    for(stdromano::fs::WalkIterator it(directory_path, stdromano::fs::WalkFlags_ListAll);
        it != stdromano::fs::WalkIterator();
        ++it)
    {
        const auto& item = *it;
        STDROMANO_CHECK(!item.get_current_path().empty());
        STDROMANO_CHECK(item.is_file() || item.is_directory());
        STDROMANO_CHECK(!(item.is_file() && item.is_directory()));
        count++;
    }

    STDROMANO_CHECK(count > 0);
}

STDROMANO_TEST_CASE(walk_files_only)
{
    const stdromano::StringD directory_path = stdromano::fs::parent_dir(__FILE__).copy();

    for(stdromano::fs::WalkIterator it(directory_path, stdromano::fs::WalkFlags_ListFiles);
        it != stdromano::fs::WalkIterator();
        ++it)
    {
        STDROMANO_CHECK(it->is_file());
        STDROMANO_CHECK(!(it->is_directory()));
    }
}

STDROMANO_TEST_CASE(walk_dirs_only)
{
    const stdromano::StringD directory_path = stdromano::fs::parent_dir(__FILE__).copy();

    for(stdromano::fs::WalkIterator it(directory_path, stdromano::fs::WalkFlags_ListDirs);
        it != stdromano::fs::WalkIterator();
        ++it)
    {
        STDROMANO_CHECK(!(it->is_file()));
        STDROMANO_CHECK(it->is_directory());
    }
}

STDROMANO_TEST_CASE(walk_recursive)
{
    const stdromano::StringD root = stdromano::test::temp_path("stdromano_test_walk");
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

    STDROMANO_CHECK_EQ(file_count, 3);
    STDROMANO_CHECK_EQ(dir_count, 2);

    stdromano::fs::removedir(root, true);
}

STDROMANO_TEST_CASE(walk_non_recursive)
{
    const stdromano::StringD root = stdromano::test::temp_path("stdromano_test_walk_nr");
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

    STDROMANO_CHECK_EQ(file_count, 1);

    stdromano::fs::removedir(root, true);
}

STDROMANO_TEST_CASE(walk_iterator_end)
{
    stdromano::fs::WalkIterator end;
    STDROMANO_CHECK(end == stdromano::fs::WalkIterator());
}

STDROMANO_TEST_CASE(walk_iterator_arrow)
{
    const stdromano::StringD directory_path = stdromano::fs::parent_dir(__FILE__).copy();

    stdromano::fs::WalkIterator it(directory_path, stdromano::fs::WalkFlags_ListAll);
    if(it != stdromano::fs::WalkIterator())
    {
        const stdromano::StringD& path = it->get_current_path();
        STDROMANO_CHECK(!path.empty());

        const auto& item = *it;
        STDROMANO_CHECK(!item.get_current_path().empty());
    }
}

STDROMANO_TEST_CASE(write_then_load_roundtrip)
{
    const stdromano::StringD file_path = stdromano::test::temp_path("stdromano_roundtrip.txt");

    stdromano::fs::removefile(file_path);

    const char* expected = "round trip content 12345";
    const std::size_t expected_sz = std::strlen(expected);

    STDROMANO_CHECK(!stdromano::fs::write_file_content(expected, expected_sz, file_path, "w").has_error());

    auto loaded = stdromano::fs::load_file_content(file_path, "r");
    STDROMANO_CHECK(!loaded.has_error());

    const stdromano::StringD content = loaded.unwrap();
    STDROMANO_CHECK_EQ(expected_sz, content.size());
    STDROMANO_CHECK_EQ(std::memcmp(expected, content.c_str(), expected_sz), 0);

    stdromano::fs::removefile(file_path);
}

STDROMANO_TEST_CASE(copyfile_nonexistent_src)
{
    const stdromano::StringD tmp = stdromano::fs::tmp_dir().unwrap();
    const stdromano::StringD src("/tmp/stdromano_no_such_src.txt");
    const stdromano::StringD dst = stdromano::test::temp_path("stdromano_copy_fail_dst.txt");

    auto result = stdromano::fs::copyfile(src, dst);
    STDROMANO_CHECK(result.has_error());
}

STDROMANO_TEST_CASE(fuzz_binary_write_load_round_trip)
{
    const auto report = stdromano::fuzz::run_property(fixtures::options("fs_round_trip", 100), [](stdromano::fuzz::Source& source) {
        const std::vector<std::uint8_t> bytes = source.bytes(8192);

        const stdromano::StringD path = stdromano::test::temp_path("fuzz_round_trip/data.bin");
        const stdromano::StringD copy = stdromano::test::temp_path("fuzz_round_trip/copy.bin");

        STDROMANO_FUZZ_CHECK(stdromano::fs::write_file_content(reinterpret_cast<const char*>(bytes.data()),
                                                               bytes.size(),
                                                               path,
                                                               "wb")
                                 .has_value());

        const auto size = stdromano::fs::filesize(path);
        STDROMANO_FUZZ_CHECK(size.has_value());
        STDROMANO_FUZZ_CHECK_EQ(size.value(), bytes.size());

        STDROMANO_FUZZ_CHECK(stdromano::fs::copyfile(path, copy).has_value());

        for(const stdromano::StringD* file : {&path, &copy})
        {
            const auto content = stdromano::fs::load_file_content(*file, "rb");
            STDROMANO_FUZZ_CHECK(content.has_value());
            STDROMANO_FUZZ_CHECK_EQ(content.value().size(), bytes.size());
            STDROMANO_FUZZ_CHECK(bytes.empty() || std::memcmp(content.value().data(), bytes.data(), bytes.size()) == 0);
        }

        STDROMANO_FUZZ_CHECK(stdromano::fs::removefile(copy).has_value());
        STDROMANO_FUZZ_CHECK(!stdromano::fs::path_exists(copy));

        return true;
    });

    TESTS_REQUIRE_PROPERTY(report);
}

STDROMANO_TEST_CASE(fuzz_filename_and_parent_dir_split_a_path)
{
    const auto report = stdromano::fuzz::run_property(fixtures::options("fs_path_split", 500), [](stdromano::fuzz::Source& source) {
        std::string path = "root";

        const std::size_t depth = source.range<std::size_t>(0, 6);

        for(std::size_t i = 0; i < depth; ++i)
        {
            const stdromano::StringD component = source.string(12, "abcdefgh_-0123");
            path += "/d" + std::string(component.c_str(), component.size());
        }

        const stdromano::StringD name = source.string(12, "abcxyz_");
        const std::string leaf = "f" + std::string(name.c_str(), name.size()) + ".txt";
        const std::string full = path + "/" + leaf;

        const stdromano::StringD full_path = stdromano::StringD::make_from_c_str(full.c_str(), full.size());

        const stdromano::StringD file = stdromano::fs::filename(full_path);
        const stdromano::StringD parent = stdromano::fs::parent_dir(full_path);

        STDROMANO_FUZZ_CHECK_EQ(std::string(file.c_str(), file.size()), leaf);
        STDROMANO_FUZZ_CHECK_EQ(std::string(parent.c_str(), parent.size()), path);

        return true;
    });

    TESTS_REQUIRE_PROPERTY(report);
}

STDROMANO_TEST_MAIN()
