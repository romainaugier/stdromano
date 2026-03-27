// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - Present Romain Augier
// All rights reserved.

#pragma once

#if !defined(__STDROMANO_FILESYSTEM)
#define __STDROMANO_FILESYSTEM

#include "stdromano/string.hpp"
#include "stdromano/expected.hpp"

#include <queue>

#if defined(STDROMANO_WIN)
#include <Windows.h>
#elif defined(STDROMANO_LINUX)
#include <dirent.h>
#endif /* defined(STDROMANO_WIN) */

#define FS_NAMESPACE_BEGIN namespace fs {
#define FS_NAMESPACE_END }

STDROMANO_NAMESPACE_BEGIN

FS_NAMESPACE_BEGIN

STDROMANO_API bool path_exists(const String<>& path) noexcept;

STDROMANO_API String<> parent_dir(const String<>& path) noexcept;

STDROMANO_API String<> filename(const String<>& path) noexcept;

STDROMANO_API StringD current_dir() noexcept;

STDROMANO_API bool mkdir(const StringD& dir_path) noexcept;

STDROMANO_API Expected<StringD> expand_from_executable_dir(const String<>& path_to_expand) noexcept;

STDROMANO_API Expected<StringD> expand_from_lib_dir(const String<>& path_to_expand) noexcept;

STDROMANO_API Expected<StringD> tmp_dir() noexcept;

STDROMANO_API Expected<StringD> home_dir(bool use_env = false) noexcept;

// Loads the content of file_path into a StringD
STDROMANO_API Expected<StringD> load_file_content(const String<>& file_path,
                                                  const char* mode = "rb") noexcept;

// Write data to file_path. If the path to the parent directory of file_path does not exist, it will be created
STDROMANO_API Expected<void> write_file_content(const char* data,
                                                const std::size_t data_sz,
                                                const StringD& file_path,
                                                const char* mode = "w") noexcept;

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
    friend STDROMANO_API bool list_dir(ListDirIterator&,
                                          const String<>&,
                                          const std::uint32_t) noexcept;

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

    STDROMANO_NON_COPYABLE(ListDirIterator);

    ListDirIterator(ListDirIterator&& other) noexcept : _directory_path(std::move(other._directory_path))
    {
#if defined(STDROMANO_WIN)
        this->_h_find = other._h_find;
        std::memcpy(&this->_find_data, &other._find_data, sizeof(WIN32_FIND_DATAA));

        other._h_find = INVALID_HANDLE_VALUE;
        std::memset(&other._find_data, 0, sizeof(WIN32_FIND_DATAA));
#elif defined(STDROMANO_LINUX)
        this->_dir = other._dir;
        this->_entry = other._entry;

        other._dir = nullptr;
        other._entry = nullptr;
#endif // defined(STDROMANO_WIN)
    }

    ListDirIterator& operator=(ListDirIterator&& other) noexcept
    {
        if(this != &other)
        {
            this->_directory_path = std::move(other._directory_path);

#if defined(STDROMANO_WIN)
            this->_h_find = other._h_find;
            std::memcpy(&this->_find_data, &other._find_data, sizeof(WIN32_FIND_DATAA));

            other._h_find = INVALID_HANDLE_VALUE;
            std::memset(&other._find_data, 0, sizeof(WIN32_FIND_DATAA));
#elif defined(STDROMANO_LINUX)
            this->_dir = other._dir;
            this->_entry = other._entry;

            other._dir = nullptr;
            other._entry = nullptr;
#endif // defined(STDROMANO_WIN)
        }

        return *this;
    }

    ~ListDirIterator();

    String<> get_current_path() const noexcept;

    bool is_file() const noexcept;

    bool is_directory() const noexcept;
};

STDROMANO_API bool list_dir(ListDirIterator& it,
                            const String<>& directory_path,
                            const uint32_t flags) noexcept;

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

enum WalkFlags : uint32_t
{
    WalkFlags_ListFiles = 0x1,
    WalkFlags_ListDirs = 0x2,
    WalkFlags_ListHidden = 0x4,
    WalkFlags_ListAll = WalkFlags_ListFiles | WalkFlags_ListDirs,
    WalkFlags_Recursive = 0x8,
};

class STDROMANO_API WalkIterator
{
public:
    struct Item
    {
        StringD _path;
        bool _is_directory;

        bool is_directory() const noexcept { return this->_is_directory; }
        bool is_file() const noexcept { return !this->_is_directory; }

        const StringD& get_current_path() const noexcept { return this->_path; }
    };

private:
    std::queue<StringD> _pending_dirs;
    StringD _current_dir;

    Item _current_item;

    uint32_t _flags;
    bool _is_end;

#if defined(STDROMANO_WIN)
    HANDLE _h_find = INVALID_HANDLE_VALUE;
#elif defined(STDROMANO_LINUX)
    DIR* _dir = nullptr;
#endif /* defined(STDROMANO_WIN) */

public:
    WalkIterator(const StringD& root, std::uint32_t flags = 0) : _flags(flags), _is_end(false)
    {
        if(root.is_ref())
        {
            this->_pending_dirs.push(root.copy());
        }
        else
        {
            this->_pending_dirs.push(root);
        }

        this->advance();
    }

    WalkIterator() : _is_end(true) {}

    ~WalkIterator()
    {
#if defined(STDROMANO_WIN)
        if(this->_h_find != INVALID_HANDLE_VALUE)
        {
            FindClose(this->_h_find);
        }
#endif /* defined(STDROMANO_WIN) */
    }

    const Item& operator*() const noexcept { return this->_current_item; }
    const Item* operator->() const noexcept { return &this->_current_item; }

    WalkIterator& operator++() noexcept
    {
        this->advance();
        return *this;
    }

    WalkIterator operator++(int) = delete;

    bool operator==(const WalkIterator& other) const noexcept
    {
        return this->_is_end == other._is_end;
    }

    bool operator!=(const WalkIterator& other) const noexcept
    {
        return !(*this == other);
    }

private:
    void advance() noexcept
    {
        while(!this->_is_end)
        {
            if(this->process_current_directory())
            {
                return;
            }

            this->move_to_next_directory();
        }
    }

    bool process_current_directory() noexcept;

    void move_to_next_directory() noexcept;

    bool should_skip_entry(const char* name
#if defined(STDROMANO_WIN)
                          , DWORD attrs) const noexcept;
#else
                           ) const noexcept;
#endif /* defined(STDROMANO_WIN) */
};

FS_NAMESPACE_END

STDROMANO_NAMESPACE_END

#if defined(STDROMANO_WIN)
STDROMANO_EXPIMP_TEMPLATE template class STDROMANO_API std::queue<stdromano::StringD>;
#endif /* defined(STDROMANO_WIN) */

#endif // !defined(__STDROMANO_FILESYSTEM)
