// SPDX-License-Identifier: BSD-3-Clause 
// Copyright (c) 2025 - Present Romain Augier 
// All rights reserved. 

#pragma once

#if !defined(__STDROMANO_HASHMAP)
#define __STDROMANO_HASHMAP

#include "stdromano/bits.h"
#include "stdromano/vector.h"
#include "stdromano/optional.h"

#include <memory>
#include <functional>
#include <stdexcept>
#include <cmath>
#include <random>

STDROMANO_NAMESPACE_BEGIN

template<class Key, class Value, class Hash = std::hash<Key>>
class HashMap 
{
private:
    static constexpr float MAX_LOAD_FACTOR = 0.9f;
    static constexpr size_t INITIAL_CAPACITY = 1024;

    struct Bucket 
    {
        Optional<Key> key;
        Optional<Value> value;
        uint32_t hash = 0;
        uint16_t probe_length = 0;
        
        bool is_empty() const 
        { 
            return !this->key.has_value(); 
        }

        void clear() 
        { 
            this->key.reset();
            this->value.reset();
            this->hash = 0;
            this->probe_length = 0;
        }
    };

    std::vector<Bucket> buckets;
    size_t items_count = 0;
    uint32_t hash_key;
    uint32_t max_probes;

    inline uint32_t get_hash(const Key& key) const 
    {
        return Hash{}(key) ^ hash_key;
    }

    inline size_t get_index(const uint32_t hash) const 
    {
        return hash & (this->buckets.size() - 1);
    }

    inline size_t get_new_capacity() const 
    {
        return bit_ceil(buckets.size() + 1);
    }

    inline uint32_t generate_hash_key() 
    {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        static std::uniform_int_distribution<uint32_t> dis;
        return dis(gen);
    }

    void move_entry(Bucket& entry, const bool rehash) 
    {
        if(rehash) 
        {
            entry.hash = this->get_hash(entry.key.value());
        }

        entry.probe_length = 0;

        size_t index = this->get_index(entry.hash);
        
        while(true) 
        {
            auto& bucket = this->buckets[index];
            
            if(!bucket.is_empty()) 
            {
                if(entry.probe_length > bucket.probe_length) 
                {
                    std::swap(entry, bucket);
                }
                
                index = (index + 1) & (buckets.size() - 1);
                entry.probe_length++;
            } 
            else 
            {
                bucket = std::move(entry);
                this->items_count++;
                return;
            }
        }
    }

    void grow(size_t new_capacity, bool rehash) 
    {
        std::vector<Bucket> old_buckets = std::move(this->buckets);

        this->buckets.resize(new_capacity);
        this->items_count = 0;
        
        if(rehash) 
        {
            this->hash_key ^= generate_hash_key();
        }

        this->max_probes = static_cast<uint32_t>(std::log2(static_cast<float>(buckets.size())));

        for(auto& bucket : old_buckets) 
        {
            if(!bucket.is_empty()) 
            {
                this->move_entry(bucket, rehash);
            }
        }
    }

public:
    class iterator 
    {
        friend class HashMap;
        HashMap* map;
        size_t index;
        
        void advance() 
        {
            while(this->index < this->map->buckets.size() && 
                  this->map->buckets[index].is_empty()) 
            {
                this->index++;
            }
        }
        
    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = std::pair<const Key&, Value&>;
        using difference_type = std::ptrdiff_t;
        using pointer = value_type*;
        using reference = value_type&;
        
        iterator(HashMap* m, size_t i) : map(m), index(i) { this->advance(); }
        
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
        
        value_type operator*() 
        {
            return { this->map->buckets[index].key.value(), this->map->buckets[index].value.value() };
        }

        const Key& key() const { return this->map->buckets[index].key.value(); }
        Value& value() { return this->map->buckets[index].value.value(); }
    };

    class const_iterator 
    {
        friend class HashMap;
        const HashMap* map;
        size_t index;
        
        void advance() 
        {
            while(this->index < this->map->buckets.size() && 
                  this->map->buckets[index].is_empty()) 
            {
                this->index++;
            }
        }
        
    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = std::pair<const Key&, const Value&>;
        using difference_type = std::ptrdiff_t;
        using pointer = value_type*;
        using reference = value_type&;
        
        const_iterator(const HashMap* m, size_t i) : map(m), index(i) { this->advance(); }
        
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
        
        value_type operator*() const 
        {
            return { this->map->buckets[index].key.value(), this->map->buckets[index].value.value() };
        }

        const Key& key() const { return this->map->buckets[index].key.value(); }
        const Value& value() const { return this->map->buckets[index].value.value(); }
    };

    iterator begin() { return iterator(this, 0); }
    iterator end() { return iterator(this, this->buckets.size()); }
    const_iterator begin() const { return const_iterator(this, 0); }
    const_iterator end() const { return const_iterator(this, this->buckets.size()); }
    const_iterator cbegin() const { return const_iterator(this, 0); }
    const_iterator cend() const { return const_iterator(this, this->buckets.size()); }

    explicit HashMap(size_t initial_capacity = INITIAL_CAPACITY)
        : hash_key(generate_hash_key())
    {
        if (initial_capacity == 0) 
        {
            initial_capacity = INITIAL_CAPACITY;
        } 
        else 
        {
            initial_capacity = bit_ceil(initial_capacity + 1);
        }

        this->buckets.resize(initial_capacity);
        this->max_probes = static_cast<uint32_t>(std::log2(static_cast<float>(initial_capacity)));
    }

    void insert(const Key& key, const Value& value) 
    {
        if (this->load_factor() > MAX_LOAD_FACTOR) 
        {
            this->grow(this->get_new_capacity(), false);
        }

        uint32_t hash = this->get_hash(key);
        size_t index = this->get_index(hash);

        Bucket entry;
        entry.key = key;
        entry.value = value;
        entry.hash = hash;
        entry.probe_length = 0;

        while (true) 
        {
            auto& bucket = this->buckets[index];

            if (!bucket.is_empty()) 
            {
                if (bucket.hash == hash && bucket.key.value() == key) 
                {
                    return;
                }

                if (entry.probe_length > bucket.probe_length) 
                {
                    std::swap(entry, bucket);
                }

                index = (index + 1) & (this->buckets.size() - 1);
                entry.probe_length++;

                if (entry.probe_length >= this->max_probes) 
                {
                    this->grow(this->get_new_capacity(), false);
                    this->insert(entry.key.value(), entry.value.value());
                    return;
                }
            } 
            else 
            {
                bucket = std::move(entry);
                this->items_count++;
                return;
            }
        }
    }

    iterator find(const Key& key) 
    {
        const uint32_t hash = this->get_hash(key);
        size_t index = this->get_index(hash);
        uint16_t probe_length = 0;

        while(true) 
        {
            auto& bucket = this->buckets[index];

            if(bucket.is_empty() || probe_length > bucket.probe_length) 
            {
                return this->end();
            }

            if(bucket.hash == hash && bucket.key.value() == key) 
            {
                return iterator(this, index);
            }

            index = (index + 1) & (this->buckets.size() - 1);
            probe_length++;
        }
    }

    const_iterator find(const Key& key) const 
    {
        const uint32_t hash = this->get_hash(key);
        size_t index = this->get_index(hash);
        uint16_t probe_length = 0;

        while(true) 
        {
            const auto& bucket = this->buckets[index];

            if(bucket.is_empty() || probe_length > bucket.probe_length) 
            {
                return this->cend();
            }

            if(bucket.hash == hash && bucket.key.value() == key) 
            {
                return const_iterator(this, index);
            }

            index = (index + 1) & (this->buckets.size() - 1);
            probe_length++;
        }
    }

    const_iterator cfind(const Key& key) const
    {
        return this->find(key);
    }

    void erase(const Key& key) 
    {
        const uint32_t hash = this->get_hash(key);
        size_t index = this->get_index(hash);
        uint16_t probe_length = 0;

        while(true) 
        {
            auto& bucket = this->buckets[index];

            if(bucket.is_empty() || probe_length > bucket.probe_length) 
            {
                return;
            }

            if(bucket.hash == hash && bucket.key.value() == key) 
            {
                bucket.clear();
                this->items_count--;

                while(true) 
                {
                    const size_t next_index = (index + 1) & (buckets.size() - 1);
                    auto& next_bucket = buckets[next_index];

                    if(next_bucket.is_empty() || next_bucket.probe_length == 0) 
                    {
                        return;
                    }

                    next_bucket.probe_length--;
                    this->buckets[index] = std::move(next_bucket);
                    index = next_index;
                }
            }

            index = (index + 1) & (this->buckets.size() - 1);
            probe_length++;
        }
    }

    size_t size() const 
    { 
        return this->items_count;
    }

    size_t capacity() const 
    { 
        return this->buckets.size();
    }

    float load_factor() const
    {
        return static_cast<float>(this->items_count) / static_cast<float>(this->buckets.size());
    }

    bool empty() const 
    { 
        return this->items_count == 0; 
    }

    void clear() 
    {
        this->buckets.clear();
        this->buckets.resize(INITIAL_CAPACITY);
        this->items_count = 0;
    }

    void reserve(size_t new_capacity) 
    {
        if(new_capacity <= this->buckets.size()) 
        {
            return;
        }
        
        new_capacity = bit_ceil(new_capacity + 1);
        
        this->grow(new_capacity, false);
    }

    Value& operator[](const Key& key) 
    {
        auto it = this->find(key);

        if(it == this->end()) 
        {
            this->insert(key, Value{});
            it = this->find(key);
        }

        return it.value();
    }
};

STDROMANO_NAMESPACE_END

#endif // !defined(__STDROMANO_HASHMAP)