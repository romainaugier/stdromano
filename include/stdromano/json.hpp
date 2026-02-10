// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - Present Romain Augier
// All rights reserved.

#pragma once

#if !defined(__STDROMANO_JSON)
#define __STDROMANO_JSON

#include "stdromano/string.hpp"

#include <cstdint>
#include <cstddef>

STDROMANO_NAMESPACE_BEGIN

/* Internal forward declarations (opaque to the user) */
struct JsonParser_;
struct JsonWriter_;

class Json;

struct JsonKeyValue
{
    const char* key;
    class JsonObject* value;
};

class STDROMANO_API JsonObject
{
    friend class Json;
    friend struct JsonParser_;
    friend struct JsonWriter_;

    uint64_t _tags;

    union
    {
        bool b;
        std::uint64_t u64;
        std::int64_t i64;
        double f64;
        const char* str;
        void* ptr;
    } _value;

public:
    JsonObject() noexcept : _tags(0), _value{} {}

    // Type checks

    bool is_null() const noexcept;
    bool is_bool() const noexcept;
    bool is_u64() const noexcept;
    bool is_i64() const noexcept;
    bool is_f64() const noexcept;
    bool is_str() const noexcept;
    bool is_array() const noexcept;
    bool is_dict() const noexcept;

    // When a float or int is parsed and has an invalid value, a invalid tag
    // will be set, and can be check here
    // If it is invalid, get_u64/get_i64/get_f64 will return

    bool is_valid() const noexcept;

    // Getters

    bool get_bool() const noexcept;
    std::uint64_t get_u64() const noexcept;
    std::int64_t get_i64() const noexcept;
    double get_f64() const noexcept;
    const char* get_str() const noexcept;
    std::size_t get_str_size() const noexcept;

    // Container sizes

    std::size_t array_size() const noexcept;
    std::size_t dict_size() const noexcept;

    // Dict lookup

    JsonObject* dict_find(const char* key) const noexcept;

    // Iterators

    class ArrayIterator
    {
        void* _current;

    public:
        explicit ArrayIterator(void* current) noexcept : _current(current) {}

        JsonObject* operator*() const noexcept;
        ArrayIterator& operator++() noexcept;
        bool operator!=(const ArrayIterator& other) const noexcept;
    };

    class DictIterator
    {
        void* _current;

    public:
        explicit DictIterator(void* current) noexcept : _current(current) {}

        JsonKeyValue operator*() const noexcept;
        DictIterator& operator++() noexcept;
        bool operator!=(const DictIterator& other) const noexcept;
    };

    class ArrayRange
    {
        void* _head;

    public:
        explicit ArrayRange(void* head) noexcept : _head(head) {}

        ArrayIterator begin() const noexcept;
        ArrayIterator end() const noexcept;
    };

    class DictRange
    {
        void* _head;

    public:
        explicit DictRange(void* head) noexcept : _head(head) {}

        DictIterator begin() const noexcept;
        DictIterator end() const noexcept;
    };

    ArrayRange array_items() const noexcept;
    DictRange dict_items() const noexcept;
};

class STDROMANO_API Json
{
    friend struct JsonParser_;
    friend struct JsonWriter_;

    JsonObject* _root;
    Arena _string_arena;
    Arena _value_arena;

public:
    Json() noexcept;
    ~Json() noexcept;

    STDROMANO_NON_COPYABLE(Json)
    STDROMANO_NON_MOVABLE(Json)

    // Root access

    JsonObject* root() const noexcept;
    void set_root(JsonObject* root) noexcept;

    // Value creation

    JsonObject* make_null() noexcept;
    JsonObject* make_bool(bool b) noexcept;
    JsonObject* make_u64(std::uint64_t u64) noexcept;
    JsonObject* make_i64(std::int64_t i64) noexcept;
    JsonObject* make_f64(double f64) noexcept;
    JsonObject* make_str(const char* str) noexcept;
    JsonObject* make_array() noexcept;
    JsonObject* make_dict() noexcept;

    // Value mutation

    void set_null(JsonObject* obj) noexcept;
    void set_bool(JsonObject* obj, bool b) noexcept;
    void set_u64(JsonObject* obj, std::uint64_t u64) noexcept;
    void set_i64(JsonObject* obj, std::int64_t i64) noexcept;
    void set_f64(JsonObject* obj, double f64) noexcept;
    void set_str(JsonObject* obj, const char* str) noexcept;

    // Array operations

    void array_append(JsonObject* array, JsonObject* value, bool reference = false) noexcept;
    void array_pop(JsonObject* array, std::size_t index) noexcept;

    // Dict operations

    void dict_append(JsonObject* dict,
                     const char* key,
                     JsonObject* value,
                     bool reference = false) noexcept;

    void dict_pop(JsonObject* dict, const char* key) noexcept;

    // Parse

    bool loads(const char* str, std::size_t len) noexcept;
    bool loadf(const StringD& path) noexcept;

    // Dump

    StringD dumps(std::size_t indent_size = 0) const noexcept;
    bool dumpf(std::size_t indent_size, const StringD& path) const noexcept;
};

STDROMANO_NAMESPACE_END

#endif /* !defined(__STDROMANO_JSON) */
