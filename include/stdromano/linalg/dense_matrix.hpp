// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - Present Romain Augier
// All rights reserved.

#pragma once

#if !defined(__STDROMANO_LINALG_DENSE_MATRIX)
#define __STDROMANO_LINALG_DENSE_MATRIX

#include "stdromano/memory.hpp"
#include "stdromano/linalg/backend.hpp"
#include "stdromano/linalg/traits.hpp"
#include "stdromano/filesystem.hpp"
#include "stdromano/logger.hpp"

#if defined(STDROMANO_ENABLE_OPENCL)
#include "stdromano/opencl.hpp"
#endif /* defined(STDROMANO_ENABLE_OPENCL) */

STDROMANO_NAMESPACE_BEGIN

namespace detail {
    STDROMANO_API void matmat_mulf(const float* __restrict A,
                                   const float* __restrict B,
                                   float* __restrict C,
                                   std::size_t M,
                                   std::size_t K,
                                   std::size_t N) noexcept;

    STDROMANO_API void matmat_muld(const double* __restrict A,
                                   const double* __restrict B,
                                   double* __restrict C,
                                   std::size_t M,
                                   std::size_t K,
                                   std::size_t N) noexcept;

    STDROMANO_API void matmat_muli(const std::int32_t* __restrict A,
                                   const std::int32_t* __restrict B,
                                   std::int32_t* __restrict C,
                                   std::size_t M,
                                   std::size_t K,
                                   std::size_t N) noexcept;

    STDROMANO_API void mat_debugf(const float* __restrict A,
                                  std::size_t M,
                                  std::size_t N,
                                  std::size_t max_M = 0,
                                  std::size_t max_N = 0) noexcept;

    STDROMANO_API void mat_debugd(const double* __restrict A,
                                  std::size_t M,
                                  std::size_t N,
                                  std::size_t max_M = 0,
                                  std::size_t max_N = 0) noexcept;

    STDROMANO_API void mat_debugi(const std::int32_t* __restrict A,
                                  std::size_t M,
                                  std::size_t N,
                                  std::size_t max_M = 0,
                                  std::size_t max_N = 0) noexcept;
}

/*
    Dense matrix stored in column-major order for the best matmul performance
*/

template<typename T>
class DenseMatrix
{
    static_assert(is_compatible_v<T>,
                  "T must be float, double or std::int32_t");

private:
    T* _data;

#if defined(STDROMANO_ENABLE_OPENCL)
    cl::Buffer _gpu_data;
#endif /* defined(STDROMANO_ENABLE_OPENCL) */

    std::size_t _nrows;
    std::size_t _ncols;

    std::uint32_t _backend;

    static constexpr size_t ALIGNMENT = 32;

    void allocate(std::size_t size) noexcept
    {
        if(this->_backend == LinAlgBackend_CPU)
        {
            if(this->_data != nullptr)
            {
                mem_aligned_free(this->_data);
            }

            this->_data = mem_aligned_alloc<T>(size, ALIGNMENT);

#if defined(STDROMANO_ENABLE_OPENCL)
            this->_gpu_data = nullptr;
#endif /* defined(STDROMANO_ENABLE_OPENCL) */
        }
        else if(this->_backend == LinAlgBackend_GPU)
        {
#if defined(STDROMANO_ENABLE_OPENCL)
            this->_gpu_data = opencl_manager.create_buffer<T>(this->size());
#else
            log_error("OpenCL Backed not available");
#endif /* defined(STDROMANO_ENABLE_OPENCL) */
        }
    }

public:
    DenseMatrix(std::uint32_t backend = get_default_backend()) : _data(nullptr),
#if defined(STDROMANO_ENABLE_OPENCL)
                                                                 _gpu_data(nullptr),
#endif /* defined(STDROMANO_ENABLE_OPENCL) */
                                                                 _nrows(0),
                                                                 _ncols(0),
                                                                 _backend(backend) {}

    DenseMatrix(std::size_t nrows,
                std::size_t ncols,
                std::uint32_t backend = get_default_backend()) : _data(nullptr),
#if defined(STDROMANO_ENABLE_OPENCL)
                                                                 _gpu_data(nullptr),
#endif /* defined(STDROMANO_ENABLE_OPENCL) */
                                                                 _nrows(nrows),
                                                                 _ncols(ncols),
                                                                 _backend(backend)
    {
        this->allocate(this->nbytes());
    }

    DenseMatrix(std::size_t nrows,
                std::size_t ncols,
                const T& init_value,
                std::uint32_t backend = get_default_backend()) : _data(nullptr),
#if defined(STDROMANO_ENABLE_OPENCL)
                                                                 _gpu_data(nullptr),
#endif /* defined(STDROMANO_ENABLE_OPENCL) */
                                                                 _nrows(nrows),
                                                                 _ncols(ncols),
                                                                 _backend(backend)
    {
        this->allocate(this->nbytes());

        this->fill(init_value);
    }

    ~DenseMatrix() noexcept
    {
        if(this->_data != nullptr)
        {
            mem_aligned_free(this->_data);
        }
    }

    DenseMatrix(const DenseMatrix& other) noexcept : _data(nullptr),
#if defined(STDROMANO_ENABLE_OPENCL)
                                                     _gpu_data(nullptr),
#endif /* defined(STDROMANO_ENABLE_OPENCL) */
                                                     _nrows(other._nrows),
                                                     _ncols(other._ncols),
                                                     _backend(other._backend)
    {
        this->allocate(this->nbytes());

        if(this->_backend == LinAlgBackend_CPU)
        {
            std::memcpy(this->_data, other._data, this->nbytes());
        }
        else if(this->_backend == LinAlgBackend_GPU)
        {
#if defined(STDROMANO_ENABLE_OPENCL)
        opencl_manager.copy_buffer(this->_gpu_data, other._gpu_data, this->nbytes());
#else
        log_error("OpenCL Backend not available");
#endif /* defined(STDROMANO_ENABLE_OPENCL) */
        }
    }

    DenseMatrix& operator=(const DenseMatrix& other) noexcept
    {
        if(this != std::addressof(other))
        {
            this->_nrows = other._nrows;
            this->_ncols = other._ncols;
            this->_backend = other._backend;

            this->allocate(this->nbytes());
        }

        return *this;
    }

    DenseMatrix(DenseMatrix&& other) noexcept : _nrows(other._nrows),
                                                _ncols(other._ncols),
                                                _backend(other._backend),
#if defined(STDROMANO_ENABLE_OPENCL)
                                                _gpu_data(std::move(other._gpu_data)),
#endif /* defined(STDROMANO_ENABLE_OPENCL) */
                                                _data(other._data)
    {
        other._nrows = 0;
        other._ncols = 0;
        other._data = nullptr;
    }

    DenseMatrix& operator=(DenseMatrix&& other) noexcept
    {
        if(this != std::addressof(other))
        {
            if(this->_data != nullptr)
            {
                mem_aligned_free(this->_data);
            }

            this->_nrows = other._nrows;
            this->_ncols = other._ncols;
            this->_backend = other._backend;
            this->_data = other._data;
#if defined(STDROMANO_ENABLE_OPENCL)
            this->_gpu_data = std::move(other._gpu_data);
#endif /* defined(STDROMANO_ENABLE_OPENCL) */

            other._nrows = 0;
            other._ncols = 0;
            other._data = nullptr;
        }

        return *this;
    }

    STDROMANO_FORCE_INLINE std::size_t nrows() const { return this->_nrows; }
    STDROMANO_FORCE_INLINE std::size_t ncols() const { return this->_ncols; }
    STDROMANO_FORCE_INLINE std::size_t size() const { return this->_nrows * this->_ncols; }

    STDROMANO_FORCE_INLINE std::size_t nbytes() const noexcept
    {
        return this->_nrows * this->_ncols * sizeof(T);
    }

    STDROMANO_FORCE_INLINE T& operator()(std::size_t row, std::size_t col) noexcept
    {
        STDROMANO_ASSERT(this->_backend != LinAlgBackend_GPU,
                         "GPU Backend has no per element access");
        STDROMANO_ASSERT(row < this->_nrows && col < this->_ncols, "Out-of-bounds access");

        return this->_data[col * this->_nrows + row];
    }

    STDROMANO_FORCE_INLINE const T& operator()(size_t row, size_t col) const noexcept
    {
        STDROMANO_ASSERT(this->_backend != LinAlgBackend_GPU,
                         "GPU Backend has no per element access");
        STDROMANO_ASSERT(row < this->_nrows && col < this->_ncols, "Out-of-bounds access");

        return this->_data[col * this->_nrows + row];
    }

    STDROMANO_FORCE_INLINE T& at(size_t row, size_t col) noexcept
    {
        STDROMANO_ASSERT(this->_backend != LinAlgBackend_GPU,
                         "GPU Backend has no per element access");
        STDROMANO_ASSERT(row < this->_nrows && col < this->_ncols, "Out-of-bounds access");

        return this->_data[col * this->_nrows + row];
    }

    STDROMANO_FORCE_INLINE const T& at(size_t row, size_t col) const noexcept
    {
        STDROMANO_ASSERT(this->_backend != LinAlgBackend_GPU,
                         "GPU Backend has no per element access");
        STDROMANO_ASSERT(row < this->_nrows && col < this->_ncols, "Out-of-bounds access");

        return this->_data[col * this->_nrows + row];
    }

    STDROMANO_FORCE_INLINE T* data() noexcept { return this->_data; }
    STDROMANO_FORCE_INLINE const T* data() const noexcept { return this->_data; }

#if defined(STDROMANO_ENABLE_OPENCL)
    STDROMANO_FORCE_INLINE cl::Buffer& gpu_data() noexcept { return this->_gpu_data; }
    STDROMANO_FORCE_INLINE const cl::Buffer& gpu_data() const noexcept { return this->_gpu_data; }
#endif /* defined(STDROMANO_ENABLE_OPENCL) */

    DenseMatrix to_backend(std::uint32_t backend) const noexcept
    {
        DenseMatrix res(this->_nrows, this->_ncols, backend);

        if(this->_backend == LinAlgBackend_CPU && backend == LinAlgBackend_CPU)
        {
            std::memcpy(res.data(), this->data(), this->nbytes());
        }
        else if(this->_backend == LinAlgBackend_CPU && backend == LinAlgBackend_GPU)
        {
#if defined(STDROMANO_ENABLE_OPENCL)
            opencl_manager.write_buffer(res.gpu_data(), this->data(), this->size());
#else
            log_error("OpenCL Backend not available");
#endif /* defined(STDROMANO_ENABLE_OPENCL) */
        }
        else if(this->_backend == LinAlgBackend_GPU && backend == LinAlgBackend_CPU)
        {
#if defined(STDROMANO_ENABLE_OPENCL)
            if(!opencl_manager.read_buffer(this->gpu_data(), res.data(), this->size()))
            {
                log_error("Error when reading data from the GPU, check the log for more information");
            }
#else
            log_error("OpenCL Backend not available");
#endif /* defined(STDROMANO_ENABLE_OPENCL) */
        }
        else if(this->_backend == LinAlgBackend_GPU && backend == LinAlgBackend_GPU)
        {
#if defined(STDROMANO_ENABLE_OPENCL)
            opencl_manager.copy_buffer(this->gpu_data(), res.gpu_data(), this->nbytes());
#else
            log_error("OpenCL Backend not available");
#endif /* defined(STDROMANO_ENABLE_OPENCL) */
        }

        return res;
    }

    DenseMatrix operator*(const DenseMatrix& other) const noexcept
    {
        DenseMatrix res(this->_nrows, other._ncols, this->_backend);
        res.zero();

        if(this->_backend != other._backend)
        {
            log_error("Matmul error: backend mis-match");
            return res;
        }

        const std::size_t M = this->_nrows;
        const std::size_t K = this->_ncols;
        const std::size_t N = other._ncols;

        if(this->_backend == LinAlgBackend_CPU)
        {
            if constexpr (std::is_same_v<T, float>)
            {
                detail::matmat_mulf(this->data(), other.data(), res.data(), M, K, N);
            }
            else if constexpr (std::is_same_v<T, double>)
            {
                detail::matmat_muld(this->data(), other.data(), res.data(), M, K, N);
            }
            else if constexpr (std::is_same_v<T, std::int32_t>)
            {
                detail::matmat_muli(this->data(), other.data(), res.data(), M, K, N);
            }
        }
        else
        {
#if defined (STDROMANO_ENABLE_OPENCL)
            cl::Event event;

            const StringD kernel_name("matmul{}_kernel", type_to_cl_kernel_ext_v<T>);

            if(!opencl_manager.has_kernel_source(kernel_name))
            {
                log_error("Matmul error: cannot find opencl kernel \"{}\" (path should be: {})",
                          kernel_name,
                          fs_expand_from_lib_dir(StringD("cl/{}.cl", kernel_name)));
                return res;
            }

            const std::size_t global_x = ((N + 15) / 16) * 16;
            const std::size_t global_y = ((M + 15) / 16) * 16;

            if(!opencl_manager.schedule_task(event,
                                             opencl_manager.get_kernel_source(kernel_name),
                                             kernel_name,
                                             [&](cl::Kernel& kernel) {
                                                 kernel.setArg(0, this->gpu_data());
                                                 kernel.setArg(1, other.gpu_data());
                                                 kernel.setArg(2, res.gpu_data());
                                                 kernel.setArg(3, static_cast<std::uint32_t>(M));
                                                 kernel.setArg(4, static_cast<std::uint32_t>(K));
                                                 kernel.setArg(5, static_cast<std::uint32_t>(N));
                                             },
                                             cl::NDRange(global_x, global_y),
                                             cl::NDRange(16, 16)))
            {
                log_error("Matmul error: error caught during scheduling on GPU, check the log for more details");
                return res;
            }

            event.wait();

            const double time = opencl_manager.get_execution_time_ms(event);

            log_debug("Matmul real time: {:.3f} ms", time);
#else
            log_error("OpenCL Backend not available");
#endif /* defined(STDROMANO_ENABLE_OPENCL) */
        }

        return res;
    }

    DenseMatrix transpose() const noexcept
    {
        DenseMatrix result(this->_ncols, this->_nrows);

        for(std::size_t i = 0; i < this->_nrows; ++i)
        {
            for(std::size_t j = 0; j < this->_ncols; ++j)
            {
                result(j, i) = (*this)(i, j);
            }
        }

        return result;
    }

    void fill(T value) noexcept
    {
        if(this->_backend == LinAlgBackend_CPU)
        {
            for(std::size_t i = 0; i < this->size(); i++)
            {
                this->_data[i] = value;
            }
        }
        else if(this->_backend == LinAlgBackend_GPU)
        {
#if defined(STDROMANO_ENABLE_OPENCL)
            const StringD kernel_name("matfill{}_kernel", type_to_cl_kernel_ext_v<T>);

            if(!opencl_manager.has_kernel_source(kernel_name))
            {
                log_error("Fill error: cannot find opencl kernel \"{}\" (path should be: {})",
                          kernel_name,
                          fs_expand_from_executable_dir(StringD("cl/{}.cl", kernel_name)));
                return;
            }

            cl::Event event;

            if(!opencl_manager.schedule_task(event,
                                             opencl_manager.get_kernel_source(kernel_name),
                                             kernel_name,
                                             [&](cl::Kernel& kernel) {
                                                 kernel.setArg(0, this->gpu_data());
                                                 kernel.setArg(1, value);
                                             },
                                             cl::NDRange(this->_ncols, this->_nrows)))
            {
                log_error("Fill error: error caught during scheduling on GPU, check the log for more details");
                return;
            }

            event.wait();
#else
            log_error("OpenCL Backend not available");
#endif /* defined(STDROMANO_ENABLE_OPENCL) */
        }
    }

    void zero() noexcept
    {
        this->fill(T{});
    }

    void debug(std::size_t max_rows = 0, std::size_t max_cols = 0) const noexcept
    {
        STDROMANO_ASSERT(this->_backend != LinAlgBackend_GPU, "GPU Backend has no debug");

        if constexpr (std::is_same_v<T, float>)
        {
            detail::mat_debugf(this->_data,
                               this->_nrows,
                               this->_ncols,
                               max_rows,
                               max_cols);
        }
        else if constexpr (std::is_same_v<T, double>)
        {
            detail::mat_debugd(this->_data,
                               this->_nrows,
                               this->_ncols,
                               max_rows,
                               max_cols);
        }
        else if constexpr (std::is_same_v<T, std::int32_t>)
        {
            detail::mat_debugi(this->_data,
                               this->_nrows,
                               this->_ncols,
                               max_rows,
                               max_cols);
        }
    }

    /*
        Trace is only defined for square matrices,
        so if the matrix is not square it returns 0 by default
    */
    T trace() const noexcept
    {
        STDROMANO_ASSERT(this->_backend != LinAlgBackend_GPU, "GPU Backend has no trace");

        if(this->_backend == LinAlgBackend_GPU)
        {
            return static_cast<T>(0);
        }

        if(this->_nrows != this->_ncols)
        {
            return static_cast<T>(0);
        }

        T result = static_cast<T>(0);

        for(std::size_t i = 0; i < this->_nrows; i++)
        {
            result += (*this)(i, i);
        }

        return result;
    }
};

using DenseMatrixF = DenseMatrix<float>;
using DenseMatrixD = DenseMatrix<double>;
using DenseMatrixI = DenseMatrix<std::int32_t>;

STDROMANO_NAMESPACE_END

#endif /* !defined(__STDROMANO_LINALG_DENSE_MATRIX) */
