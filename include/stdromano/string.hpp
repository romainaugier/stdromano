// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - Present Romain Augier
// All rights reserved.

#pragma once

#if !defined(__STDROMANO_STRING)
#define __STDROMANO_STRING

#include "stdromano/char.hpp"
#include "stdromano/hash.hpp"
#include "stdromano/memory.hpp"

#include "spdlog/fmt/fmt.h"

STDROMANO_NAMESPACE_BEGIN

DETAIL_NAMESPACE_BEGIN

STDROMANO_API void tolower(char* str, std::size_t length) noexcept;

STDROMANO_API void tolower(const char* src, char* dst, std::size_t length) noexcept;

STDROMANO_API bool strcmp(const char* __restrict lhs,
                          const char* __restrict rhs,
                          const std::size_t length,
                          const bool case_sensitive = true) noexcept;

DETAIL_NAMESPACE_END

/* Validate a sequence of characters to be utf-8 */
STDROMANO_API bool validate_utf8(const char* str, std::size_t size) noexcept;

/* Finds the last occurence of needle in haystack (reverse strstr) */
STDROMANO_API char* strrstr(const char* haystack, const char* needle) noexcept;

template <std::size_t LocalCapacity = 7>
class String
{
    static constexpr float STRING_GROWTH_RATE = 1.61f;
    static constexpr std::size_t STRING_ALIGNMENT = 32;
    static constexpr std::size_t INVALID_REF_SIZE = std::numeric_limits<size_t>::max();
    static constexpr std::size_t NPOS = std::numeric_limits<size_t>::max();

    template <std::size_t OtherCapacity>
    friend class String;

public:
    using value_type = char;
    using split_iterator = std::size_t;

private:
    union
    {
        char* _heap_data;
        char _local_data[LocalCapacity + 1];
    };

    std::uint32_t _size : 31;
    std::uint32_t _capacity : 31; 
    std::uint32_t _is_local : 1;
    std::uint32_t _is_ref : 1;

    void reallocate(std::size_t new_capacity) noexcept
    {
        STDROMANO_ASSERT(!this->_is_ref, "Cannot modify a reference string");

        char* new_data = mem_aligned_alloc<char>((new_capacity + 1) * sizeof(char),
                                                 STRING_ALIGNMENT);

        const std::size_t copy_size = std::min(static_cast<std::size_t>(this->_size),
                                               new_capacity - 1);

        if(this->_size > 0)
        {
            mem_cpy(new_data, this->data(), copy_size);
        }

        new_data[copy_size] = '\0';

        if(!this->_is_local && this->_heap_data != nullptr)
        {
            mem_aligned_free(this->_heap_data);
        }

        this->_heap_data = new_data;
        this->_size = copy_size;
        this->_capacity = static_cast<std::uint32_t>(new_capacity);
        this->_is_local = 0;
    }

public:
    constexpr String() noexcept
        : _size(0),
          _capacity(LocalCapacity),
          _is_local(1),
          _is_ref(0)
    {
        this->_local_data[0] = '\0';
    }

    constexpr String(const char* str, const std::size_t len = String::NPOS) : String()
    {
        if(str == nullptr)
        {
            return;
        }

        const size_t size = len == String::NPOS ? str_len(str) : len;

        if(size == 0)
        {
            return;
        }

        if(size > LocalCapacity)
        {
            this->reallocate(size);
        }

        mem_cpy(this->data(), str, size + 1);
        this->_size = static_cast<uint32_t>(size);
        this->data()[this->_size] = '\0';
    }

    template <typename... Args>
    String(fmt::format_string<Args...> fmt, Args&&... args) : String()
    {
        fmt::format_to(std::back_inserter(*this), fmt, std::forward<Args>(args)...);

        if(this->_size > 0 || this->_is_local)
        {
            this->data()[this->_size] = '\0';
        }
    }
    
    constexpr String(const String& other) noexcept : _size(other._size),
                                                     _capacity(other._is_local ? LocalCapacity : other._capacity),
                                                     _is_local(other._is_local),
                                                     _is_ref(0)
    {
        if(other._is_ref)
        {
            this->_heap_data = other._heap_data;
            this->_capacity = other._capacity;
            this->_is_local = 0;
            this->_is_ref = 1;
        }
        else if(other._size <= LocalCapacity)
        {
            this->_is_local = 1;
            this->_capacity = LocalCapacity;
            mem_cpy(this->_local_data, other.data(), other._size + 1);
        }
        else
        {
            this->_is_local = 0;
            this->_capacity = other._capacity;
            this->_heap_data = mem_aligned_alloc<char>((this->_capacity + 1) * sizeof(char),
                                                           STRING_ALIGNMENT);
            mem_cpy(this->_heap_data, other.data(), other._size + 1);
        }
    }

    constexpr String& operator=(const String& other) noexcept
    {
        if(this == &other)
        {
            return *this;
        }

        if(!this->_is_ref && !this->_is_local && this->_heap_data != nullptr)
        {
            mem_aligned_free(this->_heap_data);
            this->_heap_data = nullptr;
        }

        this->_size = other._size;
        this->_capacity = other._is_local ? LocalCapacity : other._capacity;
        this->_is_local = other._is_local;
        this->_is_ref = 0;

        if(other._is_ref)
        {
            this->_heap_data = other._heap_data;
            this->_is_ref = 1;
        }
        else if(this->_is_local)
        {
            mem_cpy(this->_local_data, other._local_data, this->_size + 1);
        }
        else
        {
            this->_heap_data = mem_aligned_alloc<char>((this->_capacity + 1) * sizeof(char),   
                                                           STRING_ALIGNMENT);
            mem_cpy(this->_heap_data, other._heap_data, this->_size + 1);
        }

        return *this;
    }


    constexpr String(String&& other) noexcept : _size(other._size),
                                                _capacity(other._capacity),
                                                _is_local(other._is_local),
                                                _is_ref(other._is_ref)
    {
        if(other._is_ref)
        {
            this->_heap_data = other._heap_data;
        }
        else if(other._is_local)
        {
            mem_cpy(this->_local_data, other._local_data, other._size + 1);
            this->_capacity = LocalCapacity;
        }
        else
        {
            this->_heap_data = other._heap_data;
            other._heap_data = nullptr;
        }

        other._size = 0;
        other._capacity = LocalCapacity;
        other._is_local = 1;
        other._is_ref = 0;

        if(other._is_local)
        {
           other._local_data[0] = '\0';
        }
    }

    constexpr String& operator=(String&& other) noexcept
    {
        if(this == &other)
        {
            return *this;
        }

        if(!this->_is_ref && !this->_is_local && this->_heap_data != nullptr)
        {
            mem_aligned_free(this->_heap_data);
            this->_heap_data = nullptr;
        }

        this->_size = other._size;
        this->_capacity = other._capacity;
        this->_is_local = other._is_local;
        this->_is_ref = other._is_ref;

        if(other._is_ref)
        {
            this->_heap_data = other._heap_data;
        }
        else if(other._is_local)
        {
            mem_cpy(this->_local_data, other._local_data, other._size + 1);
            this->_capacity = LocalCapacity;
        }
        else
        {
            this->_heap_data = other._heap_data;
            other._heap_data = nullptr;
        }

        other._size = 0;
        other._capacity = LocalCapacity;
        other._is_local = 1;
        other._is_ref = 0;

        if(other._is_local)
        {
            other._local_data[0] = '\0';
        }
        
        return *this;
    }

    template<size_t OtherCapacity>
    constexpr String(const String<OtherCapacity>& other) : String()
    {
        this->_size = other._size;
        this->_is_ref = 0;

        if(other._is_ref)
        {
            this->_heap_data = other._heap_data;
            this->_capacity = other._capacity;
            this->_is_local = 0;
            this->_is_ref = 1;
        }
        else if(other._size <= LocalCapacity)
        {
            this->_is_local = 1;
            this->_capacity = LocalCapacity;
            mem_cpy(this->_local_data, other.data(), other._size + 1);
        }
        else
        {
            this->_is_local = 0;
            this->_capacity = other._capacity;
            this->_heap_data = mem_aligned_alloc<char>((this->_capacity + 1) * sizeof(char),
                                                           STRING_ALIGNMENT);
            mem_cpy(this->_heap_data, other.data(), other._size + 1);
        }
    }

    template<size_t OtherCapacity>
    constexpr String& operator=(const String<OtherCapacity>& other) noexcept
    {
        if(!this->_is_ref && !this->_is_local && this->_heap_data != nullptr)
        {
            mem_aligned_free(this->_heap_data);
            this->_heap_data = nullptr;
        }

        this->_size = other._size;

        if(other._is_ref)
        {
            this->_heap_data = const_cast<char*>(other._heap_data);
            this->_capacity = other._capacity;
            this->_is_local = 0;
            this->_is_ref = 1;
        }
        else
        {
            this->_is_ref = 0;
            if (other._size <= LocalCapacity)
            {
                this->_is_local = 1;
                this->_capacity = LocalCapacity;
                mem_cpy(this->_local_data, other.data(), other._size + 1);
            }
            else
            {
                this->_is_local = 0;
                this->_capacity = other._capacity;
                this->_heap_data = mem_aligned_alloc<char>((this->_capacity + 1) * sizeof(char),
                                                               STRING_ALIGNMENT);
                mem_cpy(this->_heap_data, other.data(), other._size + 1);
            }
        }
        return *this;
    }

    template<size_t OtherCapacity>
    constexpr String(String<OtherCapacity>&& other) noexcept
    {
        this->_size = other._size;
        this->_capacity = other._capacity;
        this->_is_local = other._is_local;
        this->_is_ref = other._is_ref;

        if(this->_is_ref)
        {
            this->_heap_data = other._heap_data;
        }
        else if(this->_is_local)
        {
            mem_cpy(this->_local_data, other._local_data, this->_size + 1);
            this->_capacity = LocalCapacity;
        }
        else
        {
            this->_heap_data = other._heap_data;
            other._heap_data = nullptr;
        }

        other._size = 0;
        other._capacity = OtherCapacity;
        other._is_local = 1;
        other._is_ref = 0;
        other._local_data[0] = '\0';
    }


    template<size_t OtherCapacity>
    constexpr String& operator=(String<OtherCapacity>&& other) noexcept
    {
        if(!this->_is_ref && !this->_is_local && this->_heap_data != nullptr)
        {
            mem_aligned_free(this->_heap_data);
            this->_heap_data = nullptr;
        }
        
        this->_size = other._size;
        this->_capacity = other._capacity;
        this->_is_local = other._is_local;
        this->_is_ref = other._is_ref;

        if(this->_is_ref)
        {
            this->_heap_data = other._heap_data;
        }
        else if(this->_is_local)
        {
            mem_cpy(this->_local_data, other._local_data, this->_size + 1);
            this->_capacity = LocalCapacity;
        }
        else
        {
            this->_heap_data = other._heap_data;
            other._heap_data = nullptr;
        }

        other._size = 0;
        other._capacity = OtherCapacity;
        other._is_local = 1;
        other._is_ref = 0;
        other._local_data[0] = '\0';
        
        return *this;
    }

    ~String()
    {
        if(!this->_is_ref && !this->_is_local && this->_heap_data != nullptr)
        {
            mem_aligned_free(this->_heap_data);
        }

        this->_heap_data = nullptr;
        this->_size = 0;
        this->_capacity = 0;
        this->_is_local = 1;
        this->_is_ref = 0;
    }

    constexpr STDROMANO_FORCE_INLINE char& operator[](const size_t i) noexcept
    {
        STDROMANO_ASSERT(i < this->_size, "Index out of bounds");
        return this->data()[i];
    }

    constexpr STDROMANO_FORCE_INLINE const char& operator[](const size_t i) const noexcept
    {
        STDROMANO_ASSERT(i < this->_size, "Index out of bounds");
        return this->data()[i];
    }

    /* constexpr */ bool operator==(const String<>& other) const
    {
        if(this->size() != other.size()) 
        {
            return false;
        }

        if(this->empty())
        {
            return true;
        }

        /* 
            TODO: when switching to C++20 use std::is_constant_evaluated()
            to use a constexpr friendly algorithm
        */

        return detail::strcmp(this->c_str(), other.c_str(), this->length());
    }

    constexpr bool operator!=(const String<>& other) const
    {
        return !this->operator==(other);
    }

    constexpr static String make_ref(const char* str, size_t size_param = INVALID_REF_SIZE) noexcept
    {
        if(str == nullptr) 
        {
            return String();
        } 

        const size_t size = size_param == INVALID_REF_SIZE ? str_len(str) : size_param;

        String s;
        s._heap_data = const_cast<char*>(str);
        s._size = static_cast<uint32_t>(size);
        s._capacity = static_cast<uint32_t>(size);
        s._is_local = 0;
        s._is_ref = 1;

        return s;
    }

    constexpr static String make_ref(const String& str) noexcept
    {
        return String::make_ref(str.data(), str.size());
    }

    constexpr static String make_zeroed(const size_t size) noexcept
    {
        String s;

        if(size > LocalCapacity)
        {
            s.reallocate(size);
        }

        s._size = static_cast<uint32_t>(size);
        
        mem_set(s.data(), 0, size + 1);

        return s;
    }

    template <typename... Args>
    static String make_fmt(fmt::format_string<Args...> fmt, Args&&... args)
    {
        String s;

        fmt::format_to(std::back_inserter(s), fmt, std::forward<Args>(args)...);

        if(s._size > 0 || s._is_local)
        {
            s.data()[s._size] = '\0';
        }

        return s;
    }

    static constexpr String make_from_c_str(const char* str, const std::size_t len = String::NPOS) noexcept
    {
        String s;

        if(str == nullptr)
        {
            return s;
        }

        const std::size_t size = len == String::NPOS ? str_len(str) : len;

        if(size == 0)
        {
            return s;
        }

        if(size > LocalCapacity)
        {
            s.reallocate(size);
        }

        mem_cpy(s.data(), str, size + 1);
        s._size = static_cast<std::uint32_t>(size);
        s.data()[s._size] = '\0';

        return s;
    }

    constexpr String copy() const noexcept
    {
        if(this->_is_ref) 
        {
            return String(this->data(), this->size());
        }

        return String(*this);
    }

    constexpr STDROMANO_FORCE_INLINE const char* data() const noexcept
    {
        return this->_is_local ? this->_local_data : this->_heap_data;
    }

    constexpr STDROMANO_FORCE_INLINE char* data() noexcept
    {
        return this->_is_local ? this->_local_data : this->_heap_data;
    }

    constexpr STDROMANO_FORCE_INLINE const char* c_str() const noexcept
    {
        return this->_is_local ? this->_local_data : this->_heap_data;
    }

    constexpr STDROMANO_FORCE_INLINE char* c_str() noexcept
    {
        return this->_is_local ? this->_local_data : this->_heap_data;
    }

    constexpr STDROMANO_FORCE_INLINE const char* back() const noexcept
    {
        STDROMANO_ASSERT(this->_size > 0 || (this->_is_local || this->_heap_data != nullptr),
                         "String must be valid to get back's address");

        return this->data() + this->_size;
    }

    constexpr STDROMANO_FORCE_INLINE char* back() noexcept
    {
        STDROMANO_ASSERT(this->_size > 0 || (this->_is_local || this->_heap_data != nullptr),
                         "String must be valid to get back's address");

        return this->data() + this->_size;
    }

    /* Number of code units */
    constexpr STDROMANO_FORCE_INLINE size_t size() const noexcept
    {
        return this->_size;
    }

    /* Number of code units */
    constexpr STDROMANO_FORCE_INLINE size_t length() const noexcept
    {
        return this->_size;
    }

    constexpr STDROMANO_FORCE_INLINE size_t capacity() const noexcept
    {
        return this->_capacity;
    }

    constexpr STDROMANO_FORCE_INLINE bool empty() const noexcept
    {
        return this->_size == 0;
    }

    constexpr STDROMANO_FORCE_INLINE bool is_ref() const noexcept
    {
        return this->_is_ref;
    }

    class iterator
    {
    public:
        friend class const_iterator;

        using iterator_category = std::random_access_iterator_tag;
        using value_type = char;
        using difference_type = std::ptrdiff_t;
        using pointer = char*;
        using reference = char&;

        iterator() noexcept : _ptr(nullptr) {}
        explicit iterator(pointer p) noexcept : _ptr(p) {}

        reference operator*() const noexcept { return *this->_ptr; }
        pointer operator->() const noexcept { return this->_ptr; }

        iterator& operator++() noexcept { ++this->_ptr; return *this; }
        iterator operator++(int) noexcept { iterator tmp = *this; ++this->_ptr; return tmp; }

        iterator& operator--() noexcept { --this->_ptr; return *this; }
        iterator operator--(int) noexcept { iterator tmp = *this; --this->_ptr; return tmp; }

        iterator& operator+=(difference_type n) noexcept { this->_ptr += n; return *this; }
        iterator& operator-=(difference_type n) noexcept { this->_ptr -= n; return *this; }

        friend iterator operator+(iterator it, difference_type n) noexcept { return iterator(it._ptr + n); }
        friend iterator operator+(difference_type n, iterator it) noexcept { return iterator(it._ptr + n); }
        friend iterator operator-(iterator it, difference_type n) noexcept { return iterator(it._ptr - n); }
        friend difference_type operator-(const iterator& lhs, const iterator& rhs) noexcept { return lhs._ptr - rhs._ptr; }

        reference operator[](difference_type n) const noexcept { return this->_ptr[n]; }

        bool operator==(const iterator& other) const noexcept { return this->_ptr == other._ptr; }
        bool operator!=(const iterator& other) const noexcept { return this->_ptr != other._ptr; }
        bool operator<(const iterator& other) const noexcept { return this->_ptr < other._ptr; }
        bool operator>(const iterator& other) const noexcept { return this->_ptr > other._ptr; }
        bool operator<=(const iterator& other) const noexcept { return this->_ptr <= other._ptr; }
        bool operator>=(const iterator& other) const noexcept { return this->_ptr >= other._ptr; }

    private:
        pointer _ptr;
    };

    class const_iterator
    {
    public:
        using iterator_category = std::random_access_iterator_tag;
        using value_type = const char;
        using difference_type = std::ptrdiff_t;
        using pointer = const char*;
        using reference = const char&;

        const_iterator() noexcept : _ptr(nullptr) {}
        explicit const_iterator(pointer p) noexcept : _ptr(p) {}
        const_iterator(const iterator& it) noexcept : _ptr(it._ptr) {}

        reference operator*() const noexcept { return *this->_ptr; }
        pointer operator->() const noexcept { return this->_ptr; }

        const_iterator& operator++() noexcept { ++this->_ptr; return *this; }
        const_iterator operator++(int) noexcept { const_iterator tmp = *this; ++this->_ptr; return tmp; }

        const_iterator& operator--() noexcept { --this->_ptr; return *this; }
        const_iterator operator--(int) noexcept { const_iterator tmp = *this; --this->_ptr; return tmp; }

        const_iterator& operator+=(difference_type n) noexcept { this->_ptr += n; return *this; }
        const_iterator& operator-=(difference_type n) noexcept { this->_ptr -= n; return *this; }

        friend const_iterator operator+(const_iterator it, difference_type n) noexcept { return const_iterator(it._ptr + n); }
        friend const_iterator operator+(difference_type n, const_iterator it) noexcept { return const_iterator(it._ptr + n); }
        friend const_iterator operator-(const_iterator it, difference_type n) noexcept { return const_iterator(it._ptr - n); }
        friend difference_type operator-(const const_iterator& lhs, const const_iterator& rhs) noexcept { return lhs._ptr - rhs._ptr; }

        reference operator[](difference_type n) const noexcept { return this->_ptr[n]; }

        bool operator==(const const_iterator& other) const noexcept { return this->_ptr == other._ptr; }
        bool operator!=(const const_iterator& other) const noexcept { return this->_ptr != other._ptr; }
        bool operator<(const const_iterator& other) const noexcept { return this->_ptr < other._ptr; }
        bool operator>(const const_iterator& other) const noexcept { return this->_ptr > other._ptr; }
        bool operator<=(const const_iterator& other) const noexcept { return this->_ptr <= other._ptr; }
        bool operator>=(const const_iterator& other) const noexcept { return this->_ptr >= other._ptr; }

    private:
        pointer _ptr;
    };

    iterator begin() noexcept { return iterator(this->data()); }
    iterator end() noexcept { return iterator(this->data() + this->_size); }

    const_iterator begin() const noexcept { return const_iterator(this->data()); }
    const_iterator end() const noexcept { return const_iterator(this->data() + this->_size); }

    const_iterator cbegin() const noexcept { return const_iterator(this->data()); }
    const_iterator cend() const noexcept { return const_iterator(this->data() + this->_size); }

    constexpr void clear() noexcept
    {
        STDROMANO_ASSERT(!this->_is_ref, "Cannot modify a reference string");

        this->_size = 0;
        this->data()[this->_size] = '\0';
    }

    void push_back(char c)
    {
        STDROMANO_ASSERT(!this->_is_ref, "Cannot modify a reference string");

        if(this->_size >= this->_capacity)
        {
            size_t new_cap = static_cast<size_t>(static_cast<float>(this->_capacity) * STRING_GROWTH_RATE);

            if(new_cap <= this->_size) 
            {
                new_cap = this->_size + 1;
            }

            this->reallocate(new_cap);
        }

        this->data()[this->_size++] = c;
        this->data()[this->_size] = '\0';
    }

    void appendc(const char* c, const size_t n = 0) noexcept
    {
        STDROMANO_ASSERT(!this->_is_ref, "Cannot modify a reference string");

        if(c == nullptr || *c == '\0') 
        {
            return;
        }

        const size_t len_to_append = (n == 0) ? str_len(c) : n;

        if(len_to_append == 0)
        {
            return;
        }

        const size_t required_content_size = this->_size + len_to_append;

        if(required_content_size > this->_capacity)
        {
            size_t new_cap = std::max(required_content_size,
                                      static_cast<size_t>(this->_capacity * STRING_GROWTH_RATE));
            this->reallocate(new_cap);
        }

        mem_cpy(this->data() + this->_size, c, len_to_append);
        this->_size = static_cast<uint32_t>(required_content_size);
        this->data()[this->_size] = '\0';
    }

    void appends(const String& other) noexcept
    {
        STDROMANO_ASSERT(!this->_is_ref, "Cannot modify a reference string");

        if(other.empty())
        {
            return;
        }

        const size_t len_to_append = other.size();
        const size_t required_content_size = this->_size + len_to_append;

        if(required_content_size > this->_capacity)
        {
            size_t new_cap = std::max(required_content_size, static_cast<size_t>(this->_capacity * STRING_GROWTH_RATE));
            this->reallocate(new_cap);
        }
        
        mem_cpy(this->data() + this->_size, other.c_str(), len_to_append);
        this->_size = static_cast<uint32_t>(required_content_size);
        this->data()[this->_size] = '\0';
    }

    template <typename... Args>
    void appendf(fmt::format_string<Args...> fmt, Args&&... args) noexcept
    {
        STDROMANO_ASSERT(!this->_is_ref, "Cannot modify a reference string");

        fmt::format_to(std::back_inserter(*this), fmt, std::forward<Args>(args)...);
    }

    void prependc(const char* c, const size_t n = 0) noexcept
    {
        STDROMANO_ASSERT(!this->_is_ref, "Cannot modify a reference string");

        if(c == nullptr || *c == '\0')
        {
            return;
        }

        const size_t len_to_prepend = (n == 0) ? str_len(c) : n;

        if(len_to_prepend == 0)
        {
            return;
        }

        const size_t required_content_size = this->_size + len_to_prepend;

        if(required_content_size > this->_capacity)
        {
            size_t new_cap = std::max(required_content_size,
                                      static_cast<size_t>(this->_capacity * STRING_GROWTH_RATE));
            this->reallocate(new_cap);
        }

        mem_move(this->data() + len_to_prepend, this->data(), this->_size + 1);
        mem_cpy(this->data(), c, len_to_prepend);

        this->_size = static_cast<uint32_t>(required_content_size);
    }

    void prepends(const String& other) noexcept
    {
        STDROMANO_ASSERT(!this->_is_ref, "Cannot modify a reference string");
        if (other.empty()) return;

        const size_t len_to_prepend = other.size();
        const size_t required_content_size = this->_size + len_to_prepend;

        if(required_content_size > this->_capacity)
        {
            size_t new_cap = std::max(required_content_size, static_cast<size_t>(this->_capacity * STRING_GROWTH_RATE));
            this->reallocate(new_cap);
        }

        mem_move(this->data() + len_to_prepend, this->data(), this->_size + 1);
        mem_cpy(this->data(), other.c_str(), len_to_prepend);
        
        this->_size = static_cast<uint32_t>(required_content_size);
    }


    template <typename... Args>
    void prependf(fmt::format_string<Args...> fmt, Args&&... args) noexcept
    {
        STDROMANO_ASSERT(!this->_is_ref, "Cannot modify a reference string");
        String tmp(fmt, std::forward<Args>(args)...);
        this->prepends(tmp);
    }

    void insertc(size_t position, const char* c, size_t n = 0) noexcept
    {
        STDROMANO_ASSERT(!this->_is_ref, "Cannot modify a reference string");
        STDROMANO_ASSERT(c != nullptr, "Null pointer passed to insertc");
        
        if(position > this->_size)
        {
            position = this->_size;
        }

        if(*c == '\0')
        {
            return;
        }
        
        const size_t insert_len = (n == 0) ? str_len(c) : n;

        if(insert_len == 0)
        {
            return;
        }
        
        const size_t new_size = this->_size + insert_len;
        
        if(new_size > this->_capacity)
        {
            size_t new_cap = std::max(new_size, 
                                      static_cast<size_t>(this->_capacity * STRING_GROWTH_RATE));

            this->reallocate(new_cap);
        }
        
        char* d = this->data();

        mem_move(d + position + insert_len, 
                 d + position, 
                 this->_size - position + 1);
                     
        mem_cpy(d + position, c, insert_len);
        
        this->_size = static_cast<uint32_t>(new_size);
    }

    void inserts(size_t position, const String& other) noexcept
    {
        STDROMANO_ASSERT(!this->_is_ref, "Cannot modify a reference string");

        if(other.empty())
        {
            return;
        }
        
        if(position > this->_size)
        {
            position = this->_size;
        }
        
        const size_t insert_len = other.size();
        const size_t new_size = this->_size + insert_len;
        
        if(new_size > this->_capacity)
        {
            size_t new_cap = std::max(new_size, 
                                      static_cast<size_t>(this->_capacity * STRING_GROWTH_RATE));

            this->reallocate(new_cap);
        }
        
        char* d = this->data();

        mem_move(d + position + insert_len, 
                 d + position, 
                 this->_size - position + 1);

        mem_cpy(d + position, other.c_str(), insert_len);
        
        this->_size = static_cast<uint32_t>(new_size);
    }

    template <typename... Args>
    void insertf(std::size_t position,
                 fmt::format_string<Args...> fmt,
                 Args&&... args) noexcept
    {
        STDROMANO_ASSERT(!this->_is_ref, "Cannot modify a reference string");

        if(position > this->_size)
        {
            position = this->_size;
        }
        
        String tmp;
        fmt::format_to(std::back_inserter(tmp), fmt, std::forward<Args>(args)...);
        
        this->inserts(position, tmp);
    }

    void erase(std::size_t start = 0, std::size_t length = String::NPOS) noexcept
    {
        STDROMANO_ASSERT(!this->_is_ref, "Cannot modify a reference string");

        if (length == 0 || this->_size == 0 || start >= this->_size) 
        {
            return;
        }

        const std::size_t actual_length = (length == String::NPOS || start + length > this->_size)
                               ? this->_size - start
                               : length;

        if (actual_length == 0) 
        {
            return;
        }

        char* d = this->data();
        const std::size_t tail_start = start + actual_length;
        const std::size_t tail_size = this->_size - tail_start + 1;

        mem_move(d + start, d + tail_start, tail_size);
        this->_size -= static_cast<std::uint32_t>(actual_length);
    }

    void shrink_to_fit(const std::size_t size = String::NPOS) noexcept
    {
        STDROMANO_ASSERT(!this->_is_ref, "Cannot modify a reference string");

        const std::size_t new_capacity = std::min(size, static_cast<std::size_t>(this->_size)) + 1;

        this->reallocate(new_capacity);
    }

    /* Python-like string methods */

    constexpr String upper() const noexcept
    {
        if(this->empty())
        {
            return String();
        }

        String result = this->copy();

        STDROMANO_ASSERT(!result.is_ref(), "Copy should not be a reference for modification");

        for(uint32_t i = 0; i < result.size(); i++)
        {
            if(is_letter(result[i]))
            {
                result[i] = to_upper(result[i]);
            }
        }

        return result;
    }

    /* constexpr */ String lower() const noexcept
    {
        if(this->empty())
        {
            return String();
        }

        String result = this->copy();

        STDROMANO_ASSERT(!result.is_ref(), "Copy should not be a reference for modification");

        detail::tolower(result.c_str(), result.size());

        /* TODO: switch to C++20 and use std::is_constant_evaluated() */

        /*
        for(uint32_t i = 0; i < result.size(); i++)
        {
            if(is_letter(result[i]))
            {
                result[i] = to_lower(result[i]);
            }
        }
        */

        return result;
    }

    constexpr String capitalize() const noexcept
    {
        if(this->empty()) 
        {
            return String();
        }

        String result = this->copy(); 

        STDROMANO_ASSERT(!result.is_ref(), "Copy should not be a reference for modification");

        if(is_letter(result[0])) 
        {
            result[0] = to_upper(result[0]);
        }

        for(uint32_t i = 1; i < result.size(); i++)
        {
            if(is_letter(result[i]))
            {
                result[i] = to_lower(result[i]);
            }
        }

        return result;
    }

    String lstrip(char c = ' ') const noexcept
    {
        if (this->empty()) return String::make_ref(this->data(), 0);

        const char* start_ptr = this->data();
        const char* end_ptr = this->data() + this->_size;

        const char* current_ptr = start_ptr;

        while(current_ptr < end_ptr && *current_ptr == c)
        {
            current_ptr++;
        }

        return String::make_ref(current_ptr, static_cast<size_t>(end_ptr - current_ptr));
    }

    String rstrip(char c = ' ') const noexcept
    {
        if (this->empty()) return String::make_ref(this->data(), 0);

        const char* start_ptr = this->data();
        const char* current_end_ptr = this->data() + this->_size;

        while(current_end_ptr > start_ptr && *(current_end_ptr - 1) == c)
        {
            current_end_ptr--;
        }

        return String::make_ref(start_ptr, static_cast<size_t>(current_end_ptr - start_ptr));
    }

    String strip(char c = ' ') const noexcept
    {
        if (this->empty()) return String::make_ref(this->data(), 0);
        
        const char* original_start_ptr = this->data();
        const char* original_end_ptr = this->data() + this->_size;

        const char* new_start_ptr = original_start_ptr;

        while(new_start_ptr < original_end_ptr && *new_start_ptr == c)
        {
            new_start_ptr++;
        }

        if(new_start_ptr == original_end_ptr) 
        {
            return String::make_ref(new_start_ptr, 0);
        }

        const char* new_end_ptr = original_end_ptr;

        while(new_end_ptr > new_start_ptr && *(new_end_ptr - 1) == c)
        {
            new_end_ptr--;
        }
        
        return String::make_ref(new_start_ptr, static_cast<size_t>(new_end_ptr - new_start_ptr));
    }


    constexpr String substr(const size_t position) const noexcept
    {
        STDROMANO_ASSERT(position <= this->_size, "Substring position out of bounds");

        if(position >= this->_size)
        {
            return String::make_ref(this->data() + this->_size, 0);
        }

        return String::make_ref(this->data() + position, this->_size - position);
    }

    constexpr String substr(const size_t start_pos, const size_t length) const noexcept
    {
        STDROMANO_ASSERT(start_pos <= this->_size, "Substring start position out of bounds");

        size_t actual_length = length;

        if(start_pos + length > this->_size)
        {
            actual_length = this->_size - start_pos;
        }

        if(start_pos >= this->_size)
        {
            return String::make_ref(this->data() + this->_size, 0);
        }

        return String::make_ref(this->data() + start_pos, actual_length);
    }

    String replace(char occurence, char replacement) const noexcept
    {
        String res = this->copy();

        STDROMANO_ASSERT(!res.is_ref(), "Copy should not be a reference for modification");

        for(uint32_t i = 0; i < res.size(); i++)
        {
            if(res[i] == occurence)
            {
                res[i] = replacement;
            }
        }

        return res;
    }

    constexpr bool startswith(const String& prefix) const noexcept
    {
        if(prefix.size() > this->size())
        {
            return false;
        }

        return std::strncmp(this->data(), prefix.data(), prefix.size()) == 0;
    }

    constexpr bool endswith(const String& suffix) const noexcept
    {
        if(suffix.size() > this->size())
        {
            return false;
        }

        return std::strncmp(this->data() + this->size() - suffix.size(), suffix.data(), suffix.size()) == 0;
    }

    int find(const String& substring) const noexcept
    {
        if(substring.empty()) 
        {
            return 0;
        }

        if(substring.size() > this->size()) 
        {
            return -1;
        }

        const char* found = std::strstr(this->c_str(), substring.c_str());

        return found != nullptr ? static_cast<int>(found - this->c_str()) : -1;
    }

    bool split(const String& sep, split_iterator& it, String& split_out) const noexcept
    {
        if(sep.empty())
        {
            split_out = String::make_ref(*this);
            it = this->_size;
            return false;
        }

        if(it >= this->_size)
        {
            split_out = String();
            return false;
        }

        const char* current_start = this->data() + it;
        const char* found_sep = std::strstr(current_start, sep.c_str());

        if(found_sep != nullptr)
        {
            split_out = String::make_ref(current_start, static_cast<size_t>(found_sep - current_start));
            it = static_cast<size_t>(found_sep - this->data()) + sep.size();
            return true;
        }
        else
        {
            split_out = String::make_ref(current_start, this->_size - it);
            it = this->_size;
            return true;
        }
    }

    String lsplit(const String& sep, String* rsplit_out = nullptr) const noexcept
    {
        if(sep.empty())
        {
            if(rsplit_out != nullptr) 
            {
                *rsplit_out = String::make_ref(this->data() + this->size(), 0);
            }

            return String::make_ref(*this);
        }

        const char* found_sep = std::strstr(this->data(), sep.c_str());

        if(found_sep != nullptr)
        {
            const size_t lsplit_len = static_cast<size_t>(found_sep - this->data());

            if(rsplit_out != nullptr)
            {
                *rsplit_out = String::make_ref(found_sep + sep.size(),
                                               this->size() - lsplit_len - sep.size());
            }

            return String::make_ref(this->data(), lsplit_len);
        }
        
        if(rsplit_out != nullptr) 
        {
            *rsplit_out = String::make_ref(this->data() + this->size(), 0);
        }

        return String::make_ref(*this);
    }

    String rsplit(const String& sep, String* lsplit_out = nullptr) const noexcept
    {
        if(sep.empty())
        {
            if(lsplit_out != nullptr)
            {
                *lsplit_out = String::make_ref(this->data(), 0);
            }

            return String::make_ref(*this);
        }

        const char* found_sep = strrstr(this->data(), sep.c_str());

        if(found_sep != nullptr)
        {
            const size_t rsplit_start_offset = static_cast<size_t>(found_sep - this->data()) + sep.size();
            const size_t rsplit_len = this->size() - rsplit_start_offset;

            if(lsplit_out != nullptr)
            {
                *lsplit_out = String::make_ref(this->data(), static_cast<size_t>(found_sep - this->data()));
            }

            return String::make_ref(this->data() + rsplit_start_offset, rsplit_len);
        }

        if(lsplit_out != nullptr) 
        {
            *lsplit_out = String::make_ref(this->data(), 0);
        }

        return String::make_ref(*this);
    }

    String zfill(const uint32_t total_width) const noexcept
    {
        if(this->_size >= total_width) 
        {
            return String::make_ref(*this);
        }
        
        const uint32_t num_zeros = total_width - this->_size;
        
        String result;
        result.reallocate(total_width);
        
        mem_set(result.data(), '0', num_zeros);
        mem_cpy(result.data() + num_zeros, this->data(), this->_size);
        
        result._size = total_width;
        result.data()[result._size] = '\0';
        
        return result;
    }

    constexpr bool is_digit() const noexcept
    {
        for(std::size_t i = 0; i < this->_size; i++)
        {
            if(static_cast<int>(this->data()[i] - 48) > 9)
            {
                return false;
            }
        }

        return true;
    }

    /* Conversion */

    long long to_long_long() const noexcept
    {
        if(this->empty())
        {
            return 0;
        }

        return std::atoll(this->c_str());
    }

    double to_double() const noexcept
    {
        if(this->empty()) 
        {
            return 0.0;
        }

        return std::strtod(this->c_str(), nullptr);
    }

    bool to_bool() const noexcept
    {
        if(this->empty()) return false;
        
        if(this->size() == 1) 
        {
            if (this->data()[0] == '0') return false;
            if (this->data()[0] == '1') return true;
        }

        if(this->size() == 4 && 
           (to_lower(this->data()[0]) == 't') &&
           (to_lower(this->data()[1]) == 'r') &&
           (to_lower(this->data()[2]) == 'u') &&
           (to_lower(this->data()[3]) == 'e')) 
        {
            return true;
        }

        if(this->size() == 5 &&
           (to_lower(this->data()[0]) == 'f') &&
           (to_lower(this->data()[1]) == 'a') &&
           (to_lower(this->data()[2]) == 'l') &&
           (to_lower(this->data()[3]) == 's') &&
           (to_lower(this->data()[4]) == 'e')) 
        {
            return false;
        }
        
        return false;
    }
};

/* Returns -1 if lhs < rhs, 0 if lhs == rhs, 1 if lhs > rhs (uses lexicographical compare) */
STDROMANO_API int strcmp(const String<>& lhs, const String<>& rhs, bool case_sensitive = true) noexcept;

/* String dynamically sized */
using StringD = String<>;

/* Fixed size strings */
using String1024 = String<1024>;
using String260 = String<260>;
using String128 = String<128>;
using String32 = String<32>;

#if defined(STDROMANO_WIN)
STDROMANO_EXPIMP_TEMPLATE template class STDROMANO_API String<>;
#endif /* defined(STDROMANO_WIN) */

STDROMANO_NAMESPACE_END

template <size_t Capacity>
struct fmt::formatter<stdromano::String<Capacity>>
{
    constexpr auto parse(format_parse_context& ctx)
    {
        return ctx.begin();
    }

    auto format(const stdromano::String<Capacity>& s, format_context& ctx) const
    {
        return !s.empty() ? format_to(ctx.out(), "{}", fmt::string_view(s.c_str(), s.size()))
                          : format_to(ctx.out(), "");
    }
};

template <size_t Capacity>
struct std::hash<stdromano::String<Capacity>>
{
    std::size_t operator()(const stdromano::String<Capacity>& s) const
    {
        return static_cast<std::size_t>(
            stdromano::hash_murmur3(static_cast<const void*>(s.c_str()), s.size(), 483910));
    }
};

#endif /* !defined(__STDROMANO_STRING) */