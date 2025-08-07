// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - Present Romain Augier
// All rights reserved.

#include "stdromano/cpu.hpp"
#include "stdromano/linalg/dense_matrix.hpp"
#include "stdromano/simd.hpp"
#include "stdromano/threading.hpp"

#include "spdlog/fmt/fmt.h"

#include <iostream>

STDROMANO_NAMESPACE_BEGIN

template <typename T>
inline DenseMatrix<T> DenseMatrix<T>::matmat_add(const DenseMatrix& other) const noexcept
{
    DenseMatrix<T> res(this->_nrows, this->_ncols);

    for(std::size_t j = 0; j < this->_nrows; j++)
    {
        for(std::size_t i = 0; i < this->_ncols; i++)
        {
            res(j, i) = (*this)(j, i) + other(j, i);
        }
    }

    return res;
}

template <typename T>
inline DenseMatrix<T> DenseMatrix<T>::matmat_sub(const DenseMatrix& other) const noexcept
{
    DenseMatrix<T> res(this->_nrows, this->_ncols);

    for(std::size_t j = 0; j < this->_nrows; j++)
    {
        for(std::size_t i = 0; i < this->_ncols; i++)
        {
            res(j, i) = (*this)(j, i) - other(j, i);
        }
    }

    return res;
}

/********************************/
/* Mat-Scalar Multiplication */
/********************************/

template <typename T>
inline DenseMatrix<T> DenseMatrix<T>::matscalar_mul(const T& other) const noexcept
{
    DenseMatrix<T> res(this->_nrows, this->_ncols);

    for(std::size_t j = 0; j < this->_nrows; j++)
    {
        for(std::size_t i = 0; i < this->_ncols; i++)
        {
            res(j, i) = (*this)(j, i) * other;
        }
    }

    return res;
}

/********************************/
/* Debug */
/********************************/

using DenseMatrixBuffer = fmt::memory_buffer;
using DenseMatrixBufferIt = std::back_insert_iterator<DenseMatrixBuffer>;

static DenseMatrixBuffer g_densematrix_debug_buf;
static DenseMatrixBufferIt g_densematrix_debug_bufit(g_densematrix_debug_buf);

template <typename T>
void densematrix_debug_full(const T* __restrict A,
                            std::size_t M,
                            std::size_t N) noexcept
{
    for(std::size_t i = 0; i < M; ++i)
    {
        for(std::size_t j = 0; j < N; ++j)
        {
            fmt::format_to(g_densematrix_debug_bufit,
                           std::is_floating_point_v<T> ? "{:.3f}{}" : "{}{}",
                           A[j * M + i],
                           j == (N - 1) ? "" : " ");
        }

        fmt::format_to(g_densematrix_debug_bufit, "\n");
    }

    std::cout.write(g_densematrix_debug_buf.data(), g_densematrix_debug_buf.size());
    std::cout.flush();
}

template <typename T>
void densematrix_debug_limited(const T* __restrict A,
                               std::size_t M,
                               std::size_t N,
                               std::size_t max_rows,
                               std::size_t max_cols) noexcept
{
    const std::size_t max_rows_to_debug = (max_rows - (max_rows % 2)) / 2;
    const std::size_t max_cols_to_debug = (max_cols - (max_cols % 2)) / 2;

    std::size_t i = 0;
    std::size_t j = 0;

    for(; i < M && i < max_rows_to_debug; ++i)
    {
        for(j = 0; j < M && j < max_cols_to_debug; ++j)
        {
            fmt::format_to(g_densematrix_debug_bufit, 
                           std::is_floating_point_v<T> ? "{:.3f} " : "{} ",
                           A[j * M + i]);
        }

        if(j < (N / 2))
        {
            fmt::format_to(g_densematrix_debug_bufit, "... ");
        }

        for(j = (N - max_cols_to_debug); j < N; ++j)
        {
            fmt::format_to(g_densematrix_debug_bufit,
                           std::is_floating_point_v<T> ? "{:.3f}" : "{}",
                           A[j * M + i]);

            if(j != (N - 1))
            {
                fmt::format_to(g_densematrix_debug_bufit, " ");
            }
        }

        fmt::format_to(g_densematrix_debug_bufit, "\n");
    }

    if(i < (M / 2))
    {
        fmt::format_to(g_densematrix_debug_bufit, "...\n");
    }

    for(i = (M - max_rows_to_debug); i < M; ++i)
    {
        for(j = 0; j < N && j < max_cols_to_debug; ++j)
        {
            fmt::format_to(g_densematrix_debug_bufit,
                            std::is_floating_point_v<T> ? "{:.3f} " : "{} ",
                            A[j * M + i]);
        }

        if(j < (N / 2))
        {
            fmt::format_to(g_densematrix_debug_bufit, "... ");
        }

        for(j = (N - max_cols_to_debug); j < N; ++j)
        {
            fmt::format_to(g_densematrix_debug_bufit,
                            std::is_floating_point_v<T> ? "{:.3f}" : "{}",
                            A[j * M + i]);

            if(j != (N - 1))
            {
                fmt::format_to(g_densematrix_debug_bufit, " ");
            }
        }

        fmt::format_to(g_densematrix_debug_bufit, "\n");
    }

    std::cout.write(g_densematrix_debug_buf.data(), g_densematrix_debug_buf.size());
    std::cout.flush();
}

void detail::mat_debugf(const float* __restrict A,
                        std::size_t M,
                        std::size_t N,
                        std::size_t max_M,
                        std::size_t max_N) noexcept
{
    g_densematrix_debug_buf.clear();

    fmt::format_to(g_densematrix_debug_bufit, "Dense Matrix (float, {}x{})\n", M, N);

    if(max_M == 0 || max_N == 0)
    {
        densematrix_debug_full(A, M, N);
    }
    else 
    {
        densematrix_debug_limited(A, M, N, max_M, max_N);
    }
}

void detail::mat_debugd(const double* __restrict A,
                        std::size_t M,
                        std::size_t N,
                        std::size_t max_M,
                        std::size_t max_N) noexcept
{
    g_densematrix_debug_buf.clear();

    fmt::format_to(g_densematrix_debug_bufit, "Dense Matrix (float, {}x{})\n", M, N);

    if(max_M == 0 || max_N == 0)
    {
        densematrix_debug_full(A, M, N);
    }
    else 
    {
        densematrix_debug_limited(A, M, N, max_M, max_N);
    }
}

void detail::mat_debugi(const std::int32_t* __restrict A,
                        std::size_t M,
                        std::size_t N,
                        std::size_t max_M,
                        std::size_t max_N) noexcept
{
    g_densematrix_debug_buf.clear();

    fmt::format_to(g_densematrix_debug_bufit, "Dense Matrix (float, {}x{})\n", M, N);

    if(max_M == 0 || max_N == 0)
    {
        densematrix_debug_full(A, M, N);
    }
    else 
    {
        densematrix_debug_limited(A, M, N, max_M, max_N);
    }
}

STDROMANO_NAMESPACE_END