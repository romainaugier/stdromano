// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - Present Romain Augier
// All rights reserved.

#pragma once

#if !defined(__STDROMANO_HASHSET)
#define __STDROMANO_HASHSET

#include "stdromano/hashcontainer.hpp"

#include <initializer_list>

STDROMANO_NAMESPACE_BEGIN

template <class Key, class Hash = std::hash<Key>>
class HashSet : public HashContainer<Key, Key, detail::IdentityKeySelect<Key>, Hash>
{
private:
    using Base = HashContainer<Key, Key, detail::IdentityKeySelect<Key>, Hash>;

public:
    using key_type = Key;
    using value_type = Key;
    using size_type = typename Base::size_type;
    using hasher = typename Base::hasher;

    // Keys must not be mutated through iterators, so only const views are exposed.
    using iterator = typename Base::const_iterator;
    using const_iterator = typename Base::const_iterator;

    explicit HashSet(std::size_t initial_capacity = Base::INITIAL_CAPACITY,
                     const Hash& hash = Hash()) : Base(initial_capacity, hash) {}

    HashSet(std::initializer_list<value_type> init,
            std::size_t initial_capacity = 0,
            const Hash& hash = Hash())
        : Base(initial_capacity, hash)
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

    iterator begin() const { return this->cbegin(); }
    iterator end() const { return this->cend(); }

    std::pair<iterator, bool> insert(const key_type& key)
    {
        auto result = this->emplace(key);
        return {iterator(result.first), result.second};
    }

    std::pair<iterator, bool> insert(key_type&& key)
    {
        auto result = this->emplace(std::move(key));
        return {iterator(result.first), result.second};
    }

    STDROMANO_FORCE_INLINE size_type count(const key_type& key) const
    {
        return this->contains(key) ? 1 : 0;
    }

    iterator erase(const_iterator pos)
    {
        if(pos != this->cend())
        {
            key_type key_to_erase = *pos;
            Base::erase(key_to_erase);
        }

        return this->end();
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
};

STDROMANO_NAMESPACE_END

#endif // !defined(__STDROMANO_HASHSET)
