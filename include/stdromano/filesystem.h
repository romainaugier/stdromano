// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - Present Romain Augier
// All rights reserved.

#pragma once

#if !defined(__STDROMANO_FILESYSTEM)
#define __STDROMANO_FILESYSTEM

#include "stdromano/string.h"

#if defined(STDROMANO_WIN)
#include <Windows.h>
#elif defined(STDROMANO_LINUX)
#include <dirent.h>
#endif /* defined(STDROMANO_WIN) */

STDROMANO_NAMESPACE_BEGIN

STDROMANO_API bool fs_path_exists(const String<>& path) noexcept;

STDROMANO_API String<> fs_parent_dir(const String<>& path) noexcept;

STDROMANO_API String<> fs_filename(const String<>& path) noexcept;

STDROMANO_API String<> expand_from_executable_dir(const String<>& path_to_expand) noexcept;

STDROMANO_API String<> load_file_content(const String<>& file_path) noexcept;

enum ListDirFlags : uint32_t
{
    ListDirFlags_ListFiles = 0x1,
    ListDirFlags_ListDirs = 0x2,
    ListDirFlags_ListHidden = 0x4,
    ListDirFlags_ListAll = ListDirFlags_ListFiles | ListDirFlags_ListDirs
};

class STDROMANO_API ListDirIterator
{
public:
    friend STDROMANO_API bool fs_list_dir(ListDirIterator&, const String<>&, const uint32_t) noexcept;

private:
    String<> _directory_path;

#if defined(STDROMANO_WIN)
    WIN32_FIND_DATAA _find_data;
    HANDLE _h_find = INVALID_HANDLE_VALUE;
#elif defined(STDROMANO_LINUX)
    DIR* _dir = nullptr;
    struct dirent* _entry = nullptr;
#endif /* defined(STDROMANO_WIN) */

public:
    ListDirIterator() = default;

    ~ListDirIterator();

    String<> get_current_path() const noexcept;

    bool is_file() const noexcept;

    bool is_directory() const noexcept;
};

STDROMANO_API bool fs_list_dir(ListDirIterator& it, const String<>& directory_path, const uint32_t flags) noexcept;

enum FileDialogMode_ : uint32_t
{
    FileDialogMode_OpenFile,
    FileDialogMode_SaveFile,
    FileDialogMode_OpenDir,
};

// filter should be formatted like: *.h|*.cpp

STDROMANO_API String<> open_file_dialog(FileDialogMode_ mode, 
                                        const String<>& title,
                                        const String<>& initial_path,
                                        const String<>& filter) noexcept;

STDROMANO_NAMESPACE_END

#endif // !defined(__STDROMANO_FILESYSTEM)