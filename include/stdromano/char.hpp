// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - Present Romain Augier
// All rights reserved.

#pragma once

#if !defined(__STDROMANO_CHAR)
#define __STDROMANO_CHAR

#include "stdromano/stdromano.hpp"

STDROMANO_NAMESPACE_BEGIN

constexpr STDROMANO_FORCE_INLINE bool is_digit(unsigned int c) noexcept
{
    return (c - 48) < 10;
}

constexpr STDROMANO_FORCE_INLINE bool is_letter(unsigned int c) noexcept
{
    return ((c & ~0x20) - 65) < 26;
}

constexpr STDROMANO_FORCE_INLINE bool is_alnum(unsigned int c) noexcept
{
    return ((c - 48) < 10) | (((c & ~0x20) - 65) < 26);
}

constexpr STDROMANO_FORCE_INLINE bool is_letter_upper(unsigned int c) noexcept
{
    return (c - 65) < 26;
}

constexpr STDROMANO_FORCE_INLINE bool is_letter_lower(unsigned int c) noexcept
{
    return (c - 97) < 26;
}

constexpr STDROMANO_FORCE_INLINE char to_lower(unsigned int c) noexcept
{
    if(is_letter(c))
    {
        return static_cast<char>(c | 0x60);
    }

    return c;
}

constexpr STDROMANO_FORCE_INLINE char to_upper(unsigned int c) noexcept
{
    if(is_letter(c))
    {
        return static_cast<char>(c & ~0x20);
    }

    return c;
}

STDROMANO_NAMESPACE_END

#endif /* !defined(__STDROMANO_CHAR) */
