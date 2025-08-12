// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - Present Romain Augier
// All rights reserved.

#pragma once

#if !defined(__STDROMANO_REGEX)
#define __STDROMANO_REGEX

#include "stdromano/vector.hpp"
#include "stdromano/string.hpp"

STDROMANO_NAMESPACE_BEGIN

enum RegexFlags_ : std::uint32_t
{
    RegexFlags_DebugCompilation = 0x1,
};

class STDROMANO_API Regex
{
public:
    using ByteCode = Vector<std::byte>;

private:
    ByteCode _bytecode;

    bool compile(const StringD& regex, std::uint32_t flags) noexcept;

public:
    Regex(const StringD& regex, std::uint32_t flags = 0)
    {
        this->compile(regex, flags);
    }

    bool match(const StringD& str) const noexcept;
};

STDROMANO_NAMESPACE_END

#endif /* !defined(__STDROMANO_REGEX) */