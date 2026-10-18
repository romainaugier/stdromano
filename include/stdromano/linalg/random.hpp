// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - Present Romain Augier
// All rights reserved.

#pragma once

#if !defined(__STDROMANO_LINALG_RANDOM)
#define __STDROMANO_LINALG_RANDOM

#include "stdromano/linalg/vector.hpp"
#include "stdromano/random.hpp"

STDROMANO_NAMESPACE_BEGIN

/*
    Multidimensional hashes from Jarzynski and Olano, "Hash Functions for GPU Rendering" (JCGT 2020),
    each output component depends on every input component. The _float variants map to [0, 1)
*/

STDROMANO_HOST_DEVICE STDROMANO_FORCE_INLINE Vec2U pcg2d_hash(Vec2U v) noexcept
{
    v = v * 1664525U + 1013904223U;

    v.x += v.y * 1664525U;
    v.y += v.x * 1664525U;

    v ^= v >> 16U;

    v.x += v.y * 1664525U;
    v.y += v.x * 1664525U;

    v ^= v >> 16U;

    return v;
}

STDROMANO_HOST_DEVICE STDROMANO_FORCE_INLINE Vec3U pcg3d_hash(Vec3U v) noexcept
{
    v = v * 1664525U + 1013904223U;

    v.x += v.y * v.z;
    v.y += v.z * v.x;
    v.z += v.x * v.y;

    v ^= v >> 16U;

    v.x += v.y * v.z;
    v.y += v.z * v.x;
    v.z += v.x * v.y;

    return v;
}

STDROMANO_HOST_DEVICE STDROMANO_FORCE_INLINE Vec4U pcg4d_hash(Vec4U v) noexcept
{
    v = v * 1664525U + 1013904223U;

    v.x += v.y * v.w;
    v.y += v.z * v.x;
    v.z += v.x * v.y;
    v.w += v.y * v.z;

    v ^= v >> 16U;

    v.x += v.y * v.w;
    v.y += v.z * v.x;
    v.z += v.x * v.y;
    v.w += v.y * v.z;

    return v;
}

STDROMANO_HOST_DEVICE STDROMANO_FORCE_INLINE Vec2F pcg2d_hash_float(const Vec2U v) noexcept
{
    const Vec2U h = pcg2d_hash(v);
    return Vec2F(detail::u32_to_float_01(h.x), detail::u32_to_float_01(h.y));
}

STDROMANO_HOST_DEVICE STDROMANO_FORCE_INLINE Vec3F pcg3d_hash_float(const Vec3U v) noexcept
{
    const Vec3U h = pcg3d_hash(v);
    return Vec3F(detail::u32_to_float_01(h.x), detail::u32_to_float_01(h.y), detail::u32_to_float_01(h.z));
}

STDROMANO_HOST_DEVICE STDROMANO_FORCE_INLINE Vec4F pcg4d_hash_float(const Vec4U v) noexcept
{
    const Vec4U h = pcg4d_hash(v);
    return Vec4F(detail::u32_to_float_01(h.x),
                 detail::u32_to_float_01(h.y),
                 detail::u32_to_float_01(h.z),
                 detail::u32_to_float_01(h.w));
}

STDROMANO_NAMESPACE_END

#endif /* !defined(__STDROMANO_LINALG_RANDOM) */
