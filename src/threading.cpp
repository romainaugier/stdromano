// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - Present Romain Augier
// All rights reserved.

#include "stdromano/threading.hpp"

STDROMANO_NAMESPACE_BEGIN

ThreadPool& ThreadPool::get_global_threadpool() noexcept
{
    static ThreadPool g_threadpool;

    return g_threadpool;
}

STDROMANO_NAMESPACE_END