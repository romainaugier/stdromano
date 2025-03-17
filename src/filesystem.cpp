// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - Present Romain Augier
// All rights reserved.

#include "stdromano/filesystem.h"

#if defined(STDROMANO_WIN)
#include "ShlObj_core.h"
#include "Shlwapi.h"
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

String<> fs_filename(const String<>& path) noexcept
{
    size_t path_len = path.size() - 1;

    while(path_len > 0 && (path[path_len] != '/' && path[path_len] != '\\'))
    {
        path_len--;
    }

    return String<>::make_ref(path.c_str() + path_len + 1, path.size() - path_len - 1);
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
        FindClose(this->_h_find);
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
    return String<>("{}/{}",
                    fmt::string_view(this->_directory_path.c_str(), this->_directory_path.size()),
                    this->_entry->d_name);
#endif /* defined(STDROMANO_WIN) */
    return String<>();
}

bool fs_list_dir(ListDirIterator& it, const String<>& directory_path, const uint32_t flags) noexcept
{
    if(!fs_path_exists(directory_path))
    {
        return false;
    }

#if defined(STDROMANO_WIN)
    if(it._h_find == INVALID_HANDLE_VALUE)
    {
        it._directory_path = directory_path.copy();
        it._directory_path.appendc("\\*");
        it._h_find = FindFirstFileA(it._directory_path.c_str(), &it._find_data);

        if(it._h_find == INVALID_HANDLE_VALUE)
        {
            DWORD err = GetLastError();

            std::fprintf(stderr, "Error during fs_list_dir. Error code: %u", err);

            return false;
        }

        if((it._find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
        {
            const size_t c_file_name_size = std::strlen(it._find_data.cFileName);

            if((it._find_data.cFileName[0] != '.' || c_file_name_size > 2) && (flags & ListDirFlags_ListDirs))
            {
                return true;
            }
        }
        else if(flags & ListDirFlags_ListFiles)
        {
            return true;
        }
    }

    while(1)
    {
        if(FindNextFileA(it._h_find, &it._find_data))
        {
            if((it._find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
            {
                const size_t c_file_name_size = std::strlen(it._find_data.cFileName);

                if((it._find_data.cFileName[0] != '.' || c_file_name_size > 2) && (flags & ListDirFlags_ListDirs))
                {
                    return true;
                }
            }
            else if(flags & ListDirFlags_ListFiles)
            {
                return true;
            }
        }
        else
        {
            DWORD err = GetLastError();

            if(err != ERROR_NO_MORE_FILES)
            {
                std::fprintf(stderr, "Error during fs_list_dir. Error code: %u", err);
            }

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