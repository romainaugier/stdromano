// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - Present Romain Augier
// All rights reserved.

#pragma once

#if !defined(__STDROMANO_HASHSET)
#define __STDROMANO_HASHSET

#include "stdromano/hashmap.h"

#include <type_traits>

STDROMANO_NAMESPACE_BEGIN

namespace detail
{
struct EmptyValue
{
};
} // namespace detail

template <class Key, class Hash = std::hash<Key>>
class HashSet : private HashMap<Key, detail::EmptyValue, Hash>
{
  private:
    using Base = HashMap<Key, detail::EmptyValue, Hash>;
    using DummyValue = detail::EmptyValue;

    using BaseKeySelect = typename Base::KeySelect;

  public:
    using key_type = Key;
    using value_type = Key;
    using size_type = typename Base::size_type;
    using difference_type = std::ptrdiff_t;
    using hasher = typename Base::hasher;
    using reference = const value_type&;
    using const_reference = const value_type&;
    using pointer = const value_type*;
    using const_pointer = const value_type*;

    class const_iterator
    {
        friend class HashSet;
        typename Base::const_iterator base_iterator;

        const_iterator(typename Base::const_iterator it)
            : base_iterator(it)
        {
        }

      public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = const Key;
        using difference_type = std::ptrdiff_t;
        using pointer = const Key*;
        using reference = const Key&;

        const_iterator() = default;

        reference operator*() const
        {
            return BaseKeySelect()(*base_iterator);
        }

        pointer operator->() const
        {
            return std::addressof(BaseKeySelect()(*base_iterator));
        }

        const_iterator& operator++()
        {
            ++base_iterator;
            return *this;
        }

        const_iterator operator++(int)
        {
            const_iterator temp = *this;
            ++(*this);
            return temp;
        }

        friend bool operator==(const const_iterator& lhs, const const_iterator& rhs)
        {
            return lhs.base_iterator == rhs.base_iterator;
        }

        friend bool operator!=(const const_iterator& lhs, const const_iterator& rhs)
        {
            return !(lhs == rhs);
        }
    };

    using iterator = const_iterator;

    explicit HashSet(size_type initial_capacity = Base::INITIAL_CAPACITY)
        : Base(initial_capacity)
    {
    }

    iterator begin() const
    {
        return const_iterator(Base::cbegin());
    }
    iterator end() const
    {
        return const_iterator(Base::cend());
    }
    const_iterator cbegin() const
    {
        return const_iterator(Base::cbegin());
    }
    const_iterator cend() const
    {
        return const_iterator(Base::cend());
    }

    using Base::capacity;
    using Base::empty;
    using Base::load_factor;
    using Base::memory_usage;
    using Base::reserve;
    using Base::size;

    using Base::clear;

    std::pair<iterator, bool> insert(const key_type& key)
    {
        auto result = Base::emplace(key, DummyValue{});
        return {const_iterator(result.first), result.second};
    }

    std::pair<iterator, bool> insert(key_type&& key)
    {
        auto result = Base::emplace(std::move(key), DummyValue{});
        return {const_iterator(result.first), result.second};
    }

    template <typename... Args>
    std::pair<iterator, bool> emplace(Args&&... args)
    {
        key_type temp_key(std::forward<Args>(args)...);

        auto result = Base::emplace(std::move(temp_key), DummyValue{});

        return {const_iterator(result.first), result.second};
    }

    iterator erase(const_iterator pos)
    {
        if(pos != cend())
        {
            key_type key_to_erase = *pos;
            Base::erase(key_to_erase);
            return end();
        }

        return pos;
    }

    STDROMANO_FORCE_INLINE size_type erase(const key_type& key)
    {
        auto it = Base::find(key);

        if(it != Base::end())
        {
            Base::erase(it);
            return 1;
        }

        return 0;
    }

    STDROMANO_FORCE_INLINE size_type count(const key_type& key) const
    {
        return Base::find(key) != Base::end() ? 1 : 0;
    }

    STDROMANO_FORCE_INLINE iterator find(const key_type& key)
    {
        return const_iterator(Base::find(key));
    }

    STDROMANO_FORCE_INLINE const_iterator find(const key_type& key) const
    {
        return const_iterator(Base::find(key));
    }

    STDROMANO_FORCE_INLINE bool contains(const key_type& key) const
    {
        return Base::find(key) != Base::end();
    }
};

STDROMANO_NAMESPACE_END

#endif /* !defined(__STDROMANO_HASHSET) */