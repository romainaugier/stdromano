// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - Present Romain Augier
// All rights reserved.

#pragma once

#if !defined(__STDROMANO_HASHMAP)
#define __STDROMANO_HASHMAP

#include "stdromano/bits.h"
#include "stdromano/optional.h"
#include "stdromano/random.h"

#include <cmath>
#include <functional>
#include <memory>
#include <stdexcept>
#include <type_traits>
#include <vector>
#include <initializer_list>

STDROMANO_NAMESPACE_BEGIN

template <class Key, class Value, class Hash = std::hash<Key>>
class HashMap
{
public:
    static constexpr float MAX_LOAD_FACTOR = 0.9f;
    static constexpr size_t INITIAL_CAPACITY = 8;

    class KeySelect
    {
    public:
        using key_type = Key;

        const key_type& operator()(const std::pair<Key, Value>& key_value) const noexcept
        {
            return key_value.first;
        }

        key_type& operator()(std::pair<Key, Value>& key_value) noexcept
        {
            return key_value.first;
        }
    };

    class ValueSelect
    {
    public:
        using value_type = Value;

        const value_type& operator()(const std::pair<Key, Value>& key_value) const noexcept
        {
            return key_value.second;
        }

        value_type& operator()(std::pair<Key, Value>& key_value) noexcept
        {
            return key_value.second;
        }
    };

    using key_type = Key;
    using mapped_type = Value;
    using value_type = std::pair<Key, Value>;
    using size_type = std::size_t;
    using hasher = Hash;

private:
    struct Bucket
    {
        static constexpr int16_t EMPTY_BUCKET_MARKER = -1;

        using value_type = std::pair<Key, Value>;

    private:
        uint32_t _hash;
        int16_t _probe_length;

        alignas(value_type) unsigned char _storage[sizeof(value_type)];

    public:
        Bucket() noexcept
            : _hash(0),
              _probe_length(EMPTY_BUCKET_MARKER)
        {
            STDROMANO_ASSERT(this->is_empty(), "Bucket should be empty on default initialisation");
        }

        Bucket(const Bucket& other) noexcept(std::is_nothrow_copy_constructible<value_type>::value)
        {
            if(!other.is_empty())
            {
                ::new(static_cast<void*>(std::addressof(this->_storage))) value_type(other.value());

                this->_hash = other._hash;
                this->_probe_length = other._probe_length;
            }

            STDROMANO_ASSERT(this->is_empty() == other.is_empty(), "Both buckets should be empty");
        }

        Bucket(Bucket&& other) noexcept(std::is_nothrow_move_constructible<value_type>::value)
        {
            if(!other.is_empty())
            {
                ::new(static_cast<void*>(std::addressof(this->_storage)))
                    value_type(std::move(other.value()));

                this->_hash = other._hash;
                this->_probe_length = other._probe_length;
            }

            STDROMANO_ASSERT(this->is_empty() == other.is_empty(),
                             "Both buckets should have the same value for empty");
        }

        Bucket& operator=(const Bucket& other) noexcept(
            std::is_nothrow_copy_assignable<value_type>::value)
        {
            if(this != &other)
            {
                this->clear();

                if(!other.is_empty())
                {
                    ::new(static_cast<void*>(std::addressof(this->_storage)))
                        value_type(other.value());
                }

                this->_hash = other._hash;
                this->_probe_length = other._probe_length;
            }

            return *this;
        }

        Bucket& operator=(Bucket&& other) = delete;

        ~Bucket() noexcept
        {
            this->clear();
        }

        template <typename... Args>
        void construct(Args&&... args)
        {
            ::new(static_cast<void*>(std::addressof(this->_storage)))
                value_type(std::forward<Args>(args)...);
        }

        void swap_bucket_content(value_type& value, uint32_t& hash, int16_t& probe_length)
        {
            STDROMANO_ASSERT(!this->is_empty(), "Cannot swap an empty bucket");
            STDROMANO_ASSERT(probe_length > this->_probe_length,
                             "Cannot swap a bucket with a lower probe_length");

            std::swap(value, this->value());
            std::swap(hash, this->_hash);
            std::swap(probe_length, this->_probe_length);
        }

        template <typename... Args>
        void set_bucket_content(const uint32_t hash,
                                const int16_t probe_length,
                                Args&&... value_args)
        {
            STDROMANO_ASSERT(this->is_empty(), "Cannot set content of non empty bucket");
            STDROMANO_ASSERT(probe_length >= 0,
                             "Cannot set content of bucket with uninitialized probe_length");

            ::new(static_cast<void*>(std::addressof(this->_storage)))
                value_type(std::forward<Args>(value_args)...);

            this->_hash = hash;
            this->_probe_length = probe_length;

            STDROMANO_ASSERT(!this->is_empty(), "Set bucket content failed");
        }

        STDROMANO_FORCE_INLINE value_type& value() noexcept
        {
            return *reinterpret_cast<value_type*>(this->_storage);
        }

        STDROMANO_FORCE_INLINE const value_type& value() const noexcept
        {
            return *reinterpret_cast<const value_type*>(this->_storage);
        }

        STDROMANO_FORCE_INLINE bool is_empty() const noexcept
        {
            return this->_probe_length == EMPTY_BUCKET_MARKER;
        }

        STDROMANO_FORCE_INLINE int16_t probe_length() const noexcept
        {
            return this->_probe_length;
        }

        STDROMANO_FORCE_INLINE void reset_probe_length() noexcept
        {
            this->_probe_length = 0;
        }

        STDROMANO_FORCE_INLINE void set_probe_length(int16_t probe_length) noexcept
        {
            this->_probe_length = probe_length;
        }

        STDROMANO_FORCE_INLINE void increment_probe_length() noexcept
        {
            this->_probe_length++;
        }

        STDROMANO_FORCE_INLINE void decrement_probe_length() noexcept
        {
            this->_probe_length--;
        }

        STDROMANO_FORCE_INLINE uint32_t hash() const noexcept
        {
            return this->_hash;
        }

        STDROMANO_FORCE_INLINE void set_hash(const uint32_t hash) noexcept
        {
            this->_hash = hash;
        }

        void clear() noexcept
        {
            if(!this->is_empty())
            {
                this->value().~value_type();
                this->_hash = 0;
                this->_probe_length = EMPTY_BUCKET_MARKER;
            }
        }
    };

    Hash _hash_func;
    std::vector<Bucket> _buckets;
    size_t _items_count = 0;
    uint32_t _hash_key;
    int16_t _max_probes;

    KeySelect _key_select;

public:
    class iterator
    {
        friend class HashMap;
        HashMap* map;
        size_t index;

        void advance()
        {
            while(this->index < this->map->_buckets.size() && this->map->_buckets[index].is_empty())
            {
                this->index++;
            }
        }

    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = std::pair<Key, Value>;
        using difference_type = std::ptrdiff_t;
        using pointer = value_type*;
        using reference = value_type&;

        iterator(HashMap* m, size_t i)
            : map(m),
              index(i)
        {
            this->advance();
        }

        iterator& operator++()
        {
            this->index++;
            this->advance();
            return *this;
        }

        bool operator==(const iterator& other) const
        {
            return this->index == other.index && this->map == other.map;
        }

        bool operator!=(const iterator& other) const
        {
            return !(*this == other);
        }

        reference operator*() const
        {
            return this->map->_buckets[index].value();
        }

        pointer operator->() const
        {
            return std::addressof(this->map->_buckets[this->index].value());
        }

        const typename KeySelect::key_type& key() const
        {
            return KeySelect()(this->map->_buckets[index].value());
        }

        typename ValueSelect::value_type& value() const
        {
            return ValueSelect()(this->map->_buckets[index].value());
        }
    };

    class const_iterator
    {
        friend class HashMap;
        const HashMap* map;
        size_t index;

        void advance()
        {
            while(this->index < this->map->_buckets.size() && this->map->_buckets[index].is_empty())
            {
                this->index++;
            }
        }

    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = std::pair<Key, Value>;
        using difference_type = std::ptrdiff_t;
        using pointer = const value_type*;
        using reference = const value_type&;

        const_iterator(const HashMap* m, size_t i)
            : map(m),
              index(i)
        {
            this->advance();
        }

        const_iterator(const iterator& other)
            : map(other.map),
              index(other.index)
        {
        }

        const_iterator& operator++()
        {
            this->index++;
            this->advance();
            return *this;
        }

        bool operator==(const const_iterator& other) const
        {
            return this->index == other.index && this->map == other.map;
        }

        bool operator!=(const const_iterator& other) const
        {
            return !(*this == other);
        }

        reference operator*() const
        {
            return this->map->_buckets[index].value();
        }

        pointer operator->() const
        {
            return std::addressof(this->map->_buckets[this->index].value());
        }

        const typename KeySelect::key_type& key() const
        {
            return KeySelect()(this->map->_buckets[index].value());
        }

        const typename ValueSelect::value_type& value() const
        {
            return ValueSelect()(this->map->_buckets[index].value());
        }
    };

private:
    STDROMANO_FORCE_INLINE uint32_t get_hash(const key_type& key) const
    {
        return static_cast<uint32_t>(this->_hash_func(key) ^ this->_hash_key);
    }

    STDROMANO_FORCE_INLINE size_t get_index(const uint32_t hash) const
    {
        return hash & (this->_buckets.size() - 1);
    }

    STDROMANO_FORCE_INLINE size_t get_next_index(const size_t index) const
    {
        return (index + 1) & (this->_buckets.size() - 1);
    }

    STDROMANO_FORCE_INLINE size_t get_new_capacity() const
    {
        return bit_ceil(this->_buckets.size() + 1);
    }

    STDROMANO_FORCE_INLINE uint32_t generate_hash_key()
    {
        return next_random_uint32();
    }

    void insert_internal(value_type&& value)
    {
        const key_type& key = this->_key_select(value);
        uint32_t hash = this->get_hash(key);
        size_type index = this->get_index(hash);
        int16_t probe_length = 0;

        while(true)
        {
            Bucket& bucket = this->_buckets[index];

            if(bucket.is_empty())
            {
                bucket.set_bucket_content(hash, probe_length, std::forward<value_type>(value));

                this->_items_count++;

                return;
            }

            if(bucket.hash() == hash && this->_key_select(bucket.value()) == key)
            {
                bucket.construct(std::forward<value_type&&>(value));
                return;
            }

            if(probe_length > bucket.probe_length())
            {
                bucket.swap_bucket_content(value, hash, probe_length);
            }

            index = this->get_next_index(index);
            probe_length++;

            if(probe_length >= this->_max_probes)
            {
                this->grow(this->get_new_capacity(), false);
                this->insert_internal(std::forward<value_type>(value));

                return;
            }
        }
    }

    std::pair<iterator, bool> emplace_internal(value_type&& value_to_insert)
    {
        key_type key = this->_key_select(value_to_insert);

        uint32_t hash = this->get_hash(key);
        size_type index = this->get_index(hash);
        int16_t probe_length = 0;

        while(true)
        {
            Bucket& bucket = this->_buckets[index];

            if(bucket.is_empty())
            {
                bucket.set_bucket_content(hash, probe_length, std::move(value_to_insert));
                this->_items_count++;
                return {iterator(this, index), true};
            }

            if(bucket.hash() == hash && this->_key_select(bucket.value()) == key)
            {
                return {iterator(this, index), false};
            }

            if(probe_length > bucket.probe_length())
            {
                bucket.swap_bucket_content(value_to_insert, hash, probe_length);

                key = this->_key_select(value_to_insert);
            }

            index = this->get_next_index(index);
            probe_length++;

            if(probe_length >= this->_max_probes)
            {
                this->grow(this->get_new_capacity(), false);

                hash = this->get_hash(key);
                index = this->get_index(hash);
                probe_length = 0;
            }
        }
    }

    void grow(const size_t new_capacity, bool rehash)
    {
        std::vector<Bucket> old_buckets = std::move(this->_buckets);

        this->_buckets.resize(new_capacity);
        this->_items_count = 0;

        if(rehash)
        {
            this->_hash_key ^= this->generate_hash_key();
        }

        this->_max_probes = static_cast<int16_t>(std::log2(static_cast<float>(new_capacity)));

        for(auto& bucket : old_buckets)
        {
            if(!bucket.is_empty())
            {
                this->insert_internal(std::move(bucket.value()));
            }
        }
    }

public:
    iterator begin()
    {
        return iterator(this, 0);
    }

    iterator end()
    {
        return iterator(this, this->_buckets.size());
    }

    const_iterator begin() const
    {
        return const_iterator(this, 0);
    }

    const_iterator end() const
    {
        return const_iterator(this, this->_buckets.size());
    }

    const_iterator cbegin() const
    {
        return const_iterator(this, 0);
    }

    const_iterator cend() const
    {
        return const_iterator(this, this->_buckets.size());
    }

    explicit HashMap(size_t initial_capacity = INITIAL_CAPACITY, const Hash& hash = Hash())
        : _hash_func(hash), _hash_key(this->generate_hash_key())
    {
        if(initial_capacity == 0)
        {
            initial_capacity = INITIAL_CAPACITY;
        }
        else
        {
            initial_capacity = bit_ceil(initial_capacity + 1);
        }

        this->_buckets.resize(initial_capacity);
        this->_max_probes = static_cast<int16_t>(std::log2(static_cast<float>(initial_capacity)));
    }

    HashMap(std::initializer_list<value_type> init,
            std::size_t initial_capacity = 0,
            const Hash& hash = Hash()) : HashMap(initial_capacity, hash)
    {
        if(init.size() > 0)
        {
            const std::size_t required_buckets = static_cast<std::size_t>(std::ceil(init.size() / MAX_LOAD_FACTOR));
            const std::size_t get_new_capacity = bit_ceil(required_buckets);

            if(get_new_capacity > this->_buckets.size())
            {
                this->grow(get_new_capacity, false);
            }

            for(const auto& item : init)
            {
                this->emplace(item);
            }
        }
    }

    template <typename P>
    void insert(P&& value)
    {
        if(this->load_factor() > MAX_LOAD_FACTOR)
        {
            this->grow(this->get_new_capacity(), false);
        }

        return this->insert_internal(std::forward<P>(value));
    }

    template <typename... Args>
    std::pair<iterator, bool> emplace(Args&&... args)
    {
        if(this->load_factor() > MAX_LOAD_FACTOR)
        {
            this->grow(this->get_new_capacity(), false);
        }

        value_type temp_value(std::forward<Args>(args)...);

        return this->emplace_internal(std::move(temp_value));
    }

    iterator find(const key_type& key)
    {
        uint32_t hash = this->get_hash(key);
        size_type index = this->get_index(hash);
        int16_t probe_length = 0;

        while(true)
        {
            Bucket& bucket = this->_buckets[index];

            if(bucket.is_empty() || probe_length > bucket.probe_length())
            {
                return this->end();
            }

            if(bucket.hash() == hash && this->_key_select(bucket.value()) == key)
            {
                return iterator(this, index);
            }

            index = this->get_next_index(index);
            probe_length++;
        }
    }

    const_iterator find(const key_type& key) const
    {
        uint32_t hash = this->get_hash(key);
        size_type index = this->get_index(hash);
        int16_t probe_length = 0;

        while(true)
        {
            const Bucket& bucket = this->_buckets[index];

            if(bucket.is_empty() || probe_length > bucket.probe_length())
            {
                return this->cend();
            }

            if(bucket.hash() == hash && this->_key_select(bucket.value()) == key)
            {
                return const_iterator(this, index);
            }

            index = this->get_next_index(index);
            probe_length++;
        }
    }

    const_iterator cfind(const key_type& key) const noexcept
    {
        return this->find(key);
    }

    bool contains(const key_type& key) const noexcept
    {
        return this->cfind(key) != this->cend();
    }

    void erase(iterator pos)
    {
        if(pos == this->end())
        {
            return;
        }

        size_type index = pos.index;
        size_type next_index = this->get_next_index(index);

        this->_buckets[index].clear();
        this->_items_count--;

        while(true)
        {
            Bucket& next_bucket = this->_buckets[next_index];

            if(next_bucket.is_empty() || next_bucket.probe_length() == 0)
            {
                return;
            }

            next_bucket.decrement_probe_length();

            this->_buckets[index].construct(std::move(next_bucket.value()));
            this->_buckets[index].set_hash(next_bucket.hash());
            this->_buckets[index].set_probe_length(next_bucket.probe_length());

            next_bucket.clear();

            index = next_index;
            next_index = this->get_next_index(next_index);
        }
    }

    void erase(const key_type& key)
    {
        auto it = this->find(key);

        if(it != this->end())
        {
            this->erase(it);
        }
    }

    void clear()
    {
        for(auto& bucket : this->_buckets)
        {
            bucket.clear();
        }

        this->_items_count = 0;
    }

    size_t size() const
    {
        return this->_items_count;
    }

    size_t capacity() const
    {
        return this->_buckets.size();
    }

    float load_factor() const
    {
        return static_cast<float>(this->_items_count) / static_cast<float>(this->_buckets.size());
    }

    bool empty() const
    {
        return this->_items_count == 0;
    }

    void reserve(size_t new_capacity)
    {
        if(new_capacity == 0)
        {
            return;
        }

        const std::size_t required_buckets = static_cast<std::size_t>(std::ceil(new_capacity / MAX_LOAD_FACTOR));

        if(required_buckets <= this->_buckets.size())
        {
            return;
        }

        new_capacity = bit_ceil(required_buckets);

        this->grow(new_capacity, false);
    }

    mapped_type& operator[](const key_type& key)
    {
        auto it = this->find(key);

        if(it == this->end())
        {
            this->insert(std::pair<key_type, mapped_type>(key, Value{}));

            it = this->find(key);
        }

        return it.value();
    }

    STDROMANO_FORCE_INLINE size_t memory_usage() const noexcept
    {
        return this->capacity() * sizeof(Bucket) + sizeof(size_t) + sizeof(uint32_t) +
               sizeof(uint32_t) + sizeof(KeySelect);
    }
};

STDROMANO_NAMESPACE_END

#endif // !defined(__STDROMANO_HASHMAP)