// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - Present Romain Augier
// All rights reserved.

#pragma once

#if !defined(__STDROMANO_HASHCONTAINER)
#define __STDROMANO_HASHCONTAINER

#include "stdromano/bits.hpp"
#include "stdromano/random.hpp"

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>

STDROMANO_NAMESPACE_BEGIN

DETAIL_NAMESPACE_BEGIN

template <class K, class V>
struct PairKeySelect
{
    using key_type = K;

    const K& operator()(const std::pair<K, V>& kv) const noexcept { return kv.first; }
    K& operator()(std::pair<K, V>& kv) noexcept { return kv.first; }
};

template <class K>
struct IdentityKeySelect
{
    using key_type = K;

    const K& operator()(const K& k) const noexcept { return k; }
    K& operator()(K& k) noexcept { return k; }
};

DETAIL_NAMESPACE_END

template <class Key, class T, class KeySelect, class Hash = std::hash<Key>>
class HashContainer
{
public:
    static constexpr float MAX_LOAD_FACTOR = 0.9f;
    static constexpr std::size_t INITIAL_CAPACITY = 8;

    using key_type = Key;
    using value_type = T;
    using size_type = std::size_t;
    using hasher = Hash;

protected:
    struct Bucket
    {
        static constexpr std::int16_t EMPTY_BUCKET_MARKER = -1;

    private:
        std::uint32_t _hash;
        std::int16_t _probe_length;

        alignas(value_type) unsigned char _storage[sizeof(value_type)];

    public:
        Bucket() noexcept : _hash(0),
                            _probe_length(EMPTY_BUCKET_MARKER)
        {
            STDROMANO_ASSERT(this->is_empty(), "Bucket should be empty on default initialisation");
        }

        Bucket(const Bucket& other) noexcept(std::is_nothrow_copy_constructible<value_type>::value) : _hash(0),
                                                                                                      _probe_length(EMPTY_BUCKET_MARKER)
        {
            if(!other.is_empty())
            {
                ::new(static_cast<void*>(std::addressof(this->_storage))) value_type(other.value());

                this->_hash = other._hash;
                this->_probe_length = other._probe_length;
            }
        }

        Bucket(Bucket&& other) noexcept(std::is_nothrow_move_constructible<value_type>::value) : _hash(0),
                                                                                                 _probe_length(EMPTY_BUCKET_MARKER)
        {
            if(!other.is_empty())
            {
                ::new(static_cast<void*>(std::addressof(this->_storage))) value_type(std::move(other.value()));

                this->_hash = other._hash;
                this->_probe_length = other._probe_length;
            }
        }

        Bucket& operator=(const Bucket& other) noexcept(std::is_nothrow_copy_assignable<value_type>::value)
        {
            if(this != &other)
            {
                this->clear();

                if(!other.is_empty())
                {
                    ::new(static_cast<void*>(std::addressof(this->_storage))) value_type(other.value());

                    this->_hash = other._hash;
                    this->_probe_length = other._probe_length;
                }
            }

            return *this;
        }

        Bucket& operator=(Bucket&& other) = delete;

        ~Bucket() noexcept { this->clear(); }

        template <typename... Args>
        void construct(Args&&... args)
        {
            ::new(static_cast<void*>(std::addressof(this->_storage))) value_type(std::forward<Args>(args)...);
        }

        void swap_bucket_content(value_type& value, std::uint32_t& hash, std::int16_t& probe_length)
        {
            STDROMANO_ASSERT(!this->is_empty(), "Cannot swap an empty bucket");
            STDROMANO_ASSERT(probe_length > this->_probe_length,
                             "Cannot swap a bucket with a lower probe_length");

            std::swap(value, this->value());
            std::swap(hash, this->_hash);
            std::swap(probe_length, this->_probe_length);
        }

        template <typename... Args>
        void set_bucket_content(const std::uint32_t hash,
                                const std::int16_t probe_length,
                                Args&&... value_args)
        {
            STDROMANO_ASSERT(this->is_empty(), "Cannot set content of non empty bucket");
            STDROMANO_ASSERT(probe_length >= 0,
                             "Cannot set content of bucket with uninitialized probe_length");

            ::new(static_cast<void*>(std::addressof(this->_storage))) value_type(std::forward<Args>(value_args)...);

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

        STDROMANO_FORCE_INLINE std::int16_t probe_length() const noexcept { return this->_probe_length; }
        STDROMANO_FORCE_INLINE void reset_probe_length() noexcept { this->_probe_length = 0; }
        STDROMANO_FORCE_INLINE void set_probe_length(std::int16_t probe_length) noexcept
        {
            this->_probe_length = probe_length;
        }
        STDROMANO_FORCE_INLINE void increment_probe_length() noexcept { this->_probe_length++; }
        STDROMANO_FORCE_INLINE void decrement_probe_length() noexcept { this->_probe_length--; }

        STDROMANO_FORCE_INLINE std::uint32_t hash() const noexcept { return this->_hash; }
        STDROMANO_FORCE_INLINE void set_hash(const std::uint32_t hash) noexcept { this->_hash = hash; }

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
    std::size_t _items_count = 0;
    std::size_t _grow_threshold = 0;
    std::uint32_t _hash_key;
    std::int16_t _max_probes;

    KeySelect _key_select;

    STDROMANO_FORCE_INLINE std::uint32_t get_hash(const key_type& key) const
    {
        std::uint64_t h = static_cast<std::uint64_t>(this->_hash_func(key)) ^ this->_hash_key;
        h ^= h >> 33;
        h *= 0xff51afd7ed558ccdULL;
        h ^= h >> 33;
        h *= 0xc4ceb9fe1a85ec53ULL;
        h ^= h >> 33;
        return static_cast<std::uint32_t>(h);
    }

    STDROMANO_FORCE_INLINE std::size_t get_index(const std::uint32_t hash) const
    {
        return hash & (this->_buckets.size() - 1);
    }

    STDROMANO_FORCE_INLINE std::size_t get_next_index(const std::size_t index) const
    {
        return (index + 1) & (this->_buckets.size() - 1);
    }

    STDROMANO_FORCE_INLINE std::size_t get_new_capacity() const
    {
        return bit_ceil(this->_buckets.size() + 1);
    }

    STDROMANO_FORCE_INLINE std::uint32_t generate_hash_key() { return next_random_uint32(); }

    STDROMANO_FORCE_INLINE void update_grow_threshold()
    {
        this->_grow_threshold = static_cast<std::size_t>(static_cast<float>(this->_buckets.size()) * MAX_LOAD_FACTOR);
    }

    STDROMANO_FORCE_INLINE bool needs_grow() const
    {
        return this->_items_count >= this->_grow_threshold;
    }

    void insert_internal(value_type&& value)
    {
        key_type key = this->_key_select(value);
        std::uint32_t hash = this->get_hash(key);
        size_type index = this->get_index(hash);
        std::int16_t probe_length = 0;

        while(true)
        {
            Bucket& bucket = this->_buckets[index];

            if(bucket.is_empty())
            {
                bucket.set_bucket_content(hash, probe_length, std::move(value));
                this->_items_count++;
                return;
            }

            // Match std semantics: insert does not overwrite an existing key
            if(bucket.hash() == hash && this->_key_select(bucket.value()) == key)
                return;

            if(probe_length > bucket.probe_length())
            {
                bucket.swap_bucket_content(value, hash, probe_length);
                key = this->_key_select(value);
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

    template <class Iterator>
    std::pair<Iterator, bool> emplace_internal(value_type&& value_to_insert, Iterator /*tag*/)
    {
        key_type key = this->_key_select(value_to_insert);

        std::uint32_t hash = this->get_hash(key);
        size_type index = this->get_index(hash);
        std::int16_t probe_length = 0;

        while(true)
        {
            Bucket& bucket = this->_buckets[index];

            if(bucket.is_empty())
            {
                bucket.set_bucket_content(hash, probe_length, std::move(value_to_insert));
                this->_items_count++;
                return {Iterator(this, index), true};
            }

            if(bucket.hash() == hash && this->_key_select(bucket.value()) == key)
                return {Iterator(this, index), false};

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

    void grow(const std::size_t new_capacity, bool rehash)
    {
        std::vector<Bucket> old_buckets = std::move(this->_buckets);

        this->_buckets.resize(new_capacity);
        this->_items_count = 0;

        if(rehash)
            this->_hash_key ^= this->generate_hash_key();

        this->_max_probes = static_cast<int16_t>(std::log2(static_cast<float>(new_capacity)));
        this->update_grow_threshold();

        for(auto& bucket : old_buckets)
            if(!bucket.is_empty())
                this->insert_internal(std::move(bucket.value()));
    }

public:
    class const_iterator;

    class iterator
    {
        friend class HashContainer;
        HashContainer* map;
        std::size_t index;

        void advance()
        {
            while(this->index < this->map->_buckets.size() &&
                  this->map->_buckets[this->index].is_empty())
                this->index++;
        }

    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = T;
        using difference_type = std::ptrdiff_t;
        using pointer = value_type*;
        using reference = value_type&;

        iterator(HashContainer* m, std::size_t i) : map(m),
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

        bool operator==(const iterator& other) const noexcept
        {
            return this->index == other.index && this->map == other.map;
        }

        bool operator!=(const iterator& other) const
        {
            return !(*this == other);
        }

        bool operator==(const const_iterator& other) const noexcept
        {
            return this->index == other.index && this->map == other.map;
        }

        bool operator!=(const const_iterator& other) const noexcept
        {
            return !(*this == other);
        }

        reference operator*() const
        {
            return this->map->_buckets[this->index].value();
        }

        pointer operator->() const
        {
            return std::addressof(this->map->_buckets[this->index].value());
        }

        const typename KeySelect::key_type& key() const
        {
            return KeySelect()(this->map->_buckets[this->index].value());
        }
    };

    class const_iterator
    {
        friend class HashContainer;
        const HashContainer* map;
        std::size_t index;

        void advance()
        {
            while(this->index < this->map->_buckets.size() &&
                  this->map->_buckets[this->index].is_empty())
                this->index++;
        }

    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = T;
        using difference_type = std::ptrdiff_t;
        using pointer = const value_type*;
        using reference = const value_type&;

        const_iterator(const HashContainer* m, std::size_t i) : map(m),
                                                                index(i)
        {
            this->advance();
        }

        const_iterator(const iterator& other) : map(other.map), index(other.index) {}

        const_iterator& operator++()
        {
            this->index++;
            this->advance();
            return *this;
        }

        bool operator==(const const_iterator& other) const noexcept
        {
            return this->index == other.index && this->map == other.map;
        }

        bool operator!=(const const_iterator& other) const noexcept
        {
            return !(*this == other);
        }

        bool operator==(const iterator& other) const noexcept
        {
            return this->index == other.index && this->map == other.map;
        }

        bool operator!=(const iterator& other) const noexcept
        {
            return !(*this == other);
        }

        reference operator*() const
        {
            return this->map->_buckets[this->index].value();
        }

        pointer operator->() const
        {
            return std::addressof(this->map->_buckets[this->index].value());
        }

        const typename KeySelect::key_type& key() const
        {
            return KeySelect()(this->map->_buckets[this->index].value());
        }
    };

protected:
    explicit HashContainer(std::size_t initial_capacity = INITIAL_CAPACITY,
                           const Hash& hash = Hash()) : _hash_func(hash),
                                                        _hash_key(this->generate_hash_key())
    {
        if(initial_capacity == 0)
            initial_capacity = INITIAL_CAPACITY;
        else
            initial_capacity = bit_ceil(initial_capacity + 1);

        this->_buckets.resize(initial_capacity);
        this->_max_probes = static_cast<std::int16_t>(std::log2(static_cast<float>(initial_capacity)));
        this->update_grow_threshold();
    }

    HashContainer(const HashContainer& other) = default;

    HashContainer& operator=(const HashContainer& other) = default;

    HashContainer(HashContainer&& other) noexcept : _hash_func(std::move(other._hash_func)),
                                                    _buckets(std::move(other._buckets)),
                                                    _items_count(other._items_count),
                                                    _grow_threshold(other._grow_threshold),
                                                    _hash_key(other._hash_key),
                                                    _max_probes(other._max_probes),
                                                    _key_select(std::move(other._key_select))
    {
        other._items_count = 0;
        other._grow_threshold = 0;
    }

    HashContainer& operator=(HashContainer&& other) noexcept
    {
        if(this != &other)
        {
            this->_hash_func = std::move(other._hash_func);
            this->_buckets = std::move(other._buckets);
            this->_items_count = other._items_count;
            this->_grow_threshold = other._grow_threshold;
            this->_hash_key = other._hash_key;
            this->_max_probes = other._max_probes;
            this->_key_select = std::move(other._key_select);

            other._items_count = 0;
            other._grow_threshold = 0;
        }

        return *this;
    }

public:
    iterator begin() { return iterator(this, 0); }
    iterator end() { return iterator(this, this->_buckets.size()); }
    const_iterator begin() const { return const_iterator(this, 0); }
    const_iterator end() const { return const_iterator(this, this->_buckets.size()); }
    const_iterator cbegin() const { return const_iterator(this, 0); }
    const_iterator cend() const { return const_iterator(this, this->_buckets.size()); }

    template <typename... Args>
    std::pair<iterator, bool> emplace(Args&&... args)
    {
        if(this->needs_grow())
            this->grow(this->get_new_capacity(), false);

        value_type temp_value(std::forward<Args>(args)...);
        return this->emplace_internal(std::move(temp_value), iterator(this, 0));
    }

    iterator find(const key_type& key)
    {
        std::uint32_t hash = this->get_hash(key);
        size_type index = this->get_index(hash);
        std::int16_t probe_length = 0;

        while(true)
        {
            Bucket& bucket = this->_buckets[index];

            if(bucket.is_empty() || probe_length > bucket.probe_length())
                return this->end();

            if(bucket.hash() == hash && this->_key_select(bucket.value()) == key)
                return iterator(this, index);

            index = this->get_next_index(index);
            probe_length++;
        }
    }

    const_iterator find(const key_type& key) const
    {
        std::uint32_t hash = this->get_hash(key);
        size_type index = this->get_index(hash);
        std::int16_t probe_length = 0;

        while(true)
        {
            const Bucket& bucket = this->_buckets[index];

            if(bucket.is_empty() || probe_length > bucket.probe_length())
                return this->cend();

            if(bucket.hash() == hash && this->_key_select(bucket.value()) == key)
                return const_iterator(this, index);

            index = this->get_next_index(index);
            probe_length++;
        }
    }

    const_iterator cfind(const key_type& key) const noexcept { return this->find(key); }

    bool contains(const key_type& key) const noexcept
    {
        return this->cfind(key) != this->cend();
    }

    void erase(iterator pos)
    {
        if(pos == this->end())
            return;

        size_type index = pos.index;
        size_type next_index = this->get_next_index(index);

        this->_buckets[index].clear();
        this->_items_count--;

        while(true)
        {
            Bucket& next_bucket = this->_buckets[next_index];

            if(next_bucket.is_empty() || next_bucket.probe_length() == 0)
                return;

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
            this->erase(it);
    }

    void clear()
    {
        for(auto& bucket : this->_buckets)
            bucket.clear();

        this->_items_count = 0;
    }

    std::size_t size() const { return this->_items_count; }

    std::size_t capacity() const { return this->_buckets.size(); }

    bool empty() const { return this->_items_count == 0; }

    float load_factor() const
    {
        return static_cast<float>(this->_items_count) / static_cast<float>(this->_buckets.size());
    }

    void reserve(std::size_t new_capacity)
    {
        if(new_capacity == 0)
            return;

        const std::size_t required_buckets = static_cast<std::size_t>(std::ceil(new_capacity / MAX_LOAD_FACTOR));

        if(required_buckets <= this->_buckets.size())
            return;

        new_capacity = bit_ceil(required_buckets);
        this->grow(new_capacity, false);
    }

    STDROMANO_FORCE_INLINE std::size_t memory_usage() const noexcept
    {
        return this->capacity() * sizeof(Bucket) + sizeof(std::size_t) + sizeof(std::uint32_t) + sizeof(std::uint32_t) + sizeof(KeySelect);
    }
};

STDROMANO_NAMESPACE_END

#endif // !defined(__STDROMANO_HASHCONTAINER)
