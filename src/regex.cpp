// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - Present Romain Augier
// All rights reserved.

#include "stdromano/regex.hpp"
#include "stdromano/logger.hpp"

#include <cstring>
#include <limits>

STDROMANO_NAMESPACE_BEGIN

/*
    https://dl.acm.org/doi/pdf/10.1145/363347.363387
    https://arxiv.org/pdf/2407.20479

    Single-pass recursive descent compiler: regex string -> bytecode directly.
    Linear VM with group capture support.
*/

enum RegexInstrOpCode : std::uint8_t
{
    RegexInstrOpCode_TestSingle,        /* test char == operand */
    RegexInstrOpCode_TestNegatedSingle, /* test char != operand */
    RegexInstrOpCode_TestRange,         /* test char in [lo, hi] */
    RegexInstrOpCode_TestNegatedRange,  /* test char NOT in [lo, hi] */
    RegexInstrOpCode_TestAny,           /* test any char (except newline) */
    RegexInstrOpCode_TestDigit,         /* test isdigit */
    RegexInstrOpCode_TestWord,          /* test isalnum || '_' */
    RegexInstrOpCode_TestWhitespace,    /* test isspace */
    RegexInstrOpCode_TestLowerCase,     /* test islower */
    RegexInstrOpCode_TestUpperCase,     /* test isupper */
    RegexInstrOpCode_Jump,              /* jump */
    RegexInstrOpCode_JumpEq,            /* jump if status_flag == true */
    RegexInstrOpCode_JumpNeq,           /* jump if status_flag == false */
    RegexInstrOpCode_Accept,            /* match succeeded */
    RegexInstrOpCode_Fail,              /* match failed */
    RegexInstrOpCode_GroupStart,        /* record sp as group start  (1-byte id) */
    RegexInstrOpCode_GroupEnd,          /* record sp as group end    (1-byte id) */
    RegexInstrOpCode_IncPos,            /* ++sp */
    RegexInstrOpCode_IncPosEq,          /* sp += status_flag ? 1 : 0 */
    RegexInstrOpCode_SetFlag,           /* status_flag = (bool)operand */
};

template<typename T, typename = std::enable_if_t<sizeof(T) == 1>>
inline std::byte BYTE(const T& t) noexcept
{
    std::byte b;
    std::memcpy(&b, &t, 1);
    return b;
}

template<typename T, typename = std::enable_if_t<sizeof(T) == 1>>
inline std::byte BYTE(T&& t) noexcept
{
    std::byte b;
    const T t2 = t;
    std::memcpy(&b, &t2, 1);
    return b;
}

inline char CHAR(std::byte b) noexcept
{
    char c;
    std::memcpy(&c, &b, 1);
    return c;
}

// All jumps are relative to the end of the jump instruction (size of a jump instruction is 1 (opcode) + 4 (offset))

static constexpr int JUMP_FAIL = std::numeric_limits<int>::max();

inline void encode_jump_at(Regex::ByteCode& code, std::size_t pos, int value) noexcept
{
    code[pos + 0] = BYTE(static_cast<std::uint8_t>((value >> 24) & 0xFF));
    code[pos + 1] = BYTE(static_cast<std::uint8_t>((value >> 16) & 0xFF));
    code[pos + 2] = BYTE(static_cast<std::uint8_t>((value >> 8) & 0xFF));
    code[pos + 3] = BYTE(static_cast<std::uint8_t>((value >> 0) & 0xFF));
}

inline int decode_jump(const std::byte* p) noexcept
{
    return static_cast<int>(static_cast<std::uint8_t>(p[0])) << 24 |
           static_cast<int>(static_cast<std::uint8_t>(p[1])) << 16 |
           static_cast<int>(static_cast<std::uint8_t>(p[2])) << 8  |
           static_cast<int>(static_cast<std::uint8_t>(p[3])) << 0;
}

/* Emit a jump instruction, returns the position of the 4-byte offset field */
inline std::size_t emit_jump(Regex::ByteCode& code, RegexInstrOpCode op) noexcept
{
    code.push_back(BYTE(op));
    std::size_t pos = code.size();
    code.push_back(BYTE(std::uint8_t(0)));
    code.push_back(BYTE(std::uint8_t(0)));
    code.push_back(BYTE(std::uint8_t(0)));
    code.push_back(BYTE(std::uint8_t(0)));
    return pos;
}

inline std::size_t emit_fail_jump(Regex::ByteCode& code, RegexInstrOpCode op) noexcept
{
    code.push_back(BYTE(op));
    std::size_t pos = code.size();
    code.push_back(BYTE(static_cast<std::uint8_t>((JUMP_FAIL >> 24) & 0xFF)));
    code.push_back(BYTE(static_cast<std::uint8_t>((JUMP_FAIL >> 16) & 0xFF)));
    code.push_back(BYTE(static_cast<std::uint8_t>((JUMP_FAIL >> 8) & 0xFF)));
    code.push_back(BYTE(static_cast<std::uint8_t>((JUMP_FAIL >> 0) & 0xFF)));
    return pos;
}

inline void patch_jump(Regex::ByteCode& code, std::size_t jump_pos, int offset) noexcept
{
    encode_jump_at(code, jump_pos, offset);
}

struct RegexCompiler
{
    const char* pattern;
    std::size_t len;
    std::size_t pos;
    std::uint32_t next_group_id; /* 0 = implicit whole-match group, 1+ = explicit */
    Regex::ByteCode bytecode;
    bool has_error;

    RegexCompiler(const char* pat, std::size_t length) : pattern(pat),
                                                         len(length),
                                                         pos(0),
                                                         next_group_id(1),
                                                         has_error(false) {}

    char peek() const noexcept { return this->pos < this->len ? this->pattern[this->pos] : '\0'; }
    char advance() noexcept { return this->pos < this->len ? this->pattern[this->pos++] : '\0'; }
    bool at_end() const noexcept { return this->pos >= this->len; }

    void error(const char* msg) noexcept
    {
        log_error("Regex compile error at position {}: {}", this->pos, msg);
        this->has_error = true;
    }

    void emit_range_instrs(char lo, char hi, bool add_inc_pos_eq = true, bool negated = false) noexcept
    {
        if(negated)
        {
            this->bytecode.push_back(BYTE(RegexInstrOpCode_TestNegatedRange));
            this->bytecode.push_back(BYTE(lo));
            this->bytecode.push_back(BYTE(hi));
        }
        else
        {
            if(lo == '0' && hi == '9')
            {
                this->bytecode.push_back(BYTE(RegexInstrOpCode_TestDigit));
            }
            else if(lo == 'a' && hi == 'z')
            {
                this->bytecode.push_back(BYTE(RegexInstrOpCode_TestLowerCase));
            }
            else if(lo == 'A' && hi == 'Z')
            {
                this->bytecode.push_back(BYTE(RegexInstrOpCode_TestUpperCase));
            }
            else
            {
                this->bytecode.push_back(BYTE(RegexInstrOpCode_TestRange));
                this->bytecode.push_back(BYTE(lo));
                this->bytecode.push_back(BYTE(hi));
            }
        }

        if(add_inc_pos_eq)
            this->bytecode.push_back(BYTE(RegexInstrOpCode_IncPosEq));
    }

    /* alternation = concatenation ('|' concatenation)* */
    bool compile_alternation() noexcept
    {
        if(!this->compile_concatenation())
            return false;

        Vector<std::size_t> jumps;

        while(!this->at_end() && this->peek() == '|')
        {
            this->advance();

            std::size_t jump_over_pos = emit_jump(this->bytecode, RegexInstrOpCode_JumpEq);
            jumps.push_back(jump_over_pos);

            if(!this->compile_concatenation())
                return false;

        }

        std::size_t end_label = this->bytecode.size();

        for(const auto jump : jumps)
            patch_jump(this->bytecode, jump, static_cast<int>(end_label - (jump + 4)));

        return true;
    }

    /* concatenation = quantified+ (stop at '|', ')', or end) */
    bool compile_concatenation() noexcept
    {
        bool first = true;

        Vector<std::size_t> jumps;

        while(!this->at_end() && this->peek() != ')' && this->peek() != '|')
        {
            if(!first)
            {
                std::size_t fail_jump = emit_jump(this->bytecode, RegexInstrOpCode_JumpNeq);
                jumps.push_back(fail_jump);
            }

            if(!this->compile_quantified())
                return false;

            first = false;
        }

        std::size_t end_label = this->bytecode.size();

        for(const auto jump : jumps)
            patch_jump(this->bytecode, jump, static_cast<int>(end_label - (jump + 4)));

        return true;
    }

    /* quantified = primary ('*' | '+' | '?')? */
    bool compile_quantified() noexcept
    {
        std::size_t primary_start = this->bytecode.size();

        if(!this->compile_primary())
            return false;

        if(this->at_end())
            return true;

        char q = this->peek();

        switch(q)
        {
            case '*':
            {
                this->advance();

                std::size_t loop_jump = emit_jump(this->bytecode, RegexInstrOpCode_JumpEq);
                int offset_back = static_cast<int>(primary_start) - static_cast<int>(loop_jump + 4);
                patch_jump(this->bytecode, loop_jump, offset_back);

                this->bytecode.push_back(BYTE(RegexInstrOpCode_SetFlag));
                this->bytecode.push_back(BYTE(std::uint8_t(1)));

                break;
            }
            case '+':
            {
                this->advance();

                std::size_t primary_end = this->bytecode.size();

                emit_fail_jump(this->bytecode, RegexInstrOpCode_JumpNeq);

                std::size_t loop_start = this->bytecode.size();

                Regex::ByteCode primary_copy(this->bytecode.begin() + static_cast<Regex::ByteCode::difference_type>(primary_start),
                                             this->bytecode.begin() + static_cast<Regex::ByteCode::difference_type>(primary_end));

                this->bytecode.insert(this->bytecode.end(), primary_copy.begin(), primary_copy.end());

                std::size_t exit_loop_jump = emit_jump(this->bytecode, RegexInstrOpCode_JumpNeq);

                std::size_t loop_jump = emit_jump(this->bytecode, RegexInstrOpCode_JumpEq);
                int offset_back = static_cast<int>(loop_start) - static_cast<int>(loop_jump + 4);
                patch_jump(this->bytecode, loop_jump, offset_back);

                patch_jump(this->bytecode,
                           exit_loop_jump,
                           static_cast<int>(this->bytecode.size() - (exit_loop_jump + 4)));

                this->bytecode.push_back(BYTE(RegexInstrOpCode_SetFlag));
                this->bytecode.push_back(BYTE(std::uint8_t(1)));

                break;
            }
            case '?':
            {
                this->advance();

                this->bytecode.push_back(BYTE(RegexInstrOpCode_SetFlag));
                this->bytecode.push_back(BYTE(std::uint8_t(1)));

                break;
            }
            default:
                break;
        }

        return true;
    }

    /* primary = literal | '.' | '[' class ']' | '(' alternation ')' | '\\' escape */
    bool compile_primary() noexcept
    {
        if(this->at_end())
        {
            this->error("Unexpected end of pattern");
            return false;
        }

        char c = this->peek();

        switch(c)
        {
            case '(':
            {
                this->advance();

                std::uint32_t group_id = this->next_group_id++;

                this->bytecode.push_back(BYTE(RegexInstrOpCode_GroupStart));
                this->bytecode.push_back(BYTE(static_cast<std::uint8_t>(group_id & 0xFF)));

                if(!this->compile_alternation())
                    return false;

                if(this->at_end() || this->peek() != ')')
                {
                    this->error("Mismatched parentheses: expected ')'");
                    return false;
                }

                this->advance();

                this->bytecode.push_back(BYTE(RegexInstrOpCode_GroupEnd));
                this->bytecode.push_back(BYTE(static_cast<std::uint8_t>(group_id & 0xFF)));

                return true;
            }

            case '[':
            {
                this->advance();

                bool negated = false;

                if(!this->at_end() && this->peek() == '^')
                {
                    negated = true;
                    this->advance();
                }

                if(this->at_end())
                {
                    this->error("Unclosed character class");
                    return false;
                }

                /* For a multi-range class like [a-zA-Z0-9], we compile each range
                   as a test and use alternation logic between them:
                   test_range1 -> if match, jump to end -> test_range2 -> ... */

                struct RangePair { char lo; char hi; inline bool is_range() const { return this->lo != this->hi; } };
                Vector<RangePair> ranges;

                while(!this->at_end() && this->peek() != ']')
                {
                    char lo = this->advance();

                    if(!this->at_end() && this->peek() == '-')
                    {
                        this->advance();

                        if(this->at_end() || this->peek() == ']')
                        {
                            /* Trailing '-', treat as literal */
                            ranges.push_back({ lo, lo });
                            ranges.push_back({ '-', '-' });
                        }
                        else
                        {
                            char hi = this->advance();
                            ranges.push_back({ lo, hi });
                        }
                    }
                    else
                    {
                        ranges.push_back({ lo, lo });
                    }
                }

                if(this->at_end())
                {
                    this->error("Unclosed character class: expected ']'");
                    return false;
                }

                this->advance();

                if(ranges.size() == 1 && ranges[0].is_range() && !negated)
                {
                    this->emit_range_instrs(ranges[0].lo, ranges[0].hi);
                }
                else if(ranges.size() == 1 && !ranges[0].is_range() && !negated)
                {
                    this->bytecode.push_back(BYTE(RegexInstrOpCode_TestSingle));
                    this->bytecode.push_back(BYTE(ranges[0].lo));
                    this->bytecode.push_back(BYTE(RegexInstrOpCode_IncPosEq));
                }
                else
                {
                    Vector<std::size_t> success_jumps;

                    for(std::size_t r = 0; r < ranges.size(); ++r)
                    {
                        if(ranges[r].is_range())
                        {
                            this->emit_range_instrs(ranges[r].lo, ranges[r].hi, false, negated);
                        }
                        else
                        {
                            if(negated)
                                this->bytecode.push_back(BYTE(RegexInstrOpCode_TestNegatedSingle));
                            else
                                this->bytecode.push_back(BYTE(RegexInstrOpCode_TestSingle));

                            this->bytecode.push_back(BYTE(ranges[r].lo));
                        }

                        std::size_t jump_pos = emit_jump(this->bytecode, RegexInstrOpCode_JumpEq);
                        success_jumps.push_back(jump_pos);
                    }

                    this->bytecode.push_back(BYTE(RegexInstrOpCode_SetFlag));
                    this->bytecode.push_back(BYTE(static_cast<std::uint8_t>(0)));

                    std::size_t success_label = this->bytecode.size();

                    for(std::size_t j = 0; j < success_jumps.size(); ++j)
                        patch_jump(this->bytecode, success_jumps[j], static_cast<int>(success_label - (success_jumps[j] + 4)));

                    this->bytecode.push_back(BYTE(RegexInstrOpCode_IncPosEq));
                }

                return true;
            }

            case '.':
            {
                this->advance();
                this->bytecode.push_back(BYTE(RegexInstrOpCode_TestAny));
                this->bytecode.push_back(BYTE(RegexInstrOpCode_IncPosEq));
                return true;
            }

            case '\\':
            {
                this->advance();

                if(this->at_end())
                {
                    this->error("Trailing backslash in pattern");
                    return false;
                }

                char esc = this->advance();

                switch(esc)
                {
                    case 'd':
                        this->bytecode.push_back(BYTE(RegexInstrOpCode_TestDigit));
                        this->bytecode.push_back(BYTE(RegexInstrOpCode_IncPosEq));
                        break;
                    case 'w':
                        this->bytecode.push_back(BYTE(RegexInstrOpCode_TestWord));
                        this->bytecode.push_back(BYTE(RegexInstrOpCode_IncPosEq));
                        break;
                    case 's':
                        this->bytecode.push_back(BYTE(RegexInstrOpCode_TestWhitespace));
                        this->bytecode.push_back(BYTE(RegexInstrOpCode_IncPosEq));
                        break;
                    default:
                        /* Escaped literal (handles \\, \., \*, \+, \?, \(, \), \[, \], \|) */
                        this->bytecode.push_back(BYTE(RegexInstrOpCode_TestSingle));
                        this->bytecode.push_back(BYTE(esc));
                        this->bytecode.push_back(BYTE(RegexInstrOpCode_IncPosEq));
                        break;
                }

                return true;
            }

            default:
            {
                /* Literal character (alphanumeric, underscore, or other non-special chars) */
                if(c == '*' || c == '+' || c == '?' || c == ')' || c == ']' || c == '|')
                {
                    this->error("Unexpected operator or delimiter");
                    return false;
                }

                this->advance();

                this->bytecode.push_back(BYTE(RegexInstrOpCode_TestSingle));
                this->bytecode.push_back(BYTE(c));
                this->bytecode.push_back(BYTE(RegexInstrOpCode_IncPosEq));

                return true;
            }
        }
    }
};

void finalize_bytecode(Regex::ByteCode& bytecode) noexcept
{
    // Append the final fail-check, accept, and fail instructions
    emit_fail_jump(bytecode, RegexInstrOpCode_JumpNeq);

    bytecode.push_back(BYTE(RegexInstrOpCode_Accept));
    bytecode.push_back(BYTE(RegexInstrOpCode_Fail));

    // Resolve all JUMP_FAIL offsets to point to the Fail instruction
    std::size_t fail_addr = bytecode.size() - 1;
    std::size_t i = 0;

    while(i < bytecode.size())
    {
        switch(static_cast<std::uint8_t>(bytecode[i]))
        {
            case RegexInstrOpCode_Jump:
            case RegexInstrOpCode_JumpEq:
            case RegexInstrOpCode_JumpNeq:
            {
                int offset = decode_jump(&bytecode[i + 1]);

                if(offset == JUMP_FAIL)
                {
                    encode_jump_at(bytecode, i + 1, static_cast<int>(fail_addr - (i + 5)));
                }

                i += 5;
                break;
            }

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
}

/* ======================================================================== */
/* Disassembler (debug)                                                     */
/* ======================================================================== */

static void regex_disasm(const Regex::ByteCode& bytecode) noexcept
{
    std::size_t i = 0;

    while(i < bytecode.size())
    {
        log_debug("{:04d}: ", i);

        switch(static_cast<std::uint8_t>(bytecode[i]))
        {
            case RegexInstrOpCode_TestSingle:
                log_debug("  TESTSINGLE '{}'", CHAR(bytecode[i + 1]));
                i += 2;
                break;
            case RegexInstrOpCode_TestNegatedSingle:
                log_debug("  TESTNEGSINGLE '{}'", CHAR(bytecode[i + 1]));
                i += 2;
                break;
            case RegexInstrOpCode_TestRange:
                log_debug("  TESTRANGE '{}'-'{}'", CHAR(bytecode[i + 1]), CHAR(bytecode[i + 2]));
                i += 3;
                break;
            case RegexInstrOpCode_TestNegatedRange:
                log_debug("  TESTNEGRANGE '{}'-'{}'", CHAR(bytecode[i + 1]), CHAR(bytecode[i + 2]));
                i += 3;
                break;
            case RegexInstrOpCode_TestAny:
                log_debug("  TESTANY");
                i++;
                break;
            case RegexInstrOpCode_TestDigit:
                log_debug("  TESTDIGIT");
                i++;
                break;
            case RegexInstrOpCode_TestWord:
                log_debug("  TESTWORD");
                i++;
                break;
            case RegexInstrOpCode_TestWhitespace:
                log_debug("  TESTWHITESPACE");
                i++;
                break;
            case RegexInstrOpCode_TestLowerCase:
                log_debug("  TESTLOWERCASE");
                i++;
                break;
            case RegexInstrOpCode_TestUpperCase:
                log_debug("  TESTUPPERCASE");
                i++;
                break;
            case RegexInstrOpCode_Jump:
            case RegexInstrOpCode_JumpEq:
            case RegexInstrOpCode_JumpNeq:
            {
                int offset = decode_jump(&bytecode[i + 1]);
                int target = static_cast<int>(i) + 5 + offset;

                switch(static_cast<std::uint8_t>(bytecode[i]))
                {
                    case RegexInstrOpCode_Jump:
                        log_debug("  JUMP {}{} -> {:04d}", offset > 0 ? "+" : "", offset, target);
                        break;
                    case RegexInstrOpCode_JumpEq:
                        log_debug("  JUMPEQ {}{} -> {:04d}", offset > 0 ? "+" : "", offset, target);
                        break;
                    case RegexInstrOpCode_JumpNeq:
                        log_debug("  JUMPNEQ {}{} -> {:04d}", offset > 0 ? "+" : "", offset, target);
                        break;
                    default:
                        break;
                }

                i += 5;

                break;
            }
            case RegexInstrOpCode_Accept:
                log_debug("  ACCEPT");
                i++;
                break;
            case RegexInstrOpCode_Fail:
                log_debug("  FAIL");
                i++;
                break;
            case RegexInstrOpCode_GroupStart:
                log_debug("  GROUPSTART {}", static_cast<std::uint8_t>(bytecode[i + 1]));
                i += 2;
                break;
            case RegexInstrOpCode_GroupEnd:
                log_debug("  GROUPEND {}", static_cast<std::uint8_t>(bytecode[i + 1]));
                i += 2;
                break;
            case RegexInstrOpCode_IncPos:
                log_debug("  INCPOS");
                i++;
                break;
            case RegexInstrOpCode_IncPosEq:
                log_debug("  INCPOSEQ");
                i++;
                break;
            case RegexInstrOpCode_SetFlag:
                log_debug("  SETFLAG {}", static_cast<std::uint8_t>(bytecode[i + 1]));
                i += 2;
                break;
            default:
                log_debug("  UNKNOWN 0x{:02x}", static_cast<std::uint8_t>(bytecode[i]));
                i++;
                break;
        }
    }
}

/* ======================================================================== */
/* Virtual machine                                                          */
/* ======================================================================== */

struct RegexVM
{
    const Regex::ByteCode& bytecode;
    const char* str;
    std::size_t str_len;
    std::size_t sp;         /* string position (relative to str) */
    std::size_t pc;         /* program counter */
    bool status_flag;

    RegexGroup groups[REGEX_MAX_GROUPS];
    std::uint32_t group_count;

    RegexVM(const Regex::ByteCode& bc, const char* s, std::size_t slen, std::uint32_t gc)
        : bytecode(bc), str(s), str_len(slen), sp(0), pc(0),
          status_flag(false), group_count(gc)
    {
        for(std::uint32_t i = 0; i < REGEX_MAX_GROUPS; ++i)
        {
            groups[i] = RegexGroup();
        }
    }
};

bool regex_exec(RegexVM* vm) noexcept
{
    while(vm->pc < vm->bytecode.size())
    {
        std::uint8_t opcode = static_cast<std::uint8_t>(vm->bytecode[vm->pc]);

        switch(opcode)
        {
            case RegexInstrOpCode_TestSingle:
            {
                if(vm->sp >= vm->str_len)
                {
                    vm->status_flag = false;
                    vm->pc += 2;
                }
                else
                {
                    vm->status_flag = vm->str[vm->sp] == CHAR(vm->bytecode[vm->pc + 1]);
                    vm->pc += 2;
                }

                break;
            }
            case RegexInstrOpCode_TestNegatedSingle:
            {
                if(vm->sp >= vm->str_len)
                {
                    vm->status_flag = false;
                    vm->pc += 2;
                }
                else
                {
                    vm->status_flag = vm->str[vm->sp] != CHAR(vm->bytecode[vm->pc + 1]);
                    vm->pc += 2;
                }

                break;
            }
            case RegexInstrOpCode_TestRange:
            {
                if(vm->sp >= vm->str_len)
                {
                    vm->status_flag = false;
                    vm->pc += 3;
                }
                else
                {
                    vm->status_flag = vm->str[vm->sp] >= CHAR(vm->bytecode[vm->pc + 1]) &&
                                      vm->str[vm->sp] <= CHAR(vm->bytecode[vm->pc + 2]);
                    vm->pc += 3;
                }

                break;
            }
            case RegexInstrOpCode_TestNegatedRange:
            {
                if(vm->sp >= vm->str_len)
                {
                    vm->status_flag = false;
                    vm->pc += 3;
                }
                else
                {
                    vm->status_flag = vm->str[vm->sp] < CHAR(vm->bytecode[vm->pc + 1]) ||
                                      vm->str[vm->sp] > CHAR(vm->bytecode[vm->pc + 2]);
                    vm->pc += 3;
                }

                break;
            }
            case RegexInstrOpCode_TestAny:
            {
                if(vm->sp >= vm->str_len)
                {
                    vm->status_flag = false;
                }
                else
                {
                    vm->status_flag = (vm->str[vm->sp] != '\n');
                }

                vm->pc += 1;

                break;
            }
            case RegexInstrOpCode_TestDigit:
            {
                if(vm->sp >= vm->str_len)
                    vm->status_flag = false;
                else
                    vm->status_flag = std::isdigit(static_cast<unsigned char>(vm->str[vm->sp]));

                vm->pc += 1;

                break;
            }
            case RegexInstrOpCode_TestWord:
            {
                if(vm->sp >= vm->str_len)
                    vm->status_flag = false;
                else
                {
                    char c = vm->str[vm->sp];
                    vm->status_flag = std::isalnum(static_cast<unsigned char>(c)) || c == '_';
                }

                vm->pc += 1;

                break;
            }
            case RegexInstrOpCode_TestWhitespace:
            {
                if(vm->sp >= vm->str_len)
                    vm->status_flag = false;
                else
                    vm->status_flag = std::isspace(static_cast<unsigned char>(vm->str[vm->sp]));

                vm->pc += 1;

                break;
            }
            case RegexInstrOpCode_TestLowerCase:
            {
                if(vm->sp >= vm->str_len)
                    vm->status_flag = false;
                else
                    vm->status_flag = std::islower(static_cast<unsigned char>(vm->str[vm->sp]));

                vm->pc += 1;

                break;
            }
            case RegexInstrOpCode_TestUpperCase:
            {
                if(vm->sp >= vm->str_len)
                    vm->status_flag = false;
                else
                    vm->status_flag = std::isupper(static_cast<unsigned char>(vm->str[vm->sp]));

                vm->pc += 1;

                break;
            }
            case RegexInstrOpCode_Jump:
            {
                int jmp = decode_jump(&vm->bytecode[vm->pc + 1]);
                vm->pc = static_cast<std::size_t>(static_cast<int>(vm->pc) + 5 + jmp);

                break;
            }
            case RegexInstrOpCode_JumpEq:
            {
                if(vm->status_flag)
                {
                    int jmp = decode_jump(&vm->bytecode[vm->pc + 1]);
                    vm->pc = static_cast<std::size_t>(static_cast<int>(vm->pc) + 5 + jmp);
                }
                else
                {
                    vm->pc += 5;
                }

                break;
            }
            case RegexInstrOpCode_JumpNeq:
            {
                if(!vm->status_flag)
                {
                    int jmp = decode_jump(&vm->bytecode[vm->pc + 1]);
                    vm->pc = static_cast<std::size_t>(static_cast<int>(vm->pc) + 5 + jmp);
                }
                else
                {
                    vm->pc += 5;
                }

                break;
            }
            case RegexInstrOpCode_Accept:
                return true;
            case RegexInstrOpCode_Fail:
                return false;
            case RegexInstrOpCode_GroupStart:
            {
                std::uint8_t gid = static_cast<std::uint8_t>(vm->bytecode[vm->pc + 1]);

                if(gid < REGEX_MAX_GROUPS)
                {
                    vm->groups[gid].start = vm->sp;
                }

                vm->pc += 2;
                break;
            }
            case RegexInstrOpCode_GroupEnd:
            {
                std::uint8_t gid = static_cast<std::uint8_t>(vm->bytecode[vm->pc + 1]);

                if(gid < REGEX_MAX_GROUPS)
                {
                    vm->groups[gid].end = vm->sp;
                }

                vm->pc += 2;
                break;
            }
            case RegexInstrOpCode_IncPos:
            {
                vm->sp++;
                vm->pc++;
                break;
            }
            case RegexInstrOpCode_IncPosEq:
            {
                vm->sp += static_cast<std::size_t>(vm->status_flag);
                vm->pc++;
                break;
            }
            case RegexInstrOpCode_SetFlag:
            {
                vm->status_flag = static_cast<bool>(vm->bytecode[vm->pc + 1]);
                vm->pc += 2;
                break;
            }
            default:
            {
                log_error("Unknown instruction in regex VM: 0x{:02x} (pc {})", opcode, vm->pc);
                return false;
            }
        }
    }

    return vm->status_flag;
}

/* ======================================================================== */
/* Regex public API                                                         */
/* ======================================================================== */

bool Regex::compile(const StringD& regex, std::uint32_t flags) noexcept
{
    this->_bytecode.clear();
    this->_group_count = 0;

    if(flags & RegexFlags_DebugCompilation)
    {
        log_debug("Compiling regex: {}", regex);
    }

    RegexCompiler compiler(regex.data(), regex.size());

    if(!compiler.compile_alternation())
    {
        log_error("Failed to compile regex: {}", regex);
        return false;
    }

    if(!compiler.at_end())
    {
        log_error("Unexpected trailing characters in regex at position {}", compiler.pos);
        return false;
    }

    this->_bytecode = std::move(compiler.bytecode);

    finalize_bytecode(this->_bytecode);

    this->_group_count = compiler.next_group_id;

    if(flags & RegexFlags_DebugCompilation)
    {
        log_debug("Regex disasm ({} groups)", this->_group_count);
        regex_disasm(this->_bytecode);
    }

    return true;
}

bool Regex::exec_at(const StringD& str, std::size_t start_pos,
                    RegexGroup* groups) const noexcept
{
    RegexVM vm(this->_bytecode,
               str.data() + start_pos,
               str.size() - start_pos,
               this->_group_count);

    bool result = regex_exec(&vm);

    if(result && groups != nullptr)
    {
        groups[0] = RegexGroup(start_pos, std::min(start_pos + vm.sp, str.size()));

        for(std::uint32_t g = 1; g < this->_group_count && g < REGEX_MAX_GROUPS; ++g)
        {
            if(vm.groups[g].matched())
            {
                groups[g] = RegexGroup(start_pos + vm.groups[g].start,
                                       start_pos + vm.groups[g].end);
            }
        }
    }

    return result;
}

RegexMatch Regex::match(const StringD& str) const noexcept
{
    if(this->_bytecode.empty())
        return RegexMatch();

    RegexGroup groups[REGEX_MAX_GROUPS] = {};

    bool result = this->exec_at(str, 0, groups);

    if(result)
        return RegexMatch(str, true, groups, this->_group_count);

    return RegexMatch();
}

RegexMatch Regex::search(const StringD& str) const noexcept
{
    if(this->_bytecode.empty())
        return RegexMatch();

    for(std::size_t start = 0; start <= str.size(); ++start)
    {
        RegexGroup groups[REGEX_MAX_GROUPS] = {};

        if(this->exec_at(str, start, groups))
            if(groups[0].start < groups[0].end || start == str.size())
                return RegexMatch(str, true, groups, this->_group_count);
    }

    return RegexMatch();
}

void Regex::match_iter(const StringD& str,
                       const std::function<void(const RegexMatch&)>& callback) const noexcept
{
    if(this->_bytecode.empty())
        return;

    std::size_t search_start = 0;

    while(search_start <= str.size())
    {
        RegexGroup groups[REGEX_MAX_GROUPS] = {};

        if(this->exec_at(str, search_start, groups))
        {
            std::size_t match_end = groups[0].end;

            /* Skip empty matches to avoid infinite loops */
            if(match_end <= search_start)
            {
                search_start++;
                continue;
            }

            RegexMatch m(str, true, groups, this->_group_count);
            callback(m);

            search_start = match_end;
        }
        else
        {
            search_start++;
        }
    }
}

StringD Regex::replace_iter(const StringD& str,
                            const std::function<StringD(const RegexMatch&)>& callback) const noexcept
{
    if(this->_bytecode.empty())
        return StringD();

    StringD res;

    std::size_t search_start = 0;

    while(search_start <= str.size())
    {
        RegexGroup groups[REGEX_MAX_GROUPS] = {};

        if(this->exec_at(str, search_start, groups))
        {
            std::size_t match_end = groups[0].end;

            if(match_end <= search_start)
            {
                res.push_back(str[search_start++]);
                continue;
            }

            RegexMatch m(str, true, groups, this->_group_count);

            res.appends(callback(m));

            search_start = match_end;
        }
        else
        {
            if(search_start < str.size())
                res.push_back(str[search_start++]);
            else
                search_start++;
        }
    }

    return res;
}

Vector<RegexMatch> Regex::match_all(const StringD& str) const noexcept
{
    Vector<RegexMatch> results;

    this->match_iter(str, [&results](const RegexMatch& m) {
        results.push_back(m);
    });

    return results;
}

StringD Regex::replace_all(const StringD& str, const StringD& replace) const noexcept
{
    return this->replace_iter(str, [&](const RegexMatch&) {
        return replace;
    });
}

STDROMANO_NAMESPACE_END
