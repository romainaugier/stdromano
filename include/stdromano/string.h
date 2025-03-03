// SPDX-License-Identifier: BSD-3-Clause 
// Copyright (c) 2025 - Present Romain Augier 
// All rights reserved. 

#pragma once

#if !defined(__STDROMANO_STRING)
#define __STDROMANO_STRING

#include "stdromano/memory.h"
#include "stdromano/hash.h"

#include "spdlog/fmt/fmt.h"

STDROMANO_NAMESPACE_BEGIN

template <size_t LocalCapacity = 7>
class String 
{
    STDROMANO_STATIC_ASSERT(LocalCapacity <= 7, "LocalCapacity must be <= 7 for 16-byte total size");

    static constexpr float STRING_GROWTH_RATE = 1.61f;

public:
    using value_type = char;

private:

    union {
        char* _heap_data;
        char _local_data[LocalCapacity + 1];
    };
    
    uint32_t _size : 31;
    uint32_t _capacity : 31;
    uint32_t _is_local : 1;
    uint32_t _is_ref : 1;

    void reallocate(size_t new_capacity) noexcept
    {
        STDROMANO_ASSERT(!this->_is_ref && "Cannot modify a reference string");

        char* new_data = (char*)mem_aligned_alloc((new_capacity + 1) * sizeof(char), 32);
        std::memcpy(new_data, this->data(), this->_size);
        new_data[this->_size] = '\0';
        
        if(!this->_is_local) 
        {
            mem_aligned_free(this->_heap_data);
        }

        this->_heap_data = new_data;
        this->_capacity = static_cast<uint32_t>(new_capacity);
        this->_is_local = 0;
    }

public:
    String() noexcept : _size(0), _capacity(LocalCapacity), _is_local(1), _is_ref(0) 
    {
        this->_local_data[0] = '\0';
    }

    String(const char* str) : String() 
    {
        const size_t size = std::strlen(str);

        if(size > LocalCapacity) 
        {
            this->reallocate(size);
        }

        std::memcpy(this->data(), str, size + 1);
        this->_size = static_cast<uint32_t>(size);
    }

    template<typename ...Args>
    String(fmt::format_string<Args...> fmt, Args&&... args) : String()
    {
        fmt::format_to(std::back_inserter(*this), fmt, std::forward<Args>(args)...);
        this->data()[this->_size] = '\0';
    }

    String(const String& other) : 
        _size(other._size), 
        _capacity(other._is_local ? LocalCapacity : other._capacity),
        _is_local(other._is_local),
        _is_ref(0) 
    {
        if(other._is_ref) 
        {
            this->_heap_data = other._heap_data;
            this->_is_ref = 1;
        }
        else if (this->_is_local) 
        {
            std::memcpy(this->_local_data, other._local_data, this->_size + 1);
        }
        else
        {
            this->_heap_data = (char*)mem_aligned_alloc((this->_capacity + 1) * sizeof(char), 32);
            std::memcpy(this->_heap_data, other._heap_data, this->_size + 1);
        }
    }

    ~String() {
        if(!this->_is_ref && !this->_is_local) 
        {
            mem_aligned_free(this->_heap_data);
        }
    }

    bool operator==(const String<>& other) const
    {
        if(this->empty())
        {
            return other.empty() ? true : false;
        }
        else if(other.empty())
        {
            return false;
        }

        return this->size() == other.size() && std::memcmp(this->c_str(), other.c_str(), this->size()) == 0;
    }

    bool operator!=(const String<>& other) const
    {
        return !this->operator==(other);
    }

    static String make_ref(const char* str, size_t size) noexcept 
    {
        String s;
        s._heap_data = const_cast<char*>(str);
        s._size = static_cast<uint32_t>(size);
        s._capacity = static_cast<uint32_t>(size);
        s._is_local = 0;
        s._is_ref = 1;
        return s;
    }

    const char* data() const noexcept { return this->_is_local ? this->_local_data : this->_heap_data; }
    char* data() noexcept { return this->_is_local ? this->_local_data : this->_heap_data; }

    const char* c_str() const noexcept { return this->_is_local ? this->_local_data : this->_heap_data; }
    char* c_str() noexcept { return this->_is_local ? this->_local_data : this->_heap_data; }

    const char* back() const noexcept { return this->c_str() + this->_size; }
    char* back() noexcept { return this->c_str() + this->_size; }
    
    size_t size() const noexcept { return this->_size; }
    size_t capacity() const noexcept { return this->_capacity; }
    bool empty() const noexcept { return this->_size == 0; }
    bool is_ref() const noexcept { return this->_is_ref; }
    
    void push_back(char c) 
    {
        STDROMANO_ASSERT(!this->_is_ref && "Cannot modify a reference string");

        if(this->_size >= this->_capacity) 
        {
            this->reallocate(this->_capacity * STRING_GROWTH_RATE);
        }

        this->data()[this->_size++] = c;
        this->data()[this->_size] = '\0';
    }

    void appendc(const char* c, const size_t n = 0) noexcept
    {
        STDROMANO_ASSERT(!this->_is_ref && "Cannot modify a reference string");

        const size_t size = n == 0 ? std::strlen(c) : n;

        if((this->_size + size + 1) > this->_capacity)
        {
            this->reallocate(size + 1);
        }

        std::memcpy(this->back(), c, size + 1);
        this->_size += static_cast<uint32_t>(size);
    }

    void appends(const String& other) noexcept
    {
        STDROMANO_ASSERT(!this->_is_ref && "Cannot modify a reference string");

        if((this->_size + other._size + 1) > this->_capacity)
        {
            this->reallocate(this->_size + other._size + 1);
        }

        std::memcpy(this->back(), other.c_str(), other.size() + 1);
        this->_size += static_cast<uint32_t>(other._size);
    }

    template<typename ...Args>
    void appendf(fmt::format_string<Args...> fmt, Args&&... args) noexcept
    {
        STDROMANO_ASSERT(!this->_is_ref && "Cannot modify a reference string");

        fmt::format_to(std::back_inserter(*this), fmt, std::forward<Args>(args)...);
    }

    void prependc(const char* c, const size_t n = 0) noexcept
    {
        STDROMANO_ASSERT(!this->_is_ref && "Cannot modify a reference string");

        const size_t size = n == 0 ? std::strlen(c) : n;

        if((this->_size + size + 1) > this->_capacity)
        {
            this->reallocate(this->_size + size + 1);
        }

        std::memmove(this->c_str() + size, this->c_str(), this->_size + 1);
        std::memcpy(this->c_str(), c, size);
            
        this->_size += static_cast<uint32_t>(size);
    }

    void prepends(const String& other) noexcept
    {
        STDROMANO_ASSERT(!this->_is_ref && "Cannot modify a reference string");

        if((this->_size + other._size + 1) > this->_capacity)
        {
            this->reallocate(this->_size + other._size + 1);
        }

        std::memmove(this->c_str() + other._size, this->c_str(), this->_size + 1);
        std::memcpy(this->c_str(), other.c_str(), other._size);
            
        this->_size += static_cast<uint32_t>(other._size);
    }

    template<typename ...Args>
    void prependf(fmt::format_string<Args...> fmt, Args&&... args) noexcept
    {
        STDROMANO_ASSERT(!this->_is_ref && "Cannot modify a reference string");

        String tmp(fmt, std::forward<Args>(args)...);

        this->prepends(tmp);
    }
};

STDROMANO_NAMESPACE_END

template <>
struct fmt::formatter<stdromano::String<>> {
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }    
    
    auto format(const stdromano::String<>& s, format_context& ctx) const { return !s.empty() ? format_to(ctx.out(), "{}", fmt::string_view(s.c_str(), s.size())) : format_to(ctx.out(), ""); }
};

template<>
struct std::hash<stdromano::String<>>
{
    std::size_t operator()(const stdromano::String<>& s) const
    {
        return static_cast<std::size_t>(stdromano::hash_murmur3(static_cast<const void*>(s.c_str()), s.size(), 483910));
    }
};

#endif /* !defined(__STDROMANO_STRING) */