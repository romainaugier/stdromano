// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - Present Romain Augier
// All rights reserved.

#include "parity.hpp"

#include <cuda_runtime.h>

namespace parity {

__global__ void evaluate_kernel(const Input* inputs, const std::size_t count, float* outputs)
{
    const std::size_t i = static_cast<std::size_t>(blockIdx.x) * blockDim.x + threadIdx.x;

    if(i < count)
        evaluate(inputs[i], outputs + i * Function_Count);
}

bool evaluate_on_device(const Input* inputs, const std::size_t count, float* outputs) noexcept
{
    int device_count = 0;

    if(cudaGetDeviceCount(&device_count) != cudaSuccess || device_count == 0)
        return false;

    Input* device_inputs = nullptr;
    float* device_outputs = nullptr;

    const std::size_t outputs_size = count * Function_Count * sizeof(float);

    bool success = cudaMalloc(&device_inputs, count * sizeof(Input)) == cudaSuccess &&
                   cudaMalloc(&device_outputs, outputs_size) == cudaSuccess &&
                   cudaMemcpy(device_inputs, inputs, count * sizeof(Input), cudaMemcpyHostToDevice) == cudaSuccess;

    if(success)
    {
        const unsigned int block_size = 128;
        const unsigned int grid_size = static_cast<unsigned int>((count + block_size - 1) / block_size);

        evaluate_kernel<<<grid_size, block_size>>>(device_inputs, count, device_outputs);

        success = cudaGetLastError() == cudaSuccess &&
                  cudaMemcpy(outputs, device_outputs, outputs_size, cudaMemcpyDeviceToHost) == cudaSuccess;
    }

    cudaFree(device_inputs);
    cudaFree(device_outputs);

    return success;
}

} // namespace parity
