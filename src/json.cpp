// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - Present Romain Augier
// All rights reserved.

#include "stdromano/json.hpp"
#include "stdromano/bits.hpp"

#include <charconv>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <new>

#if defined(__cpp_lib_to_chars) && __cpp_lib_to_chars >= 201611L
#define JSON_HAS_FROM_CHARS
#endif // defined(__cpp_lib_to_chars)

#if defined(STDROMANO_MSVC)
#define JSON_UNLIKELY(x) (x)
#define JSON_NO_INLINE __declspec(noinline)
#else
#define JSON_UNLIKELY(x) __builtin_expect(!!(x), 0)
#define JSON_NO_INLINE __attribute__((noinline))
#endif // defined(STDROMANO_MSVC)

#if defined(STDROMANO_INTEL)
#include <emmintrin.h>
#elif defined(STDROMANO_AARCH64)
#include <arm_neon.h>
#endif // defined(STDROMANO_INTEL)

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
    std::size_t key_size;
};

struct JsonDictInfo
{
    JsonDictElement* head;
    JsonDictElement* tail;
    std::size_t size;
};

// Element and value share one allocation, used by the parser and by copying appends
struct JsonArrayNode
{
    JsonArrayElement element;
    JsonObject storage;
};

struct JsonDictNode
{
    JsonDictElement element;
    JsonObject storage;
};

/***************************/
/* Internal string scanning */
/***************************/

static constexpr bool make_needs_attention_entry(unsigned int c) noexcept
{
    return c == '"' || c == '\\' || c < 0x20;
}

struct StringSpecialTable
{
    bool values[256];

    constexpr StringSpecialTable() noexcept : values()
    {
        for(unsigned int c = 0; c < 256; ++c)
            values[c] = make_needs_attention_entry(c);
    }
};

static constexpr StringSpecialTable STRING_SPECIAL{};

// First character that is a quote, a backslash or a control character, or end
STDROMANO_FORCE_INLINE const char* find_string_special(const char* p, const char* end) noexcept
{
#if defined(STDROMANO_INTEL)
    const __m128i quote = _mm_set1_epi8('"');
    const __m128i backslash = _mm_set1_epi8('\\');
    const __m128i control = _mm_set1_epi8(0x1F);

    while(end - p >= 16)
    {
        const __m128i chunk = _mm_loadu_si128(reinterpret_cast<const __m128i*>(p));
        const __m128i special = _mm_or_si128(_mm_or_si128(_mm_cmpeq_epi8(chunk, quote),
                                                          _mm_cmpeq_epi8(chunk, backslash)),
                                             _mm_cmpeq_epi8(_mm_min_epu8(chunk, control), chunk));
        const std::uint32_t mask = static_cast<std::uint32_t>(_mm_movemask_epi8(special));

        if(mask != 0)
            return p + ctz_u64(mask);

        p += 16;
    }
#elif defined(STDROMANO_AARCH64)
    const uint8x16_t quote = vdupq_n_u8('"');
    const uint8x16_t backslash = vdupq_n_u8('\\');
    const uint8x16_t control = vdupq_n_u8(0x20);

    while(end - p >= 16)
    {
        const uint8x16_t chunk = vld1q_u8(reinterpret_cast<const std::uint8_t*>(p));
        const uint8x16_t special = vorrq_u8(vorrq_u8(vceqq_u8(chunk, quote), vceqq_u8(chunk, backslash)),
                                            vcltq_u8(chunk, control));
        const std::uint64_t mask = vget_lane_u64(vreinterpret_u64_u8(vshrn_n_u16(vreinterpretq_u16_u8(special), 4)), 0);

        if(mask != 0)
            return p + (ctz_u64(mask) >> 2);

        p += 16;
    }
#endif // defined(STDROMANO_INTEL)

    while(p < end && !STRING_SPECIAL.values[static_cast<std::uint8_t>(*p)])
        ++p;

    return p;
}

/****************************/
/* JsonObject: type checks  */
/****************************/

bool JsonObject::is_null() const noexcept { return this->_tags & JsonTag_Null; }
bool JsonObject::is_bool() const noexcept { return this->_tags & JsonTag_Bool; }
bool JsonObject::is_u64() const noexcept { return this->_tags & JsonTag_U64; }
bool JsonObject::is_i64() const noexcept { return this->_tags & JsonTag_I64; }
bool JsonObject::is_f64() const noexcept { return this->_tags & JsonTag_F64; }
bool JsonObject::is_str() const noexcept { return this->_tags & JsonTag_Str; }
bool JsonObject::is_array() const noexcept { return this->_tags & JsonTag_Array; }
bool JsonObject::is_dict() const noexcept { return this->_tags & JsonTag_Dict; }

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
    const std::size_t key_size = std::strlen(key);

    for(const JsonDictElement* current = info->head; current != nullptr; current = current->next)
    {
        if(current->key_size == key_size && std::memcmp(current->kv.key, key, key_size) == 0)
            return current->kv.value;
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

static constexpr std::size_t STRING_ARENA_INITIAL_SIZE = 16 * 1024;
static constexpr std::size_t VALUE_ARENA_INITIAL_SIZE = 16 * 1024;
static constexpr std::size_t ARENA_BLOCK_SIZE = 256 * 1024;

Json::Json() noexcept : _root(nullptr),
                        _string_arena(STRING_ARENA_INITIAL_SIZE, ARENA_BLOCK_SIZE),
                        _value_arena(VALUE_ARENA_INITIAL_SIZE, ARENA_BLOCK_SIZE)
{
}

Json::~Json() noexcept = default;

JsonObject* Json::root() const noexcept { return this->_root; }

void Json::set_root(JsonObject* root) noexcept { this->_root = root; }

/******************************/
/* Json: value creation       */
/******************************/

static char* copy_c_str(Arena& arena, const char* str, std::size_t& size) noexcept
{
    size = std::strlen(str);
    char* copy = static_cast<char*>(arena.allocate(size + 1));
    std::memcpy(copy, str, size);
    copy[size] = '\0';
    return copy;
}

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
    auto* obj = this->_value_arena.emplace<JsonObject>();
    this->set_str(obj, str);
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
    std::size_t str_sz = 0;
    const char* copy = copy_c_str(this->_string_arena, str, str_sz);

    tag_set_type(obj->_tags, JsonTag_Str);
    tag_set_sz(obj->_tags, static_cast<std::uint32_t>(str_sz));

    obj->_value.str = copy;
}

/******************************/
/* Json: array operations     */
/******************************/

void Json::array_append(JsonObject* array, JsonObject* value, bool reference) noexcept
{
    auto* info = static_cast<JsonArrayInfo*>(array->_value.ptr);
    JsonArrayElement* element;

    if(reference)
    {
        element = this->_value_arena.emplace<JsonArrayElement>();
        element->value = value;
    }
    else
    {
        auto* node = this->_value_arena.emplace<JsonArrayNode>();
        std::memcpy(&node->storage, value, sizeof(JsonObject));
        element = &node->element;
        element->value = &node->storage;
    }

    element->next = nullptr;

    if(info->head == nullptr)
        info->head = element;
    else
        info->tail->next = element;

    info->tail = element;
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
    JsonDictElement* element;

    if(reference)
    {
        element = this->_value_arena.emplace<JsonDictElement>();
        element->kv.value = value;
    }
    else
    {
        auto* node = this->_value_arena.emplace<JsonDictNode>();
        std::memcpy(&node->storage, value, sizeof(JsonObject));
        element = &node->element;
        element->kv.value = &node->storage;
    }

    element->kv.key = copy_c_str(this->_string_arena, key, element->key_size);
    element->next = nullptr;

    if(info->head == nullptr)
        info->head = element;
    else
        info->tail->next = element;

    info->tail = element;
    info->size++;
}

void Json::dict_pop(JsonObject* dict, const char* key) noexcept
{
    auto* info = static_cast<JsonDictInfo*>(dict->_value.ptr);
    const std::size_t key_size = std::strlen(key);

    JsonDictElement* previous = nullptr;
    auto* current = info->head;

    while(current != nullptr)
    {
        if(current->key_size == key_size && std::memcmp(current->kv.key, key, key_size) == 0)
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

static constexpr std::size_t JSON_MAX_DEPTH = 512;

static constexpr bool WHITESPACE[256] = {
    false, false, false, false, false, false, false, false,
    false, true,  true,  true,  true,  true,  false, false,
    false, false, false, false, false, false, false, false,
    false, false, false, false, false, false, false, false,
    true,
};

STDROMANO_FORCE_INLINE bool is_ascii_digit(const char c) noexcept
{
    return static_cast<unsigned int>(c - '0') < 10u;
}

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

// Decoded output is never longer than the escaped input, so out needs end - src bytes
static bool decode_escaped(const char* src, const char* end, char* out, std::size_t& out_size) noexcept
{
    char* const out_start = out;

    while(src < end)
    {
        const char* backslash = static_cast<const char*>(std::memchr(src, '\\', static_cast<std::size_t>(end - src)));

        if(backslash == nullptr)
        {
            std::memcpy(out, src, static_cast<std::size_t>(end - src));
            out += end - src;
            break;
        }

        std::memcpy(out, src, static_cast<std::size_t>(backslash - src));
        out += backslash - src;

        const char escaped = backslash[1];
        src = backslash + 2;

        switch(escaped)
        {
            case '"':  *out++ = '"';  break;
            case '\\': *out++ = '\\'; break;
            case '/':  *out++ = '/';  break;
            case 'b':  *out++ = '\b'; break;
            case 'f':  *out++ = '\f'; break;
            case 'n':  *out++ = '\n'; break;
            case 'r':  *out++ = '\r'; break;
            case 't':  *out++ = '\t'; break;
            case 'u':
            {
                std::uint32_t cp = 0;

                if(end - src < 4 || !read_hex4(src, cp))
                    return false;

                src += 4;

                if(cp >= 0xD800 && cp <= 0xDBFF)
                {
                    std::uint32_t low = 0;

                    if(end - src >= 6 && src[0] == '\\' && src[1] == 'u' && read_hex4(src + 2, low) &&
                       low >= 0xDC00 && low <= 0xDFFF)
                    {
                        cp = 0x10000 + ((cp - 0xD800) << 10) + (low - 0xDC00);
                        src += 6;
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

                out += encode_utf8(cp, out);
                break;
            }
            default:
                return false;
        }
    }

    out_size = static_cast<std::size_t>(out - out_start);
    return true;
}

// Out of range values (inf, denormal underflow) and toolchains without floating-point from_chars.
// Kept out of line so the stack buffer does not add stack protector setup to every number parse.
JSON_NO_INLINE static bool parse_f64_fallback(const char* start, const char* end, double& out) noexcept
{
    const std::size_t number_len = static_cast<std::size_t>(end - start);

    char stack_buffer[64];
    char* buffer = number_len < sizeof(stack_buffer) ? stack_buffer
                                                     : static_cast<char*>(std::malloc(number_len + 1));

    if(buffer == nullptr)
        return false;

    std::memcpy(buffer, start, number_len);
    buffer[number_len] = '\0';

    out = std::strtod(buffer, nullptr);

    if(buffer != stack_buffer)
        std::free(buffer);

    return true;
}

struct JsonParser_
{
    const char* p;
    const char* end;
    Json* json;
    std::size_t depth;

    // Values are bump-allocated from chunks taken from the arena, keeping the hot path in registers
    static constexpr std::size_t CHUNK_SIZE = 4096;

    char* value_cursor;
    char* value_end;
    char* string_cursor;
    char* string_end;

    template <typename T>
    STDROMANO_FORCE_INLINE T* allocate() noexcept
    {
        static_assert(alignof(T) <= alignof(std::max_align_t) && sizeof(T) % alignof(T) == 0);

        if(JSON_UNLIKELY(static_cast<std::size_t>(value_end - value_cursor) < sizeof(T)))
            this->refill_values();

        T* object = reinterpret_cast<T*>(value_cursor);
        value_cursor += sizeof(T);

        return object;
    }

    STDROMANO_FORCE_INLINE char* allocate_string(const std::size_t size) noexcept
    {
        if(JSON_UNLIKELY(static_cast<std::size_t>(string_end - string_cursor) < size))
            return this->allocate_string_slow(size);

        char* str = string_cursor;
        string_cursor += size;

        return str;
    }

    JSON_NO_INLINE void refill_values() noexcept
    {
        value_cursor = static_cast<char*>(json->_value_arena.allocate_aligned(CHUNK_SIZE, alignof(std::max_align_t)));
        value_end = value_cursor + CHUNK_SIZE;
    }

    JSON_NO_INLINE char* allocate_string_slow(const std::size_t size) noexcept
    {
        // Large strings get their own allocation so the current chunk is not wasted
        if(size > CHUNK_SIZE / 4)
            return static_cast<char*>(json->_string_arena.allocate(size));

        string_cursor = static_cast<char*>(json->_string_arena.allocate(CHUNK_SIZE));
        string_end = string_cursor + CHUNK_SIZE;

        char* str = string_cursor;
        string_cursor += size;

        return str;
    }

    STDROMANO_FORCE_INLINE void skip_whitespace() noexcept
    {
        while(p < end && WHITESPACE[static_cast<std::uint8_t>(*p)])
            ++p;
    }

    bool parse_string_data(const char*& data, std::size_t& size) noexcept;
    bool parse_number(JsonObject* out) noexcept;
    bool parse_array(JsonObject* out) noexcept;
    bool parse_dict(JsonObject* out) noexcept;
    bool parse_value(JsonObject* out) noexcept;
};

bool JsonParser_::parse_string_data(const char*& data, std::size_t& size) noexcept
{
    const char* const start = ++p;
    bool has_escape = false;

    for(;;)
    {
        p = find_string_special(p, end);

        if(p == end)
            return false;

        if(*p == '"')
            break;

        if(*p != '\\' || end - p < 2)
            return false;

        has_escape = true;
        p += 2;
    }

    const std::size_t raw_size = static_cast<std::size_t>(p - start);
    ++p;

    if(raw_size > UINT32_MAX)
        return false;

    char* str = this->allocate_string(raw_size + 1);

    if(!has_escape)
    {
        std::memcpy(str, start, raw_size);
        size = raw_size;
    }
    else if(!decode_escaped(start, start + raw_size, str, size))
    {
        return false;
    }

    str[size] = '\0';
    data = str;

    return true;
}

bool JsonParser_::parse_number(JsonObject* out) noexcept
{
    const char* const start = p;
    const bool is_negative = *p == '-';

    if(is_negative)
        ++p;

    if(p == end || !is_ascii_digit(*p))
        return false;

    const char* const digits = p;
    std::uint64_t magnitude = 0;

    while(p < end && is_ascii_digit(*p))
    {
        magnitude = magnitude * 10 + static_cast<std::uint64_t>(*p - '0');
        ++p;
    }

    const char* const digits_end = p;
    bool is_float = false;

    if(p < end && *p == '.')
    {
        is_float = true;
        ++p;

        if(p == end || !is_ascii_digit(*p))
            return false;

        while(p < end && is_ascii_digit(*p))
            ++p;
    }

    if(p < end && (*p == 'e' || *p == 'E'))
    {
        is_float = true;
        ++p;

        if(p < end && (*p == '-' || *p == '+'))
            ++p;

        if(p == end || !is_ascii_digit(*p))
            return false;

        while(p < end && is_ascii_digit(*p))
            ++p;
    }

    if(!is_float)
    {
        const char* significant = digits;

        while(significant + 1 < digits_end && *significant == '0')
            ++significant;

        const std::size_t num_digits = static_cast<std::size_t>(digits_end - significant);
        const bool overflow = num_digits > 20 ||
                              (num_digits == 20 && std::memcmp(significant, "18446744073709551615", 20) > 0);

        if(!overflow)
        {
            constexpr std::uint64_t min_i64_magnitude = static_cast<std::uint64_t>(INT64_MAX) + 1;

            if(!is_negative)
            {
                out->_tags = JsonTag_U64;
                out->_value.u64 = magnitude;
                return true;
            }

            if(magnitude <= min_i64_magnitude)
            {
                out->_tags = JsonTag_I64;
                out->_value.i64 = magnitude == min_i64_magnitude ? INT64_MIN : -static_cast<std::int64_t>(magnitude);
                return true;
            }
        }
    }

    out->_tags = JsonTag_F64;

#if defined(JSON_HAS_FROM_CHARS)
    const std::from_chars_result result = std::from_chars(start, p, out->_value.f64);

    if(result.ec == std::errc() && result.ptr == p)
        return true;
#endif // defined(JSON_HAS_FROM_CHARS)

    return parse_f64_fallback(start, p, out->_value.f64);
}

bool JsonParser_::parse_array(JsonObject* out) noexcept
{
    ++p;

    auto* info = this->allocate<JsonArrayInfo>();
    info->head = nullptr;
    info->tail = nullptr;
    info->size = 0;

    out->_tags = JsonTag_Array;
    out->_value.ptr = info;

    this->skip_whitespace();

    if(p < end && *p == ']')
    {
        ++p;
        return true;
    }

    if(++depth > JSON_MAX_DEPTH)
        return false;

    JsonArrayElement* head = nullptr;
    JsonArrayElement* tail = nullptr;
    std::size_t size = 0;

    for(;;)
    {
        auto* node = this->allocate<JsonArrayNode>();

        if(!this->parse_value(&node->storage))
            return false;

        node->element.value = &node->storage;
        node->element.next = nullptr;

        if(tail == nullptr)
            head = &node->element;
        else
            tail->next = &node->element;

        tail = &node->element;
        ++size;

        this->skip_whitespace();

        if(p == end)
            return false;

        const char c = *p++;

        if(c == ']')
            break;

        if(c != ',')
            return false;
    }

    info->head = head;
    info->tail = tail;
    info->size = size;

    --depth;

    return true;
}

bool JsonParser_::parse_dict(JsonObject* out) noexcept
{
    ++p;

    auto* info = this->allocate<JsonDictInfo>();
    info->head = nullptr;
    info->tail = nullptr;
    info->size = 0;

    out->_tags = JsonTag_Dict;
    out->_value.ptr = info;

    this->skip_whitespace();

    if(p < end && *p == '}')
    {
        ++p;
        return true;
    }

    if(++depth > JSON_MAX_DEPTH)
        return false;

    JsonDictElement* head = nullptr;
    JsonDictElement* tail = nullptr;
    std::size_t size = 0;

    for(;;)
    {
        this->skip_whitespace();

        if(p == end || *p != '"')
            return false;

        auto* node = this->allocate<JsonDictNode>();

        if(!this->parse_string_data(node->element.kv.key, node->element.key_size))
            return false;

        this->skip_whitespace();

        if(p == end || *p != ':')
            return false;

        ++p;

        if(!this->parse_value(&node->storage))
            return false;

        node->element.kv.value = &node->storage;
        node->element.next = nullptr;

        if(tail == nullptr)
            head = &node->element;
        else
            tail->next = &node->element;

        tail = &node->element;
        ++size;

        this->skip_whitespace();

        if(p == end)
            return false;

        const char c = *p++;

        if(c == '}')
            break;

        if(c != ',')
            return false;
    }

    info->head = head;
    info->tail = tail;
    info->size = size;

    --depth;

    return true;
}

bool JsonParser_::parse_value(JsonObject* out) noexcept
{
    this->skip_whitespace();

    if(p == end)
        return false;

    switch(*p)
    {
        case '"':
        {
            const char* str = nullptr;
            std::size_t size = 0;

            if(!this->parse_string_data(str, size))
                return false;

            out->_tags = JsonTag_Str | (static_cast<std::uint64_t>(size) << 32);
            out->_value.str = str;
            return true;
        }

        case '{':
            return this->parse_dict(out);

        case '[':
            return this->parse_array(out);

        case 'n':
            if(end - p < 4 || std::memcmp(p, "null", 4) != 0)
                return false;

            p += 4;
            out->_tags = JsonTag_Null;
            out->_value.u64 = 0;
            return true;

        case 't':
            if(end - p < 4 || std::memcmp(p, "true", 4) != 0)
                return false;

            p += 4;
            out->_tags = JsonTag_Bool;
            out->_value.u64 = 0;
            out->_value.b = true;
            return true;

        case 'f':
            if(end - p < 5 || std::memcmp(p, "false", 5) != 0)
                return false;

            p += 5;
            out->_tags = JsonTag_Bool;
            out->_value.u64 = 0;
            out->_value.b = false;
            return true;

        default:
            if(*p == '-' || is_ascii_digit(*p))
                return this->parse_number(out);

            return false;
    }
}

/****************/
/* Json writer  */
/****************/

struct JsonWriter_
{
    static constexpr std::size_t INITIAL_CAPACITY = 64 * 1024;

    char* data = nullptr;
    std::size_t size = 0;
    std::size_t capacity = 0;
    std::size_t indent_size = 0;
    std::size_t indent = 0;

    JsonWriter_() noexcept = default;

    ~JsonWriter_() noexcept
    {
        if(this->data != nullptr)
            mem_free(this->data);
    }

    JsonWriter_(const JsonWriter_&) = delete;
    JsonWriter_& operator=(const JsonWriter_&) = delete;

    bool grow(const std::size_t needed) noexcept
    {
        std::size_t new_capacity = this->capacity < INITIAL_CAPACITY ? INITIAL_CAPACITY : this->capacity;

        while(new_capacity < needed)
            new_capacity *= 2;

        char* new_data = mem_realloc<char>(this->data, new_capacity);

        if(new_data == nullptr)
            return false;

        this->data = new_data;
        this->capacity = new_capacity;

        return true;
    }

    // One spare byte is always kept for the null terminator added by finish()
    STDROMANO_FORCE_INLINE char* reserve(const std::size_t n) noexcept
    {
        if(this->size + n >= this->capacity && !this->grow(this->size + n + 1))
            return nullptr;

        return this->data + this->size;
    }

    STDROMANO_FORCE_INLINE bool write(const char* str, const std::size_t n) noexcept
    {
        char* dst = this->reserve(n);

        if(dst == nullptr)
            return false;

        std::memcpy(dst, str, n);
        this->size += n;

        return true;
    }

    STDROMANO_FORCE_INLINE bool write(const char c) noexcept
    {
        char* dst = this->reserve(1);

        if(dst == nullptr)
            return false;

        *dst = c;
        this->size++;

        return true;
    }

    // Writes the optional separator, then the newline and indentation of the next line
    STDROMANO_FORCE_INLINE bool write_line_break(const bool separator) noexcept
    {
        char* dst = this->reserve(2 + this->indent);

        if(dst == nullptr)
            return false;

        if(separator)
            *dst++ = ',';

        *dst++ = '\n';
        std::memset(dst, ' ', this->indent);

        this->size += (separator ? 2 : 1) + this->indent;

        return true;
    }

    bool finish() noexcept
    {
        char* dst = this->reserve(0);

        if(dst == nullptr)
            return false;

        *dst = '\0';

        return true;
    }

    bool write_str(const char* str, std::size_t size) noexcept;
    bool write_f64(double f64) noexcept;
    bool write_array(const JsonObject* array) noexcept;
    bool write_dict(const JsonObject* dict) noexcept;
    bool write_value(const JsonObject* value) noexcept;
};

bool JsonWriter_::write_str(const char* str, const std::size_t size) noexcept
{
    static constexpr char HEX_DIGITS[] = "0123456789abcdef";

    const char* const end = str + size;

    if(!this->write('"'))
        return false;

    while(str < end)
    {
        const char* special = find_string_special(str, end);

        if(!this->write(str, static_cast<std::size_t>(special - str)))
            return false;

        if(special == end)
            break;

        const unsigned char c = static_cast<unsigned char>(*special);
        char escaped[6] = {'\\', 0, 0, 0, 0, 0};
        std::size_t escaped_size = 2;

        switch(c)
        {
            case '"':  escaped[1] = '"';  break;
            case '\\': escaped[1] = '\\'; break;
            case '\b': escaped[1] = 'b';  break;
            case '\f': escaped[1] = 'f';  break;
            case '\n': escaped[1] = 'n';  break;
            case '\r': escaped[1] = 'r';  break;
            case '\t': escaped[1] = 't';  break;
            default:
                escaped[1] = 'u';
                escaped[2] = '0';
                escaped[3] = '0';
                escaped[4] = HEX_DIGITS[c >> 4];
                escaped[5] = HEX_DIGITS[c & 0xF];
                escaped_size = 6;
                break;
        }

        if(!this->write(escaped, escaped_size))
            return false;

        str = special + 1;
    }

    return this->write('"');
}

bool JsonWriter_::write_f64(const double f64) noexcept
{
    if(!std::isfinite(f64))
        return this->write("null", 4);

    // Shortest round-trip representation is at most 24 characters, plus ".0"
    char* dst = this->reserve(64);

    if(dst == nullptr)
        return false;

    char* const number_end = fmt::format_to(dst, "{}", f64);
    const std::size_t number_size = static_cast<std::size_t>(number_end - dst);

    bool has_float_marker = false;

    for(std::size_t i = 0; i < number_size; ++i)
    {
        if(dst[i] == '.' || dst[i] == 'e' || dst[i] == 'E')
        {
            has_float_marker = true;
            break;
        }
    }

    this->size += number_size;

    // Keeps the value a float when read back
    return has_float_marker ? true : this->write(".0", 2);
}

bool JsonWriter_::write_array(const JsonObject* array) noexcept
{
    const auto* info = static_cast<const JsonArrayInfo*>(array->_value.ptr);
    const bool pretty = this->indent_size > 0;

    if(!this->write('['))
        return false;

    this->indent += this->indent_size;

    for(const JsonArrayElement* element = info->head; element != nullptr; element = element->next)
    {
        const bool separator = element != info->head;

        if(pretty ? !this->write_line_break(separator) : (separator && !this->write(',')))
            return false;

        if(!this->write_value(element->value))
            return false;
    }

    this->indent -= this->indent_size;

    if(pretty && !this->write_line_break(false))
        return false;

    return this->write(']');
}

bool JsonWriter_::write_dict(const JsonObject* dict) noexcept
{
    const auto* info = static_cast<const JsonDictInfo*>(dict->_value.ptr);
    const bool pretty = this->indent_size > 0;

    if(!this->write('{'))
        return false;

    this->indent += this->indent_size;

    for(const JsonDictElement* element = info->head; element != nullptr; element = element->next)
    {
        const bool separator = element != info->head;

        if(pretty ? !this->write_line_break(separator) : (separator && !this->write(',')))
            return false;

        if(!this->write_str(element->kv.key, element->key_size) || !this->write(": ", 2))
            return false;

        if(!this->write_value(element->kv.value))
            return false;
    }

    this->indent -= this->indent_size;

    if(pretty && !this->write_line_break(false))
        return false;

    return this->write('}');
}

bool JsonWriter_::write_value(const JsonObject* value) noexcept
{
    switch(value->_tags & JSON_TAGS_MASK)
    {
        case JsonTag_Null:
            return this->write("null", 4);

        case JsonTag_Bool:
            return value->_value.b ? this->write("true", 4) : this->write("false", 5);

        case JsonTag_U64:
        case JsonTag_I64:
        {
            char* dst = this->reserve(24);

            if(dst == nullptr)
                return false;

            const std::to_chars_result result = (value->_tags & JsonTag_U64)
                                                    ? std::to_chars(dst, dst + 24, value->_value.u64)
                                                    : std::to_chars(dst, dst + 24, value->_value.i64);

            this->size += static_cast<std::size_t>(result.ptr - dst);
            return true;
        }

        case JsonTag_F64:
            return this->write_f64(value->_value.f64);

        case JsonTag_Str:
            return this->write_str(value->_value.str, static_cast<std::size_t>(tag_get_sz(value->_tags)));

        case JsonTag_Array:
            return this->write_array(value);

        case JsonTag_Dict:
            return this->write_dict(value);

        default:
            return false;
    }
}

/******************************/
/* Json: parse / dump         */
/******************************/

bool Json::loads(const char* str, size_t len) noexcept
{
    // Reuses the arena blocks of the previous document, invalidating its objects
    this->_root = nullptr;
    this->_value_arena.clear();
    this->_string_arena.clear();

    if(str == nullptr || len == 0)
        return false;

    JsonParser_ parser;
    parser.p = str;
    parser.end = str + len;
    parser.json = this;
    parser.depth = 0;
    parser.value_cursor = nullptr;
    parser.value_end = nullptr;
    parser.string_cursor = nullptr;
    parser.string_end = nullptr;

    auto* root = parser.allocate<JsonObject>();

    if(!parser.parse_value(root))
        return false;

    parser.skip_whitespace();

    if(parser.p != parser.end)
        return false;

    this->_root = root;

    return true;
}

bool Json::loadf(const StringD& path) noexcept
{
    FILE* file = std::fopen(path.c_str(), "rb");

    if(file == nullptr)
        return false;

    std::fseek(file, 0, SEEK_END);
    const long file_size = std::ftell(file);
    std::rewind(file);

    if(file_size <= 0)
    {
        std::fclose(file);
        return false;
    }

    auto* buffer = static_cast<char*>(std::malloc(static_cast<std::size_t>(file_size)));

    if(buffer == nullptr)
    {
        std::fclose(file);
        return false;
    }

    const std::size_t read_size = std::fread(buffer, sizeof(char), static_cast<std::size_t>(file_size), file);
    std::fclose(file);

    const bool res = read_size == static_cast<std::size_t>(file_size) && this->loads(buffer, read_size);

    std::free(buffer);

    return res;
}

StringD Json::dumps(size_t indent_size) const noexcept
{
    if(this->_root == nullptr)
        return StringD();

    JsonWriter_ writer;
    writer.indent_size = indent_size;

    if(!writer.write_value(this->_root) || !writer.finish())
        return StringD();

    return StringD(writer.data, writer.size);
}

bool Json::dumpf(size_t indent_size, const StringD& path) const noexcept
{
    JsonWriter_ writer;
    writer.indent_size = indent_size;

    if(this->_root != nullptr && !writer.write_value(this->_root))
        return false;

    FILE* file = std::fopen(path.c_str(), "wb");

    if(file == nullptr)
        return false;

    const std::size_t written = writer.size > 0 ? std::fwrite(writer.data, sizeof(char), writer.size, file) : 0;
    std::fclose(file);

    return written == writer.size;
}

STDROMANO_NAMESPACE_END
