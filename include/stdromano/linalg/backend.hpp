// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - Present Romain Augier
// All rights reserved.

#pragma once

#if !defined(__STDROMANO_LINALG_BACKEND)
#define __STDROMANO_LINALG_BACKEND

#include "stdromano/stdromano.hpp"

STDROMANO_NAMESPACE_BEGIN

enum LinAlgBackend : std::uint32_t
{
    LinAlgBackend_CPU,
    LinAlgBackend_GPU,
};

STDROMANO_API std::uint32_t get_default_backend() noexcept;

STDROMANO_API void set_default_backend(std::uint32_t backend) noexcept;

STDROMANO_NAMESPACE_END

#endif /* !defined(__STDROMANO_LINALG_BACKEND) */