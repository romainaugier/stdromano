// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - Present Romain Augier
// All rights reserved.

#pragma once

#if !defined(__STDROMANO_VECTOR)
#define __STDROMANO_VECTOR

#include "stdromano/memory.h"

#include <algorithm>
#include <cstdint>
#include <functional>

STDROMANO_NAMESPACE_BEGIN

template <typename T>
class Vector
{
    static constexpr float GROW_RATE = 1.61803398875f;
    static constexpr uint32_t MIN_SIZE = 128;

private:
    T* _data = nullptr;
    uint32_t _capacity = 0;
    uint32_t _size = 0;

    STDROMANO_FORCE_INLINE void set_capacity(const uint32_t capacity) noexcept
    {
        this->_capacity = capacity;
    }

    STDROMANO_FORCE_INLINE void set_size(const uint32_t size) noexcept
    {
        this->_size = size;
    }

    STDROMANO_FORCE_INLINE void incr_size() noexcept
    {
        this->_size++;
    }

    STDROMANO_FORCE_INLINE void decr_size() noexcept
    {
        this->_size--;
    }

    static T* allocate(const uint32_t capacity)
    {
        if(capacity == 0)
        {
            return nullptr;
        }

        void* memory = mem_aligned_alloc(capacity * sizeof(T), std::alignment_of<T>::value);

        if(memory == nullptr)
        {
            return nullptr;
        }

        return static_cast<T*>(memory);
    }

    static void deallocate(T* data)
    {
        if(data != nullptr)
        {
            mem_aligned_free(data);
        }
    }

    STDROMANO_FORCE_INLINE void grow() noexcept
    {
        if(this->_data == nullptr)
        {
            this->resize(MIN_SIZE);
        }
        else
        {
            this->resize(static_cast<uint32_t>(static_cast<float>(this->capacity()) * GROW_RATE));
        }
    }

public:
    using value_type = T;
    using size_type = uint32_t;
    using difference_type = int32_t;
    using reference = value_type&;
    using const_reference = const value_type&;
    using pointer = value_type*;
    using const_pointer = const value_type*;

    Vector()
        : _data(nullptr),
          _capacity(0),
          _size(0)
    {
    }

    Vector(const uint32_t initial_capacity)
    {
        const uint32_t capacity = initial_capacity == 0 ? MIN_SIZE : initial_capacity;
        this->_data = Vector::allocate(capacity);
        this->set_capacity(capacity);
        this->set_size(0);
    }

    Vector(const uint32_t count, const T& value)
    {
        if(count == 0)
        {
            this->_data = Vector::allocate(MIN_SIZE);
            this->set_capacity(MIN_SIZE);
            this->set_size(0);
            return;
        }

        this->_data = Vector::allocate(count);
        this->set_capacity(count);

        if(this->_data)
        {
            for(uint32_t i = 0; i < count; ++i)
            {
                ::new(this->_data + i) T(value);
            }

            this->set_size(count);
        }
    }

    ~Vector()
    {
        if(this->_data != nullptr)
        {
            this->clear();
            Vector::deallocate(this->_data);
        }

        this->_data = nullptr;
        this->_capacity = 0;
        this->_size = 0;
    }

    Vector(const Vector<T>& other)
    {
        if(other._data == nullptr)
        {
            this->_data = nullptr;
            this->_capacity = 0;
            this->_size = 0;
            return;
        }

        this->_data = Vector::allocate(other.capacity());
        this->set_capacity(other.capacity());

        if(this->_data)
        {
            for(uint32_t i = 0; i < other.size(); i++)
            {
                ::new(this->data() + i) T(other[i]);
            }

            this->set_size(other.size());
        }
    }

    Vector(Vector<T>&& other) noexcept
    {
        this->_data = other._data;
        this->_capacity = other._capacity;
        this->_size = other._size;

        other._data = nullptr;
        other._capacity = 0;
        other._size = 0;
    }

    Vector& operator=(const Vector<T>& other)
    {
        if(this != &other)
        {
            if(this->_data != nullptr)
            {
                this->clear();
                Vector::deallocate(this->_data);
            }

            this->_data = nullptr;
            this->_capacity = 0;
            this->_size = 0;

            if(other._data != nullptr)
            {
                this->_data = Vector::allocate(other.capacity());
                this->set_capacity(other.capacity());

                if(this->_data)
                {
                    for(uint32_t i = 0; i < other.size(); i++)
                    {
                        ::new(this->data() + i) T(other[i]);
                    }

                    this->set_size(other.size());
                }
            }
        }
        return *this;
    }

    Vector& operator=(Vector<T>&& other) noexcept
    {
        if(this != &other)
        {
            if(this->_data != nullptr)
            {
                this->clear();
                Vector::deallocate(this->_data);
            }

            this->_data = other._data;
            this->_capacity = other._capacity;
            this->_size = other._size;

            other._data = nullptr;
            other._capacity = 0;
            other._size = 0;
        }
        return *this;
    }

    STDROMANO_FORCE_INLINE uint32_t capacity() const noexcept
    {
        return this->_capacity;
    }

    STDROMANO_FORCE_INLINE uint32_t size() const noexcept
    {
        return this->_size;
    }

    STDROMANO_FORCE_INLINE bool empty() const noexcept
    {
        return this->size() == 0;
    }

    class iterator
    {
        friend class Vector;
        Vector* vector;
        uint32_t index;

        void advance()
        {
            if(this->index >= this->vector->size())
                this->index = this->vector->size();
        }

    public:
        using iterator_category = std::random_access_iterator_tag;
        using value_type = T;
        using difference_type = int32_t;
        using pointer = T*;
        using reference = T&;

        iterator(Vector* v, uint32_t i)
            : vector(v),
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

        const reference operator*() const
        {
            return (*this->vector)[this->index];
        }

        pointer operator->()
        {
            return &(*this->vector)[this->index];
        }

        const pointer operator->() const
        {
            return &(*this->vector)[this->index];
        }

        iterator& operator--()
        {
            this->index--;
            return *this;
        }

        iterator operator+(difference_type n) const
        {
            return iterator(this->vector, this->index + n);
        }

        iterator operator-(difference_type n) const
        {
            return iterator(this->vector, this->index - n);
        }

        difference_type operator-(const iterator& other) const
        {
            return static_cast<difference_type>(this->index - other.index);
        }

        iterator& operator+=(difference_type n)
        {
            this->index += n;
            this->advance();
            return *this;
        }

        iterator& operator-=(difference_type n)
        {
            this->index -= n;
            this->advance();
            return *this;
        }

        reference operator[](difference_type n)
        {
            return (*this->vector)[this->index + n];
        }

        bool operator<(const iterator& other) const
        {
            return this->index < other.index;
        }

        bool operator>(const iterator& other) const
        {
            return this->index > other.index;
        }

        bool operator<=(const iterator& other) const
        {
            return this->index <= other.index;
        }

        bool operator>=(const iterator& other) const
        {
            return this->index >= other.index;
        }
    };

    class const_iterator
    {
        friend class Vector;
        const Vector* vector;
        uint32_t index;

        void advance()
        {
            if(this->index >= this->vector->size())
                this->index = this->vector->size();
        }

    public:
        using iterator_category = std::random_access_iterator_tag;
        using value_type = T;
        using difference_type = int32_t;
        using pointer = const T*;
        using reference = const T&;

        const_iterator(const Vector* v, uint32_t i)
            : vector(v),
              index(i)
        {
            this->advance();
        }

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

        const_iterator& operator--()
        {
            this->index--;
            return *this;
        }

        const_iterator operator+(difference_type n) const
        {
            return const_iterator(this->vector, this->index + n);
        }

        const_iterator operator-(difference_type n) const
        {
            return const_iterator(this->vector, this->index - n);
        }

        difference_type operator-(const const_iterator& other) const
        {
            return static_cast<difference_type>(this->index - other.index);
        }

        const_iterator& operator+=(difference_type n)
        {
            this->index += n;
            this->advance();
            return *this;
        }

        const_iterator& operator-=(difference_type n)
        {
            this->index -= n;
            this->advance();
            return *this;
        }

        reference operator[](difference_type n) const
        {
            return (*this->vector)[this->index + n];
        }

        bool operator<(const const_iterator& other) const
        {
            return this->index < other.index;
        }

        bool operator>(const const_iterator& other) const
        {
            return this->index > other.index;
        }

        bool operator<=(const const_iterator& other) const
        {
            return this->index <= other.index;
        }

        bool operator>=(const const_iterator& other) const
        {
            return this->index >= other.index;
        }
    };

    iterator begin()
    {
        return iterator(this, 0);
    }

    iterator end()
    {
        return iterator(this, this->size());
    }

    const_iterator begin() const
    {
        return const_iterator(this, 0);
    }

    const_iterator end() const
    {
        return const_iterator(this, this->size());
    }

    const_iterator cbegin() const
    {
        return const_iterator(this, 0);
    }

    const_iterator cend() const
    {
        return const_iterator(this, this->size());
    }

    STDROMANO_FORCE_INLINE T* data() noexcept
    {
        return this->_data;
    }

    STDROMANO_FORCE_INLINE const T* data() const noexcept
    {
        return this->_data;
    }

    STDROMANO_FORCE_INLINE T* at(const uint32_t index) noexcept
    {
        STDROMANO_ASSERT(index < this->size(), "Out of bounds access");
        return this->data() + index;
    }

    STDROMANO_FORCE_INLINE const T* at(const uint32_t index) const noexcept
    {
        STDROMANO_ASSERT(index < this->size(), "Out of bounds access");
        return this->data() + index;
    }

    STDROMANO_FORCE_INLINE T& operator[](const uint32_t i) noexcept
    {
        return *this->at(i);
    }

    STDROMANO_FORCE_INLINE const T& operator[](const uint32_t i) const noexcept
    {
        return *this->at(i);
    }

    STDROMANO_FORCE_INLINE T& front()
    {
        return this->operator[](0);
    }

    STDROMANO_FORCE_INLINE const T& front() const
    {
        return this->operator[](0);
    }

    STDROMANO_FORCE_INLINE T& back()
    {
        return this->operator[](this->size() - 1);
    }

    STDROMANO_FORCE_INLINE const T& back() const
    {
        return this->operator[](this->size() - 1);
    }

    void resize(const uint32_t new_capacity) noexcept
    {
        const uint32_t old_capacity = this->capacity();

        if(new_capacity <= old_capacity)
        {
            return;
        }

        T* new_data = Vector<T>::allocate(new_capacity);

        if(new_data == nullptr)
        {
            return;
        }

        const uint32_t new_size = std::min(this->_size, new_capacity);

        if(this->_data != nullptr)
        {

#if defined(STDROMANO_VECTOR_COPY_ON_RESIZE)
            std::memcpy(new_data, this->_data, new_size * sizeof(T));
#else
            for(uint32_t i = 0; i < new_size; ++i)
            {
                ::new(new_data + i) T(std::move_if_noexcept(this->_data[i]));
                this->_data[i].~T();
            }
#endif /* defined(STDROMANO_VECTOR_COPY_ON_RESIZE) */

            Vector<T>::deallocate(this->_data);
        }

        this->_data = new_data;
        this->_capacity = new_capacity;
        this->_size = new_size;
    }

    STDROMANO_FORCE_INLINE void reserve(const uint32_t new_capacity) noexcept
    {
        this->resize(this->capacity() + new_capacity + 1);
    }

    void push_back(const T& element) noexcept
    {
        if(this->size() == this->capacity())
        {
            this->grow();
        }

        ::new(this->data() + this->size()) T(element);
        this->incr_size();
    }

    void push_back(T&& element) noexcept
    {
        if(this->size() == this->capacity())
        {
            this->grow();
        }

        ::new(this->data() + this->size()) T(std::move(element));
        this->incr_size();
    }

    template <typename... Args>
    void emplace_back(Args&&... args) noexcept
    {
        if(this->size() == this->capacity())
        {
            this->grow();
        }

        ::new(this->data() + this->size()) T(std::forward<Args>(args)...);
        this->incr_size();
    }

    void insert(const T& element, const uint32_t position) noexcept
    {
        STDROMANO_ASSERT(this->_data != nullptr, "Vector has not been allocated");
        STDROMANO_ASSERT(position <= this->size(), "Position must be lower than the vector size");

        if(this->size() == this->capacity())
        {
            this->grow();
        }

        std::memmove(this->at(position + 1),
                     this->at(position),
                     (this->size() - position) * sizeof(T));

        ::new(this->at(position)) T(element);
        this->incr_size();
    }

    iterator insert(const_iterator pos, uint32_t count, const T& value) noexcept
    {
        STDROMANO_ASSERT(this->_data != nullptr, "Vector has not been allocated");

        const uint32_t position = pos.index;

        if(position > this->size())
        {
            return this->end();
        }

        const uint32_t new_size = this->size() + count;

        while(this->capacity() < new_size)
        {
            this->grow();
        }

        std::memmove(this->at(position + count),
                     this->at(position),
                     (this->size() - position) * sizeof(T));

        for(uint32_t i = 0; i < count; ++i)
        {
            ::new(this->at(position + i)) T(value);
        }

        this->set_size(new_size);

        return iterator(this, position);
    }

    template <class InputIt>
    iterator insert(const_iterator pos, InputIt first, InputIt last) noexcept
    {
        STDROMANO_ASSERT(this->_data != nullptr, "Vector has not been allocated");

        if(first == last)
        {
            return iterator(this, pos.index);
        }

        const uint32_t position = pos.index;

        if(position > this->size())
        {
            return this->end();
        }

        const uint32_t count = static_cast<uint32_t>(std::distance(first, last));
        const uint32_t new_size = this->size() + count;

        while(this->capacity() < new_size)
        {
            this->grow();
        }

        std::memmove(this->at(position + count),
                     this->at(position),
                     (this->size() - position) * sizeof(T));
        uint32_t offset = 0;

        for(auto it = first; it != last; ++it, ++offset)
        {
            ::new(this->at(position + offset)) T(*it);
        }

        this->set_size(new_size);

        return iterator(this, position);
    }

    iterator erase(const_iterator pos) noexcept
    {
        STDROMANO_ASSERT(this->_data != nullptr, "Vector has not been allocated");
        STDROMANO_ASSERT(pos.index < this->size(),
                         "Iterator position must be lower than vector size");

        const uint32_t position = pos.index;

        this->at(position)->~T();
        std::memmove(this->at(position),
                     this->at(position + 1),
                     (this->size() - position - 1) * sizeof(T));
        this->decr_size();

        return iterator(this, position);
    }

    iterator erase(iterator first, iterator last) noexcept
    {
        STDROMANO_ASSERT(this->_data != nullptr, "Vector has not been allocated");

        if(first.index <= this->size() && first.index < last.index && last.index <= this->size())
        {
            const uint32_t start_pos = first.index;
            const uint32_t count = last.index - first.index;

            for(uint32_t i = 0; i < count; ++i)
            {
                this->at(start_pos + i)->~T();
            }

            std::memmove(this->at(start_pos),
                         this->at(start_pos + count),
                         (this->size() - start_pos - count) * sizeof(T));
            this->set_size(this->size() - count);

            return iterator(this, start_pos);
        }

        return this->end();
    }

    void remove(const uint32_t position) noexcept
    {
        STDROMANO_ASSERT(this->_data != nullptr, "Vector has not been allocated");
        STDROMANO_ASSERT(this->size() > 0, "No elements present in the vector");
        STDROMANO_ASSERT(position < this->size(), "Position must be lower than the vector size");

        this->at(position)->~T();
        std::memmove(this->at(position),
                     this->at(position + 1),
                     (this->size() - position - 1) * sizeof(T));
        this->decr_size();
    }

    T pop_back() noexcept
    {
        STDROMANO_ASSERT(this->_data != nullptr, "Vector has not been allocated");
        STDROMANO_ASSERT(this->size() > 0, "No elements present in the vector");

        T object = std::move_if_noexcept((*this)[this->size() - 1]);

        (*this)[this->size() - 1].~T();

        this->decr_size();

        return object;
    }

    iterator find(const T& other) noexcept
    {
        STDROMANO_ASSERT(this->_data != nullptr, "Vector has not been allocated");

        for(uint32_t i = 0; i < this->size(); i++)
        {
            if(other == this->operator[](i))
                return iterator(this, i);
        }

        return this->end();
    }

    const_iterator cfind(const T& other) const noexcept
    {
        STDROMANO_ASSERT(this->_data != nullptr, "Vector has not been allocated");

        for(uint32_t i = 0; i < this->size(); i++)
        {
            if(other == this->operator[](i))
                return const_iterator(this, i);
        }

        return this->cend();
    }

    STDROMANO_FORCE_INLINE void shrink_to_fit() noexcept
    {
        STDROMANO_ASSERT(this->_data != nullptr, "Vector has not been allocated");

        if(this->_capacity > this->_size)
        {
            this->resize(this->size());
        }
    }

    void clear() noexcept
    {
        STDROMANO_ASSERT(this->_data != nullptr, "Vector has not been allocated");

        for(uint32_t i = 0; i < this->size(); i++)
        {
            this->operator[](i).~T();
        }

        this->set_size(0);
    }

    template <typename F = std::less<T>>
    STDROMANO_FORCE_INLINE void sort(F&& cmp = F()) noexcept
    {
        STDROMANO_ASSERT(this->_data != nullptr, "Vector has not been allocated");
        STDROMANO_ASSERT(this->size() > 1, "Size must be greater than one");

        std::qsort(this->data(), this->size(), sizeof(T), std::forward<F>(cmp));
    }

    STDROMANO_FORCE_INLINE size_t memory_usage() const noexcept
    {
        return sizeof(T) * this->_size;
    }
};

STDROMANO_NAMESPACE_END

#endif // !defined(__STDROMANO_VECTOR)