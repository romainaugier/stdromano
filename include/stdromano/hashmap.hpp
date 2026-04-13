// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - Present Romain Augier
// All rights reserved.

#pragma once

#if !defined(__STDROMANO_HASHMAP)
#define __STDROMANO_HASHMAP

#include "stdromano/hashcontainer.hpp"

#include <initializer_list>

STDROMANO_NAMESPACE_BEGIN

template <class Key, class Value, class Hash = std::hash<Key>>
class HashMap : public HashContainer<Key,
                                     std::pair<Key, Value>,
                                     detail::PairKeySelect<Key, Value>,
                                     Hash>
{
private:
    using Base = HashContainer<Key, std::pair<Key, Value>, detail::PairKeySelect<Key, Value>, Hash>;

public:
    using key_type = Key;
    using mapped_type = Value;
    using value_type = std::pair<Key, Value>;
    using size_type = typename Base::size_type;
    using hasher = typename Base::hasher;
    using iterator = typename Base::iterator;
    using const_iterator = typename Base::const_iterator;

    explicit HashMap(std::size_t initial_capacity = Base::INITIAL_CAPACITY,
                     const Hash& hash = Hash()) : Base(initial_capacity, hash) {}

    HashMap(std::initializer_list<value_type> init,
            std::size_t initial_capacity = 0,
            const Hash& hash = Hash()) : Base(initial_capacity, hash)
    {
        if(init.size() > 0)
        {
            const std::size_t required_buckets = static_cast<std::size_t>(std::ceil(init.size() / Base::MAX_LOAD_FACTOR));
            const std::size_t new_capacity = bit_ceil(required_buckets);

            if(new_capacity > this->_buckets.size())
                this->grow(new_capacity, false);

            for(const auto& item : init)
                this->emplace(item);
        }
    }

    HashMap(const HashMap&) = default;
    HashMap(HashMap&&) noexcept = default;
    HashMap& operator=(const HashMap&) = default;
    HashMap& operator=(HashMap&&) noexcept = default;

    template <typename P>
    void insert(P&& value)
    {
        if(this->needs_grow())
            this->grow(this->get_new_capacity(), false);

        this->insert_internal(value_type(std::forward<P>(value)));
    }

    mapped_type& operator[](const key_type& key)
    {
        auto it = this->find(key);

        if(it == this->end())
        {
            auto result = this->emplace(key, Value{});
            it = result.first;
        }

        return it->second;
    }
};

STDROMANO_NAMESPACE_END

#endif // !defined(__STDROMANO_HASHMAP)
