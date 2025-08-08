// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - Present Romain Augier
// All rights reserved.

#include "stdromano/linalg/backend.hpp"

#include <atomic>

STDROMANO_NAMESPACE_BEGIN

static std::atomic<std::uint32_t> g_backend = LinAlgBackend_CPU;

std::uint32_t get_default_backend() noexcept
{
    return g_backend.load();
}

void set_default_backend(std::uint32_t backend) noexcept
{
    g_backend.store(backend);
}

STDROMANO_NAMESPACE_END