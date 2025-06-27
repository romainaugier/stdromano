// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - Present Romain Augier
// All rights reserved.

#pragma once

#if !defined(__STDROMANO_STACKVECTOR)
#define __STDROMANO_STACKVECTOR

#include "stdromano/memory.h"

#include <new>
#include <initializer_list>
#include <utility>
#include <algorithm>

STDROMANO_NAMESPACE_BEGIN

/* Vector stored on the stack. If N is reached, it moves to a heap-allocated vector */

template<typename T, size_t N>
class StackVector 
{
public:
    static constexpr size_t MAX_STACK_SIZE = 16 * 1024;
    static constexpr size_t STACK_CAPACITY = N;
    
    static_assert((sizeof(T) * N) <= MAX_STACK_SIZE, "StackVector exceeds maximum stack size of 16KB");
    
    using value_type = T;
    using size_type = size_t;
    using reference = T&;
    using const_reference = const T&;
    using pointer = T*;
    using const_pointer = const T*;
    
    using iterator = T*;
    using const_iterator = const T*;

private:
    alignas(T) char _stack_storage[sizeof(T) * N];

    T* _heap_storage;

    size_t _size;
    size_t _capacity;

    STDROMANO_FORCE_INLINE bool uses_heap() const noexcept
    {
        return this->_heap_storage != nullptr;
    }

    STDROMANO_FORCE_INLINE T* stack_data() noexcept
    { 
        return reinterpret_cast<T*>(this->_stack_storage);
    }

    STDROMANO_FORCE_INLINE const T* stack_data() const noexcept
    { 
        return reinterpret_cast<const T*>(this->_stack_storage);
    }
    
    STDROMANO_FORCE_INLINE T* data() noexcept
    {
        return this->uses_heap() ? this->_heap_storage : this->stack_data();
    }
    
    STDROMANO_FORCE_INLINE const T* data() const noexcept
    {
        return this->uses_heap() ? this->_heap_storage : this->stack_data();
    }
    
    void transition_to_heap(size_t new_capacity) 
    {
        if(this->uses_heap()) 
        {
            return;
        }

        this->_heap_storage = static_cast<T*>(mem_alloc(new_capacity));

        if(this->_heap_storage == nullptr)
        {
            throw std::bad_alloc();
        }
        
        if constexpr (std::is_nothrow_move_constructible_v<T>) 
        {
            for(size_t i = 0; i < this->_size; i++) 
            {
                ::new (this->_heap_storage + i) T(std::move(stack_data()[i]));
                stack_data()[i].~T();
            }
        }
        else 
        {
            for(size_t i = 0; i < this->_size; i++) 
            {
                ::new (this->_heap_storage + i) T(stack_data()[i]);
                stack_data()[i].~T();
            }
        }
        
        this->_capacity = new_capacity;
    }
    
    void grow_if_needed() 
    {
        if(this->_size >= this->_capacity) 
        {
            const size_t new_capacity = this->uses_heap() ? this->_capacity * 2 : std::max(N * 2, size_t(8));

            if(!this->uses_heap()) 
            {
                transition_to_heap(new_capacity);
            } 
            else 
            {
                T* new_storage = static_cast<T*>(mem_realloc(this->_heap_storage, new_capacity));

                if(new_storage == nullptr)
                {
                    throw std::bad_alloc();
                }

                this->_heap_storage = new_storage;
                this->_capacity = new_capacity;
            }
        }
    }

public:
    StackVector() : _size(0), _capacity(N), _heap_storage(nullptr) {}
    
    explicit StackVector(size_t count, const T& value = T{}) : _size(0), _capacity(N), _heap_storage(nullptr) 
    {
        if(count > N) 
        {
            this->transition_to_heap(count);
        }

        for(size_t i = 0; i < count; i++) 
        {
            this->push_back(value);
        }
    }
    
    StackVector(std::initializer_list<T> init) : _size(0), _capacity(N), _heap_storage(nullptr) 
    {
        if(init.size() > N) 
        {
            this->transition_to_heap(init.size());
        }

        for(const auto& item : init) 
        {
            this->push_back(item);
        }
    }
    
    StackVector(const StackVector& other) : _size(0), _capacity(N), _heap_storage(nullptr) 
    {
        if(other._size > N) 
        {
            this->transition_to_heap(other._size);
        }

        for(size_t i = 0; i < other._size; i++) 
        {
            this->push_back(other[i]);
        }
    }
    
    StackVector(StackVector&& other) noexcept
    {
        if(!other.uses_heap())
        {
            if constexpr (std::is_nothrow_move_constructible_v<T>) 
            {
                for(size_t i = 0; i < other._size; i++) 
                {
                    ::new (this->stack_data() + i) T(std::move(other.stack_data()[i]));
                }
            } 
            else 
            {
                for(size_t i = 0; i < this->_size; i++) 
                {
                    ::new (this->stack_data() + i) T(other.stack_data()[i]);
                }
            }
        }

        this->_size = other._size;
        this->_capacity = other._capacity;
        this->_heap_storage = other._heap_storage;
        
        other._size = 0;
        other._capacity = N;
        other._heap_storage = nullptr;
    }
    
    StackVector& operator=(const StackVector& other) 
    {
        if(this != &other) 
        {
            this->clear();

            if(other._size > this->_capacity) 
            {
                if(!this->uses_heap()) 
                {
                    this->transition_to_heap(other._size);
                } 
                else 
                {
                    T* new_storage = static_cast<T*>(mem_realloc(this->_heap_storage, other._size));

                    if(new_storage == nullptr)
                    {
                        throw std::bad_alloc();
                    }

                    this->_capacity = other._size;
                    this->_heap_storage = new_storage;
                }
            }

            for(size_t i = 0; i < other._size; i++) 
            {
                this->push_back(other[i]);
            }
        }

        return *this;
    }
    
    StackVector& operator=(StackVector&& other) noexcept 
    {
        if(this != &other) 
        {
            this->~StackVector();

            this->_size = other._size;
            this->_capacity = other._capacity;
            this->_heap_storage = other._heap_storage;
            
            if(!this->uses_heap()) 
            {
                if constexpr (std::is_nothrow_move_constructible_v<T>) 
                {
                    for(size_t i = 0; i < this->_size; i++)
                    {
                        ::new (this->stack_data() + i) T(std::move(other.stack_data()[i]));
                    }
                } 
                else 
                {
                    for(size_t i = 0; i < this->_size; i++)
                    {
                        ::new (this->stack_data() + i) T(other.stack_data()[i]);
                    }
                }
            }
            
            other._size = 0;
            other._capacity = N;
            other._heap_storage = nullptr;
        }

        return *this;
    }
    
    ~StackVector() 
    {
        this->clear();

        if(this->uses_heap())
        {
            this->clear();
            mem_free(this->_heap_storage);
            this->_heap_storage = nullptr;
        }
    }
    
    STDROMANO_FORCE_INLINE reference operator[](size_t pos) noexcept
    {
        STDROMANO_ASSERT(pos < this->_size, "Out of bounds access");

        return this->data()[pos];
    }
    
    STDROMANO_FORCE_INLINE const_reference operator[](size_t pos) const noexcept
    {
        STDROMANO_ASSERT(pos < this->_size, "Out of bounds access");

        return this->data()[pos];
    }
    
    STDROMANO_FORCE_INLINE reference at(size_t pos) noexcept
    {
        STDROMANO_ASSERT(pos < this->_size, "Out of bounds access");

        return this->data()[pos];
    }
    
    STDROMANO_FORCE_INLINE const_reference at(size_t pos) const noexcept
    {
        STDROMANO_ASSERT(pos < this->_size, "Out of bounds access");

        return this->data()[pos];
    }
    
    STDROMANO_FORCE_INLINE reference front() noexcept { return this->data()[0]; }
    STDROMANO_FORCE_INLINE const_reference front() const noexcept { return this->data()[0]; }
    
    STDROMANO_FORCE_INLINE reference back() noexcept { return this->data()[this->_size - 1]; }
    STDROMANO_FORCE_INLINE const_reference back() const noexcept { return this->data()[this->_size - 1]; }
    
    STDROMANO_FORCE_INLINE iterator begin() noexcept { return this->data(); }
    STDROMANO_FORCE_INLINE const_iterator begin() const noexcept { return this->data(); }
    STDROMANO_FORCE_INLINE const_iterator cbegin() const noexcept { return this->data(); }
    
    STDROMANO_FORCE_INLINE iterator end() noexcept { return this->data() + this->_size; }
    STDROMANO_FORCE_INLINE const_iterator end() const noexcept { return this->data() + this->_size; }
    STDROMANO_FORCE_INLINE const_iterator cend() const noexcept { return this->data() + this->_size; }
    
    STDROMANO_FORCE_INLINE bool empty() const noexcept { return this->_size == 0; }
    STDROMANO_FORCE_INLINE size_t size() const noexcept { return this->_size; }
    STDROMANO_FORCE_INLINE size_t capacity() const noexcept { return this->_capacity; }
    
    void reserve(size_t new_capacity) 
    {
        if(new_capacity > this->_capacity) 
        {
            if(!this->uses_heap() && new_capacity > N) 
            {
                this->transition_to_heap(new_capacity);
            } 
            else if(this->uses_heap()) 
            {
                T* new_storage = static_cast<T*>(mem_realloc(this->_heap_storage, new_capacity));

                if(new_storage == nullptr)
                {
                    throw std::bad_alloc();
                }

                if constexpr (std::is_nothrow_move_constructible_v<T>) 
                {
                    for(size_t i = 0; i < this->_size; i++) 
                    {
                        ::new (new_storage + i) T(std::move(this->_heap_storage[i]));
                    }
                } 
                else 
                {
                    for(size_t i = 0; i < this->_size; i++) 
                    {
                        ::new (new_storage + i) T(this->_heap_storage[i]);
                    }
                }

                this->_heap_storage = new_storage;
                this->_capacity = new_capacity;
            }
        }
    }
    
    void clear() noexcept 
    {
        for(size_t i = 0; i < this->_size; i++) 
        {
            this->data()[i].~T();
        }

        this->_size = 0;
    }
    
    void push_back(const T& value)
    {
        this->grow_if_needed();

        ::new (this->data() + this->_size) T(value);

        this->_size++;
    }
    
    void push_back(T&& value) 
    {
        this->grow_if_needed();

        ::new (this->data() + this->_size) T(std::move(value));

        this->_size++;
    }
    
    template<typename... Args>
    reference emplace_back(Args&&... args) 
    {
        this->grow_if_needed();

        ::new (data() + this->_size) T(std::forward<Args>(args)...);

        return this->data()[this->_size++];
    }
    
    void pop_back() noexcept
    {
        if(this->_size > 0) 
        {
            this->_size--;
            this->data()[this->_size].~T();
        }
    }
    
    void resize(size_t count, const T& value = T{}) 
    {
        if(count > this->_size) 
        {
            this->reserve(count);
        }
        
        if(count > this->_size) 
        {
            for(size_t i = this->_size; i < count; i++) 
            {
                ::new (this->data() + i) T(value);
            }
        } 
        else if (count < this->_size) 
        {
            for (size_t i = count; i < this->_size; i++) 
            {
                this->data()[i].~T();
            }
        }

        this->_size = count;
    }
};

STDROMANO_NAMESPACE_END

#endif /* !defined(__STDROMANO_STACKVECTOR) */