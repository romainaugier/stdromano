// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - Present Romain Augier
// All rights reserved.

#include "stdromano/json.hpp"

#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <new>

STDROMANO_NAMESPACE_BEGIN

/***********************/
/* Internal tag layout */
/***********************/

/*
 * Bits 0-7   : type flags
 * Bit 8      : invalid value when parsed (invalid float/int)
 * Bits 32-63 : size for sized elements (str, array, dict)
 */

enum JsonTag : uint64_t
{
    JsonTag_Null = 1u << 0,
    JsonTag_Bool = 1u << 1,
    JsonTag_U64 = 1u << 2,
    JsonTag_I64 = 1u << 3,
    JsonTag_F64 = 1u << 4,
    JsonTag_Str = 1u << 5,
    JsonTag_Array = 1u << 6,
    JsonTag_Dict = 1u << 7,
    JsonTag_Invalid = 1u << 8,
};

static constexpr std::uint64_t JSON_TAGS_MASK = (1u << 8) - 1;
static constexpr std::uint64_t JSON_SZ_MASK = 0xFFFFFFFFULL << 32;

STDROMANO_FORCE_INLINE void tag_set_type(uint64_t& tags, uint64_t tag) noexcept
{
    tags &= ~JSON_TAGS_MASK;
    tags |= tag;
}

STDROMANO_FORCE_INLINE void tag_set_sz(uint64_t& tags, uint32_t sz) noexcept
{
    tags &= ~JSON_SZ_MASK;
    tags |= (static_cast<uint64_t>(sz) & 0xFFFFFFFFULL) << 32;
}

STDROMANO_FORCE_INLINE uint32_t tag_get_sz(uint64_t tags) noexcept
{
    return static_cast<uint32_t>((tags >> 32) & 0xFFFFFFFFULL);
}

STDROMANO_FORCE_INLINE void tag_set_invalid(uint64_t& tags) noexcept
{
    tags |= JsonTag_Invalid;
}

STDROMANO_FORCE_INLINE void tag_clear_invalid(uint64_t& tags) noexcept
{
    tags &= ~(JsonTag_Invalid);
}

STDROMANO_FORCE_INLINE bool tag_get_invalid(std::uint64_t& tags) noexcept
{
    return (tags & (JsonTag_Invalid)) > 0;
}

/*******************************/
/* Internal container structs  */
/*******************************/

struct JsonArrayElement
{
    JsonObject* value;
    JsonArrayElement* next;
};

struct JsonArrayInfo
{
    JsonArrayElement* head;
    JsonArrayElement* tail;
    std::size_t size;
};

struct JsonDictElement
{
    JsonKeyValue kv;
    JsonDictElement* next;
};

struct JsonDictInfo
{
    JsonDictElement* head;
    JsonDictElement* tail;
    std::size_t size;
};

/****************************/
/* JsonObject: type checks  */
/****************************/

bool JsonObject::is_null() const noexcept  { return this->_tags & JsonTag_Null; }
bool JsonObject::is_bool() const noexcept  { return this->_tags & JsonTag_Bool; }
bool JsonObject::is_u64() const noexcept   { return this->_tags & JsonTag_U64; }
bool JsonObject::is_i64() const noexcept   { return this->_tags & JsonTag_I64; }
bool JsonObject::is_f64() const noexcept   { return this->_tags & JsonTag_F64; }
bool JsonObject::is_str() const noexcept   { return this->_tags & JsonTag_Str; }
bool JsonObject::is_array() const noexcept { return this->_tags & JsonTag_Array; }
bool JsonObject::is_dict() const noexcept  { return this->_tags & JsonTag_Dict; }

/************************/
/* JsonObject: getters  */
/************************/

bool JsonObject::get_bool() const noexcept
{
    if(!(this->_tags & JsonTag_Bool))
        return false;

    return _value.b;
}

uint64_t JsonObject::get_u64() const noexcept
{
    if(!(this->_tags & JsonTag_U64))
        return 0;

    return _value.u64;
}

int64_t JsonObject::get_i64() const noexcept
{
    if(!(this->_tags & JsonTag_I64))
        return 0;

    return _value.i64;
}

double JsonObject::get_f64() const noexcept
{
    if(!(this->_tags & JsonTag_F64))
        return 0;

    return _value.f64;
}

const char* JsonObject::get_str() const noexcept
{
    if(!(this->_tags & JsonTag_Str))
        return nullptr;

    return _value.str;
}

size_t JsonObject::get_str_size() const noexcept
{
    if(!(this->_tags & JsonTag_Str))
        return 0;

    return static_cast<size_t>(tag_get_sz(_tags));
}

size_t JsonObject::array_size() const noexcept
{
    if(!(this->_tags & JsonTag_Array))
        return 0;

    return static_cast<const JsonArrayInfo*>(this->_value.ptr)->size;
}

size_t JsonObject::dict_size() const noexcept
{
    if(!(this->_tags & JsonTag_Dict))
        return 0;

    return static_cast<const JsonDictInfo*>(this->_value.ptr)->size;
}

JsonObject* JsonObject::dict_find(const char* key) const noexcept
{
    if(!(this->_tags & JsonTag_Dict))
        return nullptr;

    const auto* info = static_cast<const JsonDictInfo*>(_value.ptr);
    auto* current = info->head;

    const std::size_t key_sz = std::strlen(key);

    while(current != nullptr)
    {
        const std::size_t current_key_sz = std::strlen(current->kv.key);

        if(key_sz == current_key_sz && std::memcmp(current->kv.key, key, key_sz) == 0)
            return current->kv.value;

        current = current->next;
    }

    return nullptr;
}

/*************************************/
/* JsonObject: iterator impls        */
/*************************************/

JsonObject* JsonObject::ArrayIterator::operator*() const noexcept
{
    return static_cast<JsonArrayElement*>(this->_current)->value;
}

JsonObject::ArrayIterator& JsonObject::ArrayIterator::operator++() noexcept
{
    this->_current = static_cast<JsonArrayElement*>(this->_current)->next;
    return *this;
}

bool JsonObject::ArrayIterator::operator!=(const ArrayIterator& other) const noexcept
{
    return this->_current != other._current;
}

JsonKeyValue JsonObject::DictIterator::operator*() const noexcept
{
    return static_cast<JsonDictElement*>(this->_current)->kv;
}

JsonObject::DictIterator& JsonObject::DictIterator::operator++() noexcept
{
    this->_current = static_cast<JsonDictElement*>(this->_current)->next;
    return *this;
}

bool JsonObject::DictIterator::operator!=(const DictIterator& other) const noexcept
{
    return this->_current != other._current;
}

JsonObject::ArrayIterator JsonObject::ArrayRange::begin() const noexcept
{
    return ArrayIterator(this->_head);
}

JsonObject::ArrayIterator JsonObject::ArrayRange::end() const noexcept
{
    return ArrayIterator(nullptr);
}

JsonObject::DictIterator JsonObject::DictRange::begin() const noexcept
{
    return DictIterator(this->_head);
}

JsonObject::DictIterator JsonObject::DictRange::end() const noexcept
{
    return DictIterator(nullptr);
}

JsonObject::ArrayRange JsonObject::array_items() const noexcept
{
    if(!(this->_tags & JsonTag_Array))
        return ArrayRange(nullptr);

    return ArrayRange(static_cast<const JsonArrayInfo*>(this->_value.ptr)->head);
}

JsonObject::DictRange JsonObject::dict_items() const noexcept
{
    if(!(this->_tags & JsonTag_Dict))
        return DictRange(nullptr);

    return DictRange(static_cast<const JsonDictInfo*>(this->_value.ptr)->head);
}

/******************************/
/* Json: construction         */
/******************************/

Json::Json() noexcept : _root(nullptr),
                        _string_arena(128 * 1024),
                        _value_arena(1024 * sizeof(JsonObject))
{
}

Json::~Json() noexcept = default;

JsonObject* Json::root() const noexcept { return this->_root; }

void Json::set_root(JsonObject* root) noexcept { this->_root = root; }

/******************************/
/* Json: value creation       */
/******************************/

JsonObject* Json::make_null() noexcept
{
    auto* obj = this->_value_arena.emplace<JsonObject>();
    tag_set_type(obj->_tags, JsonTag_Null);
    return obj;
}

JsonObject* Json::make_bool(bool b) noexcept
{
    auto* obj = this->_value_arena.emplace<JsonObject>();
    tag_set_type(obj->_tags, JsonTag_Bool);
    obj->_value.b = b;
    return obj;
}

JsonObject* Json::make_u64(std::uint64_t u64) noexcept
{
    auto* obj = this->_value_arena.emplace<JsonObject>();
    tag_set_type(obj->_tags, JsonTag_U64);
    obj->_value.u64 = u64;
    return obj;
}

JsonObject* Json::make_i64(std::int64_t i64) noexcept
{
    auto* obj = this->_value_arena.emplace<JsonObject>();
    tag_set_type(obj->_tags, JsonTag_I64);
    obj->_value.i64 = i64;
    return obj;
}

JsonObject* Json::make_f64(double f64) noexcept
{
    auto* obj = this->_value_arena.emplace<JsonObject>();
    tag_set_type(obj->_tags, JsonTag_F64);
    obj->_value.f64 = f64;
    return obj;
}

JsonObject* Json::make_str(const char* str) noexcept
{
    const size_t str_sz = std::strlen(str);
    char* str_ptr = static_cast<char*>(this->_string_arena.allocate(str_sz + 1));

    std::memcpy(str_ptr, str, str_sz);
    str_ptr[str_sz] = '\0';

    auto* obj = this->_value_arena.emplace<JsonObject>();
    tag_set_type(obj->_tags, JsonTag_Str);
    tag_set_sz(obj->_tags, static_cast<std::uint32_t>(str_sz));
    obj->_value.str = str_ptr;

    return obj;
}

JsonObject* Json::make_array() noexcept
{
    auto* obj = this->_value_arena.emplace<JsonObject>();
    auto* info = this->_value_arena.emplace<JsonArrayInfo>();

    info->head = nullptr;
    info->tail = nullptr;
    info->size = 0;

    tag_set_type(obj->_tags, JsonTag_Array);
    obj->_value.ptr = info;

    return obj;
}

JsonObject* Json::make_dict() noexcept
{
    auto* obj = this->_value_arena.emplace<JsonObject>();
    auto* info = this->_value_arena.emplace<JsonDictInfo>();

    info->head = nullptr;
    info->tail = nullptr;
    info->size = 0;

    tag_set_type(obj->_tags, JsonTag_Dict);
    obj->_value.ptr = info;

    return obj;
}

/******************************/
/* Json: value mutation       */
/******************************/

void Json::set_null(JsonObject* obj) noexcept
{
    tag_set_type(obj->_tags, JsonTag_Null);
}

void Json::set_bool(JsonObject* obj, bool b) noexcept
{
    tag_set_type(obj->_tags, JsonTag_Bool);
    obj->_value.b = b;
}

void Json::set_u64(JsonObject* obj, uint64_t u64) noexcept
{
    tag_set_type(obj->_tags, JsonTag_U64);
    obj->_value.u64 = u64;
}

void Json::set_i64(JsonObject* obj, int64_t i64) noexcept
{
    tag_set_type(obj->_tags, JsonTag_I64);
    obj->_value.i64 = i64;
}

void Json::set_f64(JsonObject* obj, double f64) noexcept
{
    tag_set_type(obj->_tags, JsonTag_F64);
    obj->_value.f64 = f64;
}

void Json::set_str(JsonObject* obj, const char* str) noexcept
{
    const size_t str_sz = std::strlen(str);
    char* str_ptr = static_cast<char*>(this->_string_arena.allocate(str_sz + 1));

    std::memcpy(str_ptr, str, str_sz);
    str_ptr[str_sz] = '\0';

    tag_set_type(obj->_tags, JsonTag_Str);
    tag_set_sz(obj->_tags, static_cast<uint32_t>(str_sz));
    obj->_value.str = str_ptr;
}

/******************************/
/* Json: array operations     */
/******************************/

void Json::array_append(JsonObject* array, JsonObject* value, bool reference) noexcept
{
    auto* info = static_cast<JsonArrayInfo*>(array->_value.ptr);
    auto* element = this->_value_arena.emplace<JsonArrayElement>();

    element->next = nullptr;

    if(info->head == nullptr)
    {
        info->head = element;
        info->tail = element;
    }
    else
    {
        info->tail->next = element;
        info->tail = element;
    }

    if(!reference)
    {
        auto* new_value = _value_arena.emplace<JsonObject>();
        std::memcpy(new_value, value, sizeof(JsonObject));
        element->value = new_value;
    }
    else
    {
        element->value = value;
    }

    info->size++;
}

void Json::array_pop(JsonObject* array, std::size_t index) noexcept
{
    auto* info = static_cast<JsonArrayInfo*>(array->_value.ptr);

    if(index >= info->size)
        return;

    if(index == 0)
    {
        info->head = info->head->next;

        if(info->head == nullptr)
            info->tail = nullptr;

        info->size--;
        return;
    }

    auto* previous = info->head;

    for(std::size_t i = 0; i < index - 1; i++)
        previous = previous->next;

    auto* target = previous->next;
    previous->next = target->next;

    if(target == info->tail)
        info->tail = previous;

    info->size--;
}

/******************************/
/* Json: dict operations      */
/******************************/

void Json::dict_append(JsonObject* dict,
                       const char* key,
                       JsonObject* value,
                       bool reference) noexcept
{
    auto* info = static_cast<JsonDictInfo*>(dict->_value.ptr);
    auto* element = _value_arena.emplace<JsonDictElement>();

    element->next = nullptr;

    if(info->head == nullptr)
    {
        info->head = element;
        info->tail = element;
    }
    else
    {
        info->tail->next = element;
        info->tail = element;
    }

    const size_t key_sz = std::strlen(key);
    char* new_key = static_cast<char*>(this->_string_arena.allocate(key_sz + 1));

    std::memcpy(new_key, key, key_sz);
    new_key[key_sz] = '\0';

    if(!reference)
    {
        auto* new_value = _value_arena.emplace<JsonObject>();
        std::memcpy(new_value, value, sizeof(JsonObject));
        element->kv.value = new_value;
    }
    else
    {
        element->kv.value = value;
    }

    element->kv.key = new_key;

    info->size++;
}

void Json::dict_pop(JsonObject* dict, const char* key) noexcept
{
    auto* info = static_cast<JsonDictInfo*>(dict->_value.ptr);

    JsonDictElement* previous = nullptr;
    auto* current = info->head;

    while(current != nullptr)
    {
        if(std::strcmp(current->kv.key, key) == 0)
            break;

        previous = current;
        current = current->next;
    }

    if(current == nullptr)
        return;

    if(previous == nullptr)
    {
        info->head = current->next;

        if(info->head == nullptr)
            info->tail = nullptr;
    }
    else
    {
        previous->next = current->next;

        if(current == info->tail)
            info->tail = previous;
    }

    info->size--;
}

/****************/
/* Json parser  */
/****************/

struct JsonParser_
{
    const char* str;
    std::size_t pos;
    std::size_t len;
    Json* json;

    STDROMANO_FORCE_INLINE void skip_whitespace() noexcept
    {
        static constexpr bool ws[256] = {
                //  0      1      2      3      4      5      6      7
                //  NUL    SOH    STX    ETX    EOT    ENQ    ACK    BEL
                    false, false, false, false, false, false, false, false,
                //  BS     HT     LF     VT     FF     CR     SO     SI
                    false, true,  true,  true,  true,  true,  false, false,
                //  16-31
                    false, false, false, false, false, false, false, false,
                    false, false, false, false, false, false, false, false,
                //  SP     !
                    true,  false,
        };

        while(this->pos < this->len && ws[(std::uint8_t)this->str[this->pos]])
            this->pos++;
    }

    JsonObject* parse_value() noexcept;
    JsonObject* parse_string() noexcept;
    JsonObject* parse_number() noexcept;
    JsonObject* parse_array() noexcept;
    JsonObject* parse_dict() noexcept;
    JsonObject* parse_literal() noexcept;
};

static int hex_value(const char c) noexcept
{
    if(c >= '0' && c <= '9')
        return c - '0';

    if(c >= 'a' && c <= 'f')
        return c - 'a' + 10;

    if(c >= 'A' && c <= 'F')
        return c - 'A' + 10;

    return -1;
}

static bool read_hex4(const char* p, std::uint32_t& out) noexcept
{
    out = 0;

    for(int i = 0; i < 4; ++i)
    {
        const int v = hex_value(p[i]);

        if(v < 0)
            return false;

        out = (out << 4) | static_cast<std::uint32_t>(v);
    }

    return true;
}

static std::size_t encode_utf8(std::uint32_t cp, char* out) noexcept
{
    if(cp < 0x80)
    {
        out[0] = static_cast<char>(cp);
        return 1;
    }

    if(cp < 0x800)
    {
        out[0] = static_cast<char>(0xC0 | (cp >> 6));
        out[1] = static_cast<char>(0x80 | (cp & 0x3F));
        return 2;
    }

    if(cp < 0x10000)
    {
        out[0] = static_cast<char>(0xE0 | (cp >> 12));
        out[1] = static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
        out[2] = static_cast<char>(0x80 | (cp & 0x3F));
        return 3;
    }

    out[0] = static_cast<char>(0xF0 | (cp >> 18));
    out[1] = static_cast<char>(0x80 | ((cp >> 12) & 0x3F));
    out[2] = static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
    out[3] = static_cast<char>(0x80 | (cp & 0x3F));
    return 4;
}

JsonObject* JsonParser_::parse_string() noexcept
{
    if(pos >= len || str[pos] != '"')
        return nullptr;

    pos++;

    const std::size_t start = pos;
    bool has_escape = false;

    while(pos < len)
    {
        const char c = str[pos];

        if(c == '"')
            break;

        if(c == '\\')
        {
            has_escape = true;
            pos++;

            if(pos >= len)
                return nullptr;

            if(str[pos] == 'u')
            {
                std::uint32_t unused;

                if(pos + 4 >= len || !read_hex4(str + pos + 1, unused))
                    return nullptr;

                pos += 4;
            }
            else if(std::strchr("\"\\/bfnrt", str[pos]) == nullptr || str[pos] == '\0')
            {
                return nullptr;
            }
        }
        else if(static_cast<unsigned char>(c) < 0x20)
        {
            return nullptr;
        }

        pos++;
    }

    if(pos >= len)
        return nullptr;

    const std::size_t raw_len = pos - start;

    JsonObject* obj = json->_value_arena.emplace<JsonObject>();
    char* s = static_cast<char*>(json->_string_arena.allocate(raw_len + 1));
    std::size_t slen = 0;

    if(!has_escape)
    {
        std::memcpy(s, str + start, raw_len);
        slen = raw_len;
    }
    else
    {
        for(std::size_t i = start; i < pos; i++)
        {
            if(str[i] != '\\')
            {
                s[slen++] = str[i];
                continue;
            }

            i++;

            switch(str[i])
            {
                case '"':  s[slen++] = '"';  break;
                case '\\': s[slen++] = '\\'; break;
                case '/':  s[slen++] = '/';  break;
                case 'b':  s[slen++] = '\b'; break;
                case 'f':  s[slen++] = '\f'; break;
                case 'n':  s[slen++] = '\n'; break;
                case 'r':  s[slen++] = '\r'; break;
                case 't':  s[slen++] = '\t'; break;
                default:
                {
                    std::uint32_t cp = 0;
                    read_hex4(str + i + 1, cp);
                    i += 4;

                    if(cp >= 0xD800 && cp <= 0xDBFF)
                    {
                        std::uint32_t low = 0;

                        if(i + 6 < pos && str[i + 1] == '\\' && str[i + 2] == 'u' &&
                           read_hex4(str + i + 3, low) && low >= 0xDC00 && low <= 0xDFFF)
                        {
                            cp = 0x10000 + ((cp - 0xD800) << 10) + (low - 0xDC00);
                            i += 6;
                        }
                        else
                        {
                            cp = 0xFFFD;
                        }
                    }
                    else if(cp >= 0xDC00 && cp <= 0xDFFF)
                    {
                        cp = 0xFFFD;
                    }

                    slen += encode_utf8(cp, s + slen);
                    break;
                }
            }
        }
    }

    s[slen] = '\0';

    pos++;

    tag_set_type(obj->_tags, JsonTag_Str);
    tag_set_sz(obj->_tags, static_cast<std::uint32_t>(slen));
    obj->_value.str = s;

    return obj;
}

JsonObject* JsonParser_::parse_number() noexcept
{
    const std::size_t start = pos;

    bool is_negative = false;
    bool is_float = false;
    bool overflow = false;
    std::uint64_t magnitude = 0;

    if(pos < len && str[pos] == '-')
    {
        is_negative = true;
        pos++;
    }

    if(pos >= len || !is_digit(str[pos]))
        return nullptr;

    while(pos < len && is_digit(str[pos]))
    {
        const std::uint64_t digit = static_cast<std::uint64_t>(str[pos] - '0');

        if(magnitude > (UINT64_MAX - digit) / 10)
            overflow = true;
        else
            magnitude = magnitude * 10 + digit;

        pos++;
    }

    if(pos < len && str[pos] == '.')
    {
        is_float = true;
        pos++;

        if(pos >= len || !is_digit(str[pos]))
            return nullptr;

        while(pos < len && is_digit(str[pos]))
            pos++;
    }

    if(pos < len && (str[pos] == 'e' || str[pos] == 'E'))
    {
        is_float = true;
        pos++;

        if(pos < len && (str[pos] == '-' || str[pos] == '+'))
            pos++;

        if(pos >= len || !is_digit(str[pos]))
            return nullptr;

        while(pos < len && is_digit(str[pos]))
            pos++;
    }

    constexpr std::uint64_t min_i64_magnitude = static_cast<std::uint64_t>(INT64_MAX) + 1;

    if(!is_float && !overflow)
    {
        if(!is_negative)
            return json->make_u64(magnitude);

        if(magnitude <= min_i64_magnitude)
            return json->make_i64(magnitude == min_i64_magnitude ? INT64_MIN : -static_cast<std::int64_t>(magnitude));
    }

    const std::size_t number_len = pos - start;

    char stack_buffer[64];
    char* buffer = number_len < sizeof(stack_buffer) ? stack_buffer
                                                     : static_cast<char*>(std::malloc(number_len + 1));

    if(buffer == nullptr)
        return nullptr;

    std::memcpy(buffer, str + start, number_len);
    buffer[number_len] = '\0';

    const double value = std::strtod(buffer, nullptr);

    if(buffer != stack_buffer)
        std::free(buffer);

    return json->make_f64(value);
}

JsonObject* JsonParser_::parse_array() noexcept
{
    if(pos >= len || str[pos] != '[')
        return nullptr;

    pos++;

    JsonObject* array = json->make_array();
    this->skip_whitespace();

    if(pos < len && str[pos] == ']')
    {
        pos++;
        return array;
    }

    while(pos < len)
    {
        JsonObject* element = parse_value();

        if(element == nullptr)
            return nullptr;

        json->array_append(array, element, true);
        this->skip_whitespace();

        if(pos >= len)
            return nullptr;

        if(str[pos] == ',')
        {
            pos++;
            this->skip_whitespace();
        }
        else if(str[pos] == ']')
        {
            pos++;
            return array;
        }
        else
        {
            return nullptr;
        }
    }

    return nullptr;
}

JsonObject* JsonParser_::parse_dict() noexcept
{
    if(pos >= len || str[pos] != '{')
        return nullptr;

    pos++;

    JsonObject* dict = json->make_dict();
    this->skip_whitespace();

    if(pos < len && str[pos] == '}')
    {
        pos++;
        return dict;
    }

    while(pos < len)
    {
        this->skip_whitespace();

        if(pos >= len || str[pos] != '"')
            return nullptr;

        JsonObject* key_val = this->parse_string();

        if(key_val == nullptr)
            return nullptr;

        const char* key = key_val->_value.str;

        this->skip_whitespace();

        if(pos >= len || str[pos] != ':')
            return nullptr;

        pos++;
        this->skip_whitespace();

        JsonObject* value = this->parse_value();

        if(value == nullptr)
            return nullptr;

        json->dict_append(dict, key, value, true);
        this->skip_whitespace();

        if(pos >= len)
            return nullptr;

        if(str[pos] == ',')
        {
            pos++;
            this->skip_whitespace();
        }
        else if(str[pos] == '}')
        {
            pos++;
            return dict;
        }
        else
        {
            return nullptr;
        }
    }

    return nullptr;
}

JsonObject* JsonParser_::parse_literal() noexcept
{
    if(pos + 4 <= len && std::memcmp(str + pos, "null", 4) == 0)
    {
        pos += 4;
        return json->make_null();
    }

    if(pos + 4 <= len && std::memcmp(str + pos, "true", 4) == 0)
    {
        pos += 4;
        return json->make_bool(true);
    }

    if(pos + 5 <= len && std::memcmp(str + pos, "false", 5) == 0)
    {
        pos += 5;
        return json->make_bool(false);
    }

    return nullptr;
}

JsonObject* JsonParser_::parse_value() noexcept
{
    this->skip_whitespace();

    if(pos >= len)
        return nullptr;

    const char c = str[pos];

    if(c == '"')
        return this->parse_string();
    else if(c == '{')
        return this->parse_dict();
    else if(c == '[')
        return this->parse_array();
    else if(c == '-' || is_digit(c))
        return this->parse_number();
    else if(c == 't' || c == 'f' || c == 'n')
        return this->parse_literal();

    return nullptr;
}

/****************/
/* Json writer  */
/****************/

struct JsonWriter_
{
    const Json* json;
    StringD* out;
    std::size_t indent_size;
    std::size_t indent;

    STDROMANO_FORCE_INLINE void write_indent() noexcept
    {
        out->appendf("{:{}}", "", indent);
    }

    bool write_value(const JsonObject* value) noexcept;
    void write_str(const char* s, const std::size_t size) noexcept;
    bool write_array(const JsonObject* array) noexcept;
    bool write_dict(const JsonObject* dict) noexcept;
};

STDROMANO_FORCE_INLINE bool char_needs_escape(char c) noexcept
{
    return c == '"' || c == '\\' || static_cast<unsigned char>(c) < 0x20;
}

STDROMANO_FORCE_INLINE char escape_char(char c) noexcept
{
    switch(c)
    {
        case '"':  return '"';
        case '\\': return '\\';
        case '\b': return 'b';
        case '\f': return 'f';
        case '\n': return 'n';
        case '\r': return 'r';
        case '\t': return 't';
        default:   return c;
    }
}

void JsonWriter_::write_str(const char* s, const std::size_t size) noexcept
{
    out->appendc("\"", 1);

    const char* seg_start = s;
    const char* p = s;
    const char* end = s + size;

    while(p < end)
    {
        if(char_needs_escape(*p))
        {
            if(p > seg_start)
                out->appendc(seg_start, static_cast<size_t>(p - seg_start));

            const char escaped = escape_char(*p);

            if(escaped != *p || *p == '"' || *p == '\\')
            {
                const char esc[2] = {'\\', escaped};
                out->appendc(esc, 2);
            }
            else
            {
                out->appendf("\\u{:04x}", static_cast<unsigned int>(static_cast<unsigned char>(*p)));
            }

            seg_start = p + 1;
        }

        p++;
    }

    if(p > seg_start)
        out->appendc(seg_start, static_cast<size_t>(p - seg_start));

    out->appendc("\"", 1);
}

bool JsonWriter_::write_array(const JsonObject* array) noexcept
{
    out->appendc("[", 1);

    if(indent_size > 0)
        indent += indent_size;

    size_t i = 0;

    for(auto* element : array->array_items())
    {
        if(i > 0)
            out->appendc(",", 1);

        if(indent_size > 0)
        {
            out->appendc("\n", 1);
            write_indent();
        }

        if(!write_value(element))
            return false;

        i++;
    }

    if(indent_size > 0)
    {
        out->appendc("\n", 1);
        indent -= indent_size;
        write_indent();
    }

    out->appendc("]", 1);
    return true;
}

bool JsonWriter_::write_dict(const JsonObject* dict) noexcept
{
    out->appendc("{", 1);

    if(indent_size > 0)
        indent += indent_size;

    size_t i = 0;

    for(auto [key, value] : dict->dict_items())
    {
        if(i > 0)
            out->appendc(",", 1);

        if(indent_size > 0)
        {
            out->appendc("\n", 1);
            write_indent();
        }

        write_str(key, std::strlen(key));
        out->appendc(": ", 2);

        if(!write_value(value))
            return false;

        i++;
    }

    if(indent_size > 0)
    {
        out->appendc("\n", 1);
        indent -= indent_size;
        write_indent();
    }

    out->appendc("}", 1);
    return true;
}

bool JsonWriter_::write_value(const JsonObject* value) noexcept
{
    const std::uint64_t tag = value->_tags & JSON_TAGS_MASK;

    switch(tag)
    {
        case JsonTag_Null:
            out->appendc("null", 4);
            return true;

        case JsonTag_Bool:
            if(value->_value.b)
                out->appendc("true", 4);
            else
                out->appendc("false", 5);
            return true;

        case JsonTag_U64:
            out->appendf("{}", value->_value.u64);
            return true;

        case JsonTag_I64:
            out->appendf("{}", value->_value.i64);
            return true;

        case JsonTag_F64:
        {
            const double f64 = value->_value.f64;

            if(!std::isfinite(f64))
            {
                out->appendc("null", 4);
                return true;
            }

            const std::size_t number_start = out->size();
            out->appendf("{}", f64);

            const char* written = out->c_str() + number_start;

            if(std::strpbrk(written, ".eE") == nullptr)
                out->appendc(".0", 2);

            return true;
        }

        case JsonTag_Str:
            write_str(value->_value.str, static_cast<std::size_t>(tag_get_sz(value->_tags)));
            return true;

        case JsonTag_Array:
            return write_array(value);

        case JsonTag_Dict:
            return write_dict(value);

        default:
            return false;
    }
}

/******************************/
/* Json: parse / dump         */
/******************************/

bool Json::loads(const char* str, size_t len) noexcept
{
    if(str == nullptr || len == 0)
        return false;

    JsonParser_ parser;
    parser.str = str;
    parser.pos = 0;
    parser.len = len;
    parser.json = this;

    JsonObject* root = parser.parse_value();

    if(root == nullptr)
    {
        return false;
    }

    parser.skip_whitespace();

    if(parser.pos != parser.len)
    {
        return false;
    }

    this->_root = root;

    return true;
}

bool Json::loadf(const StringD& path) noexcept
{
    FILE* file = std::fopen(path.c_str(), "rb");

    if(file == nullptr)
        return false;

    std::fseek(file, 0, SEEK_END);
    const auto file_size = static_cast<size_t>(std::ftell(file));
    std::rewind(file);

    auto* buffer = static_cast<char*>(std::calloc(file_size, sizeof(char)));

    if(buffer == nullptr)
    {
        std::fclose(file);
        return false;
    }

    const size_t read_size = std::fread(buffer, sizeof(char), file_size, file);
    std::fclose(file);

    if(read_size != file_size)
    {
        std::free(buffer);
        return false;
    }

    bool res = loads(buffer, file_size);

    std::free(buffer);

    return res;
}

StringD Json::dumps(size_t indent_size) const noexcept
{
    StringD result;

    if(this->_root == nullptr)
        return result;

    JsonWriter_ writer;
    writer.json = this;
    writer.out = &result;
    writer.indent_size = indent_size;
    writer.indent = 0;

    writer.write_value(this->_root);

    return result;
}

bool Json::dumpf(size_t indent_size, const StringD& path) const noexcept
{
    StringD written = this->dumps(indent_size);

    FILE* file = std::fopen(path.c_str(), "wb");

    if(file == nullptr)
        return false;

    const size_t str_sz = written.size();
    const size_t fwritten = std::fwrite(written.data(), sizeof(char), str_sz, file);
    std::fclose(file);

    return fwritten == str_sz;
}

STDROMANO_NAMESPACE_END
