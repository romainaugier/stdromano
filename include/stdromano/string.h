// SPDX-License-Identifier: BSD-3-Clause 
// Copyright (c) 2025 - Present Romain Augier 
// All rights reserved. 

#pragma once

#if !defined(__STDROMANO_STRING)
#define __STDROMANO_STRING

#include "stdromano/memory.h"
#include "stdromano/hash.h"
#include "stdromano/char.h"

#include "spdlog/fmt/fmt.h"

STDROMANO_NAMESPACE_BEGIN

template <size_t LocalCapacity = 7>
class String 
{
    static constexpr float STRING_GROWTH_RATE = 1.61f;
    static constexpr size_t STRING_ALIGNMENT = 32;

public:
    using value_type = char;
    using split_iterator = size_t;

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

        char* new_data = (char*)mem_aligned_alloc((new_capacity + 1) * sizeof(char), STRING_ALIGNMENT);
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

    String(const String& other) noexcept : 
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
            this->_heap_data = (char*)mem_aligned_alloc((this->_capacity + 1) * sizeof(char), STRING_ALIGNMENT);
            std::memcpy(this->_heap_data, other._heap_data, this->_size + 1);
        }
    }

    String& operator=(const String& other) noexcept 
    {
        if(this == &other) 
        {
            return *this;
        }

        if (!this->_is_ref && !this->_is_local) 
        {
            mem_aligned_free(this->_heap_data);
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
            std::memcpy(this->_local_data, other._local_data, this->_size + 1);
        }
        else 
        {
            this->_heap_data = (char*)mem_aligned_alloc((this->_capacity + 1) * sizeof(char), STRING_ALIGNMENT);
            std::memcpy(this->_heap_data, other._heap_data, this->_size + 1);
        }

        return *this;
    }

    String(String&& other) noexcept :
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
            this->_heap_data = other._heap_data;
            other._heap_data = nullptr;
        }

        other._size = 0;
        other._capacity = LocalCapacity;
        other._is_local = 1;
        other._is_ref = 0;
        other._local_data[0] = '\0';
    }

    String& operator=(String&& other) noexcept 
    {
        if(this == &other) 
        {
            return *this;
        }

        if (!this->_is_ref && !this->_is_local) 
        {
            mem_aligned_free(this->_heap_data);
        }

        this->_size = other._size;
        this->_capacity = other._is_local ? LocalCapacity : other._capacity;
        this->_is_local = other._is_local;
        this->_is_ref = other._is_ref;

        if(other._is_ref) 
        {
            this->_heap_data = other._heap_data;
        }
        else if (this->_is_local) 
        {
            std::memcpy(this->_local_data, other._local_data, this->_size + 1);
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
        other._local_data[0] = '\0';

        return *this;
    }

    ~String() 
    {
        if(!this->_is_ref && !this->_is_local) 
        {
            mem_aligned_free(this->_heap_data);
        }
    }

    STDROMANO_FORCE_INLINE char& operator[](const size_t i) noexcept { assert(i < this->size()); return this->data()[i]; }
    STDROMANO_FORCE_INLINE const char& operator[](const size_t i) const noexcept { assert(i < this->size()); return this->data()[i]; }

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

    static String make_ref(const String& str) noexcept
    {
        return String::make_ref(str.data(), str.size());
    }

    STDROMANO_FORCE_INLINE const char* data() const noexcept { return this->_is_local ? this->_local_data : this->_heap_data; }
    STDROMANO_FORCE_INLINE char* data() noexcept { return this->_is_local ? this->_local_data : this->_heap_data; }

    STDROMANO_FORCE_INLINE const char* c_str() const noexcept { return this->_is_local ? this->_local_data : this->_heap_data; }
    STDROMANO_FORCE_INLINE char* c_str() noexcept { return this->_is_local ? this->_local_data : this->_heap_data; }

    STDROMANO_FORCE_INLINE const char* back() const noexcept { return this->c_str() + this->_size; }
    STDROMANO_FORCE_INLINE char* back() noexcept { return this->c_str() + this->_size; }
    
    STDROMANO_FORCE_INLINE size_t size() const noexcept { return this->_size; }
    STDROMANO_FORCE_INLINE size_t capacity() const noexcept { return this->_capacity; }
    STDROMANO_FORCE_INLINE bool empty() const noexcept { return this->_size == 0; }
    STDROMANO_FORCE_INLINE bool is_ref() const noexcept { return this->_is_ref; }
    
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

    String upper() const noexcept
    {
        if(this->size() == 0) return String();

        String result(*this);

        for(uint32_t i = 0; i < result.size(); i++)
        {
            result[i] = to_upper(result[i]);
        }

        return std::move(result);
    }

    String lower() const noexcept
    {
        if(this->size() == 0) return String();

        String result(*this);

        for(uint32_t i = 0; i < result.size(); i++)
        {
            result[i] = to_lower(result[i]);
        }

        return std::move(result);
    }

    String capitalize() const noexcept
    {
        if(this->size() == 0) return String();

        String result(*this);

        for(uint32_t i = 0; i < result.size(); i++)
        {
            result[i] = to_lower(result[i]);
        }

        result[0] = to_upper(result[0]);

        return std::move(result);
    }

    String lstrip(char c = ' ') const noexcept
    {
        const char* start = this->data();
        const char* end = this->back();

        while(start < end && *start == c)
        {
            start++;
        }

        return String::make_ref(start, static_cast<size_t>(end - start));
    }

    String rstrip(char c = ' ') const noexcept
    {
        const char* start = this->data();
        const char* end = this->back();

        while(end > start && *end == c)
        {
            end--;
        }

        return String::make_ref(start, static_cast<size_t>(end - start));
    }

    String strip(char c = ' ') const noexcept
    {
        const char* start = this->data();
        const char* end = this->back();

        while(start < end && *start == c)
        {
            start++;
        }

        while(end > start && *end == c)
        {
            end--;
        }

        return String::make_ref(start, static_cast<size_t>(end - start));
    }

    bool startswith(const String& prefix) const noexcept
    {
        if(prefix.size() > this->size())
        {
            return false;
        }

        return std::strncmp(this->data(), prefix.data(), prefix.size()) == 0;
    }

    bool endswith(const String& suffix) const noexcept
    {
        if(suffix.size() > this->size())
        {
            return false;
        }

        return std::strncmp(this->back() - suffix.size(), suffix.data(), suffix.size()) == 0;
    }

    int find(const String& substring) const noexcept
    {
        if(substring.empty() || substring.size() > this->size()) return 0;

        const char* found = std::strstr(this->c_str(), substring.c_str());

        return found != nullptr ? static_cast<int>(found - this->c_str()) : -1;
    }

    bool split(const String& sep, split_iterator& it, String& split) const noexcept
    {
        if(sep.empty()) 
        {
            split = String::make_ref(*this);
            it = this->_size;
            return false;
        }

        if(it >= this->_size) 
        {
            split = String<>();
            return false;
        }

        const char* start = this->data() + it;
        const char* found = std::strstr(start, sep.c_str());
        
        if(found != nullptr) 
        {
            split = String::make_ref(start, found - start);
            it = static_cast<size_t>(found - this->data()) + sep.size();
            return true;
        }
        else 
        {
            split = String::make_ref(start, this->_size - it);
            it = this->_size;
            return false;
        }
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
