// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - Present Romain Augier
// All rights reserved.

#include "stdromano/regex.hpp"
#include "stdromano/logger.hpp"

#include <stack>

STDROMANO_NAMESPACE_BEGIN

/*
    https://dl.acm.org/doi/pdf/10.1145/363347.363387

    https://arxiv.org/pdf/2407.20479#:~:text=We%20present%20a%20tool%20and,on%20the%20baseline%2C%20and%20outperforms
*/

enum RegexOperatorType : std::uint16_t
{
    RegexOperatorType_Alternate,
    RegexOperatorType_Concatenate,
    RegexOperatorType_ZeroOrMore,
    RegexOperatorType_OneOrMore,
    RegexOperatorType_ZeroOrOne,
};

const char* regex_operator_type_to_string(std::uint16_t op)
{
    switch(op)
    {
        case RegexOperatorType_Alternate:
            return "Alternate";
        case RegexOperatorType_Concatenate:
            return "Concatenate";
        case RegexOperatorType_ZeroOrMore:
            return "ZeroOrMore";
        case RegexOperatorType_OneOrMore:
            return "OneOrMore";
        case RegexOperatorType_ZeroOrOne:
            return "ZeroOrOne";
        default:
            return "Unknown Operator";
    }
}

enum RegexCharacterType : std::uint16_t
{
    RegexCharacterType_Single,
    RegexCharacterType_Range,
    RegexCharacterType_Any,
};

const char* regex_character_type_to_string(std::uint16_t c)
{
    switch(c)
    {
        case RegexCharacterType_Single:
            return "Single";
        case RegexCharacterType_Range:
            return "Range";
        case RegexCharacterType_Any:
            return "Any";
        default:
            return "Unknown Character";
    }
}

/* Lexing / Parsing */

enum RegexTokenType : std::uint32_t
{
    RegexTokenType_Character,
    RegexTokenType_CharacterRange,
    RegexTokenType_Operator,
    RegexTokenType_GroupBegin,
    RegexTokenType_GroupEnd,
    RegexTokenType_Invalid,
};

/* We don't use string_view because we want to keep this struct 16 bytes */
struct RegexToken
{
    const char* data;
    std::uint32_t data_len;
    std::uint16_t type;
    std::uint16_t encoding; /* Either RegexOperatorType or RegexCharacterType */

    RegexToken() : data(nullptr), data_len(0), type(RegexTokenType_Invalid), encoding(0) {}

    RegexToken(const char* data,
               std::uint32_t data_len,
               std::uint32_t type,
               std::uint16_t encoding = std::uint16_t(0)) : data(data), 
                                                            data_len(data_len),
                                                            type(type),
                                                            encoding(encoding) {}

    RegexToken(const RegexToken& other) : data(other.data),
                                          data_len(other.data_len),
                                          type(other.type),
                                          encoding(other.encoding) {}

    RegexToken& operator=(const RegexToken& other) 
    {
        this->data = other.data;
        this->data_len = other.data_len;
        this->type = other.type;
        this->encoding = other.encoding;

        return *this;
    }

    RegexToken(RegexToken&& other) noexcept : data(other.data),
                                              data_len(other.data_len),
                                              type(other.type),
                                              encoding(other.encoding) {}

    RegexToken& operator=(RegexToken&& other) noexcept
    {
        this->data = other.data;
        this->data_len = other.data_len;
        this->type = other.type;
        this->encoding = other.encoding;

        return *this;
    }

    void print() const
    {
        switch(type)
        {
            case RegexTokenType_Character:
            {
                if(this->data != nullptr && this->data_len > 0)
                {
                    log_debug("CHAR({}, {})",
                              std::string_view(this->data, this->data_len),
                              regex_character_type_to_string(this->encoding));
                }
                else
                {
                    log_debug("CHAR({})",
                              regex_character_type_to_string(this->encoding));

                }

                break;
            }

            case RegexTokenType_CharacterRange:
            {
                log_debug("RANGE({})", std::string_view(this->data, this->data_len));
                break;
            }

            case RegexTokenType_Operator:
            {
                log_debug("OP({})", regex_operator_type_to_string(this->encoding));
                break;
            }

            case RegexTokenType_GroupBegin:
            {
                log_debug("GROUP_BEGIN({})", this->encoding);
                break;
            }

            case RegexTokenType_GroupEnd:
            {
                log_debug("GROUP_END({})", this->encoding);
                break;
            }

            case RegexTokenType_Invalid:
            {
                log_debug("INVALID");
                break;
            }

            default:
            {
                log_debug("UNKNOWN_TOKEN");
                break;
            }
        }
    }
};

std::tuple<bool, Vector<RegexToken>> lex_regex(const StringD& regex) noexcept
{
    Vector<RegexToken> tokens;

    std::uint32_t i = 0;
    bool need_concat = false;

    while(i < regex.size())
    {
        if(std::isalnum(regex[i]) || regex[i] == '_')
        {
            if(need_concat)
            {
                tokens.emplace_back(nullptr, 0u, RegexTokenType_Operator, RegexOperatorType_Concatenate);
            }

            tokens.emplace_back(regex.data() + i, 1u, RegexTokenType_Character, RegexCharacterType_Single);
            need_concat = true;
        }
        else
        {
            switch(regex[i])
            {
                case '|':
                {
                    tokens.emplace_back(regex.data() + i, 1u, RegexTokenType_Operator, RegexOperatorType_Alternate);
                    need_concat = false;
                    break;
                }

                case '*':
                {
                    tokens.emplace_back(regex.data() + i, 1u, RegexTokenType_Operator, RegexOperatorType_ZeroOrMore);
                    break;
                }

                case '+':
                {
                    tokens.emplace_back(regex.data() + i, 1u, RegexTokenType_Operator, RegexOperatorType_OneOrMore);
                    break;
                }

                case '?':
                {
                    tokens.emplace_back(regex.data() + i, 1u, RegexTokenType_Operator, RegexOperatorType_ZeroOrOne);
                    break;
                }

                case '.':
                {
                    if(need_concat)
                    {
                        tokens.emplace_back(nullptr, 0u, RegexTokenType_Operator, RegexOperatorType_Concatenate);
                    }

                    tokens.emplace_back(regex.data() + i, 1u, RegexTokenType_Character, RegexCharacterType_Any);
                    need_concat = true;

                    break;
                }

                case '(':
                {
                    if(need_concat)
                    {
                        tokens.emplace_back(nullptr, 0u, RegexTokenType_Operator, RegexOperatorType_Concatenate);
                    }

                    tokens.emplace_back(regex.data() + i, 1u, RegexTokenType_GroupBegin);
                    need_concat = false;

                    break;
                }

                case ')':
                {
                    tokens.emplace_back(regex.data() + i, 1u, RegexTokenType_GroupEnd);
                    need_concat = true;
                    break;
                }

                case '[':
                {
                    if(need_concat)
                    {
                        tokens.emplace_back(nullptr, 0u, RegexTokenType_Operator, RegexOperatorType_Concatenate);
                    }

                    const char* start = regex.data() + ++i;

                    while(i < regex.size() && regex[i] != ']')
                    {
                        i++;
                    }

                    if(i >= regex.size())
                    {
                        log_error("Unclosed character range in regular expression");
                        return std::make_tuple(false, tokens);
                    }

                    tokens.emplace_back(start, (regex.data() + i) - start, RegexTokenType_CharacterRange);
                    need_concat = true;
                    break;
                }

                default:
                {
                    log_error("Unsupported character found in regular expression: {}", regex[i]);
                    return std::make_tuple(false, tokens);
                }
            }
        }

        i++;
    }

    return std::make_tuple(true, tokens);
}

/* Operator precedence for infix to postfix conversion */
inline int get_operator_precedence(RegexOperatorType op) noexcept
{
    switch(op)
    {
        case RegexOperatorType_Alternate:
            return 1;
        case RegexOperatorType_Concatenate:
            return 2;
        case RegexOperatorType_ZeroOrMore:
        case RegexOperatorType_OneOrMore:
        case RegexOperatorType_ZeroOrOne:
            return 3;
        default:
            return 0;
    }
}

inline bool is_left_associative(RegexOperatorType op) noexcept
{
    switch(op)
    {
        case RegexOperatorType_Alternate:
        case RegexOperatorType_Concatenate:
            return true;
        case RegexOperatorType_ZeroOrMore:
        case RegexOperatorType_OneOrMore:
        case RegexOperatorType_ZeroOrOne:
            return false; /* These are postfix operators */
        default:
            return true;
    }
}

void print_tokens(const Vector<RegexToken>& tokens)
{
    for(std::size_t i = 0; i < tokens.size(); ++i)
    {
        tokens[i].print();
    }
}

/* Instructions / Bytecode */

enum RegexInstrOpCode : std::uint8_t
{
    RegexInstrOpCode_TestSingle,
    RegexInstrOpCode_TestRange,
    RegexInstrOpCode_TestNegatedRange,
    RegexInstrOpCode_TestAny,
    RegexInstrOpCode_TestDigit,
    RegexInstrOpCode_TestLowerCase,
    RegexInstrOpCode_TestUpperCase,
    RegexInstrOpCode_JumpEq, /* jumps if status_flag = true */
    RegexInstrOpCode_JumpNeq, /* jumps if status_flag = false */
    RegexInstrOpCode_Accept,
    RegexInstrOpCode_Fail,
    RegexInstrOpCode_GroupStart,
    RegexInstrOpCode_GroupEnd,
    RegexInstrOpCode_IncPos,     /* ++string_pos */
    RegexInstrOpCode_DecPos,     /* --string_pos */
    RegexInstrOpCode_IncPosEq, /* string_pos += (status_flag ? 1 : 0) */
    RegexInstrOpCode_JumpPos,    /* Absolute jump in string position */
    RegexInstrOpCode_SetFlag, /* status_flag = static_cast<bool>(bytecode[pc + 1]) */
};

enum RegexInstrOpType : std::uint32_t
{
    RegexInstrOpType_TestOp,
    RegexInstrOpType_UnaryOp,
    RegexInstrOpType_BinaryOp,
    RegexInstrOpType_GroupOp,
};

/* std::bitcast like functions to convert back and forth */
template<typename T,
         typename = std::enable_if<sizeof(T) == sizeof(std::byte)>>
inline std::byte BYTE(const T& t) noexcept { std::byte b; std::memcpy(&b, &t, sizeof(std::byte)); return b; }

template<typename T,
         typename = std::enable_if<sizeof(T) == sizeof(std::byte)>>
inline std::byte BYTE(T&& t) noexcept { std::byte b; const T t2 = t; std::memcpy(&b, &t2, sizeof(std::byte)); return b; }

inline char CHAR(const std::byte b) noexcept { char c; std::memcpy(&c, &b, sizeof(char)); return c; }

static constexpr int JUMP_FAIL = std::numeric_limits<int>::max();

inline std::tuple<std::byte, std::byte, std::byte, std::byte> encode_jump(const int jump_size) noexcept
{
    return std::make_tuple(BYTE((jump_size >> 24) & 0xFF),
                           BYTE((jump_size >> 16) & 0xFF),
                           BYTE((jump_size >> 8) & 0xFF),
                           BYTE((jump_size >> 0) & 0xFF));
}

inline int decode_jump(const std::byte* bytecode) noexcept
{
    return static_cast<int>(bytecode[0]) << 24 |
           static_cast<int>(bytecode[1]) << 16 |
           static_cast<int>(bytecode[2]) << 8 |
           static_cast<int>(bytecode[3]) << 0;
}

inline std::size_t emit_jump(Regex::ByteCode& code, RegexInstrOpCode op) noexcept
{
    code.push_back(BYTE(op));

    std::size_t pos = code.size();
    const auto [b1, b2, b3, b4] = encode_jump(0);

    code.insert(code.cend(), { b1, b2, b3, b4 });

    return pos;
}

inline void patch_jump(Regex::ByteCode& code, std::size_t jump_pos, int offset) noexcept
{
    const auto [b1, b2, b3, b4] = encode_jump(offset);

    code[jump_pos + 0] = b1;
    code[jump_pos + 1] = b2;
    code[jump_pos + 2] = b3;
    code[jump_pos + 3] = b4;
}

inline Regex::ByteCode get_range_instrs(char range_start, char range_end) noexcept
{
    if(range_start == '0' && range_end == '9')
    {
        return { BYTE(RegexInstrOpCode_TestDigit), BYTE(RegexInstrOpCode_IncPosEq) };
    }
    else if(range_start == 'a' && range_end == 'z')
    {
        return { BYTE(RegexInstrOpCode_TestLowerCase), BYTE(RegexInstrOpCode_IncPosEq) };
    }
    else if(range_start == 'A' && range_end == 'Z')
    {
        return { BYTE(RegexInstrOpCode_TestUpperCase), BYTE(RegexInstrOpCode_IncPosEq) };
    }
    else
    {
        return { 
                    BYTE(RegexInstrOpCode_TestRange),
                    BYTE(range_start),
                    BYTE(range_end),
                    BYTE(RegexInstrOpCode_IncPosEq)
                };
    }
}

bool emit_alternation(const Vector<RegexToken>& tokens,
                      std::size_t& pos,
                      std::uint32_t& current_group_id,
                      Regex::ByteCode& bytecode) noexcept;

bool emit_concatenation(const Vector<RegexToken>& tokens,
                        std::size_t& pos,
                        std::uint32_t& current_group_id,
                        Regex::ByteCode& bytecode) noexcept;

bool emit_quantified(const Vector<RegexToken>& tokens,
                     std::size_t& pos,
                     std::uint32_t& current_group_id,
                     Regex::ByteCode& bytecode) noexcept;

bool emit_primary(const Vector<RegexToken>& tokens,
                  std::size_t& pos,
                  std::uint32_t& current_group_id,
                  Regex::ByteCode& bytecode) noexcept;

bool emit_alternation(const Vector<RegexToken>& tokens,
                      std::size_t& pos,
                      std::uint32_t& current_group_id,
                      Regex::ByteCode& bytecode) noexcept
{
    std::size_t lhs_start = bytecode.size();
    
    if(!emit_concatenation(tokens, pos, current_group_id, bytecode))
        return false;
    
    while(pos < tokens.size() && 
          tokens[pos].type == RegexTokenType_Operator &&
          static_cast<RegexOperatorType>(tokens[pos].encoding) == RegexOperatorType_Alternate)
    {
        pos++;
        
        std::size_t jump_over_rhs_pos = emit_jump(bytecode, RegexInstrOpCode_JumpEq);
        
        std::size_t rhs_start = bytecode.size();
        
        if(!emit_concatenation(tokens, pos, current_group_id, bytecode))
            return false;
        
        int offset = static_cast<int>(bytecode.size() - (jump_over_rhs_pos - 1));
        patch_jump(bytecode, jump_over_rhs_pos, offset);
    }
    
    return true;
}

bool emit_concatenation(const Vector<RegexToken>& tokens,
                        std::size_t& pos,
                        std::uint32_t& current_group_id,
                        Regex::ByteCode& bytecode) noexcept
{
    bool first = true;
    
    while(pos < tokens.size() && 
          tokens[pos].type != RegexTokenType_GroupEnd &&
          !(tokens[pos].type == RegexTokenType_Operator &&
          static_cast<RegexOperatorType>(tokens[pos].encoding) == RegexOperatorType_Alternate))
    {
        if(!first)
        {
            std::size_t jump_pos = emit_jump(bytecode, RegexInstrOpCode_JumpNeq);
            patch_jump(bytecode, jump_pos, JUMP_FAIL);
        }
        
        if(!emit_quantified(tokens, pos, current_group_id, bytecode))
            return false;
        
        first = false;
    }
    
    return true;
}

bool emit_quantified(const Vector<RegexToken>& tokens, std::size_t& pos,
                           std::uint32_t& current_group_id, Regex::ByteCode& bytecode) noexcept
{
    std::size_t primary_start = bytecode.size();
    
    if(!emit_primary(tokens, pos, current_group_id, bytecode))
        return false;
    
    if(pos < tokens.size() && tokens[pos].type == RegexTokenType_Operator)
    {
        RegexOperatorType op_type = static_cast<RegexOperatorType>(tokens[pos].encoding);
        
        if(op_type == RegexOperatorType_ZeroOrMore)
        {
            pos++;
            
            int offset_back = static_cast<int>(primary_start - bytecode.size());
            std::size_t loop_jump_pos = emit_jump(bytecode, RegexInstrOpCode_JumpEq);
            patch_jump(bytecode, loop_jump_pos, offset_back);
            
            bytecode.push_back(BYTE(RegexInstrOpCode_SetFlag));
            bytecode.push_back(BYTE(1));
        }
        else if(op_type == RegexOperatorType_OneOrMore)
        {
            pos++;

            std::size_t loop_start = bytecode.size();

            Regex::ByteCode primary_copy;
            primary_copy.insert(primary_copy.begin(),
                                bytecode.begin() + primary_start, 
                                bytecode.begin() + loop_start);
            
            std::size_t fail_jump_pos = emit_jump(bytecode, RegexInstrOpCode_JumpNeq);
            patch_jump(bytecode, fail_jump_pos, JUMP_FAIL);

            bytecode.insert(bytecode.end(), primary_copy.begin(), primary_copy.end());

            std::size_t exit_loop_jump_pos = emit_jump(bytecode, RegexInstrOpCode_JumpNeq);
            
            bytecode.push_back(BYTE(RegexInstrOpCode_SetFlag));
            bytecode.push_back(BYTE(1));
            
            int offset_back = static_cast<int>(loop_start - bytecode.size());
            std::size_t loop_jump_pos = emit_jump(bytecode, RegexInstrOpCode_JumpEq);
            patch_jump(bytecode, loop_jump_pos, offset_back);
            
            patch_jump(bytecode, exit_loop_jump_pos, 
                      static_cast<int>(bytecode.size() - (exit_loop_jump_pos - 1)));
        }
        else if(op_type == RegexOperatorType_ZeroOrOne)
        {
            pos++;
            
            bytecode.push_back(BYTE(RegexInstrOpCode_SetFlag));
            bytecode.push_back(BYTE(1));
        }
    }
    
    return true;
}

bool emit_primary(const Vector<RegexToken>& tokens, std::size_t& pos,
                        std::uint32_t& current_group_id, Regex::ByteCode& bytecode) noexcept
{
    if(pos >= tokens.size())
    {
        log_error("Unexpected end of regex expression");
        return false;
    }
    
    const RegexToken& token = tokens[pos];
    
    switch(token.type)
    {
        case RegexTokenType_Character:
        {
            pos++;
            
            switch(token.encoding)
            {
                case RegexCharacterType_Single:
                    bytecode.push_back(BYTE(RegexInstrOpCode_TestSingle));
                    bytecode.push_back(BYTE(token.data[0]));
                    break;
                    
                case RegexCharacterType_Any:
                    bytecode.push_back(BYTE(RegexInstrOpCode_TestAny));
                    break;
            }
            
            bytecode.push_back(BYTE(RegexInstrOpCode_IncPosEq));

            return true;
        }
        
        case RegexTokenType_CharacterRange:
        {
            pos++;
            
            Regex::ByteCode range_instrs = get_range_instrs(token.data[0], token.data[2]);
            bytecode.insert(bytecode.end(), range_instrs.begin(), range_instrs.end());
            return true;
        }
        
        case RegexTokenType_GroupBegin:
        {
            std::uint32_t group_id = current_group_id++;
            pos++;
            
            bytecode.push_back(BYTE(RegexInstrOpCode_GroupStart));
            bytecode.push_back(BYTE(group_id & 0xFF));
            
            if(!emit_alternation(tokens, pos, current_group_id, bytecode))
                return false;
            
            if(pos >= tokens.size() || tokens[pos].type != RegexTokenType_GroupEnd)
            {
                log_error("Mismatched parentheses in regular expression");
                return false;
            }
            
            bytecode.push_back(BYTE(RegexInstrOpCode_GroupEnd));
            bytecode.push_back(BYTE(group_id & 0xFF));
            
            pos++;
            return true;
        }

        case RegexTokenType_Operator:
        {
            pos++;

            switch(token.encoding)
            {
                case RegexOperatorType_Alternate:
                    return emit_alternation(tokens, pos, current_group_id, bytecode);
                case RegexOperatorType_Concatenate:
                    return emit_concatenation(tokens, pos, current_group_id, bytecode);
                default:
                    log_error("Invalid operator type during bytecode emission");
                    return false;
            }
        }
        
        default:
        {
            log_error("Invalid token type during bytecode emission");

            return false;
        }
    }
}

/* Recursive descent bytecode emitter for regex */
std::tuple<bool, Regex::ByteCode> regex_emit(const Vector<RegexToken>& tokens) noexcept
{
    Regex::ByteCode bytecode;
    std::size_t pos = 0;
    std::uint32_t current_group_id = 1;
    
    if(tokens.empty())
    {
        bytecode.push_back(BYTE(RegexInstrOpCode_Accept));
        return { true, std::move(bytecode) };
    }
    
    if(!emit_alternation(tokens, pos, current_group_id, bytecode))
    {
        return { false, Regex::ByteCode() };
    }
    
    if(pos < tokens.size())
    {
        log_error("Unexpected tokens after bytecode emission");
        return { false, Regex::ByteCode() };
    }
    
    std::size_t final_fail_jump = emit_jump(bytecode, RegexInstrOpCode_JumpNeq);
    patch_jump(bytecode, final_fail_jump, JUMP_FAIL);
    
    bytecode.push_back(BYTE(RegexInstrOpCode_Accept));
    bytecode.push_back(BYTE(RegexInstrOpCode_Fail));
    
    std::size_t i = 0;

    while(i < bytecode.size())
    {
        switch(static_cast<std::uint8_t>(bytecode[i]))
        {
            case RegexInstrOpCode_JumpEq:
            case RegexInstrOpCode_JumpNeq:
            {
                int offset = decode_jump(std::addressof(bytecode[i + 1]));
                
                if(offset == JUMP_FAIL)
                {
                    offset = static_cast<int>(bytecode.size() - 1 - (i + 5));
                    
                    const auto [b1, b2, b3, b4] = encode_jump(offset);
                    bytecode[i + 1] = b1;
                    bytecode[i + 2] = b2;
                    bytecode[i + 3] = b3;
                    bytecode[i + 4] = b4;
                }
                
                i += 5;
                break;
            }
            
            case RegexInstrOpCode_Accept:
            case RegexInstrOpCode_Fail:
            case RegexInstrOpCode_TestAny:
            case RegexInstrOpCode_TestDigit:
            case RegexInstrOpCode_TestLowerCase:
            case RegexInstrOpCode_TestUpperCase:
            case RegexInstrOpCode_IncPos:
            case RegexInstrOpCode_DecPos:
            case RegexInstrOpCode_IncPosEq:
                i++;
                break;
                
            case RegexInstrOpCode_JumpPos:
                i += 5;
                break;
                
            case RegexInstrOpCode_TestSingle:
            case RegexInstrOpCode_SetFlag:
            case RegexInstrOpCode_GroupStart:
            case RegexInstrOpCode_GroupEnd:
                i += 2;
                break;
                
            case RegexInstrOpCode_TestRange:
            case RegexInstrOpCode_TestNegatedRange:
                i += 3;
                break;
                
            default:
                i++;
                break;
        }
    }
    
    return { true, std::move(bytecode) };
}

std::tuple<bool, Regex::ByteCode> regex_opt(Regex::ByteCode& bytecode) noexcept
{
    std::size_t i = 0;

    while(i < bytecode.size())
    {

    }

    return std::make_tuple(true, bytecode);
}

void regex_disasm(const Regex::ByteCode& bytecode) noexcept
{
    std::size_t i = 0;

    while(i < bytecode.size())
    {
        switch(static_cast<std::uint8_t>(bytecode[i]))
        {
            case RegexInstrOpCode_TestSingle:
                log_debug("TESTSINGLE {}",
                          CHAR(bytecode[i + 1]));
                i += 2;
                break;
            case RegexInstrOpCode_TestRange:
                log_debug("TESTRANGE {}-{}",
                          CHAR(bytecode[i + 1]),
                          CHAR(bytecode[i + 2]));
                i += 3;
                break;
            case RegexInstrOpCode_TestNegatedRange:
                log_debug("TESTNEGRANGE {}-{}",
                          CHAR(bytecode[i + 1]),
                          CHAR(bytecode[i + 2]));
                i += 3;
                break;
            case RegexInstrOpCode_TestAny:
                log_debug("TESTANY");
                i++;
                break;
            case RegexInstrOpCode_TestDigit:
                log_debug("TESTDIGIT");
                i++;
                break;
            case RegexInstrOpCode_TestLowerCase:
                log_debug("TESTLOWERCASE");
                i++;
                break;
            case RegexInstrOpCode_TestUpperCase:
                log_debug("TESTUPPERCASE");
                i++;
                break;
            case RegexInstrOpCode_JumpEq:
            case RegexInstrOpCode_JumpNeq:
                    log_debug("{} {}{}",
                              static_cast<std::uint8_t>(bytecode[i]) == RegexInstrOpCode_JumpEq ? "JUMPEQ" : "JUMPNEQ",
                              decode_jump(std::addressof(bytecode[i + 1])) >= 0 ? "+" : "",
                              decode_jump(std::addressof(bytecode[i + 1])));
                i += 5;
                break;
            case RegexInstrOpCode_Accept:
                log_debug("ACCEPT");
                i++;
                break;
            case RegexInstrOpCode_Fail:
                log_debug("FAIL");
                i++;
                break;
            case RegexInstrOpCode_GroupStart:
                log_debug("GROUPSTART {}", static_cast<std::uint8_t>(bytecode[i + 1]));
                i += 2;
                break;
            case RegexInstrOpCode_GroupEnd:
                log_debug("GROUPEND {}", static_cast<std::uint8_t>(bytecode[i + 1]));
                i += 2;
                break;
            case RegexInstrOpCode_IncPos:
                log_debug("INCPOS");
                i++;
                break;
            case RegexInstrOpCode_DecPos:
                log_debug("DECPOS");
                i++;
                break;
            case RegexInstrOpCode_IncPosEq:
                log_debug("INCPOSEQ");
                i++;
                break;
            case RegexInstrOpCode_JumpPos:
                log_debug("JUMPPOS {}{}",
                          decode_jump(std::addressof(bytecode[i + 1])) >= 0 ? "+" : "",
                          decode_jump(std::addressof(bytecode[i + 1])));
                i += 5;
                break;
            case RegexInstrOpCode_SetFlag:
                log_debug("SETFLAG {}",
                          static_cast<std::uint8_t>(bytecode[i + 1]));
                i += 2;
                break;
            default:
                log_debug("UNKNOWN {}", static_cast<std::uint8_t>(bytecode[i]));
                i++;
                break;
        }
    }
}

struct RegexVM
{
    const Regex::ByteCode& bytecode;

    StringD string;
    std::size_t sp; /* string position */
    std::size_t pc; /* program counter */

    bool status_flag;

    RegexVM(const Regex::ByteCode& bytecode, const StringD& str) : bytecode(bytecode), 
                                                                   string(str),
                                                                   sp(0),
                                                                   pc(0),
                                                                   status_flag(false) {}

    RegexVM(const RegexVM&) = delete;
    RegexVM& operator=(const RegexVM&) = delete;

    RegexVM(RegexVM&&) = delete;
    RegexVM& operator=(RegexVM&&) = delete;
};

bool regex_exec(RegexVM* vm) noexcept
{
    while(vm->pc < vm->bytecode.size() && vm->sp < vm->string.size())
    {
        switch(static_cast<std::uint8_t>(vm->bytecode[vm->pc]))
        {
            case RegexInstrOpCode_TestSingle:
                vm->status_flag = vm->string[vm->sp] == CHAR(vm->bytecode[vm->pc + 1]);
                vm->pc += 2;
                break;
            case RegexInstrOpCode_TestRange:
                vm->status_flag = vm->string[vm->sp] >= CHAR(vm->bytecode[vm->pc + 1]) &&
                                  vm->string[vm->sp] <= CHAR(vm->bytecode[vm->pc + 2]);
                vm->pc += 3;
                break;
            case RegexInstrOpCode_TestNegatedRange:
                vm->status_flag = vm->string[vm->sp] < CHAR(vm->bytecode[vm->pc + 1]) &&
                                  vm->string[vm->sp] > CHAR(vm->bytecode[vm->pc + 2]);
                vm->pc += 3;
                break;
            case RegexInstrOpCode_TestAny:
                vm->status_flag = true;
                vm->pc += 1;
                break;
            case RegexInstrOpCode_TestDigit:
                vm->status_flag = std::isdigit(static_cast<int>(vm->string[vm->sp]));
                vm->pc += 1;
                break;
            case RegexInstrOpCode_TestLowerCase:
                vm->status_flag = std::islower(static_cast<int>(vm->string[vm->sp]));
                vm->pc += 1;
                break;
            case RegexInstrOpCode_TestUpperCase:
                vm->status_flag = std::isupper(static_cast<int>(vm->string[vm->sp]));
                vm->pc += 1;
                break;
            case RegexInstrOpCode_JumpEq:
                if(vm->status_flag)
                {
                    const int jmp = decode_jump(std::addressof(vm->bytecode[vm->pc + 1]));

                    if(jmp > 0)
                    {
                        vm->pc += 5;
                    }
                    
                    vm->pc += jmp;
                }
                else
                {
                    vm->pc += 5;
                }

                break;
            case RegexInstrOpCode_JumpNeq:
                if(!vm->status_flag)
                {
                    const int jmp = decode_jump(std::addressof(vm->bytecode[vm->pc + 1]));

                    if(jmp > 0)
                    {
                        vm->pc += 5;
                    }
                    
                    vm->pc += jmp;
                }
                else
                {
                    vm->pc += 5;
                }

                break;
            case RegexInstrOpCode_Accept:
                return true;
            case RegexInstrOpCode_Fail:
                return false;
            case RegexInstrOpCode_GroupStart:
                vm->pc += 2;
                break;
            case RegexInstrOpCode_GroupEnd:
                vm->pc += 2;
                break;
            case RegexInstrOpCode_IncPos:
                vm->sp++;
                vm->pc++;
                break;
            case RegexInstrOpCode_DecPos:
                vm->sp--;
                vm->pc++;
                break;
            case RegexInstrOpCode_IncPosEq:
                vm->sp += static_cast<std::size_t>(vm->status_flag);
                vm->pc++;
                break;
            case RegexInstrOpCode_JumpPos:
                vm->sp += decode_jump(std::addressof(vm->bytecode[vm->pc + 1]));
                vm->pc += 5;
                break;
            case RegexInstrOpCode_SetFlag:
                vm->status_flag = static_cast<bool>(vm->bytecode[vm->pc + 1]);
                vm->pc += 2;
                break;
            default:
                log_error("Error: unknown instruction in regex vm: {} (pc {})",
                          static_cast<std::uint8_t>(vm->bytecode[vm->pc]),
                          vm->pc);
                return false;
        }
    }

    return static_cast<bool>(vm->status_flag);
}

/********************************/
/* Regex */
/********************************/

bool Regex::compile(const StringD& regex, std::uint32_t flags) noexcept
{
    this->_bytecode.clear();

    if(flags & RegexFlags_DebugCompilation)
    {
        log_debug("Compiling Regex: {}", regex);
    }

    auto [lex_success, tokens] = lex_regex(regex);

    if(!lex_success)
    {
        log_error("Failed to lex regex: {}", regex);
        return false;
    }

    if(flags & RegexFlags_DebugCompilation)
    {
        log_debug("**********");
        log_debug("Regex tokens (lex):");
        print_tokens(tokens);
    }

    auto [emit_success, bytecode] = regex_emit(tokens);

    if(!emit_success)
    {
        log_error("Failed to emit bytecode for regex: {}", regex);
        return false;
    }

    this->_bytecode = std::move(bytecode);

    if(flags & RegexFlags_DebugCompilation)
    {
        log_debug("**********");
        log_debug("Regex disasm:");
        regex_disasm(this->_bytecode);
    }

    return true;
}

bool Regex::match(const StringD& str) const noexcept
{
    if(this->_bytecode.empty())
    {
        return false;
    }

    RegexVM vm(this->_bytecode, str);

    return regex_exec(&vm);
}

STDROMANO_NAMESPACE_END