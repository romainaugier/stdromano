// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - Present Romain Augier
// All rights reserved.

#include "stdromano/linalg/dense_matrix.hpp"

#if defined(STDROMANO_ENABLE_OPENCL)
#include "stdromano/opencl.hpp"
#endif // defined(STDROMANO_ENABLE_OPENCL)

#include "fixtures.hpp"

#include <cmath>
#include <vector>

using namespace stdromano;

template <typename T>
static DenseMatrix<T> random_matrix(fuzz::Source& source, const std::size_t rows, const std::size_t cols)
{
    DenseMatrix<T> matrix(rows, cols, LinAlgBackend_CPU);

    for(std::size_t col = 0; col < cols; ++col)
        for(std::size_t row = 0; row < rows; ++row)
            if constexpr (std::is_floating_point_v<T>)
                matrix(row, col) = static_cast<T>(source.range<int>(-64, 64)) / T(8);
            else
                matrix(row, col) = static_cast<T>(source.range<int>(-100, 100));

    return matrix;
}

template <typename T>
static std::vector<double> reference_product(const DenseMatrix<T>& a, const DenseMatrix<T>& b)
{
    std::vector<double> result(a.nrows() * b.ncols(), 0.0);

    for(std::size_t i = 0; i < a.nrows(); ++i)
        for(std::size_t j = 0; j < b.ncols(); ++j)
            for(std::size_t k = 0; k < a.ncols(); ++k)
                result[j * a.nrows() + i] += static_cast<double>(a(i, k)) * static_cast<double>(b(k, j));

    return result;
}

template <typename T>
static bool check_product(fuzz::Source& source)
{
    const std::size_t m = source.range<std::size_t>(1, 40);
    const std::size_t k = source.range<std::size_t>(1, 40);
    const std::size_t n = source.range<std::size_t>(1, 40);

    const DenseMatrix<T> a = random_matrix<T>(source, m, k);
    const DenseMatrix<T> b = random_matrix<T>(source, k, n);

    const std::vector<double> expected = reference_product(a, b);

    bool ok = true;

    fixtures::for_each_vectorization_mode([&](std::uint32_t) {
        const auto product = a * b;

        if(!product.has_value())
        {
            ok = false;
            return;
        }

        const DenseMatrix<T>& c = product.value();

        ok &= c.nrows() == m && c.ncols() == n;

        for(std::size_t j = 0; j < n && ok; ++j)
            for(std::size_t i = 0; i < m && ok; ++i)
                ok &= static_cast<double>(c(i, j)) == expected[j * m + i];
    });

    return ok;
}

STDROMANO_TEST_CASE(construction_and_element_access)
{
    DenseMatrixF matrix(3, 2, 1.5f, LinAlgBackend_CPU);

    STDROMANO_CHECK_EQ(matrix.nrows(), 3u);
    STDROMANO_CHECK_EQ(matrix.ncols(), 2u);
    STDROMANO_CHECK_EQ(matrix.size(), 6u);
    STDROMANO_CHECK_EQ(matrix.nbytes(), 6 * sizeof(float));

    for(std::size_t i = 0; i < matrix.size(); ++i)
        STDROMANO_CHECK_EQ(matrix.data()[i], 1.5f);

    matrix(2, 1) = 7.0f;
    STDROMANO_CHECK_EQ(matrix.at(2, 1), 7.0f);
    STDROMANO_CHECK_EQ(matrix.data()[1 * 3 + 2], 7.0f);

    STDROMANO_REQUIRE(matrix.zero().has_value());
    STDROMANO_CHECK_EQ(matrix(2, 1), 0.0f);
}

STDROMANO_TEST_CASE(copy_move_and_assignment)
{
    DenseMatrixD source(4, 4, LinAlgBackend_CPU);

    for(std::size_t i = 0; i < source.size(); ++i)
        source.data()[i] = static_cast<double>(i);

    DenseMatrixD copy(source);
    STDROMANO_CHECK_EQ(copy(3, 3), 15.0);
    STDROMANO_CHECK(copy.data() != source.data());

    DenseMatrixD assigned(1, 1, LinAlgBackend_CPU);
    assigned = source;
    STDROMANO_REQUIRE_EQ(assigned.size(), 16u);

    for(std::size_t i = 0; i < assigned.size(); ++i)
        STDROMANO_CHECK_EQ(assigned.data()[i], static_cast<double>(i));

    DenseMatrixD moved(std::move(copy));
    STDROMANO_CHECK_EQ(moved(1, 2), 9.0);
    STDROMANO_CHECK_EQ(copy.size(), 0u);

    DenseMatrixD move_assigned(2, 2, LinAlgBackend_CPU);
    move_assigned = std::move(moved);
    STDROMANO_CHECK_EQ(move_assigned(0, 3), 12.0);
}

STDROMANO_TEST_CASE(transpose_and_trace)
{
    DenseMatrixI matrix(2, 3, LinAlgBackend_CPU);

    for(std::size_t i = 0; i < matrix.size(); ++i)
        matrix.data()[i] = static_cast<std::int32_t>(i + 1);

    const DenseMatrixI transposed = matrix.transpose();
    STDROMANO_REQUIRE_EQ(transposed.nrows(), 3u);
    STDROMANO_REQUIRE_EQ(transposed.ncols(), 2u);

    for(std::size_t i = 0; i < 2; ++i)
        for(std::size_t j = 0; j < 3; ++j)
            STDROMANO_CHECK_EQ(transposed(j, i), matrix(i, j));

    STDROMANO_CHECK_EQ(matrix.trace(), 0);

    DenseMatrixI square(3, 3, 2, LinAlgBackend_CPU);
    STDROMANO_CHECK_EQ(square.trace(), 6);
}

STDROMANO_TEST_CASE(product_of_known_matrices)
{
    DenseMatrixF a(2, 3, LinAlgBackend_CPU);
    DenseMatrixF b(3, 2, LinAlgBackend_CPU);

    const float a_values[2][3] = {{1, 2, 3}, {4, 5, 6}};
    const float b_values[3][2] = {{7, 8}, {9, 10}, {11, 12}};

    for(std::size_t i = 0; i < 2; ++i)
        for(std::size_t j = 0; j < 3; ++j)
            a(i, j) = a_values[i][j];

    for(std::size_t i = 0; i < 3; ++i)
        for(std::size_t j = 0; j < 2; ++j)
            b(i, j) = b_values[i][j];

    const auto product = a * b;
    STDROMANO_REQUIRE(product.has_value());

    const DenseMatrixF& c = product.value();
    STDROMANO_CHECK_EQ(c(0, 0), 58.0f);
    STDROMANO_CHECK_EQ(c(0, 1), 64.0f);
    STDROMANO_CHECK_EQ(c(1, 0), 139.0f);
    STDROMANO_CHECK_EQ(c(1, 1), 154.0f);
}

STDROMANO_TEST_CASE(product_of_large_matrices)
{
    const std::size_t size = fixtures::is_debug_build() ? 129 : 515;

    const DenseMatrixF a(size, size, 1.0f, LinAlgBackend_CPU);
    const DenseMatrixF b(size, size, 2.0f, LinAlgBackend_CPU);

    fixtures::for_each_vectorization_mode([&](std::uint32_t) {
        const auto product = a * b;
        STDROMANO_REQUIRE(product.has_value());

        const float expected = 2.0f * static_cast<float>(size);

        STDROMANO_CHECK_EQ(product.value()(0, 0), expected);
        STDROMANO_CHECK_EQ(product.value()(size - 1, size - 1), expected);
        STDROMANO_CHECK_EQ(product.value()(size / 2, 7), expected);
    });
}

STDROMANO_TEST_CASE(mismatched_dimensions_are_an_error)
{
    const DenseMatrixF a(2, 3, 1.0f, LinAlgBackend_CPU);
    const DenseMatrixF b(2, 3, 1.0f, LinAlgBackend_CPU);

    STDROMANO_CHECK((a * b).has_error());
}

STDROMANO_TEST_CASE(cpu_backend_copy)
{
    const DenseMatrixF a(5, 7, 3.0f, LinAlgBackend_CPU);
    const auto copy = a.to_backend(LinAlgBackend_CPU);

    STDROMANO_REQUIRE(copy.has_value());
    STDROMANO_CHECK_EQ(copy.value()(4, 6), 3.0f);
}

#if defined(STDROMANO_ENABLE_OPENCL)
STDROMANO_TEST_CASE(gpu_product_matches_cpu)
{
    OpenCLConfig config;

    if(!OpenCLManager::get_instance().initialize(config))
        return;

    const DenseMatrixF a(64, 33, 1.0f, LinAlgBackend_CPU);
    const DenseMatrixF b(33, 17, 2.0f, LinAlgBackend_CPU);

    const auto a_gpu = a.to_backend(LinAlgBackend_GPU);
    const auto b_gpu = b.to_backend(LinAlgBackend_GPU);
    STDROMANO_REQUIRE(a_gpu.has_value() && b_gpu.has_value());

    const auto product = a_gpu.value() * b_gpu.value();
    STDROMANO_REQUIRE(product.has_value());

    const auto back = product.value().to_backend(LinAlgBackend_CPU);
    STDROMANO_REQUIRE(back.has_value());
    STDROMANO_CHECK_EQ(back.value()(63, 16), 66.0f);
}
#else
STDROMANO_TEST_CASE(gpu_backend_reports_unavailable)
{
    const DenseMatrixF a(2, 2, 1.0f, LinAlgBackend_CPU);
    STDROMANO_CHECK(a.to_backend(LinAlgBackend_GPU).has_error());
}
#endif // defined(STDROMANO_ENABLE_OPENCL)

STDROMANO_TEST_CASE(fuzz_products_match_the_naive_reference)
{
    const auto report = fuzz::run_property(fixtures::options("densematrix_products", 200), [](fuzz::Source& source) {
        switch(source.index(3))
        {
            case 0:
                return check_product<float>(source);
            case 1:
                return check_product<double>(source);
            default:
                return check_product<std::int32_t>(source);
        }
    });

    TESTS_REQUIRE_PROPERTY(report);
}

STDROMANO_TEST_MAIN()
