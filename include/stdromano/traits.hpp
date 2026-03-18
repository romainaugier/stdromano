// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - Present Romain Augier
// All rights reserved.

#pragma once

#if !defined(__STDROMANO_TRAITS)
#define __STDROMANO_TRAITS

#include "stdromano/stdromano.hpp"

#include <iterator>

STDROMANO_NAMESPACE_BEGIN

template<class It>
using is_forward_iterator = std::is_base_of<std::forward_iterator_tag,
                                            typename std::iterator_traits<It>::iterator_category>;

STDROMANO_NAMESPACE_END

#endif /* !defined(__STDROMANO_TRAITS) */
