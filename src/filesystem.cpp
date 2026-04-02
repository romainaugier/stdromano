// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - Present Romain Augier
// All rights reserved.

#include "stdromano/filesystem.hpp"
#include "stdromano/loop_guard.hpp"

#if defined(STDROMANO_WIN)
#define STRICT_TYPED_ITEMIDS // Better type safety for IDLists
#include "ShlObj_core.h"
#include "shellapi.h"
#include "Shlwapi.h"
#include "commdlg.h"
#elif defined(STDROMANO_LINUX)
#if defined(STDROMANO_GCC)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#include <gtk/gtk.h>
#pragma GCC diagnostic pop
#endif /* defined(STDROMANO_GCC) */
#include <limits.h>
#include <pwd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/sendfile.h>
#include <unistd.h>
#include <fcntl.h>
#include <ftw.h>
#endif // defined(STDROMANO_WIN)

#include <stack>

#define STDROMANO_ERR_BUFFER_SZ 1024

STDROMANO_NAMESPACE_BEGIN

FS_NAMESPACE_BEGIN

bool path_exists(const StringD& path) noexcept
{
#if defined(STDROMANO_WIN)
    return PathFileExistsA(path.is_ref() ? path.copy().c_str() : path.c_str());
#elif defined(STDROMANO_LINUX)
    struct stat sb;
    return stat(path.is_ref() ? path.copy().c_str() : path.c_str(), &sb) == 0 && (S_ISDIR(sb.st_mode) || S_ISREG(sb.st_mode));
#endif /* defined(STDROMANO_WIN) */
}

StringD parent_dir(const StringD& path) noexcept
{
    std::size_t path_len = path.size() - 1;

    while(path_len > 0 && (path[path_len] != '/' && path[path_len] != '\\'))
        path_len--;

    return StringD::make_ref(path.c_str(), path_len);
}

StringD filename(const StringD& path) noexcept
{
    std::size_t path_len = path.size() - 1;

    while(path_len > 0 && (path[path_len] != '/' && path[path_len] != '\\'))
        path_len--;

    return StringD::make_ref(path.c_str() + path_len + 1, path.size() - path_len - 1);
}

Expected<std::size_t> filesize(const StringD& path) noexcept
{
    if(!path_exists(path))
        return Error(StringD::make_fmt("File \"{}\" does not exist", path));

#if defined(STDROMANO_WIN)
    HANDLE file = CreateFileA(path.c_str(),
                              GENERIC_READ,
                              FILE_SHARE_READ,
                              0,
                              OPEN_EXISTING,
                              FILE_ATTRIBUTE_NORMAL,
                              nullptr);

    if(file == INVALID_HANDLE_VALUE)
    {
        DWORD last_err = GetLastError();

        char buffer[1024];
        std::memset(buffer, 0, 1024 * sizeof(char));
        DWORD res = FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                                   nullptr,
                                   last_err,
                                   MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                                   buffer,
                                   1024,
                                   nullptr);

        if(res == 0)
        {
            DWORD fmt_error = GetLastError();

            return Error(StringD::make_fmt("Cannot get file size ({})", last_err));
        }

        return Error(StringD::make_fmt("Cannot get file size: {} ({})", fmt::string_view(buffer, res), last_err));
    }

    LARGE_INTEGER file_size;
    std::memset(&file_size, 0, sizeof(LARGE_INTEGER));

    if(!GetFileSizeEx(file, &file_size))
    {
        DWORD last_err = GetLastError();

        CloseHandle(file);

        return Error();
    }

    CloseHandle(file);

    return static_cast<std::size_t>(file_size.QuadPart);
#elif defined(STDROMANO_LINUX)
    struct stat file_stat;

    if(stat(path.c_str(), &file_stat) != 0)
        return Error(StringD::make_fmt("Cannot get file size ({})", errno));

    return static_cast<std::size_t>(file_stat.st_size);
#else
    STDROMANO_NOT_IMPLEMENTED;
#endif // defined(STDROMANO_WIN)
}

Expected<StringD> relative_to(const StringD& path, const StringD& other) noexcept
{
    if(path.size() <= other.size())
        return Error("path cannot be shorter than the path you want to make it relative to");

    std::size_t i = 0;

    while(i < other.size() && path[i] == other[i])
        i++;

    return StringD::make_ref(path.c_str() + i, path.size() - i);
}

StringD current_dir() noexcept
{
#if defined(STDROMANO_WIN)
    StringD res = StringD::make_zeroed(512);

    DWORD size = GetCurrentDirectoryA(512, res.c_str());

    if(size >= 512)
    {
        res = std::move(StringD::make_zeroed(size));
        size = GetCurrentDirectoryA(size, res.c_str());
    }
    else
    {
        res.shrink_to_fit(size);
    }

    return res;
#elif defined(STDROMANO_LINUX)
    std::array<char, PATH_MAX> res;

    getcwd(res.data(), res.size());

    return StringD(res.data());
#else
    STDROMANO_NOT_IMPLEMENTED;
#endif /* defined(STDROMANO_WIN) */
}

Expected<void> makedir(const StringD& dir_path) noexcept
{
    if(path_exists(dir_path))
        return Ok();

#if defined(STDROMANO_WIN)
    std::stack<StringD> to_create;
    to_create.push(dir_path);

    LoopGuard guard("Too many parent directories or error", 128);

    while(guard--)
    {
        StringD parent = parent_dir(to_create.top()).copy();

        if(!path_exists(parent))
        {
            to_create.push(std::move(parent));
            continue;
        }

        break;
    }

    while(!to_create.empty())
    {
        StringD dir = std::move(to_create.top());

        if(!CreateDirectoryA(dir.is_ref() ? dir.copy().c_str() : dir.c_str(), NULL))
        {
            DWORD last_err = GetLastError();

            char buffer[STDROMANO_ERR_BUFFER_SZ];
            std::memset(buffer, 0, STDROMANO_ERR_BUFFER_SZ * sizeof(char));
            DWORD res = FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM,
                                    nullptr,
                                    last_err,
                                    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                                    buffer,
                                    STDROMANO_ERR_BUFFER_SZ,
                                    nullptr);

            if(res == 0)
                return Error(StringD::make_fmt("Cannot format error message (CreateDirectoryA error: {}, FormatMessageA error: {})",
                                            last_err,
                                            GetLastError()));

            return Error(StringD::make_fmt("CreateDirectoryA failed (Last error: {} ({}))",
                                        fmt::string_view(buffer, res),
                                        last_err));
        }

        to_create.pop();
    }

#elif defined(STDROMANO_LINUX)
    if(mkdir(dir_path.is_ref() ? dir_path.copy().c_str() : dir_path.c_str(), 0755) != 0)
        return Error(StringD::make_fmt("mkdir failed ({})", errno));
#else
    STDROMANO_NOT_IMPLEMENTED;
#endif /* defined(STDROMANO_WIN) */

    return Ok();
}

Expected<void> removedir(const StringD& dir_path, const bool recursive) noexcept
{
    if(!path_exists(dir_path))
        return Ok();

#if defined(STDROMANO_WIN)
    char* path_buffer = static_cast<char*>(mem_alloca((dir_path.size() + 2) * sizeof(char)));
    std::memset(path_buffer, 0, dir_path.size() + 2);
    std::memcpy(path_buffer, dir_path.c_str(), dir_path.size());

    SHFILEOPSTRUCTA file_op;
    std::memset(&file_op, 0, sizeof(SHFILEOPSTRUCTA));

    file_op.wFunc = FO_DELETE;
    file_op.pFrom = path_buffer;
    file_op.fFlags = FOF_NOCONFIRMATION | FOF_NO_UI | FOF_SILENT;

    if(!recursive)
        file_op.fFlags |= FOF_NORECURSION;

    int ret = SHFileOperationA(&file_op);

    if(ret != 0)
        return Error(StringD::make_fmt("SHFileOperationA failed (error: {})", ret));
#elif defined(STDROMANO_LINUX)
    static thread_local StringD last_error;

    auto callback = [](const char* fpath, const struct stat* sb, int typeflag, struct FTW* ftw) -> int {
        int rv = remove(fpath);

        if(rv != 0)
            last_error = StringD::make_fmt("Cannot delete file: {} ({})", fpath, errno);

        return rv;
    };

    int res = nftw(dir_path.is_ref() ? dir_path.copy().c_str() : dir_path.c_str(), callback, 64, FTW_DEPTH | FTW_PHYS);

    if(res != 0)
        return Error(std::move(last_error));
#endif // defined(STDROMANO_WIN)

    return Ok();
}

Expected<void> removefile(const StringD& file_path) noexcept
{
    if(!path_exists(file_path))
        return Ok();

#if defined(STDROMANO_WIN)
    if(!DeleteFileA(file_path.is_ref() ? file_path.copy().c_str() : file_path.c_str()))
    {
        DWORD last_err = GetLastError();

        char buffer[STDROMANO_ERR_BUFFER_SZ];
        std::memset(buffer, 0, STDROMANO_ERR_BUFFER_SZ * sizeof(char));

        DWORD res = FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM,
                                   nullptr,
                                   last_err,
                                   MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                                   buffer,
                                   STDROMANO_ERR_BUFFER_SZ,
                                   nullptr);

        if(res == 0)
            return Error(StringD::make_fmt("Cannot format error message (DeleteFileA error: {}, FormatMessageA error: {})",
                                           last_err,
                                           GetLastError()));

        return Error(StringD::make_fmt("DeleteFileA failed (Last error: {} ({}))",
                                       fmt::string_view(buffer, res),
                                       last_err));
    }
#elif defined(STDROMANO_LINUX)
    int res = remove(file_path.is_ref() ? file_path.copy().c_str() : file_path.c_str());

    if(res != 0)
        return Error(StringD::make_fmt("Cannot delete file: {} ({})", file_path, errno));
#endif // defined(STDROMANO_WIN)

    return Ok();
}

Expected<void> copyfile(const StringD& src, const StringD& dst) noexcept
{
    if(!path_exists(src))
        return Error(StringD::make_fmt("Cannot find src file for copy: {}", src));

#if defined(STDROMANO_WIN)
    if(!CopyFileA(src.is_ref() ? src.copy().c_str() : src.c_str(), dst.is_ref() ? dst.copy().c_str() : dst.c_str(), true))
    {
        DWORD last_err = GetLastError();

        char buffer[STDROMANO_ERR_BUFFER_SZ];
        std::memset(buffer, 0, STDROMANO_ERR_BUFFER_SZ * sizeof(char));

        DWORD res = FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM,
                                   nullptr,
                                   last_err,
                                   MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                                   buffer,
                                   STDROMANO_ERR_BUFFER_SZ,
                                   nullptr);

        if(res == 0)
            return Error(StringD::make_fmt("Cannot format error message (CopyFileA error: {}, FormatMessageA error: {})",
                                           last_err,
                                           GetLastError()));

        return Error(StringD::make_fmt("CopyFileA failed (Last error: {} ({}))",
                                       fmt::string_view(buffer, res),
                                       last_err));
    }
#elif defined(STDROMANO_LINUX)
    int input, output;

    if((input = open(src.is_ref() ? src.copy().c_str() : src.c_str(), O_RDONLY)) == -1)
        return Error(StringD::make_fmt("Cannot open src file \"{}\" for copy", src));

    if((output = creat(dst.is_ref() ? dst.copy().c_str() : dst.c_str(), 0660)) == -1)
        return Error(StringD::make_fmt("Cannot create dst file \"{}\" for copy", dst));

    struct stat file_stat;
    std::memset(&file_stat, 0, sizeof(struct stat));

    int res = fstat(input, &file_stat);

    off_t copied = 0;

    while(res == 0 && copied < file_stat.st_size)
    {
        ssize_t written = sendfile(output, input, &copied, SSIZE_MAX);
        copied += written;

        if(written == -1)
            res = -1;
    }

    close(input);
    close(output);

    if(res == -1)
        return Error(StringD::make_fmt("Cannot copy file \"{}\" to \"{}\" ({})", src, dst, errno));
#endif // defined(STDROMANO_WIN)

    return Ok();
}

Expected<StringD> expand_from_executable_dir(const StringD& path_to_expand) noexcept
{
    std::size_t size;

#if defined(STDROMANO_WIN)
    char sz_path[MAX_PATH];

    if(GetModuleFileNameA(nullptr, sz_path, MAX_PATH) == 0)
        return Error(StringD::make_fmt("Error caught during GetModuleFileNameA: {}", GetLastError()));

    size = std::strlen(sz_path);

    while(size > 0 && sz_path[size] != '\\')
        size--;

#elif defined(STDROMANO_LINUX)
    char sz_path[PATH_MAX];
    ssize_t count = readlink("/proc/self/exe", sz_path, PATH_MAX);

    if(count < 0 || count >= PATH_MAX)
        return Error(StringD::make_fmt("Error during readlink: {}", errno));

    sz_path[count] = '\0';

    size = count - 1;

    while(size > 0 && sz_path[size] != '/')
        size--;

#endif /* defined(STDROMANO_WIN) */

    return StringD("{}/{}", fmt::string_view(sz_path, size), path_to_expand);
}

Expected<StringD> expand_from_lib_dir(const StringD& path_to_expand) noexcept
{
    std::size_t size;

#if defined(STDROMANO_WIN)
    HMODULE hm = nullptr;

    if(GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                         GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                         (LPCSTR)&expand_from_lib_dir,
                         &hm) == 0)
        return Error(StringD::make_fmt("Error caught during GetModuleHandleEx: {}", GetLastError()));

    char sz_path[MAX_PATH];

    if(GetModuleFileNameA(nullptr, sz_path, MAX_PATH) == 0)
        return Error(StringD::make_fmt("Error caught during GetModuleFileNameA: {}", GetLastError()));

    size = std::strlen(sz_path);

    while(size > 0 && sz_path[size] != '\\')
        size--;

#elif defined(STDROMANO_LINUX)
    char sz_path[PATH_MAX];
    ssize_t count = readlink("/proc/self/exe", sz_path, PATH_MAX);

    if(count < 0 || count >= PATH_MAX)
        return Error(StringD::make_fmt("Error during readlink: {}", errno));

    sz_path[count] = '\0';

    size = count - 1;

    while(size > 0 && sz_path[size] != '/')
        size--;

#endif /* defined(STDROMANO_WIN) */

    return StringD::make_fmt("{}/{}", fmt::string_view(sz_path, size), path_to_expand);
}

Expected<StringD> tmp_dir() noexcept
{
#if defined(STDROMANO_WIN)
    DWORD buf_sz = MAX_PATH + 1;
    StringD buf = StringD::make_zeroed(static_cast<std::size_t>(buf_sz));

    DWORD sz = GetTempPathA(buf_sz, buf.data());

    if(sz == 0 || sz > buf_sz)
        return Error();

    buf.erase(static_cast<std::size_t>(sz - static_cast<std::size_t>(buf[sz - 1] == '\\')));

    return buf;
#elif defined(STDROMANO_LINUX)
    return StringD::make_ref("/tmp");
#endif // defined(STDROMANO_WIN)
}

Expected<StringD> home_dir(bool use_env) noexcept
{
#if defined(STDROMANO_WIN)
    if(use_env)
    {
        const char* userprofile = std::getenv("USERPROFILE");

        if(userprofile != nullptr)
            return StringD::make_from_c_str(userprofile);
    }

    PWSTR ppsz_path;

    HRESULT hr = SHGetKnownFolderPath(FOLDERID_Profile,
                                      KF_FLAG_SIMPLE_IDLIST | KF_FLAG_DONT_VERIFY | KF_FLAG_NO_ALIAS,
                                      NULL,
                                      &ppsz_path);

    if(SUCCEEDED(hr))
    {
        std::size_t len = wcslen(ppsz_path);
        std::size_t ppsz_path_sz = WideCharToMultiByte(CP_UTF8,
                                                       0,
                                                       ppsz_path,
                                                       static_cast<int>(len),
                                                       nullptr,
                                                       0,
                                                       nullptr,
                                                       nullptr);

        StringD res = StringD::make_zeroed(ppsz_path_sz);

        WideCharToMultiByte(CP_UTF8,
                            0,
                            ppsz_path,
                            static_cast<int>(len),
                            res.c_str(),
                            static_cast<int>(res.size()),
                            nullptr,
                            nullptr);

        CoTaskMemFree(ppsz_path);

        return res;
    }
    else
    {
        return Error();
    }
#elif defined(STDROMANO_LINUX)
    const char* homedir;

    if((homedir = std::getenv("HOME")) == nullptr)
        homedir = getpwuid(getuid())->pw_dir;

    return StringD::make_from_c_str(homedir);
#endif // defined(STDROMANO_WIN)
}

Expected<StringD> load_file_content(const StringD& file_path,
                                    const char* mode) noexcept
{
    std::FILE* file_handle = std::fopen(file_path.c_str(), mode);

    if(file_handle == nullptr)
        return Error(StringD::make_fmt("Cannot open file {}", file_path));

    std::fseek(file_handle, 0, SEEK_END);
    const size_t file_size = std::ftell(file_handle);

    stdromano::StringD file_content = stdromano::StringD::make_zeroed(file_size);

    std::rewind(file_handle);
    std::fread(file_content.data(), sizeof(char), file_size, file_handle);
    std::fclose(file_handle);

    return file_content;
}

static constexpr std::size_t WRITE_BUFFER_SZ = 16384;

Expected<void> write_file_content(const char* data,
                                  const std::size_t data_sz,
                                  const StringD& file_path,
                                  const char* mode) noexcept
{
    const StringD parent = parent_dir(file_path);

    if(!path_exists(parent))
        if(!makedir(parent))
            return Error(StringD::make_fmt("Cannot create parent directory for file: {}", file_path));

    std::FILE* file_handle = std::fopen(file_path.c_str(), mode);

    if(file_handle == nullptr)
        return Error(StringD::make_fmt("Cannot open file: {}", file_path));

    std::size_t written = 0;

    while(written < data_sz)
    {
        const std::size_t to_write = std::min(WRITE_BUFFER_SZ, data_sz - written);
        const std::size_t w = std::fwrite(data + written, sizeof(char), to_write, file_handle);

        if(w != to_write)
            return Error(StringD::make_fmt("Error when writing to file: {}", file_path));

        written += to_write;
    }

    int err = std::ferror(file_handle);

    if(err != 0)
    {
        std::fclose(file_handle);
        return Error(StringD::make_fmt("Error with file {} ({})", file_path, err));
    }

    std::fclose(file_handle);

    return Ok();
}

ListDirIterator::~ListDirIterator()
{
#if defined(STDROMANO_WIN)
    if(this->_h_find != INVALID_HANDLE_VALUE)
        FindClose(this->_h_find);

#elif defined(STDROMANO_LINUX)
    if(this->_dir != nullptr)
        closedir(this->_dir);

#endif /* defined(STDROMANO_WIN) */
}

StringD ListDirIterator::get_current_path() const noexcept
{
#if defined(STDROMANO_WIN)
    return StringD("{}{}",
                   fmt::string_view(this->_directory_path.c_str(), this->_directory_path.size() - 1),
                   this->_find_data.cFileName);
#elif defined(STDROMANO_LINUX)
    return StringD("{}/{}",
                   fmt::string_view(this->_directory_path.c_str(), this->_directory_path.size()),
                   this->_entry->d_name);
#else
    return StringD();
#endif /* defined(STDROMANO_WIN) */
}

bool ListDirIterator::is_file() const noexcept
{
#if defined(STDROMANO_WIN)
    return this->_find_data.dwFileAttributes & ~FILE_ATTRIBUTE_DIRECTORY;
#elif defined(STDROMANO_LINUX)
    return this->_entry->d_type == DT_REG;
#else
    return false;
#endif /* defined(STDROMANO_WIN) */
}

bool ListDirIterator::is_directory() const noexcept
{
#if defined(STDROMANO_WIN)
    return this->_find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY;
#elif defined(STDROMANO_LINUX)
    return this->_entry->d_type == DT_DIR;
#else
    return false;
#endif /* defined(STDROMANO_WIN) */
}

bool list_dir(ListDirIterator& it, const StringD& directory_path, const std::uint32_t flags) noexcept
{
    if(!path_exists(directory_path))
        return false;

#if defined(STDROMANO_WIN)
    if(it._h_find == INVALID_HANDLE_VALUE)
    {
        it._directory_path = directory_path.copy();
        it._directory_path.appendc("\\*");
        it._h_find = FindFirstFileA(it._directory_path.c_str(), &it._find_data);

        if(it._h_find == INVALID_HANDLE_VALUE)
        {
            DWORD err = GetLastError();

            std::fprintf(stderr, "Error during fs_list_dir. Error code: %lu\n", err);

            return false;
        }

        if((it._find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
        {
            const std::size_t c_file_name_size = std::strlen(it._find_data.cFileName);

            if((it._find_data.cFileName[0] != '.' || c_file_name_size > 2) &&
               (flags & ListDirFlags_ListDirs))
            {
                return true;
            }
        }
        else if(flags & ListDirFlags_ListFiles)
        {
            return true;
        }
    }

    LoopGuard guard("Cannot find next file", 100000000);

    while(guard--)
    {
        if(FindNextFileA(it._h_find, &it._find_data))
        {
            if((it._find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
            {
                const std::size_t c_file_name_size = std::strlen(it._find_data.cFileName);

                if((it._find_data.cFileName[0] != '.' || c_file_name_size > 2) &&
                   (flags & ListDirFlags_ListDirs))
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
                std::fprintf(stderr, "Error during fs_list_dir. Error code: %lu", err);

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
        bool should_skip = (it._entry->d_name[0] == '.') && (it._entry->d_name[1] == '\0' ||
                           (it._entry->d_name[1] == '.' && it._entry->d_name[2] == '\0'));

        if(should_skip)
        {
            continue;
        }

        bool is_file = false;
        bool is_dir = false;

        if(it._entry->d_type == DT_UNKNOWN)
        {
            struct stat st;
            const StringD full_path = it.get_current_path();

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

        return (is_file && (flags & ListDirFlags_ListFiles)) ||
               (is_dir && (flags & ListDirFlags_ListDirs));
    }

    return false;

#endif /* defined(STDROMANO_WIN) */
}

StringD open_file_dialog(FileDialogMode_ mode,
                         const StringD& title,
                         const StringD& initial_path,
                         const StringD& filter) noexcept
{
#if defined(STDROMANO_WIN)
    if(mode == FileDialogMode_OpenFile || mode == FileDialogMode_SaveFile)
    {
        char filename[MAX_PATH];
        std::memset(filename, 0, sizeof(filename));

        OPENFILENAMEA ofn;
        std::memset(&ofn, 0, sizeof(OPENFILENAMEA));
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = NULL;
        ofn.lpstrFile = filename;
        ofn.nMaxFile = sizeof(filename);

        StringD windows_filter = filter.replace('|', '\0');
        windows_filter.push_back('\0');
        ofn.lpstrFilter = windows_filter.c_str();

        if(!initial_path.empty())
            ofn.lpstrInitialDir = initial_path.c_str();

        ofn.lpstrTitle = title.c_str();
        ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

        if(mode == FileDialogMode_SaveFile)
            ofn.Flags |= OFN_OVERWRITEPROMPT;

        BOOL result = GetOpenFileNameA(&ofn);

        if(result)
            return StringD(filename);

        return StringD();
    }
    else if(mode == FileDialogMode_OpenDir)
    {
        char dirname[MAX_PATH];
        std::memset(dirname, 0, sizeof(dirname));

        BROWSEINFOA bi;
        std::memset(&bi, 0, sizeof(BROWSEINFOA));
        bi.lpszTitle = title.c_str();
        bi.pszDisplayName = dirname;

        if(!initial_path.empty())
        {
            WCHAR* initial_path_wide =
                static_cast<WCHAR*>(mem_alloca((initial_path.size() + 1) * sizeof(WCHAR)));
            std::memset(initial_path_wide, 0, (initial_path.size() + 1) * sizeof(WCHAR));
            MultiByteToWideChar(CP_UTF8,
                                0,
                                initial_path.c_str(),
                                static_cast<int>(initial_path.size()),
                                initial_path_wide,
                                static_cast<int>(initial_path.size()));

            PIDLIST_ABSOLUTE pidl = NULL;
            ULONG sfgao_out = 0;

            HRESULT hr = SHParseDisplayName(initial_path_wide, NULL, &pidl, 0, &sfgao_out);

            if(SUCCEEDED(hr))
                bi.pidlRoot = pidl;
        }

        LPITEMIDLIST pidl = SHBrowseForFolderA(&bi);
        StringD dirpath = StringD::make_zeroed(MAX_PATH);

        if(pidl != NULL)
            if(!SHGetPathFromIDListA((PCIDLIST_ABSOLUTE)pidl, dirpath.c_str()))
                dirpath = StringD();

        IMalloc* p_malloc = NULL;
        if(SUCCEEDED(SHGetMalloc(&p_malloc)))
        {
            p_malloc->Free(pidl);
            p_malloc->Release();
        }

        return dirpath;
    }

    return StringD();
#elif defined(STDROMANO_LINUX)
    gtk_init(nullptr, nullptr);

    GtkWidget* dialog = nullptr;
    StringD result;

    switch(mode)
    {
        case FileDialogMode_OpenFile:
            dialog = gtk_file_chooser_dialog_new(title.c_str(),
                                                 nullptr,
                                                 GTK_FILE_CHOOSER_ACTION_OPEN,
                                                 "_Cancel",
                                                 GTK_RESPONSE_CANCEL,
                                                 "_Open",
                                                 GTK_RESPONSE_ACCEPT,
                                                 nullptr);
            break;
        case FileDialogMode_SaveFile:
            dialog = gtk_file_chooser_dialog_new(title.c_str(),
                                                 nullptr,
                                                 GTK_FILE_CHOOSER_ACTION_SAVE,
                                                 "_Cancel",
                                                 GTK_RESPONSE_CANCEL,
                                                 "_Save",
                                                 GTK_RESPONSE_ACCEPT,
                                                 nullptr);
            gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(dialog), TRUE);
            break;
        case FileDialogMode_OpenDir:
            dialog = gtk_file_chooser_dialog_new(title.c_str(),
                                                 nullptr,
                                                 GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
                                                 "_Cancel",
                                                 GTK_RESPONSE_CANCEL,
                                                 "_Select",
                                                 GTK_RESPONSE_ACCEPT,
                                                 nullptr);
            break;
    }

    if(!initial_path.empty())
        gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), initial_path.c_str());

    if(!filter.empty())
    {
        GtkFileFilter* gtkFilter = gtk_file_filter_new();
        gtk_file_filter_set_name(gtkFilter, "Specified Files");

        StringD::split_iterator it;
        StringD token;

        while(filter.split("|", it, token))
            gtk_file_filter_add_pattern(gtkFilter, token.c_str());

        gtk_file_filter_add_pattern(gtkFilter, filter.c_str());

        gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), gtkFilter);
    }

    gint res = gtk_dialog_run(GTK_DIALOG(dialog));

    if(res == GTK_RESPONSE_ACCEPT)
    {
        char* filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        result = filename;
        g_free(filename);
    }

    gtk_widget_destroy(dialog);

    while(gtk_events_pending())
        gtk_main_iteration();

    return result;
#endif // defined(STDROMANO_WIN)
}

bool WalkIterator::process_current_directory() noexcept
{
#if defined(STDROMANO_WIN)
    WIN32_FIND_DATAA find_data;

    bool first_entry = (this->_h_find == INVALID_HANDLE_VALUE);

    if(first_entry)
    {
        if(this->_pending_dirs.empty())
        {
            this->_is_end = true;
            return false;
        }

        this->_current_dir = this->_pending_dirs.front();
        this->_pending_dirs.pop();
        StringD search_path("{}\\*", this->_current_dir);

        this->_h_find = FindFirstFileA(search_path.c_str(), &find_data);

        if(this->_h_find == INVALID_HANDLE_VALUE)
            return false;
    }
    else
    {
        if(!FindNextFileA(this->_h_find, &find_data))
        {
            DWORD last_err = GetLastError();

            FindClose(this->_h_find);
            this->_h_find = INVALID_HANDLE_VALUE;

            if(last_err != ERROR_NO_MORE_FILES)
                this->_is_end = true;

            return false;
        }
    }

    LoopGuard guard("Cannot find next file", 100000000);

    while(guard--)
    {
        if(this->should_skip_entry(find_data.cFileName, find_data.dwFileAttributes))
        {
            if(!FindNextFileA(this->_h_find, &find_data))
                break;

            continue;
        }

        StringD full_path("{}/{}", this->_current_dir, find_data.cFileName);

        bool is_dir = (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;

        if(is_dir && this->_flags & WalkFlags_Recursive)
            this->_pending_dirs.push(std::move(full_path).copy());

        if((is_dir && (this->_flags & WalkFlags_ListDirs)) ||
           (!is_dir && (this->_flags & WalkFlags_ListFiles)))
        {
            this->_current_item = { std::move(full_path), is_dir };
            return true;
        }

        if(!FindNextFileA(this->_h_find, &find_data))
            break;
    }

    FindClose(this->_h_find);
    this->_h_find = INVALID_HANDLE_VALUE;

#elif defined(STDROMANO_LINUX)
    if(this->_dir == nullptr)
    {
        if(this->_pending_dirs.empty())
        {
            this->_is_end = true;
            return false;
        }

        this->_current_dir = this->_pending_dirs.front();
        this->_pending_dirs.pop();

        this->_dir = opendir(this->_current_dir.c_str());
    }

    struct dirent* entry;

    while((entry = readdir(this->_dir)))
    {
        bool is_hidden = entry->d_name[0] == '.';

        if(is_hidden && !(this->_flags & WalkFlags_ListHidden))
            continue;

        bool is_dir = false;

        StringD full_path("{}/{}", this->_current_dir, entry->d_name);

        if(entry->d_type == DT_UNKNOWN)
        {
            struct stat st;

            if(stat(full_path.c_str(), &st) == 0)
                is_dir = S_ISDIR(st.st_mode);
        }
        else if(entry->d_type == DT_DIR)
        {
            is_dir = true;

            if(this->_flags & WalkFlags_Recursive)
                this->_pending_dirs.push(std::move(full_path.copy()));
        }

        if((is_dir && (this->_flags & WalkFlags_ListFiles)) ||
           (!is_dir && (this->_flags & WalkFlags_ListDirs)))
        {
            this->_current_item = { std::move(full_path), is_dir };
            return true;
        }
    }

    closedir(this->_dir);
    this->_dir = nullptr;

#else
    STDROMANO_NOT_IMPLEMENTED;
#endif // defined(STDROMANO_WIN)

    return false;
}

void WalkIterator::move_to_next_directory() noexcept
{
    if(this->_pending_dirs.empty())
        this->_is_end = true;
}

bool WalkIterator::should_skip_entry(const char* name
#if defined(STDROMANO_WIN)
                                    , DWORD attrs) const noexcept
#else
                                     ) const noexcept
#endif // defined(STDROMANO_WIN)
{
    if(name[0] == '.' && (name[1] == '\0' || (name[1] == '.' && name[2] == '\0')))
        return true;

#if defined(STDROMANO_WIN)
    if((attrs & FILE_ATTRIBUTE_HIDDEN) && !(this->_flags & ListDirFlags_ListHidden))
        return true;
#endif // defined(STDROMANO_WIN)

    return false;
}

FS_NAMESPACE_END

STDROMANO_NAMESPACE_END
