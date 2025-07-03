// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - Present Romain Augier
// All rights reserved.

#include "stdromano/filesystem.h"
#include "stdromano/logger.h"

#if defined(STDROMANO_WIN)
#define STRICT_TYPED_ITEMIDS // Better type safety for IDLists
#include "ShlObj_core.h"
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

StringD fs_current_dir() noexcept
{
    StringD res = StringD::make_zeroed(512);

#if defined(STDROMANO_WIN)
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
    getcwd(res.c_str(), res.size());

    return res;
#else
    STDROMANO_NOT_IMPLEMENTED;
#endif /* defined(STDROMANO_WIN) */
}

void fs_mkdir(const StringD& dir_path) noexcept
{
    if(fs_path_exists(dir_path))
    {
        return;
    }

#if defined(STDROMANO_WIN)
    CreateDirectoryA(dir_path.is_ref() ? dir_path.copy().c_str() : dir_path.c_str(), NULL);
#elif defined(STDROMANO_LINUX)
    mkdir(dir_path.is_ref() ? dir_path.copy().c_str() : dir_path.c_str(), 0755);
#else
    STDROMANO_NOT_IMPLEMENTED;
#endif /* defined(STDROMANO_WIN) */
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

    return String<>("{}/{}", fmt::string_view(sz_path, size), path_to_expand);
}

String<> load_file_content(const String<>& file_path, const char* mode) noexcept
{
    std::FILE* file_handle;

    file_handle = std::fopen(file_path.c_str(), mode);

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

    return file_content;
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
    return String<>(
        "{}{}",
        fmt::string_view(this->_directory_path.c_str(), this->_directory_path.size() - 1),
        this->_find_data.cFileName);
#elif defined(STDROMANO_LINUX)
    return String<>("{}/{}",
                    fmt::string_view(this->_directory_path.c_str(), this->_directory_path.size()),
                    this->_entry->d_name);
#else
    return String<>();
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

            std::fprintf(stderr, "Error during fs_list_dir. Error code: %u\n", err);

            return false;
        }

        if((it._find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
        {
            const size_t c_file_name_size = std::strlen(it._find_data.cFileName);

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

    while(true)
    {
        if(FindNextFileA(it._h_find, &it._find_data))
        {
            if((it._find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
            {
                const size_t c_file_name_size = std::strlen(it._find_data.cFileName);

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
            const String<> full_path = it.get_current_path();

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

String<> open_file_dialog(FileDialogMode_ mode,
                          const String<>& title,
                          const String<>& initial_path,
                          const String<>& filter) noexcept
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

        String<> windows_filter = filter.replace('|', '\0');
        windows_filter.push_back('\0');
        ofn.lpstrFilter = windows_filter.c_str();

        if(!initial_path.empty())
        {
            ofn.lpstrInitialDir = initial_path.c_str();
        }

        ofn.lpstrTitle = title.c_str();
        ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

        if(mode == FileDialogMode_SaveFile)
        {
            ofn.Flags |= OFN_OVERWRITEPROMPT;
        }

        BOOL result = GetOpenFileNameA(&ofn);

        if(result)
        {
            return std::move(String<>(filename));
        }

        return String<>();
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
            {
                bi.pidlRoot = pidl;
            }
        }

        LPITEMIDLIST pidl = SHBrowseForFolderA(&bi);
        String<> dirpath = String<>::make_zeroed(MAX_PATH);

        if(pidl != NULL)
        {
            if(!SHGetPathFromIDListA((PCIDLIST_ABSOLUTE)pidl, dirpath.c_str()))
            {
                dirpath = String<>();
            }
        }

        IMalloc* p_malloc = NULL;
        if(SUCCEEDED(SHGetMalloc(&p_malloc)))
        {
            p_malloc->Free(pidl);
            p_malloc->Release();
        }

        return dirpath;
    }

    return String<>();
#elif defined(STDROMANO_LINUX)
    gtk_init(nullptr, nullptr);

    GtkWidget* dialog = nullptr;
    String<> result;

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
    {
        gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), initial_path.c_str());
    }

    if(!filter.empty())
    {
        GtkFileFilter* gtkFilter = gtk_file_filter_new();
        gtk_file_filter_set_name(gtkFilter, "Specified Files");

        String<>::split_iterator it;
        String<> token;

        while(filter.split("|", it, token))
        {
            gtk_file_filter_add_pattern(gtkFilter, token.c_str());
        }

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
    {
        gtk_main_iteration();
    }

    return result;
#endif /* defined(STDROMANO_WIN) */
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
        {
            return false;
        }
    } 
    else 
    {
        if(!FindNextFileA(this->_h_find, &find_data)) 
        {
            return false;
        }
    }

    while(true) 
    {
        if(this->should_skip_entry(find_data.cFileName, find_data.dwFileAttributes)) 
        {
            if(!FindNextFileA(this->_h_find, &find_data))
            {
                break;
            }

            continue;
        }

        StringD full_path("{}/{}", this->_current_dir, find_data.cFileName);

        bool is_dir = (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;

        if(is_dir) 
        {
            this->_pending_dirs.push(full_path);
        }

        if((is_dir && (this->_flags & ListDirFlags_ListDirs)) ||
           (!is_dir && (this->_flags & ListDirFlags_ListFiles))) 
        {
            this->_current_item = { std::move(full_path), is_dir };
            return true;
        }

        if (!FindNextFileA(this->_h_find, &find_data)) break;
    }

    FindClose(this->_h_find);
    this->_h_find = INVALID_HANDLE_VALUE;

#elif defined(STDROMANO_LINUX)
    STDROMANO_NOT_IMPLEMENTED;
#else
    STDROMANO_NOT_IMPLEMENTED;
#endif /* defined(STDROMANO_WIN) */

    return false;
}

void WalkIterator::move_to_next_directory() noexcept
{
    if(this->_pending_dirs.empty()) 
    {
        this->_is_end = true;
    }
}

bool WalkIterator::should_skip_entry(const char* name
#if defined(STDROMANO_WIN)
                                    , DWORD attrs) const noexcept
#else
                                     ) const noexcept
#endif /* defined(STDROMANO_WIN) */
{
    if(name[0] == '.' && (name[1] == '\0' || (name[1] == '.' && name[2] == '\0'))) 
    {
        return true;
    }

#if defined(STDROMANO_WIN)
    if((attrs & FILE_ATTRIBUTE_HIDDEN) && !(this->_flags & ListDirFlags_ListHidden))
    {
        return true;
    }
#endif /* defined(STDROMANO_WIN) */

    return false;
}

STDROMANO_NAMESPACE_END