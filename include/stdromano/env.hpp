// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - Present Romain Augier
// All rights reserved.

#pragma once

#if !defined(__STDROMANO_ENV)
#define __STDROMANO_ENV

#include "stdromano/string.hpp"

#if defined(STDROMANO_WIN)
#include <windows.h>
#else
#include <cstdlib>
#include <wordexp.h>
#endif

STDROMANO_NAMESPACE_BEGIN

namespace env {

#if defined(STDROMANO_WIN)
static constexpr char PATH_SEP = ';';
#else
static constexpr char PATH_SEP = ':';
#endif

STDROMANO_FORCE_INLINE StringD get(const char* name) noexcept
{
    if(name == nullptr)
        return StringD();

#if defined(STDROMANO_WIN)
    const DWORD required = GetEnvironmentVariableA(name, nullptr, 0);

    if(required == 0)
        return StringD();

    StringD result = StringD::make_zeroed(static_cast<std::size_t>(required - 1));

    const DWORD written = GetEnvironmentVariableA(name, result.data(), required);

    if(written == 0 || written >= required)
        return StringD();

    return result;
#else
    const char* value = std::getenv(name);

    if(value == nullptr)
        return StringD();

    return StringD(value);
#endif
}

STDROMANO_FORCE_INLINE StringD get(const StringD& name) noexcept
{
    return env::get(name.c_str());
}

STDROMANO_FORCE_INLINE bool set(const char* name, const char* value, bool overwrite = true) noexcept
{
    if(name == nullptr || value == nullptr)
        return false;

#if defined(STDROMANO_WIN)
    if(!overwrite)
    {
        const DWORD existing = GetEnvironmentVariableA(name, nullptr, 0);

        if(existing > 0)
            return true;
    }

    return SetEnvironmentVariableA(name, value) != 0;
#else
    return setenv(name, value, overwrite ? 1 : 0) == 0;
#endif
}

STDROMANO_FORCE_INLINE bool set(const StringD& name, const StringD& value, bool overwrite = true) noexcept
{
    return env::set(name.c_str(), value.c_str(), overwrite);
}

STDROMANO_FORCE_INLINE bool unset(const char* name) noexcept
{
    if(name == nullptr)
        return false;

#if defined(STDROMANO_WIN)
    return SetEnvironmentVariableA(name, nullptr) != 0;
#else
    return unsetenv(name) == 0;
#endif
}

STDROMANO_FORCE_INLINE bool unset(const StringD& name) noexcept
{
    return env::unset(name.c_str());
}

STDROMANO_FORCE_INLINE bool has(const char* name) noexcept
{
    if(name == nullptr)
        return false;

#if defined(STDROMANO_WIN)
    return GetEnvironmentVariableA(name, nullptr, 0) > 0;
#else
    return std::getenv(name) != nullptr;
#endif
}

STDROMANO_FORCE_INLINE bool has(const StringD& name) noexcept
{
    return env::has(name.c_str());
}

/* Append a value to an existing variable, using the given separator */
STDROMANO_FORCE_INLINE bool append(const char* name, const char* value, char sep = PATH_SEP) noexcept
{
    if(name == nullptr || value == nullptr)
        return false;

    StringD current = env::get(name);

    if(current.empty())
        return env::set(name, value);

    current.push_back(sep);
    current.appendc(value);

    return env::set(name, current.c_str());
}

STDROMANO_FORCE_INLINE bool append(const StringD& name, const StringD& value, char sep = PATH_SEP) noexcept
{
    return env::append(name.c_str(), value.c_str(), sep);
}

/* Prepend a value to an existing variable, using the given separator */
STDROMANO_FORCE_INLINE bool prepend(const char* name, const char* value, char sep = PATH_SEP) noexcept
{
    if(name == nullptr || value == nullptr)
        return false;

    StringD current = env::get(name);

    if(current.empty())
        return env::set(name, value);

    StringD new_value(value);
    new_value.push_back(sep);
    new_value.appends(current);

    return env::set(name, new_value.c_str());
}

STDROMANO_FORCE_INLINE bool prepend(const StringD& name, const StringD& value, char sep = PATH_SEP) noexcept
{
    return env::prepend(name.c_str(), value.c_str(), sep);
}

/* Expand environment variables in a string.
   Windows: %VAR% syntax (via ExpandEnvironmentStringsA)
   Linux:   $VAR / ${VAR} syntax (via wordexp) */
STDROMANO_FORCE_INLINE StringD expand(const char* str) noexcept
{
    if(str == nullptr)
        return StringD();

#if defined(STDROMANO_WIN)
    const DWORD required = ExpandEnvironmentStringsA(str, nullptr, 0);

    if(required == 0)
        return StringD(str);

    // Size is buffer sz + null + 1 so we subtract 2
    // https://learn.microsoft.com/en-us/windows/win32/api/processenv/nf-processenv-expandenvironmentstringsa
    StringD result = StringD::make_zeroed(static_cast<std::size_t>(required - 2));

    const DWORD written = ExpandEnvironmentStringsA(str, result.data(), required);

    if(written == 0 || written > required)
        return StringD(str);

    return result;
#else
    wordexp_t wx;

    const int ret = wordexp(str, &wx, WRDE_NOCMD | WRDE_UNDEF);

    if(ret != 0)
    {
        if(ret != WRDE_NOSPACE)
            wordfree(&wx);

        return StringD(str);
    }

    StringD result;

    for(std::size_t i = 0; i < wx.we_wordc; i++)
    {
        if(i > 0)
            result.push_back(' ');

        result.appendc(wx.we_wordv[i]);
    }

    wordfree(&wx);

    return result;
#endif
}

STDROMANO_FORCE_INLINE StringD expand(const StringD& str) noexcept
{
    return env::expand(str.c_str());
}

} // namespace env

STDROMANO_NAMESPACE_END

#endif // !defined(__STDROMANO_ENV)
