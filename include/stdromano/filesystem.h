// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - Present Romain Augier
// All rights reserved.

#pragma once

#if !defined(__STDROMANO_FILESYSTEM)
#define __STDROMANO_FILESYSTEM

#include "stdromano/string.h"

STDROMANO_NAMESPACE_BEGIN

STDROMANO_API stdromano::String<> expand_from_executable_dir(const stdromano::String<>& path_to_expand) noexcept;

STDROMANO_API stdromano::String<> load_file_content(const char* file_path) noexcept;

STDROMANO_NAMESPACE_END

#endif // !defined(__STDROMANO_FILESYSTEM)