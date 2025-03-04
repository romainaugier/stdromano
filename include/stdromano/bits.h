// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - Present Romain Augier
// All rights reserved.

#pragma once

#if !defined(__STDROMANO_BITS)
#define __STDROMANO_BITS

#include "stdromano/stdromano.h"

STDROMANO_NAMESPACE_BEGIN

static STDROMANO_FORCE_INLINE size_t bit_ceil(size_t n)
{
    if(n <= 1)
    {
        return 1;
    }

    size_t power = 2;
    n--;

    while(n >>= 1)
    {
        power <<= 1;
    }

    return power;
}

STDROMANO_NAMESPACE_END

#endif // !defined(__STDROMANO_BITS)