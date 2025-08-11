// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - Present Romain Augier
// All rights reserved.

#pragma once

#if !defined(__STDROMANO_ENUMERATE)
#define __STDROMANO_ENUMERATE

#include "stdromano/stdromano.hpp"

#include <iterator>

STDROMANO_NAMESPACE_BEGIN

template<typename T>
using enumerate_item = std::pair<std::size_t, T>;

template<typename Iterator>
class enumerate_iterator 
{
private:
    Iterator _iter;
    std::size_t _index;

public:
    using difference_type = typename std::iterator_traits<Iterator>::difference_type;
    using value_type = enumerate_item<typename std::iterator_traits<Iterator>::reference>;
    using pointer = value_type*;
    using reference = value_type;
    using iterator_category = typename std::iterator_traits<Iterator>::iterator_category;

    enumerate_iterator(Iterator it, std::size_t idx = 0) : _iter(it), _index(idx) {}

    reference operator*() const noexcept 
    {
        return std::make_pair(this->_index, std::ref(*this->_iter));
    }

    enumerate_iterator& operator++() noexcept
    {
        ++this->_iter;
        ++this->_index;
        return *this;
    }

    enumerate_iterator operator++(int) noexcept
    {
        auto temp = *this;
        ++(*this);
        return temp;
    }

    bool operator==(const enumerate_iterator& other) const noexcept
    {
        return this->_iter == other._iter;
    }

    bool operator!=(const enumerate_iterator& other) const noexcept
    {
        return !(*this == other);
    }
};

template<typename Container>
class enumerate_wrapper {
private:
    Container& container;

public:
    using iterator = enumerate_iterator<typename Container::iterator>;
    using const_iterator = enumerate_iterator<typename Container::const_iterator>;

    explicit enumerate_wrapper(Container& cont) : container(cont) {}

    iterator begin() noexcept
    {
        return iterator(container.begin(), 0);
    }

    iterator end() noexcept 
    {
        return iterator(container.end(), container.size());
    }

    const_iterator begin() const noexcept 
    {
        return const_iterator(container.begin(), 0);
    }

    const_iterator end() const noexcept 
    {
        return const_iterator(container.end(), container.size());
    }

    const_iterator cbegin() const noexcept
    {
        return const_iterator(container.cbegin(), 0);
    }

    const_iterator cend() const noexcept
    {
        return const_iterator(container.cend(), container.size());
    }
};

/* T will automatically be a reference */
template<typename Container>
enumerate_wrapper<Container> enumerate(Container& container) noexcept
{
    return enumerate_wrapper<Container>(container);
}

/* T will automatically be a const reference */
template<typename Container>
enumerate_wrapper<const Container> enumerate(const Container& container) noexcept
{
    return enumerate_wrapper<const Container>(container);
}

STDROMANO_NAMESPACE_END

#endif /* !defined(__STDROMANO_ENUMERATE) */