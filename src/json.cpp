// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - Present Romain Augier
// All rights reserved.

#include "stdromano/json.hpp"

#include <cmath>
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
    JsonTag_Null  = 1u << 0,
    JsonTag_Bool  = 1u << 1,
    JsonTag_U64   = 1u << 2,
    JsonTag_I64   = 1u << 3,
    JsonTag_F64   = 1u << 4,
    JsonTag_Str   = 1u << 5,
    JsonTag_Array = 1u << 6,
    JsonTag_Dict  = 1u << 7,
};

static constexpr uint64_t JSON_TAGS_MASK = (1u << 8) - 1;
static constexpr uint64_t JSON_SZ_MASK   = 0xFFFFFFFFULL << 32;

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

STDROMANO_FORCE_INLINE void tag_incr_sz(uint64_t& tags) noexcept
{
    tags += 1ULL << 32;
}

STDROMANO_FORCE_INLINE void tag_decr_sz(uint64_t& tags) noexcept
{
    tags -= 1ULL << 32;
}

STDROMANO_FORCE_INLINE void tag_set_invalid(uint64_t& tags) noexcept
{
    tags |= 1ULL << 8;
}

STDROMANO_FORCE_INLINE void tag_clear_invalid(uint64_t& tags) noexcept
{
    tags &= ~(1ULL << 8);
}

STDROMANO_FORCE_INLINE bool tag_get_invalid(std::uint64_t& tags) noexcept
{
    return (tags & (1ULL << 8)) > 0;
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

    return static_cast<size_t>(tag_get_sz(this->_tags));
}

size_t JsonObject::dict_size() const noexcept
{
    if(!(this->_tags & JsonTag_Dict))
        return 0;

    return static_cast<size_t>(tag_get_sz(this->_tags));
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
        const std::size_t current_key_sz = std::strlen(key);

        if(key_sz == current_key_sz && detail::strcmp(current->kv.key, key, key_sz) == 0)
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

    tag_incr_sz(array->_tags);
}

void Json::array_pop(JsonObject* array, std::size_t index) noexcept
{
    const size_t sz = static_cast<size_t>(tag_get_sz(array->_tags));

    if(index >= sz)
        return;

    auto* info = static_cast<JsonArrayInfo*>(array->_value.ptr);

    if(index == 0)
    {
        info->head = info->head->next;

        if(info->head == nullptr)
            info->tail = nullptr;

        tag_decr_sz(array->_tags);
        return;
    }

    auto* previous = info->head;

    for(std::size_t i = 0; i < index - 1; i++)
        previous = previous->next;

    auto* target = previous->next;
    previous->next = target->next;

    if(target == info->tail)
        info->tail = previous;

    tag_decr_sz(array->_tags);
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

    tag_incr_sz(dict->_tags);
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

    tag_decr_sz(dict->_tags);
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

JsonObject* JsonParser_::parse_string() noexcept
{
    if(pos >= len || str[pos] != '"')
        return nullptr;

    pos++;

    const std::size_t start = pos;
    std::size_t slen = 0;
    bool has_escape = false;

    while(pos < len)
    {
        const char c = str[pos];

        if(c == '"')
        {
            break;
        }
        else if(c == '\\')
        {
            has_escape = true;
            pos++;

            if(pos >= len)
                return nullptr;

            if(str[pos] == 'u')
            {
                pos += 4;

                if(pos >= len)
                    return nullptr;
            }
        }
        else if(static_cast<unsigned char>(c) < 0x20)
        {
            return nullptr;
        }

        pos++;
        slen++;
    }

    if(pos >= len)
        return nullptr;

    JsonObject* obj = json->_value_arena.emplace<JsonObject>();
    char* s;

    if(!has_escape)
    {
        s = static_cast<char*>(json->_string_arena.allocate(slen + 1));
        std::memcpy(s, str + start, slen);
        s[slen] = '\0';
    }
    else
    {
        s = static_cast<char*>(json->_string_arena.allocate(slen + 1));
        std::size_t j = 0;

        for(std::size_t i = start; i < pos; i++)
        {
            if(str[i] == '\\')
            {
                i++;

                switch(str[i])
                {
                    case '"':  s[j++] = '"';  break;
                    case '\\': s[j++] = '\\'; break;
                    case '/':  s[j++] = '/';  break;
                    case 'b':  s[j++] = '\b'; break;
                    case 'f':  s[j++] = '\f'; break;
                    case 'n':  s[j++] = '\n'; break;
                    case 'r':  s[j++] = '\r'; break;
                    case 't':  s[j++] = '\t'; break;
                    case 'u':
                    {
                        /* TODO: proper unicode handling */
                        i += 4;
                        s[j++] = '?';
                        break;
                    }
                    default: s[j++] = str[i]; break;
                }
            }
            else
            {
                s[j++] = str[i];
            }
        }

        s[j] = '\0';
        slen = j;
    }

    pos++;

    tag_set_type(obj->_tags, JsonTag_Str);
    tag_set_sz(obj->_tags, static_cast<std::uint32_t>(slen));
    obj->_value.str = s;

    return obj;
}

static const double pow10_table[] = {
    1e0, 1e1, 1e2, 1e3, 1e4, 1e5, 1e6, 1e7, 1e8, 1e9,
    1e10, 1e11, 1e12, 1e13, 1e14, 1e15, 1e16, 1e17, 1e18, 1e19,
    1e20, 1e21, 1e22
};

static const std::uint64_t pow10_int_table[] = {
    1ULL, 10ULL, 100ULL, 1000ULL, 10000ULL, 100000ULL, 1000000ULL,
    10000000ULL, 100000000ULL, 1000000000ULL, 10000000000ULL,
    100000000000ULL, 1000000000000ULL, 10000000000000ULL,
    100000000000000ULL, 1000000000000000ULL, 10000000000000000ULL,
    100000000000000000ULL, 1000000000000000000ULL, 10000000000000000000ULL
};

static constexpr std::size_t POW10_TABLE_SIZE     = sizeof(pow10_table) / sizeof(pow10_table[0]);
static constexpr std::size_t POW10_INT_TABLE_SIZE = sizeof(pow10_int_table) / sizeof(pow10_int_table[0]);

JsonObject* JsonParser_::parse_number() noexcept
{
    bool is_negative = false;
    bool is_float = false;
    std::int64_t int_val = 0;
    double float_val = 0.0;
    int exponent = 0;
    bool exp_negative = false;

    if(pos < len && str[pos] == '-')
    {
        is_negative = true;
        pos++;
    }

    if(pos >= len || !is_digit(str[pos]))
        return nullptr;

    while(pos < len && is_digit(str[pos]))
    {
        int_val = int_val * 10 + (str[pos] - '0');
        float_val = float_val * 10.0 + (str[pos] - '0');
        pos++;
    }

    if(pos < len && str[pos] == '.')
    {
        is_float = true;
        pos++;

        if(pos >= len || !is_digit(str[pos]))
            return nullptr;

        int frac_digits = 0;
        std::uint64_t frac_int = 0;

        while(pos < len && is_digit(str[pos])
              && static_cast<std::size_t>(frac_digits) < POW10_INT_TABLE_SIZE)
        {
            frac_int = frac_int * 10 + (str[pos] - '0');
            frac_digits++;
            pos++;
        }

        while(pos < len && is_digit(str[pos]))
            pos++;

        if(frac_digits > 0 && static_cast<std::size_t>(frac_digits) < POW10_TABLE_SIZE)
            float_val += static_cast<double>(frac_int) / pow10_table[frac_digits];
        else if(frac_digits > 0)
            float_val += static_cast<double>(frac_int) / std::pow(10.0, frac_digits);
    }

    if(pos < len && (str[pos] == 'e' || str[pos] == 'E'))
    {
        is_float = true;
        pos++;

        if(pos < len && str[pos] == '-')
        {
            exp_negative = true;
            pos++;
        }
        else if(pos < len && str[pos] == '+')
        {
            pos++;
        }

        if(pos >= len || !is_digit(str[pos]))
            return nullptr;

        while(pos < len && is_digit(str[pos]))
        {
            exponent = exponent * 10 + (str[pos] - '0');
            pos++;
        }
    }

    if(is_float)
    {
        if(exp_negative)
            exponent = -exponent;

        if(exponent >= -static_cast<int>(POW10_TABLE_SIZE)
           && exponent < static_cast<int>(POW10_TABLE_SIZE))
        {
            if(exponent >= 0)
                float_val *= pow10_table[exponent];
            else
                float_val /= pow10_table[-exponent];
        }
        else
        {
            float_val *= std::pow(10.0, exponent);
        }

        if(is_negative)
            float_val = -float_val;

        return json->make_f64(float_val);
    }

    if(is_negative)
        return json->make_i64(-int_val);

    return json->make_u64(static_cast<uint64_t>(int_val));
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
    void write_str(const char* s) noexcept;
    bool write_array(const JsonObject* array) noexcept;
    bool write_dict(const JsonObject* dict) noexcept;
};

STDROMANO_FORCE_INLINE bool char_needs_escape(char c) noexcept
{
    switch(c)
    {
        case '"':
        case '\\':
        case '\b':
        case '\f':
        case '\n':
        case '\r':
        case '\t':
            return true;
        default:
            return false;
    }
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

void JsonWriter_::write_str(const char* s) noexcept
{
    out->appendc("\"", 1);

    const char* seg_start = s;
    const char* p = s;

    while(*p != '\0')
    {
        if(char_needs_escape(*p))
        {
            if(p > seg_start)
                out->appendc(seg_start, static_cast<size_t>(p - seg_start));

            const char esc[2] = { '\\', escape_char(*p) };
            out->appendc(esc, 2);
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

        write_str(key);
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
            out->appendf("{:.3f}", value->_value.f64);
            return true;

        case JsonTag_Str:
            write_str(value->_value.str);
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
