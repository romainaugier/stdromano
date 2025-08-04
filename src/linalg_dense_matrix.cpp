// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - Present Romain Augier
// All rights reserved.

#include "stdromano/linalg/dense_matrix.hpp"

STDROMANO_NAMESPACE_BEGIN

template<typename T>
DenseMatrix<T> DenseMatrix<T>::matmat_add(const DenseMatrix& other) const noexcept
{
    DenseMatrix<T> res(this->_nrows, this->_ncols);

    for(std::size_t j = 0; j < this->_nrows; j++)
    {
        for(std::size_t i = 0; i < this->_ncols; i++)
        {
            res(j, i) = *this(j, i) + other(j, i);
        }
    }

    return res;
}

template<typename T>
DenseMatrix<T> DenseMatrix<T>::matmat_sub(const DenseMatrix& other) const noexcept
{
    DenseMatrix<T> res(this->_nrows, this->_ncols);

    for(std::size_t j = 0; j < this->_nrows; j++)
    {
        for(std::size_t i = 0; i < this->_ncols; i++)
        {
            res(j, i) = *this(j, i) - other(j, i);
        }
    }

    return res;
}

template<typename T>
DenseMatrix<T> DenseMatrix<T>::matmat_mul(const DenseMatrix& other) const noexcept
{
    DenseMatrix<T> res(this->_nrows, other._ncols);

    return res;
}

template<typename T>
DenseMatrix<T> DenseMatrix<T>::matscalar_mul(const T& other) const noexcept
{
    DenseMatrix<T> res(this->_nrows, this->_ncols);

    for(std::size_t j = 0; j < this->_nrows; j++)
    {
        for(std::size_t i = 0; i < this->_ncols; i++)
        {
            res(j, i) = *this(j, i) * other;
        }
    }

    return res;
}

STDROMANO_NAMESPACE_END