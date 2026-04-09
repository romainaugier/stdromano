// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - Present Romain Augier
// All rights reserved.

#pragma once

#if !defined(__STDROMANO_ALIASES)
#define __STDROMANO_ALIASES

#include "stdromano/hashmap.hpp"
#include "stdromano/string.hpp"

STDROMANO_NAMESPACE_BEGIN

using StringHashMap = HashMap<StringD, StringD>;

STDROMANO_NAMESPACE_END

#endif // !defined(__STDROMANO_ALIASES)
