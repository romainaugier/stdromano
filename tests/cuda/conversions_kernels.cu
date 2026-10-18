// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - Present Romain Augier
// All rights reserved.

#include "conversions.hpp"

#include "stdromano/linalg/random.hpp"

#include <cuda_runtime.h>

using namespace stdromano;

static_assert(std::is_convertible_v<float2, Vec2F> && std::is_convertible_v<Vec2F, float2>, "float2");
static_assert(std::is_convertible_v<float3, Vec3F> && std::is_convertible_v<Vec3F, float3>, "float3");
static_assert(std::is_convertible_v<float4, Vec4F> && std::is_convertible_v<Vec4F, float4>, "float4");
static_assert(std::is_convertible_v<double2, Vec2D> && std::is_convertible_v<Vec2D, double2>, "double2");
static_assert(std::is_convertible_v<int2, Vec2I> && std::is_convertible_v<Vec2I, int2>, "int2");
static_assert(std::is_convertible_v<int3, Vec3I> && std::is_convertible_v<Vec3I, int3>, "int3");
static_assert(std::is_convertible_v<int4, Vec4I> && std::is_convertible_v<Vec4I, int4>, "int4");
static_assert(std::is_convertible_v<uint2, Vec2U> && std::is_convertible_v<Vec2U, uint2>, "uint2");
static_assert(std::is_convertible_v<uint3, Vec3U> && std::is_convertible_v<Vec3U, uint3>, "uint3");
static_assert(std::is_convertible_v<uint4, Vec4U> && std::is_convertible_v<Vec4U, uint4>, "uint4");

/* Other component types or counts stay explicit, or do not convert at all */
static_assert(!std::is_convertible_v<float3, Vec3I> && std::is_constructible_v<Vec3I, float3>, "float3 to Vec3I");
static_assert(!std::is_convertible_v<int3, Vec3F> && std::is_constructible_v<Vec3F, int3>, "int3 to Vec3F");
static_assert(!std::is_convertible_v<float4, Vec3F> && std::is_constructible_v<Vec3F, float4>, "float4 to Vec3F");
static_assert(!std::is_convertible_v<Vec4F, float3>, "Vec4F to float3");
static_assert(!std::is_convertible_v<Vec3F, float4>, "Vec3F to float4");
static_assert(!std::is_convertible_v<Vec3D, float3>, "Vec3D to float3");
static_assert(!std::is_convertible_v<Vec3F, int3>, "Vec3F to int3");

STDROMANO_HOST_DEVICE static float3 scale_float3(const float3 v, const float s) noexcept
{
    return make_float3(v.x * s, v.y * s, v.z * s);
}

STDROMANO_HOST_DEVICE static bool check() noexcept
{
    const float3 f3 = make_float3(1.0f, 2.0f, 3.0f);
    const Vec3F v = f3;
    const float3 back = v * 2.0f;
    const Vec3F sum = v + f3;
    const Vec3F scaled = scale_float3(v, 3.0f);

    bool ok = v == Vec3F(1.0f, 2.0f, 3.0f);
    ok = ok && back.x == 2.0f && back.y == 4.0f && back.z == 6.0f;
    ok = ok && sum == Vec3F(2.0f, 4.0f, 6.0f);
    ok = ok && scaled == Vec3F(3.0f, 6.0f, 9.0f);

    const Vec4F v4 = make_float4(1.0f, 2.0f, 3.0f, 4.0f);
    const float4 f4 = Vec4F(v, 1.0f);
    ok = ok && v4.w == 4.0f && f4.x == 1.0f && f4.w == 1.0f;
    ok = ok && Vec3F(make_float4(5.0f, 6.0f, 7.0f, 8.0f)) == Vec3F(5.0f, 6.0f, 7.0f);

    const Vec2F v2 = make_float2(0.5f, 0.25f);
    const float2 f2 = v2 * 2.0f;
    ok = ok && f2.x == 1.0f && f2.y == 0.5f;

    const Vec2D d2 = make_double2(1.5, 2.5);
    const double2 back_d2 = d2 + 1.0;
    ok = ok && back_d2.x == 2.5 && back_d2.y == 3.5;

    const int3 i3 = Vec3I(make_int3(-1, 2, -3)) * 2;
    ok = ok && i3.x == -2 && i3.y == 4 && i3.z == -6;

    const Vec2I i2 = make_int2(7, 8);
    const int4 i4 = Vec4I(make_int4(1, 2, 3, 4)) + 1;
    ok = ok && i2 == Vec2I(7, 8) && i4.w == 5;

    /* the hash takes a Vec4U, a uint4 converts implicitly */
    const uint4 h = pcg4d_hash(make_uint4(1U, 2U, 3U, 4U));
    ok = ok && h.x == 0x3622CD16U && h.y == 0xF11471D8U && h.z == 0xE1109B3FU && h.w == 0x02B94C2FU;

    const uint2 h2 = pcg2d_hash(make_uint2(12U, 345U));
    ok = ok && h2.x == 0x0ADAEDDCU && h2.y == 0x98DBE451U;

    const Vec3U u3 = make_uint3(1U, 2U, 3U);
    const uint3 shifted = u3 << 4U;
    ok = ok && shifted.x == 16U && shifted.z == 48U;

    return ok;
}

namespace conversions {

__global__ void check_kernel(int* result)
{
    *result = check() ? DeviceResult_Passed : DeviceResult_Failed;
}

bool check_on_host() noexcept
{
    return check();
}

int check_on_device() noexcept
{
    int device_count = 0;

    if(cudaGetDeviceCount(&device_count) != cudaSuccess || device_count == 0)
        return DeviceResult_NoDevice;

    int* device_result = nullptr;
    int result = DeviceResult_Failed;

    if(cudaMalloc(&device_result, sizeof(int)) == cudaSuccess)
    {
        check_kernel<<<1, 1>>>(device_result);

        if(cudaGetLastError() != cudaSuccess ||
           cudaMemcpy(&result, device_result, sizeof(int), cudaMemcpyDeviceToHost) != cudaSuccess)
            result = DeviceResult_Failed;
    }

    cudaFree(device_result);

    return result;
}

} // namespace conversions
