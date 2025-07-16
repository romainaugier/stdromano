// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - Present Romain Augier
// All rights reserved.

#include "stdromano/string.hpp"

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

bool case_insensitive_less(char lhs, char rhs) noexcept
{
    return to_lower(static_cast<int>(lhs)) < to_lower(static_cast<int>(rhs));
}

bool case_sensitive_less(char lhs, char rhs) noexcept
{
    return lhs < rhs;
}

int strcmp(const StringD& lhs, const StringD& rhs, bool case_sensitive) noexcept
{
    bool ret = std::lexicographical_compare(lhs.begin(), 
                                            lhs.end(),
                                            rhs.begin(),
                                            rhs.end(),
                                            case_sensitive ? case_sensitive_less : 
                                                             case_insensitive_less);

    return ret ? -1 : 1;
}

STDROMANO_NAMESPACE_END