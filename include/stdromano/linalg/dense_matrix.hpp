// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - Present Romain Augier
// All rights reserved.

#pragma once

#if !defined(__STDROMANO_LINALG_DENSE_MATRIX)
#define __STDROMANO_LINALG_DENSE_MATRIX

#include "stdromano/memory.hpp"

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
    static_assert(std::is_floating_point_v<T> || std::is_integral_v<T>,
                  "T must be floating-point or integral type");

private:
    T* _data;
    std::size_t _nrows;
    std::size_t _ncols;
    
    static constexpr size_t ALIGNMENT = 32;

    DenseMatrix matmat_add(const DenseMatrix& other) const noexcept;
    DenseMatrix matmat_sub(const DenseMatrix& other) const noexcept;

    DenseMatrix matscalar_mul(const T& other) const noexcept;
    
public:
    DenseMatrix() : _data(nullptr), _nrows(0), _ncols(0) {}
    
    DenseMatrix(std::size_t nrows, std::size_t ncols) : _nrows(nrows), _ncols(ncols) 
    {
        this->_data = static_cast<T*>(mem_aligned_alloc(this->nbytes(), ALIGNMENT));
    }
    
    DenseMatrix(std::size_t nrows, std::size_t ncols, const T& init_value) : _nrows(nrows), _ncols(ncols) 
    {
        this->_data = static_cast<T*>(mem_aligned_alloc(this->nbytes(), ALIGNMENT));

        this->fill(init_value);
    }
    
    ~DenseMatrix() noexcept
    {
        if(this->_data != nullptr)
        {
            mem_aligned_free(this->_data);
        }
    }
    
    DenseMatrix(const DenseMatrix& other) noexcept : _nrows(other._nrows), _ncols(other._ncols)
    {
        this->_data = static_cast<T*>(mem_aligned_alloc(this->nbytes(), ALIGNMENT));

        std::memcpy(this->_data, other._data, this->nbytes());
    }

    DenseMatrix& operator=(const DenseMatrix& other) noexcept
    {
        if(this != std::addressof(other))
        {
            if(this->_data != nullptr)
            {
                mem_aligned_free(this->_data);
            }

            this->_nrows = other._nrows;
            this->_ncols = other._ncols;

            this->_data = static_cast<T*>(mem_aligned_alloc(this->nbytes(), ALIGNMENT));
        }

        return *this;
    }
    
    DenseMatrix(DenseMatrix&& other) noexcept : _nrows(other._nrows),
                                                _ncols(other._ncols),
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
            this->_data = other._data;

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
        STDROMANO_ASSERT(row < this->_nrows && col < this->_ncols, "Out-of-bounds access");

        return this->_data[col * this->_nrows + row];
    }
    
    STDROMANO_FORCE_INLINE const T& operator()(size_t row, size_t col) const noexcept
    {
        STDROMANO_ASSERT(row < this->_nrows && col < this->_ncols, "Out-of-bounds access");

        return this->_data[col * this->_nrows + row];
    }
    
    STDROMANO_FORCE_INLINE T& at(size_t row, size_t col) noexcept
    {
        STDROMANO_ASSERT(row < this->_nrows && col < this->_ncols, "Out-of-bounds access");

        return this->_data[col * this->_nrows + row];
    }
    
    STDROMANO_FORCE_INLINE const T& at(size_t row, size_t col) const noexcept
    {
        STDROMANO_ASSERT(row < this->_nrows && col < this->_ncols, "Out-of-bounds access");

        return this->_data[col * this->_nrows + row];
    }
    
    STDROMANO_FORCE_INLINE T* data() { return this->_data; }
    STDROMANO_FORCE_INLINE const T* data() const { return this->_data; }
    
    DenseMatrix operator*(const DenseMatrix& other) const noexcept
    {
        DenseMatrix res(this->_nrows, other._ncols);
        res.zero();

        const std::size_t M = this->_nrows;
        const std::size_t K = this->_ncols;
        const std::size_t N = other._ncols;

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
        else 
        {
            static_assert(0, "T not supported for matrix multiplication");
        }
        
        return res;
    }
    
    DenseMatrix operator+(const DenseMatrix& other) const noexcept
    {
        if(this->_nrows != other._nrows || this->_ncols != other._ncols) 
        {
            return DenseMatrix(this->_nrows, this->_ncols, T{});
        }
        
        return this->matmat_add(other);
    }
    
    DenseMatrix operator-(const DenseMatrix& other) const noexcept 
    {
        if(this->_nrows != other._nrows || this->_ncols != other._ncols) 
        {
            return DenseMatrix(this->_nrows, this->_ncols, T{});
        }
        
        return this->matmat_sub(other);
    }
    
    DenseMatrix operator*(const T& scalar) const noexcept
    {
        return this->matscalar_mul(scalar);
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
        for(std::size_t i = 0; i < this->size(); i++)
        {
            this->_data[i] = value;
        }
    }
    
    void zero() noexcept 
    {
        this->fill(T{});
    }

    void debug(std::size_t max_rows = 0, std::size_t max_cols = 0) const noexcept
    {
        if constexpr (std::is_same_v<T, float>)
        {
            detail::mat_debugf(this->_data,
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