// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - Present Romain Augier
// All rights reserved.

#include "stdromano/string.hpp"
#include "stdromano/simd.hpp"

STDROMANO_NAMESPACE_BEGIN

/* UTF-8 validation based on the paper of John Keiser and Daniel Lemire */

bool validate_utf8_scalar(const char* str, std::size_t size) noexcept
{
    std::size_t i = 0;

    while(i < size)
    {

    }

    return true;
}

constexpr std::uint8_t TOO_SHORT = 1 << 0;
constexpr std::uint8_t TOO_LONG = 1 << 1;
constexpr std::uint8_t OVERLONG_3 = 1 << 2;
constexpr std::uint8_t SURROGATE = 1 << 4;
constexpr std::uint8_t OVERLONG_2 = 1 << 5;
constexpr std::uint8_t TWO_CONTS = 1 << 7;
constexpr std::uint8_t TOO_LARGE = 1 << 3;
constexpr std::uint8_t TOO_LARGE_1000 = 1 << 6;
constexpr std::uint8_t OVERLONG_4 = 1 << 6;
constexpr std::uint8_t CARRY = TOO_SHORT | TOO_LONG | TWO_CONTS;

alignas(32) static const std::uint8_t table1[16] = {
    TOO_LONG,
    TOO_LONG,
    TOO_LONG,
    TOO_LONG,
    TOO_LONG,
    TOO_LONG,
    TOO_LONG,
    TOO_LONG,
    TWO_CONTS,
    TWO_CONTS,
    TWO_CONTS,
    TWO_CONTS,
    TOO_SHORT | OVERLONG_2,
    TOO_SHORT,
    TOO_SHORT | OVERLONG_3 | SURROGATE,
    TOO_SHORT | TOO_LARGE | TOO_LARGE_1000 | OVERLONG_4,
};

alignas(32) static const std::uint8_t table2[16] = {
    CARRY | OVERLONG_3 | OVERLONG_2 | OVERLONG_4,
    CARRY | OVERLONG_2,
    CARRY,
    CARRY,
    CARRY | TOO_LARGE,
    CARRY | TOO_LARGE | TOO_LARGE_1000,
    CARRY | TOO_LARGE | TOO_LARGE_1000,
    CARRY | TOO_LARGE | TOO_LARGE_1000,
    CARRY | TOO_LARGE | TOO_LARGE_1000,
    CARRY | TOO_LARGE | TOO_LARGE_1000,
    CARRY | TOO_LARGE | TOO_LARGE_1000,
    CARRY | TOO_LARGE | TOO_LARGE_1000,
    CARRY | TOO_LARGE | TOO_LARGE_1000,
    CARRY | TOO_LARGE | TOO_LARGE_1000 | SURROGATE,
    CARRY | TOO_LARGE | TOO_LARGE_1000,
    CARRY | TOO_LARGE | TOO_LARGE_1000,
};

alignas(32) static const std::uint8_t table3[16] = {
    TOO_SHORT,
    TOO_SHORT,
    TOO_SHORT,
    TOO_SHORT,
    TOO_SHORT,
    TOO_SHORT,
    TOO_SHORT,
    TOO_SHORT,
    TOO_LONG | OVERLONG_2 | TWO_CONTS | OVERLONG_3 | TOO_LARGE_1000 | OVERLONG_4,
    TOO_LONG | OVERLONG_2 | TWO_CONTS | OVERLONG_3 | TOO_LARGE,
    TOO_LONG | OVERLONG_2 | TWO_CONTS | SURROGATE | TOO_LARGE,
    TOO_LONG | OVERLONG_2 | TWO_CONTS | SURROGATE | TOO_LARGE,
    TOO_SHORT,
    TOO_SHORT,
    TOO_SHORT,
    TOO_SHORT,
};

/* SSE */

__m128i validate_utf8_sse_kernel(const __m128i input,
                                 const __m128i prev_input) noexcept
{
    static const __m128i t1 = _mm_load_si128(reinterpret_cast<const __m128i*>(table1));
    static const __m128i t2 = _mm_load_si128(reinterpret_cast<const __m128i*>(table2));
    static const __m128i t3 = _mm_load_si128(reinterpret_cast<const __m128i*>(table3));
    static const __m128i mask_0F = _mm_set1_epi8(0x0F);

    const __m128i prev1 = _mm_alignr_epi8(input, prev_input, 15);
    
    __m128i byte1_high = _mm_and_si128(_mm_srli_epi16(prev1, 4), mask_0F);
    byte1_high = _mm_shuffle_epi8(t1, byte1_high);
    
    __m128i byte1_low = _mm_and_si128(prev1, mask_0F);
    byte1_low = _mm_shuffle_epi8(t2, byte1_low);
    
    __m128i byte2_high = _mm_and_si128(_mm_srli_epi16(input, 4), mask_0F);
    byte2_high = _mm_shuffle_epi8(t3, byte2_high);
    
    __m128i potential_errors = _mm_and_si128(byte1_high, byte1_low);
    potential_errors = _mm_and_si128(potential_errors, byte2_high);

    const __m128i prev2 = _mm_alignr_epi8(input, prev_input, 14);
    const __m128i prev3 = _mm_alignr_epi8(input, prev_input, 13);

    const __m128i is_3_byte_lead = _mm_subs_epu8(prev2, _mm_set1_epi8(0xDF));
    const __m128i is_4_byte_lead = _mm_subs_epu8(prev3, _mm_set1_epi8(0xEF));

    const __m128i must_be_cont = _mm_or_si128(is_3_byte_lead, is_4_byte_lead);

    const __m128i must_be_cont_mask = _mm_and_si128(
        _mm_cmpgt_epi8(must_be_cont, _mm_setzero_si128()),
        _mm_set1_epi8(TWO_CONTS)
    );

    return _mm_xor_si128(potential_errors, must_be_cont_mask);
}

bool validate_utf8_sse(const char* str, std::size_t size) noexcept
{
    const std::uint8_t* input = reinterpret_cast<const std::uint8_t*>(str);
    std::size_t i = 0;
    
    __m128i prev_input = _mm_setzero_si128();
    __m128i err = _mm_setzero_si128();
    
    while(i + 16 <= size) 
    {
        const __m128i current_input = _mm_loadu_si128(reinterpret_cast<const __m128i*>(input + i));
        
        const __m128i err_in_current = validate_utf8_sse_kernel(current_input, prev_input);
        err = _mm_or_si128(err, err_in_current);
        
        prev_input = current_input;
        i += 16;
    }

    if(i < size) 
    {
        const std::size_t remaining = size - i;
        alignas(16) char tail_buffer[16];
        
        std::memcpy(tail_buffer, input + i, remaining);
        
        std::memset(tail_buffer + remaining, ' ', 16 - remaining);
        
        const __m128i current_input = _mm_load_si128(reinterpret_cast<const __m128i*>(tail_buffer));
        const __m128i err_in_current = validate_utf8_sse_kernel(current_input, prev_input);
        err = _mm_or_si128(err, err_in_current);
    }

    if(size > 0) 
    {
        const std::uint8_t last_byte = input[size - 1];
        if(last_byte >= 0xC0)
            return false;
    }

    if(size > 1) 
    {
        const std::uint8_t second_last = input[size - 2];
        if(second_last >= 0xE0 && (input[size-1] & 0xC0) != 0x80)
            return false;
    }

    if(size > 2) 
    {
        const std::uint8_t third_last = input[size - 3];
        if(third_last >= 0xF0 && (input[size-2] & 0xC0) != 0x80)
            return false;
    }
    
    return _mm_testz_si128(err, err) == 1;
}

/* AVX */

__m256i validate_utf8_avx_kernel(const __m256i input,
                                 const __m256i prev_input) noexcept
{
    static const __m256i t1 = _mm256_broadcastsi128_si256(_mm_load_si128(reinterpret_cast<const __m128i*>(table1)));
    static const __m256i t2 = _mm256_broadcastsi128_si256(_mm_load_si128(reinterpret_cast<const __m128i*>(table2)));
    static const __m256i t3 = _mm256_broadcastsi128_si256(_mm_load_si128(reinterpret_cast<const __m128i*>(table3)));
    static const __m256i mask_0F = _mm256_set1_epi8(0x0F);

    const __m256i prev1 = _mm256_alignr_epi8(input, _mm256_permute2x128_si256(prev_input, input, 0x21), 15);

    __m256i byte1_high = _mm256_and_si256(_mm256_srli_epi16(prev1, 4), mask_0F);
    byte1_high = _mm256_shuffle_epi8(t1, byte1_high);

    __m256i byte1_low = _mm256_and_si256(prev1, mask_0F);
    byte1_low = _mm256_shuffle_epi8(t2, byte1_low);

    __m256i byte2_high = _mm256_and_si256(_mm256_srli_epi16(input, 4), mask_0F);
    byte2_high = _mm256_shuffle_epi8(t3, byte2_high);

    __m256i potential_errors = _mm256_and_si256(byte1_high, byte1_low);
    potential_errors = _mm256_and_si256(potential_errors, byte2_high);

    const __m256i prev2 = _mm256_alignr_epi8(input, _mm256_permute2x128_si256(prev_input, input, 0x21), 14);
    const __m256i prev3 = _mm256_alignr_epi8(input, _mm256_permute2x128_si256(prev_input, input, 0x21), 13);
    
    const __m256i is_3_byte_lead = _mm256_subs_epu8(prev2, _mm256_set1_epi8(0xDF));
    const __m256i is_4_byte_lead = _mm256_subs_epu8(prev3, _mm256_set1_epi8(0xEF));

    const __m256i must_be_cont = _mm256_or_si256(is_3_byte_lead, is_4_byte_lead);

    const __m256i must_be_cont_mask = _mm256_and_si256(
        _mm256_cmpgt_epi8(must_be_cont, _mm256_setzero_si256()),
        _mm256_set1_epi8(TWO_CONTS)
    );
    
    return _mm256_xor_si256(potential_errors, must_be_cont_mask);
}

bool validate_utf8_avx(const char* str, std::size_t size) noexcept
{
    const std::uint8_t* input = reinterpret_cast<const std::uint8_t*>(str);
    std::size_t i = 0;

    __m256i prev_input = _mm256_setzero_si256();
    __m256i err = _mm256_setzero_si256();

    while(i + 32 <= size) 
    {
        const __m256i current_input = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(input + i));

        const __m256i err_in_current = validate_utf8_avx_kernel(current_input, prev_input);
        err = _mm256_or_si256(err, err_in_current);

        prev_input = current_input;
        i += 32;
    }

    if(i < size)
    {
        const std::size_t remaining = size - i;
        alignas(32) char tail_buffer[32];
        std::memcpy(tail_buffer, input + i, remaining);
        std::memset(tail_buffer + remaining, ' ', 32 - remaining);
        
        const __m256i current_input = _mm256_load_si256(reinterpret_cast<const __m256i*>(tail_buffer));
        const __m256i err_in_current = validate_utf8_avx_kernel(current_input, prev_input);
        err = _mm256_or_si256(err, err_in_current);
    }
    
    if(size > 0) 
    {
        const std::uint8_t last_byte = input[size - 1];
        if(last_byte >= 0xC0)
            return false;
    }

    if(size > 1) 
    {
        const std::uint8_t second_last = input[size - 2];
        if(second_last >= 0xE0 && (input[size-1] & 0xC0) != 0x80)
            return false;
    }

    if(size > 2) 
    {
        const std::uint8_t third_last = input[size - 3];
        if(third_last >= 0xF0 && (input[size-2] & 0xC0) != 0x80)
            return false;
    }

    return _mm256_testz_si256(err, err) == 1;
}


bool validate_utf8(const char* str, std::size_t size) noexcept
{
    switch(simd_get_vectorization_mode())
    {
        case VectorizationMode_SSE:
            return validate_utf8_sse(str, size);
        case VectorizationMode_AVX:
        case VectorizationMode_AVX2:
            return validate_utf8_avx(str, size);
        default:
            return validate_utf8_scalar(str, size);
    }
}

char* strrstr(const char* haystack, const char* needle) noexcept
{
    if(*needle == '\0')
    {
        return const_cast<char*>(haystack);
    }

    char* s = const_cast<char*>(haystack);

    char* result = nullptr;

    while(true)
    {
        char* p = const_cast<char*>(std::strstr(s, needle));

        if(p == nullptr)
        {
            break;
        }

        result = p;
        s = p + 1;
    }

    return result;
}

bool case_insensitive_less(char lhs, char rhs) noexcept
{
    return to_lower(static_cast<int>(lhs)) < to_lower(static_cast<int>(rhs));
}

bool case_sensitive_less(char lhs, char rhs) noexcept
{
    return lhs < rhs;
}

int strcmp(const StringD& lhs, const StringD& rhs, bool case_sensitive) noexcept
{
    bool ret = std::lexicographical_compare(lhs.begin(), 
                                            lhs.end(),
                                            rhs.begin(),
                                            rhs.end(),
                                            case_sensitive ? case_sensitive_less : 
                                                             case_insensitive_less);

    return ret ? -1 : 1;
}

STDROMANO_NAMESPACE_END