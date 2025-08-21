// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - Present Romain Augier
// All rights reserved.

#pragma once

#if !defined(__STDROMANO_JSON)
#define __STDROMANO_JSON

#include "stdromano/string.hpp"
#include "stdromano/vector.hpp"
#include "stdromano/hashmap.hpp"

STDROMANO_NAMESPACE_BEGIN

class STDROMANO_API JsonObject;

using JsonList = Vector<JsonObject>;
using JsonDict = HashMap<StringD, JsonObject>;

enum JsonObjectType : uint32_t
{
    JsonObjectType_ValueInt = 1,
    JsonObjectType_ValueFloat = 2,
    JsonObjectType_ValueString = 3,
    JsonObjectType_ValueBoolean = 4,
    JsonObjectType_ValueNull = 5,
    JsonObjectType_List = 6,
    JsonObjectType_Dict = 7,
};

template<typename T> struct json_traits { static const std::size_t id = 0; };
template<> struct json_traits<std::int64_t> { static const std::size_t id = 1; };
template<> struct json_traits<double> { static const std::size_t id = 2; };
template<> struct json_traits<StringD> { static const std::size_t id = 3; };
template<> struct json_traits<bool> { static const std::size_t id = 4; };
template<> struct json_traits<JsonList> { static const std::size_t id = 6; };
template<> struct json_traits<JsonDict> { static const std::size_t id = 7; };

class JsonObject
{
    static constexpr std::size_t STORAGE_MAX_SIZE = sizeof(JsonDict);
    static constexpr std::size_t STORAGE_ALIGNMENT = alignof(JsonDict);

private: 
    typename std::aligned_storage<STORAGE_MAX_SIZE, STORAGE_ALIGNMENT>::type _data;
    std::uint32_t _type = 0;

    void clear(const bool clear_if_string = true) noexcept 
    {
        switch(this->_type) 
        {
            case JsonObjectType_ValueString:
                reinterpret_cast<StringD*>(&_data)->~StringD();
                break;
            case JsonObjectType_List:
                reinterpret_cast<JsonList*>(&_data)->~Vector();
                break;
            case JsonObjectType_Dict:
                reinterpret_cast<JsonDict*>(&_data)->~HashMap();
                break;
            default:
                break;
        }

        this->_type = JsonObjectType_ValueNull;
    }

public:
    JsonObject() : _type(JsonObjectType_ValueNull) {}

    ~JsonObject() 
    {
        this->clear();
    }

    JsonObject(const JsonObject& other) : _type(0) 
    {
        this->clear();

        switch(other._type) 
        {
            case JsonObjectType_ValueInt:
                ::new (&_data) std::int64_t(*reinterpret_cast<const std::int64_t*>(&other._data));
                break;
            case JsonObjectType_ValueFloat:
                ::new (&_data) double(*reinterpret_cast<const double*>(&other._data));
                break;
            case JsonObjectType_ValueString:
                ::new (&_data) StringD(*reinterpret_cast<const StringD*>(&other._data));
                break;
            case JsonObjectType_ValueBoolean:
                ::new (&_data) bool(*reinterpret_cast<const bool*>(&other._data));
                break;
            case JsonObjectType_List:
                ::new (&_data) JsonList(*reinterpret_cast<const JsonList*>(&other._data));
                break;
            case JsonObjectType_Dict:
                ::new (&_data) JsonDict(*reinterpret_cast<const JsonDict*>(&other._data));
                break;
            case JsonObjectType_ValueNull:
                break;
        }

        this->_type = other._type;
    }

    JsonObject(JsonObject&& other) noexcept : _type(other._type) 
    {
        std::memcpy(&_data, &other._data, STORAGE_MAX_SIZE);
        other._type = JsonObjectType_ValueNull;
    }

    JsonObject& operator=(const JsonObject& other) 
    {
        if(this != &other) 
        {
            this->clear();

            switch(other._type) 
            {
                case JsonObjectType_ValueInt:
                    ::new (&_data) int64_t(*reinterpret_cast<const std::int64_t*>(&other._data));
                    break;
                case JsonObjectType_ValueFloat:
                    ::new (&_data) double(*reinterpret_cast<const double*>(&other._data));
                    break;
                case JsonObjectType_ValueString:
                    ::new (&_data) StringD(*reinterpret_cast<const StringD*>(&other._data));
                    break;
                case JsonObjectType_ValueBoolean:
                    ::new (&_data) bool(*reinterpret_cast<const bool*>(&other._data));
                    break;
                case JsonObjectType_List:
                    ::new (&_data) JsonList(*reinterpret_cast<const JsonList*>(&other._data));
                    break;
                case JsonObjectType_Dict:
                    ::new (&_data) JsonDict(*reinterpret_cast<const JsonDict*>(&other._data));
                    break;
                case JsonObjectType_ValueNull:
                    break;
            }
            _type = other._type;
        }
        return *this;
    }

    JsonObject& operator=(JsonObject&& other) noexcept 
    {
        if(this != &other) 
        {
            this->clear();
            this->_type = other._type;
            std::memcpy(&_data, &other._data, STORAGE_MAX_SIZE);
            other._type = JsonObjectType_ValueNull;
        }

        return *this;
    }

    template<typename T>
    JsonObject(const T& value) : _type(0) 
    {
        constexpr size_t type_id = json_traits<T>::id;
        static_assert(type_id > 0 && type_id < 8, "Unsupported type for JsonObject");
        
        ::new (&this->_data) T(value);
        this->_type = type_id;
    }

    template<typename T>
    JsonObject(T&& value) : _type(0) 
    {
        constexpr size_t type_id = json_traits<T>::id;
        static_assert(type_id > 0 && type_id < 8, "Unsupported type for JsonObject");
        
        ::new (&this->_data) T(value);
        this->_type = type_id;
    }

    template<typename T>
    const T& get() const 
    {
        if(_type != json_traits<T>::id) 
        {
            throw std::runtime_error("Type mismatch in JsonObject::get");
        }

        return *reinterpret_cast<const T*>(&_data);
    }

    template<typename T>
    T& get() 
    {
        if(this->_type != json_traits<T>::id) 
        {
            throw std::runtime_error("Type mismatch in JsonObject::get");
        }

        return *reinterpret_cast<T*>(&this->_data);
    }

    StringD get_as_string() const noexcept
    {
        switch(this->type())
        {
            case JsonObjectType_ValueInt:
                return StringD::make_fmt("{}", int64_t(*reinterpret_cast<const std::int64_t*>(&this->_data)));
            case JsonObjectType_ValueFloat:
                return StringD::make_fmt("{}", double(*reinterpret_cast<const double*>(&this->_data)));
            case JsonObjectType_ValueString:
                return StringD("\"{}\"", *reinterpret_cast<const StringD*>(&this->_data));
            case JsonObjectType_ValueBoolean:
                return StringD::make_fmt("{}", bool(*reinterpret_cast<const bool*>(&this->_data)));
            case JsonObjectType_List:
                break;
            case JsonObjectType_Dict:
                break;
            case JsonObjectType_ValueNull:
                return StringD("null");
        }

        return StringD();
    }

    template<typename T>
    void set(const T& value)
    {
        this->clear();

        ::new (&this->_data) T(value);
        this->_type = json_traits<T>::id;
    }

    template<typename T>
    void set(T&& value)
    {
        this->clear();

        ::new (&this->_data) std::decay_t<T>(std::forward<T>(value));
        this->_type = json_traits<T>::id;
    }

    STDROMANO_FORCE_INLINE bool is_null() const { return _type == JsonObjectType_ValueNull; }
    STDROMANO_FORCE_INLINE bool is_int() const { return _type == JsonObjectType_ValueInt; }
    STDROMANO_FORCE_INLINE bool is_float() const { return _type == JsonObjectType_ValueFloat; }
    STDROMANO_FORCE_INLINE bool is_string() const { return _type == JsonObjectType_ValueString; }
    STDROMANO_FORCE_INLINE bool is_boolean() const { return _type == JsonObjectType_ValueBoolean; }
    STDROMANO_FORCE_INLINE bool is_list() const { return _type == JsonObjectType_List; }
    STDROMANO_FORCE_INLINE bool is_dict() const { return _type == JsonObjectType_Dict; }

    STDROMANO_FORCE_INLINE std::uint32_t type() const { return _type; }
};

class STDROMANO_API Json
{
private:
    JsonObject _root;

public:
    explicit Json() noexcept {}

    ~Json() noexcept {}

    bool loadf(const StringD& file_path,
               bool do_utf8_validation = true) noexcept;

    bool loads(const StringD& text,
               bool do_utf8_validation = true) noexcept;

    bool dumpf(const StringD& file_path) noexcept;

    bool dumps(StringD& text) noexcept;

    void from_dict(const JsonDict& dict) noexcept;

    void from_list(const JsonList& list) noexcept;

    JsonObject& root() noexcept 
    {
        return this->_root;
    }
};

STDROMANO_NAMESPACE_END

#endif /* !defined(__STDROMANO_JSON) */