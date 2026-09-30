// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - Present Romain Augier
// All rights reserved.

#include "stdromano/random.hpp"

#if defined(STDROMANO_WIN)
#include <Windows.h>
#include <bcrypt.h>
#elif defined(STDROMANO_LINUX)
#include <sys/random.h>
#elif defined(STDROMANO_UNIX)
#include <stdlib.h>
#endif // defined(STDROMANO_WIN)

#include <limits>

STDROMANO_NAMESPACE_BEGIN

template <typename T>
static T fill_random_seed() noexcept
{
    T value;

#if defined(STDROMANO_WIN)
    NTSTATUS status = BCryptGenRandom(nullptr,
                                      reinterpret_cast<PUCHAR>(&value),
                                      sizeof(value),
                                      BCRYPT_USE_SYSTEM_PREFERRED_RNG);

    if(!BCRYPT_SUCCESS(status))
        return std::numeric_limits<T>::max();

#elif defined(STDROMANO_LINUX)
    ssize_t res = getrandom(&value, sizeof(value), 0);

    if(res != sizeof(value))
        return std::numeric_limits<T>::max();

#elif defined(STDROMANO_UNIX)
    /* arc4random_buf is available on macOS and on every bsd, and cannot fail */
    arc4random_buf(&value, sizeof(value));

#else
#error "random_seed not implemented on this platform"
#endif // defined(STDROMANO_WIN)

    return value;
}

std::uint32_t random_seed_u32() noexcept
{
    return fill_random_seed<std::uint32_t>();
}

std::uint64_t random_seed_u64() noexcept
{
    return fill_random_seed<std::uint64_t>();
}

static thread_local std::uint64_t ts_splitmix_state = random_seed_u64();

static thread_local std::uint32_t ts_wang_state = random_seed_u32();

static thread_local std::uint64_t ts_pcg_state = pcg_seed(random_seed_u64());

static thread_local XoshiroState ts_xoshiro_state = xoshiro_seed(random_seed_u64());

std::uint32_t ts_next_random_u32() noexcept
{
    return next_random_u32(ts_splitmix_state);
}

std::uint64_t ts_next_random_u64() noexcept
{
    return next_random_u64(ts_splitmix_state);
}

float ts_next_random_float() noexcept
{
    return next_random_float(ts_splitmix_state);
}

double ts_next_random_double() noexcept
{
    return next_random_double(ts_splitmix_state);
}

std::uint32_t ts_next_random_u32_in_range(const std::uint32_t low, const std::uint32_t high) noexcept
{
    return next_random_u32_in_range(ts_splitmix_state, low, high);
}

std::uint64_t ts_next_random_u64_in_range(const std::uint64_t low, const std::uint64_t high) noexcept
{
    return next_random_u64_in_range(ts_splitmix_state, low, high);
}

float ts_next_random_float_in_range(const float low, const float high) noexcept
{
    return next_random_float_in_range(ts_splitmix_state, low, high);
}

double ts_next_random_double_in_range(const double low, const double high) noexcept
{
    return next_random_double_in_range(ts_splitmix_state, low, high);
}

std::uint32_t ts_wang_next_random_u32() noexcept
{
    return wang_next_random_u32(ts_wang_state);
}

std::uint64_t ts_wang_next_random_u64() noexcept
{
    return wang_next_random_u64(ts_wang_state);
}

float ts_wang_next_random_float() noexcept
{
    return wang_next_random_float(ts_wang_state);
}

double ts_wang_next_random_double() noexcept
{
    return wang_next_random_double(ts_wang_state);
}

std::uint32_t ts_wang_next_random_u32_in_range(const std::uint32_t low, const std::uint32_t high) noexcept
{
    return wang_next_random_u32_in_range(ts_wang_state, low, high);
}

std::uint64_t ts_wang_next_random_u64_in_range(const std::uint64_t low, const std::uint64_t high) noexcept
{
    return wang_next_random_u64_in_range(ts_wang_state, low, high);
}

float ts_wang_next_random_float_in_range(const float low, const float high) noexcept
{
    return wang_next_random_float_in_range(ts_wang_state, low, high);
}

double ts_wang_next_random_double_in_range(const double low, const double high) noexcept
{
    return wang_next_random_double_in_range(ts_wang_state, low, high);
}

std::uint32_t ts_pcg_next_random_u32() noexcept
{
    return pcg_next_random_u32(ts_pcg_state);
}

std::uint64_t ts_pcg_next_random_u64() noexcept
{
    return pcg_next_random_u64(ts_pcg_state);
}

float ts_pcg_next_random_float() noexcept
{
    return pcg_next_random_float(ts_pcg_state);
}

double ts_pcg_next_random_double() noexcept
{
    return pcg_next_random_double(ts_pcg_state);
}

std::uint32_t ts_pcg_next_random_u32_in_range(const std::uint32_t low, const std::uint32_t high) noexcept
{
    return pcg_next_random_u32_in_range(ts_pcg_state, low, high);
}

std::uint64_t ts_pcg_next_random_u64_in_range(const std::uint64_t low, const std::uint64_t high) noexcept
{
    return pcg_next_random_u64_in_range(ts_pcg_state, low, high);
}

float ts_pcg_next_random_float_in_range(const float low, const float high) noexcept
{
    return pcg_next_random_float_in_range(ts_pcg_state, low, high);
}

double ts_pcg_next_random_double_in_range(const double low, const double high) noexcept
{
    return pcg_next_random_double_in_range(ts_pcg_state, low, high);
}

std::uint32_t ts_xoshiro_next_random_u32() noexcept
{
    return xoshiro_next_random_u32(ts_xoshiro_state);
}

std::uint64_t ts_xoshiro_next_random_u64() noexcept
{
    return xoshiro_next_random_u64(ts_xoshiro_state);
}

float ts_xoshiro_next_random_float() noexcept
{
    return xoshiro_next_random_float(ts_xoshiro_state);
}

double ts_xoshiro_next_random_double() noexcept
{
    return xoshiro_next_random_double(ts_xoshiro_state);
}

std::uint32_t ts_xoshiro_next_random_u32_in_range(const std::uint32_t low, const std::uint32_t high) noexcept
{
    return xoshiro_next_random_u32_in_range(ts_xoshiro_state, low, high);
}

std::uint64_t ts_xoshiro_next_random_u64_in_range(const std::uint64_t low, const std::uint64_t high) noexcept
{
    return xoshiro_next_random_u64_in_range(ts_xoshiro_state, low, high);
}

float ts_xoshiro_next_random_float_in_range(const float low, const float high) noexcept
{
    return xoshiro_next_random_float_in_range(ts_xoshiro_state, low, high);
}

double ts_xoshiro_next_random_double_in_range(const double low, const double high) noexcept
{
    return xoshiro_next_random_double_in_range(ts_xoshiro_state, low, high);
}

STDROMANO_NAMESPACE_END