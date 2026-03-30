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

// Returns true if the given path exists on the filesystem (file or directory), false otherwise
STDROMANO_API bool path_exists(const StringD& path) noexcept;

// Returns the parent directory of the given path (e.g. "/foo/bar/baz.txt" -> "/foo/bar")
STDROMANO_API StringD parent_dir(const StringD& path) noexcept;

// Returns the filename component of the given path (e.g. "/foo/bar/baz.txt" -> "baz.txt")
STDROMANO_API StringD filename(const StringD& path) noexcept;

// Returns the current working directory of the process
STDROMANO_API StringD current_dir() noexcept;

// Creates a directory at the given path. Returns an error if the operation fails
STDROMANO_API Expected<void> makedir(const StringD& dir_path) noexcept;

// Removes the directory at the given path. If recursive=true, removes its content.
// If the directory does not exist, return Ok (no-op)
STDROMANO_API Expected<void> removedir(const StringD& dir_path, const bool recursive = true) noexcept;

// Removes the file at the given path. If the file does not exist, return Ok (no-op)
STDROMANO_API Expected<void> removefile(const StringD& file_path) noexcept;

// Copies the file at src to dst. Returns an error if src does not exist or the operation fails
STDROMANO_API Expected<void> copyfile(const StringD& src, const StringD& dst) noexcept;

// Expands a relative path by prepending the directory of the current executable
// (e.g. "data/file.txt" -> "/path/to/exe_dir/data/file.txt")
STDROMANO_API Expected<StringD> expand_from_executable_dir(const StringD& path_to_expand) noexcept;

// Expands a relative path by prepending the directory of the current shared library
// (e.g. "data/file.txt" -> "/path/to/lib_dir/data/file.txt")
STDROMANO_API Expected<StringD> expand_from_lib_dir(const StringD& path_to_expand) noexcept;

// Returns the platform-specific temporary directory path (e.g. "/tmp" on Linux, %TEMP% on Windows)
STDROMANO_API Expected<StringD> tmp_dir() noexcept;

// Returns the current user's home directory path.
// If use_env is true, resolves from environment variables (e.g. $HOME or %USERPROFILE%) instead of system APIs
// On Windows, the first call to home_dir without use_env will be VERY SLOW as SHGetKnownFolderPath
// has to initialize whatever Winslop needs to resolve the path of a fucking directory
STDROMANO_API Expected<StringD> home_dir(const bool use_env = false) noexcept;

// Loads the entire content of the file at file_path into a StringD.
// The mode parameter is passed directly to fopen (e.g. "r" for text, "rb" for binary)
STDROMANO_API Expected<StringD> load_file_content(const StringD& file_path,
                                                  const char* mode = "rb") noexcept;

// Writes data_sz bytes from data to the file at file_path.
// The mode parameter is passed directly to fopen (e.g. "w" for write, "a" for append).
// If the parent directory of file_path does not exist, it will be created automatically
STDROMANO_API Expected<void> write_file_content(const char* data,
                                                const std::size_t data_sz,
                                                const StringD& file_path,
                                                const char* mode = "w") noexcept;

// Flags controlling which entries list_dir yields
enum ListDirFlags : std::uint32_t
{
    ListDirFlags_ListFiles = 0x1, // Include regular files
    ListDirFlags_ListDirs = 0x2, // Include directories
    ListDirFlags_ListHidden = 0x4, // Include hidden files (dotfiles, hidden attribute)
    ListDirFlags_ListAll = ListDirFlags_ListFiles | ListDirFlags_ListDirs
};

// Non-recursive directory iterator. Yields entries one at a time via repeated calls to list_dir().
// The iterator holds platform-specific handles and is move-only (non-copyable).
// Usage:
//   ListDirIterator it;
//   while(list_dir(it, path, flags)) { /* use it.get_current_path(), it.is_file(), ... */ }
class STDROMANO_API ListDirIterator
{
public:
    friend STDROMANO_API bool list_dir(ListDirIterator&,
                                       const StringD&,
                                       const std::uint32_t) noexcept;

private:
    StringD _directory_path;

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

    // Returns the full path of the current entry
    StringD get_current_path() const noexcept;

    // Returns true if the current entry is a regular file
    bool is_file() const noexcept;

    // Returns true if the current entry is a directory
    bool is_directory() const noexcept;
};

// Advances the iterator to the next entry matching flags in directory_path.
// Returns true if a new entry is available, false when iteration is complete.
// On the first call, opens the directory and positions to the first matching entry
STDROMANO_API bool list_dir(ListDirIterator& it,
                            const StringD& directory_path,
                            const uint32_t flags) noexcept;

// Mode for the native file dialog
enum FileDialogMode_ : std::uint32_t
{
    FileDialogMode_OpenFile,
    FileDialogMode_SaveFile,
    FileDialogMode_OpenDir,
};

// Opens a native file dialog and returns the selected path, or an empty string if cancelled.
// filter should be formatted like: *.h|*.cpp
STDROMANO_API StringD open_file_dialog(FileDialogMode_ mode,
                                       const StringD& title,
                                       const StringD& initial_path,
                                       const StringD& filter) noexcept;

// Flags controlling which entries WalkIterator yields and whether it recurses
enum WalkFlags : std::uint32_t
{
    WalkFlags_ListFiles = 0x1, // Include regular files
    WalkFlags_ListDirs = 0x2, // Include directories
    WalkFlags_ListHidden = 0x4, // Include hidden directories
    WalkFlags_ListAll = WalkFlags_ListFiles | WalkFlags_ListDirs,
    WalkFlags_Recursive = 0x8, // Recurse into subdirectories (breadth-first)
};

// Breadth-first directory walker. Supports optional recursion via WalkFlags_Recursive.
// Conforms to an input-iterator-like interface (operator++, operator*, operator->).
// A default-constructed WalkIterator acts as the end sentinel.
// Usage:
//   for(WalkIterator it(root, flags); it != WalkIterator(); ++it)
//       /* use it->get_current_path(), it->is_file(), ... */
class STDROMANO_API WalkIterator
{
public:
    // A single entry produced by the walker, holding its full path and type
    struct Item
    {
        StringD _path;
        bool _is_directory;

        // Returns true if the entry is a directory
        bool is_directory() const noexcept { return this->_is_directory; }

        // Returns true if the entry is a regular file
        bool is_file() const noexcept { return !this->_is_directory; }

        // Returns the full path of the entry
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
            this->_pending_dirs.push(root.copy());
        else
            this->_pending_dirs.push(root);

        this->advance();
    }

    WalkIterator() : _is_end(true) {}

    ~WalkIterator()
    {
#if defined(STDROMANO_WIN)
        if(this->_h_find != INVALID_HANDLE_VALUE)
            FindClose(this->_h_find);
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
                return;

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
