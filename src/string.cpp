// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - Present Romain Augier
// All rights reserved.

#include "stdromano/string.h"

STDROMANO_NAMESPACE_BEGIN

char* strrstr(const char* haystack, const char* needle) noexcept
{
    if(*needle == '\0')
    {
        return const_cast<char*>(haystack);
    }

    char* s = const_cast<char*>(haystack);

    char* result = nullptr;

    while(true)
    {
        char* p = const_cast<char*>(std::strstr(s, needle));

        if(p == nullptr)
        {
            break;
        }

        result = p;
        s = p + 1;
    }

    return result;
}

STDROMANO_NAMESPACE_END