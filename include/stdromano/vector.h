// SPDX-License-Identifier: BSD-3-Clause 
// Copyright (c) 2025 - Present Romain Augier 
// All rights reserved. 

#pragma once

#if !defined(__STDROMANO_VECTOR)
#define __STDROMANO_VECTOR

#include "stdromano/memory.h"

#include <functional>

STDROMANO_NAMESPACE_BEGIN

template<typename T, typename Alignment = std::alignment_of<T>>
class Vector 
{
    static constexpr float GROW_RATE = 1.61f;

    struct Header 
    {
        size_t capacity;
        size_t size;
        alignas(Alignment) unsigned char data[];
    };

private:
    Header* _header = nullptr;

    STDROMANO_FORCE_INLINE Header* header() noexcept 
    { 
        return this->_header; 
    }
    
    STDROMANO_FORCE_INLINE const Header* header() const noexcept 
    { 
        return this->_header; 
    }

    STDROMANO_FORCE_INLINE void set_capacity(const size_t capacity) noexcept 
    { 
        this->_header->capacity = capacity; 
    }
    
    STDROMANO_FORCE_INLINE void set_size(const size_t size) noexcept 
    { 
        this->_header->size = size; 
    }

    STDROMANO_FORCE_INLINE void incr_size() noexcept { this->_header->size++; }
    STDROMANO_FORCE_INLINE void decr_size() noexcept { this->_header->size--; }

    static Header* allocate(const size_t capacity) 
    {
        constexpr size_t header_size = sizeof(Header);
        const size_t aligned_array_size = (capacity * sizeof(T) + Alignment::value - 1) & ~(Alignment::value - 1);
        const size_t total_size = header_size + aligned_array_size;
        
        void* memory = mem_alloc(total_size);

        if(!memory) 
        {
            return nullptr;
        }
        
        Header* header = static_cast<Header*>(memory);
        header->capacity = capacity;
        header->size = 0;
        
        return header;
    }

    static void deallocate(Header* header) 
    {
        if(header) 
        {
            mem_free(header);
        }
    }

    STDROMANO_FORCE_INLINE void grow() noexcept { this->resize(static_cast<size_t>(static_cast<float>(this->capacity()) * GROW_RATE)); }

public:
    using value_type = T;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using reference = value_type&;
    using const_reference = const value_type&;
    using pointer = value_type*;
    using const_pointer = const value_type*;

    Vector(const size_t initial_capacity = 0)
    {
        const size_t capacity = initial_capacity == 0 ? 128 : initial_capacity;
        this->_header = Vector::allocate(capacity);
    }

    ~Vector()
    {
        if(this->_header != nullptr)
        {
            this->clear();
            this->deallocate(_header);
        }

        this->_header = nullptr;
    }

    Vector(const Vector<T, Alignment>& other)
    {
        if(other._header == nullptr)
        {
            this->_header = nullptr;
            return;
        }

        this->_header = Vector::allocate(other.capacity());

        for(size_t i = 0; i < other.size(); i++) 
        {
            ::new (this->at(i)) T(other[i]);
        }

        this->set_size(other.size());
    }

    Vector(Vector<T, Alignment>&& other) noexcept
    {
        this->_header = other._header;
        other._header = nullptr;
    }

    Vector& operator=(const Vector<T, Alignment>& other)
    {
        if(other._header == nullptr)
        {
            this->_header = nullptr;
            return *this;
        }

        this->_header = Vector::allocate(other.capacity());

        for(size_t i = 0; i < other.size(); i++) 
        {
            ::new (this->at(i)) T(other[i]);
        }

        this->set_size(other.size());

        return *this;
    }

    Vector& operator=(Vector<T, Alignment>&& other) noexcept
    {
        this->_header = other._header;
        other._header = nullptr;

        return *this;
    }

    STDROMANO_FORCE_INLINE size_t capacity() const noexcept { return _header != nullptr ? _header->capacity : 0; }
    STDROMANO_FORCE_INLINE size_t size() const noexcept { return _header != nullptr ? _header->size : 0; }
    STDROMANO_FORCE_INLINE bool empty() const noexcept { return this->size() == 0; }

    class iterator
    {
        friend class Vector;
        Vector* vector;
        size_t index;

        void advance()
        {
            if(this->index >= this->vector->size())
            {
                this->index = this->vector->size();
            }
        }

    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = T;
        using difference_type = std::ptrdiff_t;
        using pointer = T*;
        using reference = T&;

        iterator(Vector* v, size_t i) : vector(v), index(i) { this->advance(); }

        iterator& operator++()
        {
            this->index++;
            this->advance();
            return *this;
        }

        bool operator==(const iterator& other) const
        {
            return this->index == other.index && this->vector == other.vector;
        }

        bool operator!=(const iterator& other) const
        {
            return !(*this == other);
        }

        reference operator*()
        {
            return (*this->vector)[this->index];
        }

        pointer operator->()
        {
            return &(*this->vector)[this->index];
        }
    };

    class const_iterator
    {
        friend class Vector;
        const Vector* vector;
        size_t index;

        void advance()
        {
            if(this->index >= this->vector->size())
            {
                this->index = this->vector->size();
            }
        }

    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = T;
        using difference_type = std::ptrdiff_t;
        using pointer = const T*;
        using reference = const T&;

        const_iterator(const Vector* v, size_t i) : vector(v), index(i) { this->advance(); }

        const_iterator& operator++()
        {
            this->index++;
            this->advance();
            return *this;
        }

        bool operator==(const const_iterator& other) const
        {
            return this->index == other.index && this->vector == other.vector;
        }

        bool operator!=(const const_iterator& other) const
        {
            return !(*this == other);
        }

        reference operator*() const
        {
            return (*this->vector)[this->index];
        }

        pointer operator->() const
        {
            return &(*this->vector)[this->index];
        }
    };

    iterator begin() { return iterator(this, 0); }
    iterator end() { return iterator(this, this->size()); }
    const_iterator cbegin() { return const_iterator(this, 0); }
    const_iterator cend() { return const_iterator(this, this->size()); }

    STDROMANO_FORCE_INLINE T* at(const size_t index) noexcept { return this->data() + index; }
    STDROMANO_FORCE_INLINE const T* at(const size_t index) const noexcept { return this->data() + index; }

    STDROMANO_FORCE_INLINE T& operator[](const size_t i) noexcept { return *this->at(i); }
    STDROMANO_FORCE_INLINE const T& operator[](const size_t i) const noexcept { return *this->at(i); }

    STDROMANO_FORCE_INLINE T* data() noexcept { return reinterpret_cast<T*>(this->_header->data); }
    STDROMANO_FORCE_INLINE const T* data() const noexcept { return reinterpret_cast<const T*>(this->_header->data); }

    STDROMANO_FORCE_INLINE T& front() { return this->operator[](0); }
    STDROMANO_FORCE_INLINE const T& front() const { return this->operator[](0); }

    STDROMANO_FORCE_INLINE T& back() { return this->operator[](this->size() - 1); }
    STDROMANO_FORCE_INLINE const T& back() const { return this->operator[](this->size() - 1); }

    void resize(const size_t new_capacity, bool force = false) noexcept
    {
        const size_t old_capacity = this->capacity();

        if(!force && (new_capacity == 0 || new_capacity < old_capacity))
        {
            return;
        }

        Header* new_header = Vector<T, Alignment>::allocate(new_capacity);

        if(!new_header) 
        {
            return;
        }

        if(_header) 
        {
            new_header->size = _header->size;
            
            for(size_t i = 0; i < this->size(); ++i) 
            {
                new (this->data() + i) T(std::move((*this)[i]));
                (*this)[i].~T();
            }
            
            Vector<T, Alignment>::deallocate(_header);
        }

        _header = new_header;
    }

    STDROMANO_FORCE_INLINE void reserve(const size_t new_capacity) noexcept { this->resize(new_capacity); }

    void push_back(const T& element) noexcept
    {
        if(this->size() == this->capacity()) this->grow();

        ::new(this->data() + this->size()) T(element);

        this->incr_size();
    }

    void push_back(T&& element) noexcept
    {
        if(this->size() == this->capacity()) this->grow();

        ::new(this->data() + this->size()) T(std::move(element));

        this->incr_size();
    }
    
    template<typename ...Args>
    T& emplace_back(Args&&... args) noexcept
    {
        if(this->size() == this->capacity()) this->grow();

        T* ptr = ::new(this->data() + this->size()) T(std::forward<Args&&>(args)...);

        this->incr_size();

        return *ptr;
    }

    template<typename ...Args>
    T& emplace_at(const size_t position, Args&&... args) noexcept
    {
        if(this->size() == this->capacity()) this->grow();

        std::memmove(this->at(position + 1), this->at(position), (this->size() - position) * sizeof(T));

        T* ptr = ::new(this->at(position)) T(std::forward<Args&&>(args)...);

        this->incr_size();

        return *ptr;
    }

    void insert(const T& element, const size_t position) noexcept
    {
        if(this->size() == this->capacity()) this->grow();

        std::memmove(this->at(position + 1), this->at(position), (this->size() - position) * sizeof(T));

        ::new(this->at(position)) T(element);

        this->incr_size();
    }

    void insert(T&& element, const size_t position) noexcept
    {
        if(this->size() == this->capacity()) this->grow();

        std::memmove(this->at(position + 1), this->at(position), (this->size() - position) * sizeof(T));

        ::new(this->at(position)) T(std::move(element));

        this->incr_size();
    }

    void remove(const size_t position) noexcept
    {
        this->at(position)->~T();

        std::memmove(this->at(position), this->at(position + 1), (this->size() - position) * sizeof(T));

        this->decr_size();
    }

    T pop(const int64_t position = -1) noexcept
    {
        this->decr_size();

        if(position < 0)
        {
            T object = std::move(this->operator[](this->size() - 1));
            this->operator[](this->size() - 1).~T();

            return object;
        }
        else
        {
            T object = std::move(this->operator[](position));
            this->operator[](position).~T();
            std::memmove(this->at(position), this->at(position + 1), (this->size() - position + 1) * sizeof(T));

            return object;
        }
    }

    iterator find(const T& other) noexcept
    {
        for(size_t i = 0; i < this->size(); i++)
        {
            if(other == this->operator[](i)) return iterator(this, i);
        }

        return this->end();
    }

    const_iterator find(const T& other) const noexcept
    {
        for(size_t i = 0; i < this->size(); i++)
        {
            if(other == this->operator[](i)) return const_iterator(this, i);
        }

        return this->cend();
    }

    STDROMANO_FORCE_INLINE void shrink_to_fit() noexcept { this->resize(this->size(), true); }

    void clear() noexcept
    {
        for(size_t i = 0; i < this->size(); i++)
        {
            this->operator[](i).~T();
        }

        this->set_size(0);
    }

    template<typename F>
    STDROMANO_FORCE_INLINE void sort(F&& cmp) noexcept
    {
        std::qsort(this->data(), this->size(), sizeof(T), std::forward<F&&>(cmp));
    }
};

STDROMANO_NAMESPACE_END

#endif // !defined(__STDROMANO_VECTOR)