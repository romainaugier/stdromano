// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - Present Romain Augier
// All rights reserved.

#pragma once

#if !defined(__STDROMANO_OPTIONAL)
#define __STDROMANO_OPTIONAL

#include "stdromano/stdromano.h"

#include <stdexcept>
#include <type_traits>

STDROMANO_NAMESPACE_BEGIN

template <typename T>
class Optional
{
  private:
    typename std::aligned_storage<sizeof(T), alignof(T)>::type _data;
    bool _has_value;

  public:
    Optional()
        : _has_value(false)
    {
    }

    Optional(const T& value)
        : _has_value(false)
    {
        this->construct(value);
    }

    Optional(T&& value) noexcept
        : _has_value(false)
    {
        this->construct(std::move(value));
    }

    Optional(const Optional& other)
        : _has_value(false)
    {
        if(other._has_value)
        {
            this->construct(*other);
        }
    }

    Optional(Optional&& other) noexcept
        : _has_value(false)
    {
        if(other._has_value)
        {
            this->construct(std::move(*other));
            other.reset();
        }
    }

    Optional& operator=(const T& value)
    {
        this->reset();
        this->construct(value);
        return *this;
    }

    Optional& operator=(T&& value) noexcept
    {
        this->reset();
        this->construct(std::move(value));
        return *this;
    }

    Optional& operator=(const Optional& other)
    {
        if(this != &other)
        {
            this->reset();
            if(other._has_value)
            {
                this->construct(*other);
            }
        }
        return *this;
    }

    Optional& operator=(Optional&& other) noexcept
    {
        if(this != &other)
        {
            this->reset();
            if(other._has_value)
            {
                this->construct(std::move(*other));
                other.reset();
            }
        }
        return *this;
    }

    ~Optional()
    {
        this->reset();
    }

    STDROMANO_FORCE_INLINE bool has_value() const
    {
        return this->_has_value;
    }

    template <typename... Args>
    void emplace(Args&&... args)
    {
        this->reset();
        new(this->get_storage()) T(std::forward<Args>(args)...);
        this->_has_value = true;
    }

    void reset()
    {
        if(this->_has_value)
        {
            this->get_pointer()->~T();
            this->_has_value = false;
        }
    }

    T& operator*() &
    {
        return *this->get_pointer();
    }

    const T& operator*() const&
    {
        return *this->get_pointer();
    }

    T* operator->()
    {
        return this->get_pointer();
    }

    const T* operator->() const
    {
        return this->get_pointer();
    }

    T& value() &
    {
        if(!this->_has_value)
        {
            throw std::runtime_error("Accessing empty Optional");
        }

        return **this;
    }

    const T& value() const&
    {
        if(!this->_has_value)
        {
            throw std::runtime_error("Accessing empty Optional");
        }

        return **this;
    }

  private:
    T* get_pointer()
    {
        return reinterpret_cast<T*>(&this->_data);
    }

    const T* get_pointer() const
    {
        return reinterpret_cast<const T*>(&this->_data);
    }

    void* get_storage()
    {
        return &this->_data;
    }

    template <typename U>
    void construct(U&& value)
    {
        new(this->get_storage()) T(std::forward<U>(value));
        this->_has_value = true;
    }
};

STDROMANO_NAMESPACE_END

#endif /* !defined(__STDROMANO_OPTIONAL) */