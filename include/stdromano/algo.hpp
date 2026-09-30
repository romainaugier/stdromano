// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - Present Romain Augier
// All rights reserved.

#pragma once

#if !defined(__STDROMANO_ALGO)
#define __STDROMANO_ALGO

#include "stdromano/stdromano.hpp"

#define ALGO_NAMESPACE_BEGIN namespace algo {
#define ALGO_NAMESPACE_END }

STDROMANO_NAMESPACE_BEGIN

ALGO_NAMESPACE_BEGIN

DETAIL_NAMESPACE_BEGIN

template<typename T>
inline void sort_swap(T& a, T& b) noexcept
{
    T tmp = static_cast<T&&>(a);
    a = static_cast<T&&>(b);
    b = static_cast<T&&>(tmp);
}

template<typename T, typename Cmp>
void insertion_sort(T* first, T* last, Cmp& cmp)
{
    for(T* i = first + 1; i < last; ++i)
    {
        T value = static_cast<T&&>(*i);
        T* j = i;

        for(; j > first && cmp(value, *(j - 1)); --j)
            *j = static_cast<T&&>(*(j - 1));

        *j = static_cast<T&&>(value);
    }
}

template<typename T, typename Cmp>
void sift_down(T* base, std::size_t root, const std::size_t n, Cmp& cmp)
{
    for(std::size_t child = 2 * root + 1; child < n; child = 2 * root + 1)
    {
        if(child + 1 < n && cmp(base[child], base[child + 1]))
            ++child;

        if(!cmp(base[root], base[child]))
            return;

        sort_swap(base[root], base[child]);
        root = child;
    }
}

template<typename T, typename Cmp>
void heap_sort(T* first, const std::size_t n, Cmp& cmp)
{
    for(std::size_t i = n / 2; i-- > 0;)
        sift_down(first, i, n, cmp);

    for(std::size_t i = n; i-- > 1;)
    {
        sort_swap(first[0], first[i]);
        sift_down(first, 0, i, cmp);
    }
}

template<typename T, typename Cmp>
void introsort(T* first, T* last, std::uint32_t depth, Cmp& cmp)
{
    while(last - first > 16)
    {
        if(depth-- == 0)
        {
            heap_sort(first, static_cast<std::size_t>(last - first), cmp);
            return;
        }

        T* mid = first + (last - first) / 2;

        if(cmp(*mid, *first))
            sort_swap(*mid, *first);

        if(cmp(*(last - 1), *mid))
        {
            sort_swap(*(last - 1), *mid);

            if(cmp(*mid, *first))
                sort_swap(*mid, *first);
        }

        sort_swap(*first, *mid);

        T* i = first;
        T* j = last;

        for(;;)
        {
            while(cmp(*++i, *first)) {}
            while(cmp(*first, *--j)) {}

            if(i >= j)
                break;

            sort_swap(*i, *j);
        }

        sort_swap(*first, *j);

        if(j - first < last - (j + 1))
        {
            introsort(first, j, depth, cmp);
            first = j + 1;
        }
        else
        {
            introsort(j + 1, last, depth, cmp);
            last = j;
        }
    }
}

DETAIL_NAMESPACE_END

template<typename T, typename Cmp>
void sort(T* first, T* last, Cmp cmp)
{
    if(last - first < 2)
        return;

    std::uint32_t depth = 0;

    for(std::size_t n = static_cast<std::size_t>(last - first); n > 1; n >>= 1)
        depth += 2;

    detail::introsort(first, last, depth, cmp);
    detail::insertion_sort(first, last, cmp);
}

ALGO_NAMESPACE_END

STDROMANO_NAMESPACE_END

#endif // !defined(__STDROMANO_ALGO)