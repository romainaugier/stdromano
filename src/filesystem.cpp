// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - Present Romain Augier
// All rights reserved.

#include "stdromano/filesystem.h"

#if defined(STDROMANO_WIN)
#include "ShlObj_core.h"
#include "windows.h"
#elif defined(STDROMANO_LINUX)
#include <limits.h>
#include <pwd.h>
#include <sys/types.h>
#include <unistd.h>
#endif // defined(STDROMANO_WIN)

STDROMANO_NAMESPACE_BEGIN

stdromano::String<> expand_from_executable_dir(const stdromano::String<>& path_to_expand) noexcept
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
#endif

    return stdromano::String<>("{}/{}", fmt::string_view(sz_path, size), path_to_expand);
}

STDROMANO_NAMESPACE_END