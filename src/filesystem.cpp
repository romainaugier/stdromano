// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - Present Romain Augier
// All rights reserved.

#include "stdromano/filesystem.h"

#if defined(STDROMANO_WIN)
#include "ShlObj_core.h"
#elif defined(STDROMANO_LINUX)
#include <limits.h>
#include <pwd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#endif // defined(STDROMANO_WIN)

STDROMANO_NAMESPACE_BEGIN

bool fs_path_exists(const String<>& path) noexcept
{
#if defined(STDROMANO_WIN)
    return PathFileExistsA(path.c_str());
#elif defined(STDROMANO_LINUX)
    struct stat sb;
    return stat(path.c_str(), &sb) == 0 && (S_ISDIR(sb.st_mode) || S_ISREG(sb.st_mode));
#endif /* defined(STDROMANO_WIN) */
}

String<> fs_parent_dir(const String<>& path) noexcept
{
    size_t path_len = path.size() - 1;

    while(path_len > 0 && (path[path_len] != '/' && path[path_len] != '\\'))
    {
        path_len--;
    }

    return String<>::make_ref(path.c_str(), path_len);
}

String<> expand_from_executable_dir(const String<>& path_to_expand) noexcept
{
    size_t size;

#if defined(STDROMANO_WIN)
    char sz_path[MAX_PATH];
    GetModuleFileNameA(nullptr, sz_path, MAX_PATH);

    size = std::strlen(sz_path);

    while(size > 0 && sz_path[size] != '\\')
    {
        size--;
    }
#elif defined(STDROMANO_LINUX)
    char sz_path[PATH_MAX];
    ssize_t count = readlink("/proc/self/exe", sz_path, PATH_MAX);

    if(count < 0 || count >= PATH_MAX)
        return "";

    sz_path[count] = '\0';

    size = count - 1;

    while(size > 0 && sz_path[size] != '/')
    {
        size--;
    }
#endif /* defined(STDROMANO_WIN) */

    return std::move(String<>("{}/{}", fmt::string_view(sz_path, size), path_to_expand));
}

String<> load_file_content(const String<>& file_path) noexcept
{
    FILE* file_handle;

    file_handle = std::fopen(file_path.c_str(), "r");

    if(file_handle == nullptr)
    {
        return stdromano::String<>();
    }

    std::fseek(file_handle, 0, SEEK_END);
    const size_t file_size = std::ftell(file_handle);

    stdromano::String<> file_content = stdromano::String<>::make_zeroed(file_size);

    std::rewind(file_handle);
    std::fread(file_content.data(), sizeof(char), file_size, file_handle);
    std::fclose(file_handle);

    return std::move(file_content);
}

ListDirIterator::~ListDirIterator()
{
#if defined(STDROMANO_WIN)
    if(this->_h_find != INVALID_HANDLE_VALUE)
    {
        FindClose(this->h_find);
    }
#elif defined(STDROMANO_LINUX)
    if(this->_dir != nullptr)
    {
        closedir(this->_dir);
    }
#endif /* defined(STDROMANO_WIN) */
}

String<> ListDirIterator::get_current_path() const noexcept
{
#if defined(STDROMANO_WIN)
    return String<>("{}{}",
                    fmt::string_view(this->_directory_path.c_str(), this->_directory_path.size() - 1),
                    this->_find_data.cFileName);
#elif defined(STDROMANO_LINUX)
    return String<>(
        "{}/{}", fmt::string_view(this->_directory_path.c_str(), this->_directory_path.size()), this->_entry->d_name);
#endif /* defined(STDROMANO_WIN) */
    return String<>();
}

bool fs_list_dir(ListDirIterator& it, const String<>& directory_path, const uint32_t flags) noexcept
{
    if(!fs_path_exists(directory_path))
    {
        return false;
    }

#if defined(STROMANO_WIN)
#error "No implementation of fs_list_dir working on Windows"
    if(_list_dir_data_count == 0 || dir_hash != CURRENT_LIST_DIR_DATA.directory_hash)
    {
        _list_dir_data_count++;

        if(_list_dir_data_count == MAX_RECURSE_LIST_DIR)
        {
            log_error("Cannot recurse more than {} times in fs_list_dir", MAX_RECURSE_LIST_DIR);
            return false;
        }

        CURRENT_LIST_DIR_DATA = ListDirData(directory_path, directory_path_length);
        CURRENT_LIST_DIR_DATA.h_find
            = FindFirstFileA(CURRENT_LIST_DIR_DATA.directory_path.c_str(), &CURRENT_LIST_DIR_DATA.find_data);

        if(CURRENT_LIST_DIR_DATA.h_find == INVALID_HANDLE_VALUE)
        {
            DWORD err = GetLastError();

            log_error("Error during fs_list_dir. Error code: {}", err);

            CURRENT_LIST_DIR_DATA.~ListDirData();
            _list_dir_data_count--;

            return false;
        }

        if((CURRENT_LIST_DIR_DATA.find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
        {
            if(CURRENT_LIST_DIR_DATA.find_data.cFileName[0] != '.' && (mode & ListDirFlags::ListDirFlags_YieldDirs))
            {
                CURRENT_LIST_DIR_DATA.make_current_file_path(out.path);

                return true;
            }
        }
        else if(mode & ListDirFlags::ListDirFlags_YieldFiles)
        {
            CURRENT_LIST_DIR_DATA.make_current_file_path(out.path);
            return true;
        }
    }

    while(1)
    {
        if(FindNextFileA(CURRENT_LIST_DIR_DATA.h_find, &CURRENT_LIST_DIR_DATA.find_data))
        {
            if((CURRENT_LIST_DIR_DATA.find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
            {
                if(CURRENT_LIST_DIR_DATA.find_data.cFileName[0] != '.' && (mode & ListDirFlags::ListDirFlags_YieldDirs))
                {
                    CURRENT_LIST_DIR_DATA.make_current_file_path(out.path);

                    return true;
                }
            }
            else if(mode & ListDirFlags::ListDirFlags_YieldFiles)
            {
                CURRENT_LIST_DIR_DATA.make_current_file_path(out.path);

                return true;
            }
        }
        else
        {
            DWORD err = GetLastError();

            if(err != ERROR_NO_MORE_FILES)
            {
                log_error("Error during fs_list_dir. Error code: {}", err);
            }

            CURRENT_LIST_DIR_DATA.~ListDirData();
            _list_dir_data_count--;

            return false;
        }
    }

    return false;
#elif defined(STDROMANO_LINUX)
    if(it._dir == nullptr)
    {
        it._directory_path = directory_path.copy();
        it._dir = opendir(it._directory_path.c_str());
    }

    while((it._entry = readdir(it._dir)))
    {
        bool is_hidden = it._entry->d_name[0] == '.';

        if(is_hidden && !(flags & ListDirFlags_ListHidden))
        {
            continue;
        }

        bool is_file = false;
        bool is_dir = false;

        if(it._entry->d_type == DT_UNKNOWN)
        {
            struct stat st;
            const String<> full_path = std::move(it.get_current_path());

            if(stat(full_path.c_str(), &st) == 0)
            {
                is_file = S_ISREG(st.st_mode);
                is_dir = S_ISDIR(st.st_mode);
            }
        }
        else
        {
            is_file = (it._entry->d_type == DT_REG);
            is_dir = (it._entry->d_type == DT_DIR);
        }

        return (is_file && (flags & ListDirFlags_ListFiles)) || (is_dir && (flags & ListDirFlags_ListDirs));
    }

    return false;

#endif /* defined(STDROMANO_WIN) */
}

STDROMANO_NAMESPACE_END