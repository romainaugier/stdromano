// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - Present Romain Augier
// All rights reserved.

#pragma once

#if !defined(__STDROMANO_VECTOR)
#define __STDROMANO_VECTOR

#if !defined(STDROMANO_NULL_VECTOR_ASSERTIONS)
#define STDROMANO_NULL_VECTOR_ASSERTIONS 0
#endif /* !defined(STDROMANO_NULL_VECTOR_ASSERTIONS) */

#include "stdromano/memory.hpp"
#include "stdromano/traits.hpp"

#include <algorithm>
#include <cstdint>
#include <functional>
#include <initializer_list>

STDROMANO_NAMESPACE_BEGIN

// Utility to check if an iterator has an index member (for Vector iterators)
template<class, class = std::void_t<>>
struct has_index : std::false_type {};

template<class T>
struct has_index<T, std::void_t<typename T::has_index_tag>> : std::true_type {};

template<class T>
constexpr bool has_index_v = has_index<T>::value;

template <typename T>
class Vector
{
    static constexpr float GROWTH_RATE = 1.61803398875f;
    static constexpr size_t MIN_SIZE = 128;

private:
    T* _data = nullptr;
    std::size_t _capacity = 0;
    std::size_t _size = 0;

    STDROMANO_FORCE_INLINE void set_capacity(const std::size_t capacity) noexcept
    {
        this->_capacity = capacity;
    }

    STDROMANO_FORCE_INLINE void set_size(const std::size_t size) noexcept
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

    static T* allocate(const size_t capacity)
    {
        if(capacity == 0)
            return nullptr;

        void* memory = mem_aligned_alloc(capacity * sizeof(T), std::alignment_of<T>::value);

        if(memory == nullptr)
            return nullptr;

        return static_cast<T*>(memory);
    }

    static void deallocate(T* data)
    {
        if(data != nullptr)
            mem_aligned_free(data);
    }

    STDROMANO_FORCE_INLINE void grow() noexcept
    {
        if(this->_data == nullptr)
            this->resize(MIN_SIZE);
        else
            this->resize(static_cast<size_t>(static_cast<float>(this->capacity()) * GROWTH_RATE));
    }

public:
    using value_type = T;
    using size_type = std::size_t;
    using difference_type = std::int32_t;
    using reference = value_type&;
    using const_reference = const value_type&;
    using pointer = value_type*;
    using const_pointer = const value_type*;

    Vector() : _data(nullptr),
               _capacity(0),
               _size(0)
    {
    }

    Vector(const std::size_t initial_capacity)
    {
        const std::size_t capacity = initial_capacity == 0 ? MIN_SIZE : initial_capacity;
        this->_data = Vector::allocate(capacity);
        this->set_capacity(capacity);
        this->set_size(initial_capacity);

        if constexpr (std::is_default_constructible_v<T>)
            for(std::size_t i = 0; i < this->size(); i++)
                ::new(this->_data + i) T();
    }

    Vector(const std::size_t count, const T& value)
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
            for(std::size_t i = 0; i < count; ++i)
                ::new(this->_data + i) T(value);

            this->set_size(count);
        }
    }

    Vector(std::initializer_list<value_type> init)
    {
        if(init.size() > 0)
            for(const auto& item : init)
                this->emplace_back(item);
    }

    template <class InputIt>
    Vector(InputIt begin, InputIt end)
    {
        if(end > begin)
            for(auto it = begin; it != end; ++it)
                this->emplace_back(*it);

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
            for(std::size_t i = 0; i < other.size(); i++)
                ::new(this->data() + i) T(other[i]);

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
                    for(std::size_t i = 0; i < other.size(); i++)
                        ::new(this->data() + i) T(other[i]);

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

    STDROMANO_FORCE_INLINE size_t capacity() const noexcept
    {
        return this->_capacity;
    }

    STDROMANO_FORCE_INLINE size_t size() const noexcept
    {
        return this->_size;
    }

    STDROMANO_FORCE_INLINE bool empty() const noexcept
    {
        return this->size() == 0;
    }

    class const_iterator;

    class iterator
    {
        friend class Vector;

        Vector* vector;
        size_t index;

        void advance()
        {
            if(this->index >= this->vector->size())
                this->index = this->vector->size();
        }

    public:
        using has_index_tag = void;
        using iterator_category = std::random_access_iterator_tag;
        using value_type = T;
        using difference_type = std::int32_t;
        using pointer = T*;
        using reference = T&;

        iterator(Vector* v, size_t i) : vector(v),
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

        operator const_iterator() const
        {
            return const_iterator(this->vector, this->index);
        }
    };

    class const_iterator
    {
        friend class Vector;

        const Vector* vector;
        std::size_t index;

        void advance()
        {
            if(this->index >= this->vector->size())
                this->index = this->vector->size();
        }

    public:
        using has_index_tag = void;
        using iterator_category = std::random_access_iterator_tag;
        using value_type = T;
        using difference_type = int32_t;
        using pointer = const T*;
        using reference = const T&;

        const_iterator(const Vector* v, size_t i) : vector(v),
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

        operator iterator() const
        {
            return iterator(this->vector, this->index);
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

    STDROMANO_FORCE_INLINE T* at(const std::size_t index) noexcept
    {
        STDROMANO_ASSERT(index < this->size(), "Out of bounds access");

        return this->data() + index;
    }

    STDROMANO_FORCE_INLINE const T* at(const std::size_t index) const noexcept
    {
        STDROMANO_ASSERT(index < this->size(), "Out of bounds access");
        return this->data() + index;
    }

    STDROMANO_FORCE_INLINE T& operator[](const std::size_t i) noexcept
    {
        return *this->at(i);
    }

    STDROMANO_FORCE_INLINE const T& operator[](const std::size_t i) const noexcept
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

    void resize(const std::size_t new_capacity) noexcept
    {
        const std::size_t old_capacity = this->capacity();

        if(new_capacity <= old_capacity)
            return;

        T* new_data = Vector<T>::allocate(new_capacity);

        if(new_data == nullptr)
            return;

        const std::size_t new_size = std::min(this->_size, new_capacity);

        if(this->_data != nullptr)
        {

#if defined(STDROMANO_VECTOR_COPY_ON_RESIZE)
            std::memcpy(new_data, this->_data, new_size * sizeof(T));
#else
            for(std::size_t i = 0; i < new_size; ++i)
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

    STDROMANO_FORCE_INLINE void reserve(const size_t new_capacity) noexcept
    {
        this->resize(this->capacity() + new_capacity + 1);
    }

    void push_back(const T& element) noexcept
    {
        if(this->size() == this->capacity())
            this->grow();

        ::new(this->data() + this->size()) T(element);

        this->incr_size();
    }

    void push_back(T&& element) noexcept
    {
        if(this->size() == this->capacity())
            this->grow();

        ::new(this->data() + this->size()) T(std::move(element));

        this->incr_size();
    }

    template <typename... Args>
    void emplace_back(Args&&... args) noexcept
    {
        if(this->size() == this->capacity())
            this->grow();

        ::new(this->data() + this->size()) T(std::forward<Args>(args)...);

        this->incr_size();
    }

    void insert(T element, const std::size_t position) noexcept
    {
#if STDROMANO_NULL_VECTOR_ASSERTIONS
        STDROMANO_ASSERT(this->_data != nullptr, "Vector has not been allocated");
#endif /* STDROMANO_NULL_VECTOR_ASSERTIONS */
        STDROMANO_ASSERT(position <= this->size(), "Position must be lower than the vector size");

        if(this->size() == this->capacity())
            this->grow();

        if(position == this->size())
        {
            this->emplace_back(std::move(element));
        }
        else
        {
            if constexpr (std::is_trivially_copyable_v<T>)
            {
                std::memmove(this->data() + position + 1,
                            this->data() + position,
                            (this->size() - position) * sizeof(T));
            }
            else
            {
                ::new(this->data() + this->size()) T(std::move(this->operator[](this->size() - 1)));

                std::move_backward(this->data() + position,
                                this->data() + this->size() - 1,
                                this->data() + this->size());
            }

            this->operator[](position) = std::move(element);

            this->incr_size();
        }
    }

    template<class It>
    iterator insert(It pos, std::size_t count, const T& value) noexcept
    {
        static_assert(has_index_v<It>, "It has not index member (hint: use Vector::iterator or Vector::const_iterator)");

#if STDROMANO_NULL_VECTOR_ASSERTIONS
        STDROMANO_ASSERT(this->_data != nullptr, "Vector has not been allocated");
#endif /* STDROMANO_NULL_VECTOR_ASSERTIONS */

        const std::size_t position = pos.index;

        if(position > this->size())
            return this->end();

        const std::size_t old_size = this->size();
        const std::size_t tail = old_size - position;
        const std::size_t new_size = old_size + count;

        while(this->capacity() < new_size)
            this->grow();

        if constexpr (std::is_trivially_copyable_v<T>)
        {
            std::memmove(this->data() + position + count,
                         this->data() + position,
                         tail * sizeof(T));
        }
        else
        {
            if(count <= tail)
            {
                std::uninitialized_move(this->data() + old_size - count,
                                        this->data() + old_size,
                                        this->data() + old_size);

                std::move_backward(this->data() + position,
                                   this->data() + old_size,
                                   this->data() + old_size);
            }
            else
            {
                std::uninitialized_move(this->data() + position,
                                        this->data() + old_size,
                                        this->data() + position + count);
            }
        }

        std::uninitialized_fill_n(this->data() + position, count, value);

        this->set_size(new_size);

        return iterator(this, position);
    }

    template<class It, class InputIt>
    iterator insert(It pos, InputIt first, InputIt last) noexcept
    {
        static_assert(has_index_v<It>, "It has not index member (hint: use Vector::iterator or Vector::const_iterator)");

#if STDROMANO_NULL_VECTOR_ASSERTIONS
        STDROMANO_ASSERT(this->_data != nullptr, "Vector has not been allocated");
#endif /* STDROMANO_NULL_VECTOR_ASSERTIONS */

        if(first == last)
            return iterator(this, pos.index);

        const std::size_t position = pos.index;

        if(position > this->size())
            return this->end();

        const std::size_t old_size = this->size();

        if constexpr (is_forward_iterator<InputIt>::value)
        {
            const std::size_t count = static_cast<std::size_t>(std::distance(first, last));
            const std::size_t tail = old_size - position;
            const std::size_t new_size = this->size() + count;

            while(this->capacity() < new_size)
                this->grow();

            std::size_t offset = 0;

            if(pos == this->end())
            {
                for(auto it = first; it != last; ++it)
                {
                    this->emplace_back(*it);
                    ++offset;
                }
            }
            else
            {
                if constexpr (std::is_trivially_copyable_v<T>)
                {
                    std::memmove(this->data() + position + count,
                                this->data() + position,
                                tail * sizeof(T));
                }
                else
                {
                    if(count <= tail)
                    {
                        std::uninitialized_move(this->data() + old_size - count,
                                                this->data() + old_size,
                                                this->data() + old_size);

                        std::move_backward(this->data() + position,
                                        this->data() + old_size - count,
                                        this->data() + old_size);
                    }
                    else
                    {
                        std::uninitialized_move(this->data() + position,
                                                this->data() + old_size,
                                                this->data() + position + count);
                    }
                }

                std::uninitialized_copy(first, last, this->data() + position);

                this->set_size(new_size);
            }

            return iterator(this, position);
        }
        else
        {
            std::size_t pos_index = pos.index;

            for(; first != last; ++first)
                this->emplace_back(*first);

            std::rotate(this->data() + pos_index,
                        this->data() + old_size,
                        this->data() + this->size());

            return iterator(this, pos_index);
        }
    }

    template<class It>
    iterator insert(It pos, std::initializer_list<T> list) noexcept
    {
        return this->insert(pos, list.begin(), list.end());
    }

    template<class It>
    iterator erase(It pos) noexcept
    {
        static_assert(has_index_v<It>, "It has not index member (hint: use Vector::iterator or Vector::const_iterator)");

#if STDROMANO_NULL_VECTOR_ASSERTIONS
        STDROMANO_ASSERT(this->_data != nullptr, "Vector has not been allocated");
#endif /* STDROMANO_NULL_VECTOR_ASSERTIONS */
        STDROMANO_ASSERT(pos.index < this->size(),
                         "Iterator position must be lower than vector size");

        const std::size_t position = pos.index;

        if constexpr (std::is_trivially_copyable_v<T>)
        {
            std::memmove(this->data() + position,
                         this->data() + position + 1,
                         (this->size() - position - 1) * sizeof(T));
        }
        else
        {
            std::move(this->data() + position + 1,
                      this->data() + this->size(),
                      this->data() + position);
        }

        if constexpr (!std::is_trivially_destructible_v<T>)
            std::destroy_at(this->data() + (this->size() - 1));

        this->decr_size();

        return iterator(this, position);
    }

    template<typename It>
    iterator erase(It first, It last) noexcept
    {
        static_assert(has_index_v<It>, "It has not index member (hint: use Vector::iterator or Vector::const_iterator)");

#if STDROMANO_NULL_VECTOR_ASSERTIONS
        STDROMANO_ASSERT(this->_data != nullptr, "Vector has not been allocated");
#endif /* STDROMANO_NULL_VECTOR_ASSERTIONS */

        if(first.index < this->size() &&
           first.index <= last.index &&
           last.index <= this->size())
        {
            if(first.index == last.index)
                return iterator(this, first.index);

            const std::size_t start_pos = first.index;
            const std::size_t count = last.index - first.index;

            if constexpr (std::is_trivially_copyable_v<T>)
            {
                std::memmove(this->data() + start_pos,
                            this->data() + start_pos + count,
                            (this->size() - start_pos - count) * sizeof(T));
            }
            else
            {
                std::move(this->data() + start_pos + count, // first
                          this->data() + this->size(),      // last
                          this->data() + start_pos);        // output first
            }

            if constexpr (!std::is_trivially_destructible_v<T>)
                std::destroy(this->data() + (this->size() - count),
                             this->data() + this->size());

            this->set_size(this->size() - count);

            return iterator(this, start_pos);
        }

        return this->end();
    }

    T pop_back() noexcept
    {
#if STDROMANO_NULL_VECTOR_ASSERTIONS
        STDROMANO_ASSERT(this->_data != nullptr, "Vector has not been allocated");
#endif /* STDROMANO_NULL_VECTOR_ASSERTIONS */
        STDROMANO_ASSERT(this->size() > 0, "No elements present in the vector");

        T object = std::move_if_noexcept((*this)[this->size() - 1]);

        (*this)[this->size() - 1].~T();

        this->decr_size();

        return object;
    }

    iterator find(const T& other) noexcept
    {
#if STDROMANO_NULL_VECTOR_ASSERTIONS
        STDROMANO_ASSERT(this->_data != nullptr, "Vector has not been allocated");
#else
        if(this->size() == 0)
            return this->end();
#endif /* STDROMANO_NULL_VECTOR_ASSERTIONS */

        for(std::size_t i = 0; i < this->size(); i++)
            if(other == this->operator[](i))
                return iterator(this, i);

        return this->end();
    }

    iterator find(std::function<bool(const T&)>&& f) noexcept
    {
#if STDROMANO_NULL_VECTOR_ASSERTIONS
        STDROMANO_ASSERT(this->_data != nullptr, "Vector has not been allocated");
#else
        if(this->size() == 0)
            return this->end();

#endif /* STDROMANO_NULL_VECTOR_ASSERTIONS */

        for(std::size_t i = 0; i < this->size(); i++)
            if(f(this->operator[](i)))
                return iterator(this, i);

        return this->end();
    }

    const_iterator cfind(const T& other) const noexcept
    {
#if STDROMANO_NULL_VECTOR_ASSERTIONS
        STDROMANO_ASSERT(this->_data != nullptr, "Vector has not been allocated");
#else
        if(this->size() == 0)
            return this->cend();

#endif /* STDROMANO_NULL_VECTOR_ASSERTIONS */

        for(std::size_t i = 0; i < this->size(); i++)
            if(other == this->operator[](i))
                return const_iterator(this, i);

        return this->cend();
    }

    const_iterator cfind(std::function<bool(const T&)>&& f) const noexcept
        {
    #if STDROMANO_NULL_VECTOR_ASSERTIONS
            STDROMANO_ASSERT(this->_data != nullptr, "Vector has not been allocated");
    #else
            if(this->size() == 0)
                return this->end();

    #endif /* STDROMANO_NULL_VECTOR_ASSERTIONS */

            for(std::size_t i = 0; i < this->size(); i++)
                if(f(this->operator[](i)))
                    return const_iterator(this, i);

            return this->end();
        }

    STDROMANO_FORCE_INLINE void shrink_to_fit() noexcept
    {
#if STDROMANO_NULL_VECTOR_ASSERTIONS
        STDROMANO_ASSERT(this->_data != nullptr, "Vector has not been allocated");
#else
        if(this->size() == 0)
            return;
#endif /* STDROMANO_NULL_VECTOR_ASSERTIONS */

        if(this->_capacity > this->_size)
            this->resize(this->size());
    }

    void clear() noexcept
    {
#if STDROMANO_NULL_VECTOR_ASSERTIONS
        STDROMANO_ASSERT(this->_data != nullptr, "Vector has not been allocated");
#else
        if(this->size() == 0)
            return;
#endif /* STDROMANO_NULL_VECTOR_ASSERTIONS */

        for(size_t i = 0; i < this->size(); i++)
            this->operator[](i).~T();

        this->set_size(0);
    }

    template <typename F = std::less<T>>
    STDROMANO_FORCE_INLINE void sort(F&& cmp = F()) noexcept
    {
#if STDROMANO_NULL_VECTOR_ASSERTIONS
        STDROMANO_ASSERT(this->_data != nullptr, "Vector has not been allocated");
#else
        if(this->size() == 0)
            return;
#endif /* STDROMANO_NULL_VECTOR_ASSERTIONS */

        if(this->_size <= 1)
            return;

        std::qsort(this->data(), this->size(), sizeof(T), std::forward<F>(cmp));
    }

    STDROMANO_FORCE_INLINE size_t memory_usage() const noexcept
    {
        return sizeof(T) * this->_size;
    }
};

STDROMANO_NAMESPACE_END

#endif // !defined(__STDROMANO_VECTOR)
