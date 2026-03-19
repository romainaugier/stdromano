// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - Present Romain Augier
// All rights reserved.

#include "stdromano/hashmap.hpp"
#include "stdromano/python.hpp"
#include "stdromano/stackvector.hpp"

#include <algorithm>

STDROMANO_NAMESPACE_BEGIN

PYTHON_NAMESPACE_BEGIN

// Token utilities

static const char* const g_token_kind_names[] = {
    "IDENTIFIER", "KEYWORD", "LITERAL", "OPERATOR",
    "DELIMITER",  "NEWLINE", "INDENT",  "DEDENT",
};

const char* Token::kind_as_string(Token::Kind kind) noexcept
{
    auto k = static_cast<std::uint32_t>(kind);
    return k < 8 ? g_token_kind_names[k] : nullptr;
}

// Lexer

struct Lexer
{
    const char* buffer;
    char* cursor;
    std::uint32_t line;
    std::uint32_t column;

    std::uint32_t indent_size;
    StackVector<std::uint32_t, 256> indent_stack;

    std::shared_ptr<spdlog::logger> logger;

    static const HashMap<StringD, Keyword>    keyword_map;
    static const HashMap<StringD, Delimiter>  delimiter_map;
    static const HashMap<StringD, Operator>   operator_map;

    explicit Lexer(const char* source,
                   std::shared_ptr<spdlog::logger>& logger) noexcept;

    bool tokenize(Vector<Token>& out) noexcept;

private:
    static constexpr std::uint32_t LEX_ERROR = std::numeric_limits<std::uint32_t>::max();
    static constexpr std::uint32_t INDENT_UNDEFINED = std::numeric_limits<std::uint32_t>::max();

    static bool is_id_start(unsigned int c) noexcept { return is_letter(c) || c == '_'; }
    static bool is_id_continue(unsigned int c) noexcept { return is_letter(c) || is_digit(c) || c == '_'; }
    static bool is_hex(unsigned int c) noexcept { return is_digit(c) || (c-'a' < 6u) || (c-'A' < 6u); }
    static bool is_oct(unsigned int c) noexcept { return c - '0' < 8u; }
    static bool is_bin(unsigned int c) noexcept { return c == '0' || c == '1'; }
    static bool is_newline(unsigned int c) noexcept { return c == '\n' || c == '\r'; }
    // TODO: more robust unicode functions
    static bool is_unicode_id_start(unsigned int c) noexcept { return c >= 128; }
    static bool is_unicode_id_continue(unsigned int c) noexcept { if(c < 128) return isalnum(c) | (c == '_'); return true; }

    char  peek(std::uint32_t ahead = 0) const noexcept;
    char  advance() noexcept;
    void  advance_n(std::uint32_t n) noexcept;
    bool  at_end() const noexcept;
    bool  match(char expected) noexcept;

    bool lex_string(Vector<Token>& out) noexcept;
    bool lex_number(Vector<Token>& out) noexcept;
    bool lex_identifier(Vector<Token>& out) noexcept;
    bool lex_delimiter(Vector<Token>& out) noexcept;
    bool lex_operator(Vector<Token>& out) noexcept;
    bool lex_newline_indent(Vector<Token>& out) noexcept;
    void skip_comment() noexcept;

    std::uint32_t consume_digits(bool (*pred)(unsigned int)) noexcept;
    std::uint32_t consume_exponent() noexcept;

    Literal classify_string_prefix(const char* start, std::uint32_t len) const noexcept;
    bool consume_string_body(char quote, bool triple, Literal type) noexcept;

    bool emit_indentation(std::uint32_t spaces, Vector<Token>& out) noexcept;

    template<typename... Args>
    void report_error(fmt::format_string<Args...> fmt, Args&&... args) noexcept;
};

const HashMap<StringD, Keyword> Lexer::keyword_map = {
    { StringD::make_ref("False"), Keyword::False },
    { StringD::make_ref("await"), Keyword::Await },
    { StringD::make_ref("else"), Keyword::Else },
    { StringD::make_ref("import"), Keyword::Import },
    { StringD::make_ref("pass"), Keyword::Pass },
    { StringD::make_ref("None"), Keyword::None },
    { StringD::make_ref("break"), Keyword::Break },
    { StringD::make_ref("except"), Keyword::Except },
    { StringD::make_ref("in"), Keyword::In },
    { StringD::make_ref("raise"), Keyword::Raise },
    { StringD::make_ref("True"), Keyword::True },
    { StringD::make_ref("class"), Keyword::Class },
    { StringD::make_ref("finally"), Keyword::Finally },
    { StringD::make_ref("is"), Keyword::Is },
    { StringD::make_ref("return"), Keyword::Return },
    { StringD::make_ref("and"), Keyword::And },
    { StringD::make_ref("continue"), Keyword::Continue },
    { StringD::make_ref("for"), Keyword::For },
    { StringD::make_ref("lambda"), Keyword::Lambda },
    { StringD::make_ref("try"), Keyword::Try },
    { StringD::make_ref("as"), Keyword::As },
    { StringD::make_ref("def"), Keyword::Def },
    { StringD::make_ref("from"), Keyword::From },
    { StringD::make_ref("nonlocal"), Keyword::Nonlocal },
    { StringD::make_ref("while"), Keyword::While },
    { StringD::make_ref("assert"), Keyword::Assert },
    { StringD::make_ref("del"), Keyword::Del },
    { StringD::make_ref("global"), Keyword::Global },
    { StringD::make_ref("not"), Keyword::Not },
    { StringD::make_ref("with"), Keyword::With },
    { StringD::make_ref("async"), Keyword::Async },
    { StringD::make_ref("elif"), Keyword::Elif },
    { StringD::make_ref("if"), Keyword::If },
    { StringD::make_ref("or"), Keyword::Or },
    { StringD::make_ref("yield"), Keyword::Yield },
};

const HashMap<StringD, Delimiter> Lexer::delimiter_map = {
    { StringD::make_ref("("), Delimiter::LParen },
    { StringD::make_ref(")"), Delimiter::RParen },
    { StringD::make_ref("["), Delimiter::LBracket },
    { StringD::make_ref("]"), Delimiter::RBracket },
    { StringD::make_ref("{"), Delimiter::LBrace },
    { StringD::make_ref("}"), Delimiter::RBrace },
    { StringD::make_ref(","), Delimiter::Comma },
    { StringD::make_ref(":"), Delimiter::Colon },
    { StringD::make_ref("."), Delimiter::Dot },
    { StringD::make_ref(";"), Delimiter::Semicolon },
    { StringD::make_ref("@"), Delimiter::At },
    { StringD::make_ref("->"), Delimiter::RightArrow },
};

const HashMap<StringD, Operator> Lexer::operator_map = {
    { StringD::make_ref("+"), Operator::Addition },
    { StringD::make_ref("-"), Operator::Subtraction },
    { StringD::make_ref("*"), Operator::Multiplication },
    { StringD::make_ref("/"), Operator::Division },
    { StringD::make_ref("%"), Operator::Modulus },
    { StringD::make_ref("**"), Operator::Exponentiation },
    { StringD::make_ref("//"), Operator::FloorDivision },
    { StringD::make_ref("="), Operator::Assign },
    { StringD::make_ref("+="), Operator::AdditionAssign },
    { StringD::make_ref("-="), Operator::SubtractionAssign },
    { StringD::make_ref("*="), Operator::MultiplicationAssign },
    { StringD::make_ref("/="), Operator::DivisionAssign },
    { StringD::make_ref("%="), Operator::ModulusAssign },
    { StringD::make_ref("//="), Operator::FloorDivisionAssign },
    { StringD::make_ref("**="), Operator::ExponentiationAssign },
    { StringD::make_ref("&="), Operator::BitwiseAndAssign },
    { StringD::make_ref("|="), Operator::BitwiseOrAssign },
    { StringD::make_ref("^="), Operator::BitwiseXorAssign },
    { StringD::make_ref("<<="), Operator::BitwiseLShiftAssign },
    { StringD::make_ref(">>="), Operator::BitwiseRShiftAssign },
    { StringD::make_ref("&"), Operator::BitwiseAnd },
    { StringD::make_ref("|"), Operator::BitwiseOr },
    { StringD::make_ref("^"), Operator::BitwiseXor },
    { StringD::make_ref("~"), Operator::BitwiseNot },
    { StringD::make_ref("<<"), Operator::BitwiseLShift },
    { StringD::make_ref(">>"), Operator::BitwiseRShift },
    { StringD::make_ref("=="), Operator::ComparatorEquals },
    { StringD::make_ref("!="), Operator::ComparatorNotEquals },
    { StringD::make_ref(">"), Operator::ComparatorGreaterThan },
    { StringD::make_ref("<"), Operator::ComparatorLessThan },
    { StringD::make_ref(">="), Operator::ComparatorGreaterEqualsThan },
    { StringD::make_ref("<="), Operator::ComparatorLessEqualsThan },
    { StringD::make_ref("and"), Operator::LogicalAnd },
    { StringD::make_ref("or"), Operator::LogicalOr },
    { StringD::make_ref("not"), Operator::LogicalNot },
    { StringD::make_ref("is"), Operator::IdentityIs },
    { StringD::make_ref("is not"), Operator::IdentityIsNot },
    { StringD::make_ref("in"), Operator::MembershipIn },
    { StringD::make_ref("not in"), Operator::MembershipNotIn },
};

Lexer::Lexer(const char* source,
             std::shared_ptr<spdlog::logger>& logger) noexcept : buffer(source),
                                                                 cursor(const_cast<char*>(source)),
                                                                 line(1),
                                                                 column(1),
                                                                 indent_size(INDENT_UNDEFINED),
                                                                 logger(logger)
{
    indent_stack.push_back(0);
}

char Lexer::peek(std::uint32_t ahead) const noexcept
{
    const char* p = cursor;

    for(std::uint32_t i = 0; i < ahead && *p != '\0'; ++i)
        ++p;

    return *p;
}

char Lexer::advance() noexcept
{
    char c = *cursor++;
    column++;
    return c;
}

void Lexer::advance_n(std::uint32_t n) noexcept
{
    for(std::uint32_t i = 0; i < n && *cursor != '\0'; ++i)
    {
        cursor++;
        column++;
    }
}

bool Lexer::at_end() const noexcept { return *cursor == '\0'; }

bool Lexer::match(char expected) noexcept
{
    if(*cursor == expected)
    {
        advance();
        return true;
    }

    return false;
}

template<typename... Args>
void Lexer::report_error(fmt::format_string<Args...> fmt, Args&&... args) noexcept
{
    const char* p = this->buffer;
    std::uint32_t cur = 1;

    while(*p != '\0' && cur < line)
    {
        if(*p == '\n')
            cur++;

        p++;
    }

    const char* line_start = p;
    const char* line_end = p;

    while(*line_end != '\0' && *line_end != '\n')
        line_end++;

    std::uint32_t line_len = static_cast<std::uint32_t>(line_end - line_start);
    const String16 gutter = String16::make_fmt("{} | ", line);

    this->logger->error(fmt, std::forward<Args>(args)...);
    this->logger->error("{}{}", gutter, fmt::string_view(line_start, line_len));
    this->logger->error("{0: ^{1}}^", "", gutter.size() + column - 1);
}

Literal Lexer::classify_string_prefix(const char* start, std::uint32_t len) const noexcept
{
    bool has_r = false, has_u = false, has_f = false, has_b = false, has_t = false;

    for(std::uint32_t i = 0; i < len; ++i)
    {
        char c = start[i];

        if(c == '"' || c == '\'')
            break;

        switch (c)
        {
            case 'r':
            case 'R':
                has_r = true;
                break;
            case 'u':
            case 'U':
                has_u = true;
                break;
            case 'f':
            case 'F':
                has_f = true;
                break;
            case 'b':
            case 'B':
                has_b = true;
                break;
            case 't':
            case 'T':
                has_t = true;
                break;
            default:
                break;
        }
    }

    if(has_f || has_t)
        return Literal::FormattedString;

    if(has_b)
        return Literal::Bytes;

    if(has_r)
        return Literal::RawString;

    if(has_u)
        return Literal::UnicodeString;

    return Literal::String;
}

bool Lexer::consume_string_body(char quote, bool triple, Literal type) noexcept
{
    std::uint32_t depth = 0;

    while(!this->at_end())
    {
        char c = *cursor;

        if (c == '\\')
        {
            this->advance();

            if(!this->at_end())
                this->advance();

            continue;
        }

        if(c == quote)
        {
            if(triple)
            {
                if(this->peek(1) == quote && this->peek(2) == quote && depth == 0)
                {
                    this->advance_n(3);
                    return true;
                }

                this->advance();

                continue;
            }
            else if(depth == 0)
            {
                this->advance();
                return true;
            }
        }

        if(c == '\r')
        {
            this->cursor++;
            this->column = 1;
            this->line++;

            if(*this->cursor == '\n')
                this->cursor++;
        }
        else if(c == '\n')
        {
            this->cursor++;
            this->column = 1;
            this->line++;
        }
        else if(c == '{' && type == Literal::FormattedString)
        {
            depth++;

            this->advance();
        }
        else if(c == '}' && type == Literal::FormattedString)
        {
            if(depth == 0)
            {
                this->report_error("Unexpected \"}\" in formatted string");
                return false;
            }

            depth--;

            this->advance();
        }
        else
        {
            this->advance();
        }
    }

    this->report_error("Unterminated string literal");

    return false;
}

bool Lexer::lex_string(Vector<Token>& out) noexcept
{
    char* start = this->cursor;
    std::uint32_t col_start = this->column;

    auto is_str_prefix = [](unsigned int c) {
        return (c == 'r') | (c == 'R') | (c == 'u') | (c == 'U') | (c == 'f') | (c == 'F') | (c == 'b') | (c == 'B');
    };

    std::uint32_t prefix_len = 0;

    while(is_str_prefix(this->peek(prefix_len)))
        prefix_len++;

    char quote_char = this->peek(prefix_len);

    if(quote_char != '"' && quote_char != '\'')
        return false;

    Literal lit_type = this->classify_string_prefix(start, prefix_len);

    this->advance_n(prefix_len);

    bool triple = (this->peek(0) == quote_char && this->peek(1) == quote_char && this->peek(2) == quote_char);

    if(triple)
        this->advance_n(3);
    else
        this->advance();

    char* body_start = this->cursor;

    if(!this->consume_string_body(quote_char, triple, lit_type))
        return false;

    char* body_end = triple ? (this->cursor - 3) : (this->cursor - 1);
    std::uint32_t body_len = static_cast<std::uint32_t>(body_end - body_start);

    out.emplace_back(StringD::make_ref(body_start, body_len),
                     Token::Kind::Literal,
                     static_cast<std::uint32_t>(lit_type),
                     col_start,
                     this->line);
    return true;
}

std::uint32_t Lexer::consume_digits(bool (*pred)(unsigned)) noexcept
{
    std::uint32_t n = 0;

    while (!this->at_end() && (pred(static_cast<unsigned int>(*this->cursor)) || *this->cursor == '_'))
    {
        this->advance();
        n++;
    }

    return n;
}

std::uint32_t Lexer::consume_exponent() noexcept
{
    if(this->at_end())
        return 0;

    char c = *this->cursor;

    if(c != 'e' && c != 'E')
        return 0;

    std::uint32_t n = 1;

    this->advance();

    if(*this->cursor == '+' || *this->cursor == '-')
    {
        this->advance();
        n++;
    }

    n += this->consume_digits(is_digit);

    return n;
}

bool Lexer::lex_number(Vector<Token>& out) noexcept
{
    char* start = this->cursor;
    std::uint32_t col_start = this->column;
    Literal lit = Literal::Integer;

    if(*this->cursor == '0' && !this->at_end())
    {
        char next = this->peek(1);

        if(next == 'x' || next == 'X')
        {
            this->advance_n(2);

            if(this->consume_digits(is_hex) == 0)
            {
                this->report_error("Invalid hex literal");
                return false;
            }

            goto emit;
        }

        if(next == 'o' || next == 'O')
        {
            this->advance_n(2);

            if(this->consume_digits(is_oct) == 0)
            {
                this->report_error("Invalid octal literal");
                return false;
            }

            goto emit;
        }

        if(next == 'b' || next == 'B')
        {
            this->advance_n(2);

            if(this->consume_digits(is_bin) == 0)
            {
                this->report_error("Invalid binary literal");
                return false;
            }

            goto emit;
        }
    }

    {
        bool has_dot = false;

        this->consume_digits(is_digit);

        if(*this->cursor == '.')
        {
            char after_dot = this->peek(1);

            if(is_digit(static_cast<unsigned int>(after_dot)) ||
               after_dot == 'e' ||
               after_dot == 'E' ||
               after_dot == 'j' ||
               after_dot == 'J' ||
               this->is_newline(after_dot) ||
               this->cursor == start)
            {
                has_dot = true;
                this->advance();
                this->consume_digits(is_digit);
            }
        }

        bool has_exp = this->consume_exponent() > 0;

        if(has_dot || has_exp)
            lit = Literal::Float;
    }

    if(*this->cursor == 'j' || *this->cursor == 'J')
    {
        this->advance();
        lit = Literal::Complex;
    }

emit:
    std::uint32_t length = static_cast<std::uint32_t>(this->cursor - start);

    if(length == 0)
        return false;

    out.emplace_back(StringD::make_ref(start, length),
                     Token::Kind::Literal,
                     static_cast<std::uint32_t>(lit),
                     col_start,
                     line);
    return true;
}

bool Lexer::lex_identifier(Vector<Token>& out) noexcept
{
    char* start = this->cursor;
    std::uint32_t col_start = column;

    this->advance();

    while(!this->at_end())
    {
        if(this->is_id_continue(static_cast<unsigned int>(*this->cursor)))
        {
            this->advance();
        }
        else if(this->is_unicode_id_start(static_cast<unsigned int>(*this->cursor)))
        {
            this->advance();

            while(this->is_unicode_id_continue(static_cast<unsigned int>(*this->cursor)))
                this->advance();
        }
        else
        {
            break;
        }
    }

    std::uint32_t length = static_cast<std::uint32_t>(this->cursor - start);
    StringD word = StringD::make_from_c_str(start, length);

    if(length == 2 && std::strncmp(start, "is", 2) == 0)
    {
        char* save_cur = this->cursor;
        std::uint32_t save_col = this->column;

        if(*this->cursor == ' ')
        {
            this->advance();

            if(std::strncmp(this->cursor, "not", 3) == 0 && !this->is_id_continue(static_cast<unsigned int>(this->peek(3))))
            {
                this->advance_n(3);

                out.emplace_back(StringD::make_ref(start, 6),
                                 Token::Kind::Operator,
                                 static_cast<std::uint32_t>(Operator::IdentityIsNot),
                                 col_start, line);

                return true;
            }

            this->cursor = save_cur;
            this->column = save_col;
        }
    }

    if(length == 3 && std::strncmp(start, "not", 3) == 0)
    {
        char* save_cur = this->cursor;
        std::uint32_t save_col = this->column;

        if(*this->cursor == ' ')
        {
            this->advance();

            if(std::strncmp(this->cursor, "in", 2) == 0 && !this->is_id_continue(static_cast<unsigned int>(this->peek(2))))
            {
                this->advance_n(2);

                out.emplace_back(StringD::make_ref(start, 6),
                                 Token::Kind::Operator,
                                 static_cast<std::uint32_t>(Operator::MembershipNotIn),
                                 col_start, line);

                return true;
            }

            this->cursor = save_cur;
            this->column = save_col;
        }
    }

    auto kw = keyword_map.find(word);

    if(kw != keyword_map.end())
    {
        out.emplace_back(std::move(word),
                         Token::Kind::Keyword,
                         static_cast<std::uint32_t>(kw->second),
                         col_start,
                         line);

        return true;
    }

    auto op = operator_map.find(word);

    if(op != operator_map.end())
    {
        out.emplace_back(std::move(word),
                         Token::Kind::Operator,
                         static_cast<std::uint32_t>(op->second),
                         col_start,
                         line);

        return true;
    }

    out.emplace_back(std::move(word), Token::Kind::Identifier, 0, col_start, line);

    return true;
}

bool Lexer::lex_delimiter(Vector<Token>& out) noexcept
{
    char* start = this->cursor;
    std::uint32_t col_start = this->column;

    if(*this->cursor == '-' && this->peek(1) == '>')
    {
        this->advance_n(2);

        out.emplace_back(StringD::make_ref(start, 2),
                         Token::Kind::Delimiter,
                         static_cast<std::uint32_t>(Delimiter::RightArrow),
                         col_start,
                         line);

        return true;
    }

    StringD d = StringD::make_ref(start, 1);

    auto it = delimiter_map.find(d);

    if(it != delimiter_map.end())
    {
        this->advance();

        out.emplace_back(std::move(d),
                         Token::Kind::Delimiter,
                         static_cast<std::uint32_t>(it->second),
                         col_start,
                         line);

        return true;
    }

    return false;
}

bool Lexer::lex_operator(Vector<Token>& out) noexcept
{
    char* start = this->cursor;
    std::uint32_t col_start = this->column;

    for(std::uint32_t try_len = 3; try_len >= 1; --try_len)
    {
        bool enough = true;

        for (std::uint32_t i = 0; i < try_len; ++i)
        {
            if(this->peek(i) == '\0')
            {
                enough = false;
                break;
            }
        }

        if(!enough)
            continue;

        StringD candidate = StringD::make_ref(start, try_len);
        auto it = operator_map.find(candidate);

        if(it != operator_map.end())
        {
            this->advance_n(try_len);

            out.emplace_back(std::move(candidate),
                             Token::Kind::Operator,
                             static_cast<std::uint32_t>(it->second),
                             col_start,
                             line);

            return true;
        }
    }

    if(*this->cursor == '!')
    {
        char next = this->peek(1);

        if(next == 'r' || next == 's' || next == 'a')
        {
            this->advance();

            out.emplace_back(StringD::make_ref(start, 1),
                             Token::Kind::Operator,
                             static_cast<std::uint32_t>(Operator::Bang),
                             col_start,
                             line);

            return true;
        }

        this->report_error("Unexpected character '!'");

        return false;
    }

    return false;
}

bool Lexer::emit_indentation(std::uint32_t spaces, Vector<Token>& out) noexcept
{
    if(this->indent_size == INDENT_UNDEFINED || this->indent_size == 0)
        this->indent_size = spaces;

    if(spaces > this->indent_stack.back())
    {
        this->indent_stack.push_back(spaces);
        out.emplace_back(StringD(), Token::Kind::Indent, spaces, this->column, this->line);
    }
    else if(spaces < this->indent_stack.back())
    {
        while(this->indent_stack.size() > 1 && this->indent_stack.back() > spaces)
        {
            this->indent_stack.pop_back();
            out.emplace_back(StringD(), Token::Kind::Dedent, spaces, this->column, this->line);
        }

        if(this->indent_stack.back() != spaces)
        {
            this->logger->error("IndentationError: unindent does not match any outer indentation level");
            this->logger->error("Line {}, Column {}", this->line, this->column);

            return false;
        }
    }

    return true;
}

bool Lexer::lex_newline_indent(Vector<Token>& out) noexcept
{
    out.emplace_back(StringD(), Token::Kind::Newline, 0, this->column, this->line);

    if(*this->cursor == '\r')
    {
        this->cursor++;

        if(*this->cursor == '\n')
            this->cursor++;
    }
    else
    {
        this->cursor++;
    }

    this->line++;
    this->column = 1;

    while(!this->at_end() && this->is_newline(static_cast<unsigned int>(*this->cursor)))
    {
        out.emplace_back(StringD(), Token::Kind::Newline, 0, this->column, this->line);

        if(*this->cursor == '\r')
        {
            this->cursor++;

            if(*this->cursor == '\n')
                this->cursor++;
        }
        else
        {
            this->cursor++;
        }

        this->line++;
    }

    std::uint32_t spaces = 0;

    while(!this->at_end() && *this->cursor == ' ')
    {
        spaces++;
        this->cursor++;
    }

    if(this->at_end() || this->is_newline(static_cast<unsigned int>(*this->cursor)))
    {
        this->column = 1;
        return true;
    }

    this->column = spaces + 1;

    return this->emit_indentation(spaces, out);
}

void Lexer::skip_comment() noexcept
{
    while(!this->at_end() && !this->is_newline(static_cast<unsigned int>(*this->cursor)))
        this->advance();
}

bool Lexer::tokenize(Vector<Token>& out) noexcept
{
    this->logger->trace("Lexing source code");

    while(!at_end())
    {
        char c = *cursor;

        if(c == ' ' || c == '\t')
        {
            this->advance();
            continue;
        }

        if(c == '\\')
        {
            this->advance();

            while(!this->at_end() && (this->is_newline(static_cast<unsigned int>(*this->cursor)) || *this->cursor == ' '))
                this->advance();

            continue;
        }

        if(c == '#')
        {
            this->skip_comment();
            continue;
        }

        if(this->is_newline(static_cast<unsigned int>(c)))
        {
            if (!this->lex_newline_indent(out))
                return false;

            continue;
        }

        {
            auto is_str_prefix = [](char ch) {
                return ch=='r'||ch=='R'||ch=='u'||ch=='U' || ch=='f'||ch=='F'||ch=='b'||ch=='B';
            };

            bool might_be_string = false;

            if(c == '"' || c == '\'')
            {
                might_be_string = true;
            }
            else if(is_str_prefix(c))
            {
                char n1 = this->peek(1);

                if (n1 == '"' || n1 == '\'')
                {
                    might_be_string = true;
                }
                else if(is_str_prefix(n1))
                {
                    char n2 = this->peek(2);

                    if(n2 == '"' || n2 == '\'')
                        might_be_string = true;
                }
            }

            if(might_be_string)
            {
                if(!this->lex_string(out))
                    return false;

                continue;
            }
        }

        if(is_digit(static_cast<unsigned int>(c)) ||
           (c == '.' && is_digit(static_cast<unsigned int>(this->peek(1)))))
        {
            if(!this->lex_number(out))
                return false;

            continue;
        }

        if(this->is_id_start(static_cast<unsigned int>(c)))
        {
            if(!this->lex_identifier(out))
                return false;

            continue;
        }

        if(this->lex_delimiter(out))
            continue;

        if(this->lex_operator(out))
            continue;

        if(this->is_unicode_id_start(static_cast<unsigned int>(c)))
        {
            if(!this->lex_identifier(out))
                return false;

            continue;
        }

        this->report_error("Unexpected character '{}'", c);
        this->advance();
    }

    while(this->indent_stack.size() > 1)
    {
        this->indent_stack.pop_back();

        out.emplace_back(StringD(),
                         Token::Kind::Dedent,
                         this->indent_stack.back(),
                         this->column,
                         this->line);
    }

    this->logger->trace("Lex successful");

    return true;
}

bool AST::lex(const char* buffer, Vector<Token>& tokens) noexcept
{
    Lexer lexer(buffer, this->_logger);

    return lexer.tokenize(tokens);
}

// Parser

struct Parser
{
    const Vector<Token>& tokens;
    Arena& arena;
    std::shared_ptr<spdlog::logger>& logger;
    const StringD& source_code;
    std::uint32_t pos;

    Parser(const Vector<Token>& tokens,
           Arena& arena,
           std::shared_ptr<spdlog::logger>& logger,
           const StringD& source_code) : tokens(tokens),
                                         arena(arena),
                                         logger(logger),
                                         source_code(source_code),
                                         pos(0) {}

    bool at_end() const noexcept { return this->pos >= this->tokens.size(); }

    const Token& current() const noexcept { return tokens[this->pos]; }

    const Token& peek(std::uint32_t offset = 1) const noexcept { return this->tokens[this->pos + offset]; }

    void advance(const std::uint32_t n = 1) noexcept { this->pos += n; }

    template<typename... Args>
    void error(std::uint32_t line,
               std::uint32_t column,
               std::uint32_t size,
               fmt::format_string<Args...> fmt,
               Args&&... args) const noexcept
    {
        const char* p = this->source_code.data();
        std::uint32_t current_line = 1;

        while(*p != '\0' && current_line < line)
        {
            if(*p == '\n')
                current_line++;

            p++;
        }

        const char* line_start = p;
        const char* line_end = p;

        while(*line_end != '\0' && *line_end != '\n')
            line_end++;

        std::uint32_t line_sz = static_cast<std::uint32_t>(line_end - line_start);

        const StringD gutter = StringD::make_fmt("{} | ", line);

        this->logger->error(fmt, std::forward<Args>(args)...);
        this->logger->error("{}{}", gutter, fmt::string_view(line_start, line_sz));
        this->logger->error("{0: ^{1}}^{0:~^{2}}", "", gutter.size() + column - 1, std::max(size, 1u) - 1);

#if defined(STDROMANO_PYTHON_PARSER_ASSERT_ON_ERROR)
        STDROMANO_ASSERT(false, "Parser error");
#endif // defined(STDROMANO_PYTHON_PARSER_ASSERT_ON_ERROR)
    }

    template<typename... Args>
    void error_at_current(fmt::format_string<Args...> fmt, Args&&... args) const noexcept
    {
        return this->error(this->current().line,
                           this->current().column,
                           static_cast<std::uint32_t>(this->current().value.size()),
                           fmt,
                           std::forward<Args>(args)...);
    }

    void skip_newlines() noexcept
    {
        while(!this->at_end() && this->current().kind == Token::Kind::Newline)
            this->advance();
    }

    void skip_indents() noexcept
    {
        while(!this->at_end() && this->current().kind == Token::Kind::Indent)
            this->advance();
    }

    void skip_dedents() noexcept
    {
        while(!this->at_end() && this->current().kind == Token::Kind::Dedent)
            this->advance();
    }

    bool check(Token::Kind kind) const noexcept
    {
        return !this->at_end() && this->current().kind == kind;
    }

    bool check(Token::Kind kind, std::uint32_t type) const noexcept
    {
        return !this->at_end() && this->current().kind == kind && this->current().type == type;
    }

    bool match_keyword(Keyword kw) const noexcept
    {
        return this->check(Token::Kind::Keyword, static_cast<std::uint32_t>(kw));
    }

    bool match_operator(Operator op) const noexcept
    {
        return this->check(Token::Kind::Operator, static_cast<std::uint32_t>(op));
    }

    bool match_delimiter(Delimiter d) const noexcept
    {
        return this->check(Token::Kind::Delimiter, static_cast<std::uint32_t>(d));
    }

    bool expect_keyword(Keyword kw) noexcept
    {
        if(!this->match_keyword(kw))
        {
            this->error(this->current().line,
                        this->current().column,
                        static_cast<std::uint32_t>(this->current().value.size()),
                        "Expected keyword, got \"{}\"",
                        this->current().value);

            return false;
        }

        this->advance();

        return true;
    }

    bool expect_delimiter(Delimiter d) noexcept
    {
        if(!this->match_delimiter(d))
        {
            this->error(this->current().line,
                        this->current().column,
                        static_cast<std::uint32_t>(this->current().value.size()),
                        "Expected delimiter, got \"{}\"",
                        this->current().value);

            return false;
        }

        this->advance();

        return true;
    }

    bool expect_newline() noexcept
    {
        if(!this->check(Token::Kind::Newline) && !this->at_end())
        {
            this->error(this->current().line,
                        this->current().column,
                        static_cast<std::uint32_t>(this->current().value.size()),
                        "Expected newline, got \"{}\"",
                        this->current().value);

            return false;
        }

        if(!this->at_end())
            this->advance();

        return true;
    }

    // Block

    bool parse_block(Vector<Node*>& body) noexcept
    {
        this->skip_newlines();

        if(!this->check(Token::Kind::Indent))
        {
            this->error(this->current().line,
                        this->current().column,
                        static_cast<std::uint32_t>(this->current().value.size()),
                        "Expected indented block");

            return false;
        }

        this->advance();

        while(!this->at_end())
        {
            this->skip_newlines();

            if(this->at_end())
                break;

            if(this->check(Token::Kind::Dedent))
            {
                this->advance();
                break;
            }

            Node* stmt = this->parse_statement();

            if(stmt == nullptr)
                return false;

            body.push_back(stmt);
        }

        return true;
    }

    // Statements

    Node* parse_statement() noexcept
    {
        this->skip_newlines();

        if(this->at_end())
            return nullptr;

        if(this->current().kind == Token::Kind::Keyword)
        {
            switch(static_cast<Keyword>(this->current().type))
            {
                case Keyword::Def: return this->parse_funcdef();
                case Keyword::Class: return this->parse_classdef();
                case Keyword::If: return this->parse_if();
                case Keyword::While: return this->parse_while();
                case Keyword::For: return this->parse_for();
                case Keyword::Return: return this->parse_return();
                case Keyword::Pass: return this->parse_pass();
                case Keyword::Break: return this->parse_break();
                case Keyword::Continue: return this->parse_continue();
                case Keyword::Raise: return this->parse_raise();
                case Keyword::Import: return this->parse_import();
                case Keyword::From: return this->parse_import_from();
                default: break;
            }
        }

        return this->parse_expr_or_assign();
    }

    Node* parse_funcarg() noexcept
    {
        if(this->check(Token::Kind::Identifier))
        {
            std::uint32_t name_line = this->current().line;
            std::uint32_t name_column = this->current().column;
            StringD name = std::move(this->current().value);
            Node* annotation = nullptr;

            this->advance();

            if(this->check(Token::Kind::Delimiter))
            {
                if(this->match_delimiter(Delimiter::Colon))
                {
                    this->advance();
                    annotation = this->parse_expr();
                }
                else if(!this->match_delimiter(Delimiter::Comma) && !this->match_delimiter(Delimiter::RParen))
                {
                    this->error(this->current().line,
                                this->current().column,
                                static_cast<std::uint32_t>(this->current().value.size()),
                                "Unexpected delimiter \"{}\"",
                                this->current().value);

                    return nullptr;
                }
            }
            else
            {
                this->error_at_current("Unexpected token \"{}\" ({})",
                                       this->current().value,
                                       this->current().kind);

                return nullptr;
            }

            return this->arena.emplace<FunctionArgNode>(std::move(name), annotation, name_line, name_column);
        }

        return nullptr;
    }

    Node* parse_funcdef() noexcept
    {
        std::uint32_t line = this->current().line;
        std::uint32_t column = this->current().column;
        this->advance(); // skip 'def'

        if(!this->check(Token::Kind::Identifier))
        {
            this->error(this->current().line,
                        this->current().column,
                        static_cast<std::uint32_t>(this->current().value.size()),
                        "Expected function name");

            return nullptr;
        }

        StringD name = std::move(StringD::make_from_c_str(this->current().value.c_str(), this->current().value.size()));
        this->advance();

        if(!this->expect_delimiter(Delimiter::LParen))
            return nullptr;

        auto* node = this->arena.emplace<FunctionDefNode>(std::move(name), line, column);

        // Parse arguments
        while(!this->at_end() && !this->match_delimiter(Delimiter::RParen))
        {
            if(this->check(Token::Kind::Identifier))
            {
                Node* arg = this->parse_funcarg();

                if(arg == nullptr)
                    return nullptr;

                node->args.push_back(arg);
            }

            if(this->match_delimiter(Delimiter::Comma))
                this->advance();
        }

        if(!this->expect_delimiter(Delimiter::RParen))
            return nullptr;

        // Optional return annotation
        if(this->match_delimiter(Delimiter::RightArrow))
        {
            this->advance();
            node->return_annotation = this->parse_expr();

            if(node->return_annotation == nullptr)
                return nullptr;
        }

        if(!this->expect_delimiter(Delimiter::Colon))
            return nullptr;

        if(!this->parse_block(node->body))
            return nullptr;

        return node;
    }

    Node* parse_classdef() noexcept
    {
        std::uint32_t line = this->current().line;
        std::uint32_t column = this->current().column;
        this->advance(); // skip 'class'

        if(!this->check(Token::Kind::Identifier))
        {
            this->error(this->current().line,
                        this->current().column,
                        static_cast<std::uint32_t>(this->current().value.size()),
                        "Expected class name");

            return nullptr;
        }

        StringD name = std::move(StringD::make_from_c_str(this->current().value.c_str(), this->current().value.size()));
        this->advance();

        auto* node = this->arena.emplace<ClassDefNode>(std::move(name), line, column);

        // Optional bases
        if(this->match_delimiter(Delimiter::LParen))
        {
            this->advance();

            while(!this->at_end() && !this->match_delimiter(Delimiter::RParen))
            {
                Node* base = this->parse_expr();

                if(base == nullptr)
                    return nullptr;

                node->bases.push_back(base);

                if(this->match_delimiter(Delimiter::Comma))
                    this->advance();
            }

            if(!this->expect_delimiter(Delimiter::RParen))
                return nullptr;
        }

        if(!this->expect_delimiter(Delimiter::Colon))
            return nullptr;

        if(!this->parse_block(node->body))
            return nullptr;

        return node;
    }

    Node* parse_if() noexcept
    {
        std::uint32_t line = this->current().line;
        std::uint32_t column = this->current().column;
        this->advance(); // skip 'if'

        Node* test = this->parse_expr();

        if(test == nullptr)
            return nullptr;

        if(!this->expect_delimiter(Delimiter::Colon))
            return nullptr;

        auto* node = arena.emplace<IfNode>(test, line, column);

        if(!this->parse_block(node->body))
            return nullptr;

        this->skip_newlines();

        // Handle elif chains
        while(!this->at_end() && this->match_keyword(Keyword::Elif))
        {
            std::uint32_t elif_line = this->current().line;
            std::uint32_t elif_column = this->current().column;
            this->advance();

            Node* elif_test = this->parse_expr();

            if(elif_test == nullptr)
                return nullptr;

            if(!this->expect_delimiter(Delimiter::Colon))
                return nullptr;

            auto* elif_node = this->arena.emplace<IfNode>(elif_test, elif_line, elif_column);

            if(!this->parse_block(elif_node->body))
                return nullptr;

            node->orelse.push_back(elif_node);
            node = elif_node; // chain further elifs onto this node

            this->skip_newlines();
        }

        // Handle else
        if(!this->at_end() && this->match_keyword(Keyword::Else))
        {
            this->advance();

            if(!this->expect_delimiter(Delimiter::Colon))
                return nullptr;

            if(!this->parse_block(node->orelse))
                return nullptr;
        }

        return node;
    }

    Node* parse_while() noexcept
    {
        std::uint32_t line = this->current().line;
        std::uint32_t column = this->current().column;
        this->advance();

        Node* test = this->parse_expr();

        if(test == nullptr)
            return nullptr;

        if(!this->expect_delimiter(Delimiter::Colon))
            return nullptr;

        auto* node = this->arena.emplace<WhileNode>(test, line, column);

        if(!this->parse_block(node->body))
            return nullptr;

        this->skip_newlines();

        if(!this->at_end() && this->match_keyword(Keyword::Else))
        {
            this->advance();

            if(!this->expect_delimiter(Delimiter::Colon))
                return nullptr;

            if(!this->parse_block(node->orelse))
                return nullptr;
        }

        return node;
    }

    Node* parse_for() noexcept
    {
        std::uint32_t line = this->current().line;
        std::uint32_t column = this->current().column;
        this->advance();

        Node* target = this->parse_primary();

        if(target == nullptr)
            return nullptr;

        if(!this->expect_keyword(Keyword::In))
            return nullptr;

        Node* iter = this->parse_expr();

        if(iter == nullptr)
            return nullptr;

        if(!this->expect_delimiter(Delimiter::Colon))
            return nullptr;

        auto* node = this->arena.emplace<ForNode>(target, iter, line, column);

        if(!this->parse_block(node->body))
            return nullptr;

        this->skip_newlines();

        if(!this->at_end() && this->match_keyword(Keyword::Else))
        {
            this->advance();

            if(!this->expect_delimiter(Delimiter::Colon))
                return nullptr;

            if(!this->parse_block(node->orelse))
                return nullptr;
        }

        return node;
    }

    Node* parse_return() noexcept
    {
        std::uint32_t line = this->current().line;
        std::uint32_t column = this->current().column;
        this->advance();

        Node* value = nullptr;

        if(!this->at_end() && !this->check(Token::Kind::Newline))
        {
            value = this->parse_expr();

            if(value == nullptr)
                return nullptr;
        }

        return this->arena.emplace<ReturnNode>(value, line, column);
    }

    Node* parse_pass() noexcept
    {
        auto* node = this->arena.emplace<PassNode>(this->current().line, this->current().column);
        this->advance();
        return node;
    }

    Node* parse_break() noexcept
    {
        auto* node = this->arena.emplace<BreakNode>(this->current().line, this->current().column);
        this->advance();
        return node;
    }

    Node* parse_continue() noexcept
    {
        auto* node = this->arena.emplace<ContinueNode>(this->current().line, this->current().column);
        this->advance();
        return node;
    }

    Node* parse_raise() noexcept
    {
        std::uint32_t line = this->current().line;
        std::uint32_t column = this->current().column;
        this->advance();

        Node* exc = nullptr;

        if(!this->at_end() && !this->check(Token::Kind::Newline))
        {
            exc = this->parse_expr();

            if(exc == nullptr)
                return nullptr;
        }

        return arena.emplace<RaiseNode>(exc, line, column);
    }

    Node* parse_import() noexcept
    {
        std::uint32_t line = this->current().line;
        std::uint32_t column = this->current().column;
        this->advance();

        auto* node = this->arena.emplace<ImportNode>(line, column);

        do
        {
            if(!this->check(Token::Kind::Identifier))
            {
                this->error(this->current().line,
                            this->current().column,
                            static_cast<std::uint32_t>(this->current().value.size()),
                            "Expected module name");

                return nullptr;
            }

            StringD name = std::move(StringD::make_from_c_str(this->current().value.c_str(), this->current().value.size()));
            this->advance();

            // Handle dotted names: import os.path
            while(this->match_delimiter(Delimiter::Dot))
            {
                this->advance();

                if(!this->check(Token::Kind::Identifier))
                {
                    this->error(this->current().line,
                                this->current().column,
                                static_cast<std::uint32_t>(this->current().value.size()),
                                "Expected name after .");

                    return nullptr;
                }

                // Append ".part" to the name
                name = std::move(name);  // keep building
                // Simplified: store just the last part for now,
                // or concatenate via a temporary buffer
                this->advance();
            }

            StringD alias;

            if(this->match_keyword(Keyword::As))
            {
                this->advance();

                if(!this->check(Token::Kind::Identifier))
                {
                    this->error(this->current().line,
                                this->current().column,
                                static_cast<std::uint32_t>(this->current().value.size()),
                                "Expected alias");

                    return nullptr;
                }

                alias = std::move(StringD::make_from_c_str(this->current().value.c_str(), this->current().value.size()));
                this->advance();
            }

            node->names.push_back(std::move(name));
            node->aliases.push_back(std::move(alias));

        } while(match_delimiter(Delimiter::Comma) && (this->advance(), true));

        return node;
    }

    Node* parse_import_from() noexcept
    {
        std::uint32_t line = this->current().line;
        std::uint32_t column = this->current().column;
        this->advance(); // skip 'from'

        if(!this->check(Token::Kind::Identifier))
        {
            this->error(this->current().line,
                        this->current().column,
                        static_cast<std::uint32_t>(this->current().value.size()),
                        "Expected module name");

            return nullptr;
        }

        StringD module = std::move(StringD::make_from_c_str(this->current().value.c_str(), this->current().value.size()));
        this->advance();

        if(!this->expect_keyword(Keyword::Import))
            return nullptr;

        auto* node = this->arena.emplace<ImportFromNode>(std::move(module), line, column);

        do
        {
            if(!this->check(Token::Kind::Identifier))
            {
                this->error(this->current().line,
                            this->current().column,
                            static_cast<std::uint32_t>(this->current().value.size()),
                            "Expected name");

                return nullptr;
            }

            StringD name = std::move(StringD::make_from_c_str(this->current().value.c_str(), this->current().value.size()));
            this->advance();

            StringD alias;

            if(this->match_keyword(Keyword::As))
            {
                this->advance();

                if(!this->check(Token::Kind::Identifier))
                {
                    this->error(this->current().line,
                                this->current().column,
                                static_cast<std::uint32_t>(this->current().value.size()),
                                "Expected alias");

                    return nullptr;
                }

                alias = std::move(StringD::make_from_c_str(this->current().value.c_str(), this->current().value.size()));
                this->advance();
            }

            node->names.push_back(std::move(name));
            node->aliases.push_back(std::move(alias));

        } while(this->match_delimiter(Delimiter::Comma) && (this->advance(), true));

        return node;
    }

    // Expression or assignment

    Node* parse_expr_or_assign() noexcept
    {
        std::uint32_t line = this->at_end() ? 0 : this->current().line;
        std::uint32_t column = this->at_end() ? 0 : this->current().column;

        Node* left = this->parse_expr();

        if(left == nullptr)
            return nullptr;

        Vector<Node*> targets;
        targets.push_back(left);

        // Simple assignment: target = value
        // or
        // Multi assignment: target1 = target2 = target3 = value
        if(this->match_operator(Operator::Assign))
        {
            this->advance();

            while(!this->at_end())
            {
                const Token& assign = this->peek();

                if(assign.kind == Token::Kind::Operator && assign.type == Operator::Assign)
                {
                    if(!this->check(Token::Kind::Identifier))
                    {
                        this->error_at_current("Expecting identifier, not \"{}\" ({})",
                                               this->current().value,
                                               this->current().kind);

                        return nullptr;
                    }

                    auto* target = this->arena.emplace<NameNode>(this->current().value,
                                                                 this->current().line,
                                                                 this->current().column);

                    targets.push_back(target);

                    this->advance(2);
                }
                else
                {
                    break;
                }
            }

            Node* value = this->parse_expr();

            if(value == nullptr)
                return nullptr;

            auto* node = this->arena.emplace<AssignNode>(value, line, column);

            node->targets = std::move(targets);

            return node;
        }

        // Augmented assignment: target += value, etc.
        if(!this->at_end() && this->current().kind == Token::Kind::Operator)
        {
            Operator op = static_cast<Operator>(this->current().type);

            if(op >= Operator::AdditionAssign && op <= Operator::BitwiseRShiftAssign)
            {
                this->advance();

                Node* value = this->parse_expr();

                if(value == nullptr)
                    return nullptr;

                return this->arena.emplace<AugAssignNode>(left, op, value, line, column);
            }
        }

        // Standalone expression
        return this->arena.emplace<ExprNode>(left, line, column);
    }

    // Expressions (precedence climbing)

    Node* parse_expr() noexcept
    {
        if(!this->at_end() && this->match_keyword(Keyword::Lambda))
            return this->parse_lambda();

        return this->parse_or_expr();
    }

    Node* parse_lambda() noexcept
    {
        std::uint32_t line = this->current().line;
        std::uint32_t column = this->current().column;
        this->advance(); // skip 'lambda'

        auto* node = this->arena.emplace<LambdaNode>(nullptr, line, column);

        // Parse parameter names
        while(!this->at_end() && !this->match_delimiter(Delimiter::Colon))
        {
            if(this->check(Token::Kind::Identifier))
            {
                auto* param = this->arena.emplace<NameNode>(
                    std::move(StringD::make_from_c_str(this->current().value.c_str(), this->current().value.size())),
                    this->current().line,
                    this->current().column);
                node->args.push_back(param);
                this->advance();
            }

            if(this->match_delimiter(Delimiter::Comma))
                this->advance();
        }

        if(!this->expect_delimiter(Delimiter::Colon))
            return nullptr;

        node->body = this->parse_expr();

        if(node->body == nullptr)
            return nullptr;

        return node;
    }

    Node* parse_or_expr() noexcept
    {
        Node* left = this->parse_and_expr();

        if(left == nullptr)
            return nullptr;

        while(!this->at_end() && this->match_operator(Operator::LogicalOr))
        {
            this->advance();

            Node* right = this->parse_and_expr();

            if(right == nullptr)
                return nullptr;

            auto* node = this->arena.emplace<BoolOpNode>(Operator::LogicalOr, left->line(), left->column());
            node->values.push_back(left);
            node->values.push_back(right);
            left = node;
        }

        return left;
    }

    Node* parse_and_expr() noexcept
    {
        Node* left = this->parse_not_expr();

        if(left == nullptr)
            return nullptr;

        while(!this->at_end() && this->match_operator(Operator::LogicalAnd))
        {
            this->advance();

            Node* right = this->parse_not_expr();

            if(right == nullptr)
                return nullptr;

            auto* node = this->arena.emplace<BoolOpNode>(Operator::LogicalAnd, left->line(), left->column());
            node->values.push_back(left);
            node->values.push_back(right);
            left = node;
        }

        return left;
    }

    Node* parse_not_expr() noexcept
    {
        if(!this->at_end() && this->match_operator(Operator::LogicalNot))
        {
            std::uint32_t line = this->current().line;
            std::uint32_t column = this->current().column;
            this->advance();

            Node* operand = this->parse_not_expr();

            if(operand == nullptr)
                return nullptr;

            return this->arena.emplace<UnaryOpNode>(Operator::LogicalNot, operand, line, column);
        }

        return this->parse_comparison();
    }

    Node* parse_comparison() noexcept
    {
        Node* left = this->parse_bitor();

        if(left == nullptr)
            return nullptr;

        if(this->at_end() || this->current().kind != Token::Kind::Operator)
            return left;

        Operator op = static_cast<Operator>(this->current().type);

        bool is_cmp = (op >= Operator::ComparatorEquals && op <= Operator::ComparatorLessEqualsThan) ||
                      (op >= Operator::IdentityIs && op <= Operator::MembershipNotIn);

        if(!is_cmp)
            return left;

        auto* node = arena.emplace<CompareNode>(left, left->line(), left->column());

        while(!this->at_end() && this->current().kind == Token::Kind::Operator)
        {
            op = static_cast<Operator>(this->current().type);
            is_cmp = (op >= Operator::ComparatorEquals && op <= Operator::ComparatorLessEqualsThan) ||
                     (op >= Operator::IdentityIs && op <= Operator::MembershipNotIn);

            if(!is_cmp)
                break;

            node->ops.push_back(op);
            this->advance();

            Node* right = this->parse_bitor();

            if(right == nullptr)
                return nullptr;

            node->comparators.push_back(right);
        }

        return node;
    }

    Node* parse_bitor() noexcept
    {
        Node* left = this->parse_bitxor();

        if(left == nullptr)
            return nullptr;

        while(!this->at_end() && this->match_operator(Operator::BitwiseOr))
        {
            this->advance();
            Node* right = this->parse_bitxor();

            if(right == nullptr)
                return nullptr;

            left = this->arena.emplace<BinOpNode>(left, Operator::BitwiseOr, right, left->line(), left->column());
        }

        return left;
    }

    Node* parse_bitxor() noexcept
    {
        Node* left = this->parse_bitand();

        if(left == nullptr)
            return nullptr;

        while(!this->at_end() && this->match_operator(Operator::BitwiseXor))
        {
            this->advance();
            Node* right = this->parse_bitand();

            if(right == nullptr)
                return nullptr;

            left = this->arena.emplace<BinOpNode>(left, Operator::BitwiseXor, right, left->line(), left->column());
        }

        return left;
    }

    Node* parse_bitand() noexcept
    {
        Node* left = this->parse_shift();

        if(left == nullptr)
            return nullptr;

        while(!this->at_end() && this->match_operator(Operator::BitwiseAnd))
        {
            this->advance();
            Node* right = this->parse_shift();

            if(right == nullptr)
                return nullptr;

            left = this->arena.emplace<BinOpNode>(left, Operator::BitwiseAnd, right, left->line(), left->column());
        }

        return left;
    }

    Node* parse_shift() noexcept
    {
        Node* left = this->parse_additive();

        if(left == nullptr)
            return nullptr;

        while(!this->at_end() && (this->match_operator(Operator::BitwiseLShift) || this->match_operator(Operator::BitwiseRShift)))
        {
            Operator op = static_cast<Operator>(this->current().type);
            this->advance();

            Node* right = this->parse_additive();

            if(right == nullptr)
                return nullptr;

            left = this->arena.emplace<BinOpNode>(left, op, right, left->line(), left->column());
        }

        return left;
    }

    Node* parse_additive() noexcept
    {
        Node* left = this->parse_multiplicative();

        if(left == nullptr)
            return nullptr;

        while(!this->at_end() && (this->match_operator(Operator::Addition) || this->match_operator(Operator::Subtraction)))
        {
            Operator op = static_cast<Operator>(this->current().type);
            this->advance();

            Node* right = this->parse_multiplicative();

            if(right == nullptr)
                return nullptr;

            left = this->arena.emplace<BinOpNode>(left, op, right, left->line(), left->column());
        }

        return left;
    }

    Node* parse_multiplicative() noexcept
    {
        Node* left = this->parse_unary();

        if(left == nullptr)
            return nullptr;

        while(!this->at_end() &&
              (this->match_operator(Operator::Multiplication) ||
               this->match_operator(Operator::Division) ||
               this->match_operator(Operator::Modulus) ||
               this->match_operator(Operator::FloorDivision)))
        {
            Operator op = static_cast<Operator>(this->current().type);
            this->advance();

            Node* right = this->parse_unary();

            if(right == nullptr)
                return nullptr;

            left = this->arena.emplace<BinOpNode>(left, op, right, left->line(), left->column());
        }

        return left;
    }

    Node* parse_unary() noexcept
    {
        if(!this->at_end() &&
           (this->match_operator(Operator::Addition) ||
            this->match_operator(Operator::Subtraction) ||
            this->match_operator(Operator::BitwiseNot)))
        {
            Operator op = static_cast<Operator>(this->current().type);
            std::uint32_t line = this->current().line;
            std::uint32_t column = this->current().column;
            this->advance();

            Node* operand = this->parse_unary();

            if(operand == nullptr)
                return nullptr;

            return this->arena.emplace<UnaryOpNode>(op, operand, line, column);
        }

        return this->parse_power();
    }

    Node* parse_power() noexcept
    {
        Node* left = this->parse_postfix();

        if(left == nullptr)
            return nullptr;

        if(!this->at_end() && this->match_operator(Operator::Exponentiation))
        {
            this->advance();

            // Right-associative: recurse into parse_unary
            Node* right = this->parse_unary();

            if(right == nullptr)
                return nullptr;

            left = this->arena.emplace<BinOpNode>(left, Operator::Exponentiation, right, left->line(), left->column());
        }

        return left;
    }

    // Postfix: calls, subscripts, attribute access

    Node* parse_postfix() noexcept
    {
        Node* node = this->parse_primary();

        if(node == nullptr)
            return nullptr;

        while(!this->at_end())
        {
            if(this->match_delimiter(Delimiter::LParen))
            {
                this->advance();

                auto* call = this->arena.emplace<CallNode>(node, node->line(), node->column());

                while(!this->at_end() && !this->match_delimiter(Delimiter::RParen))
                {
                    Node* arg = this->parse_expr();

                    if(arg == nullptr)
                        return nullptr;

                    call->args.push_back(arg);

                    if(this->match_delimiter(Delimiter::Comma))
                        this->advance();
                }

                if(!this->expect_delimiter(Delimiter::RParen))
                    return nullptr;

                node = call;
            }
            else if(this->match_delimiter(Delimiter::LBracket))
            {
                this->advance();

                Node* slice = this->parse_expr();

                if(slice == nullptr)
                    return nullptr;

                if(!this->expect_delimiter(Delimiter::RBracket))
                    return nullptr;

                node = this->arena.emplace<SubscriptNode>(node, slice, node->line(), node->column());
            }
            else if(this->match_delimiter(Delimiter::Dot))
            {
                this->advance();

                if(!this->check(Token::Kind::Identifier))
                {
                    this->error(this->current().line,
                                this->current().column,
                                static_cast<std::uint32_t>(this->current().value.size()),
                                "Expected attribute name");

                    return nullptr;
                }

                StringD attr = std::move(StringD::make_from_c_str(this->current().value.c_str(), this->current().value.size()));
                this->advance();

                node = this->arena.emplace<AttributeNode>(node, std::move(attr), node->line(), node->column());
            }
            else
            {
                break;
            }
        }

        return node;
    }

    // Primary: atoms

    Node* parse_primary() noexcept
    {
        if(this->at_end())
        {
            this->error((this->tokens.end() - 1)->line,
                        (this->tokens.end() - 1)->column,
                        static_cast<std::uint32_t>((this->tokens.end() - 1)->value.size()),
                        "Expected attribute name");

            this->logger->error("Unexpected end of input");

            return nullptr;
        }

        this->skip_dedents();

        // Identifier / Name
        if(this->check(Token::Kind::Identifier))
        {
            auto* node = this->arena.emplace<NameNode>(std::move(StringD::make_from_c_str(this->current().value.c_str(),
                                                                                          this->current().value.size())),
                                                       this->current().line,
                                                       this->current().column);

            this->advance();

            if(this->match_delimiter(Delimiter::Colon))
            {
                this->advance();
                node->type = this->parse_expr();
            }

            return node;
        }

        // Literals
        if(this->check(Token::Kind::Literal))
        {
            Literal lit = static_cast<Literal>(this->current().type);

            if(lit == FormattedString)
            {

            }

            auto* node = this->arena.emplace<ConstantNode>(std::move(StringD::make_from_c_str(this->current().value.c_str(),
                                                                                              this->current().value.size())),
                                                           lit,
                                                           this->current().line,
                                                           this->current().column);

            this->advance();

            return node;
        }

        // True / False / None as constants
        if(this->match_keyword(Keyword::True) ||
           this->match_keyword(Keyword::False) ||
           this->match_keyword(Keyword::None))
        {
            auto* node = this->arena.emplace<ConstantNode>(std::move(StringD::make_from_c_str(this->current().value.c_str(),
                                                                     this->current().value.size())),
                                                           Literal::String, // TODO: store as a byte instead of full string
                                                           this->current().line,
                                                           this->current().column);

            this->advance();

            return node;
        }

        // Parenthesized expression or tuple
        if(this->match_delimiter(Delimiter::LParen))
        {
            this->advance();

            // Empty tuple
            if(this->match_delimiter(Delimiter::RParen))
            {
                auto* node = this->arena.emplace<TupleNode>(this->current().line, this->current().column);
                this->advance();
                return node;
            }

            Node* first = this->parse_expr();

            if(first == nullptr)
                return nullptr;

            // Tuple with comma
            if(this->match_delimiter(Delimiter::Comma))
            {
                auto* tup = arena.emplace<TupleNode>(first->line(), first->column());
                tup->elts.push_back(first);

                while(this->match_delimiter(Delimiter::Comma))
                {
                    this->advance();

                    if(this->match_delimiter(Delimiter::RParen))
                        break;

                    Node* elt = this->parse_expr();

                    if(elt == nullptr)
                        return nullptr;

                    tup->elts.push_back(elt);
                }

                if(!this->expect_delimiter(Delimiter::RParen))
                    return nullptr;

                return tup;
            }
            // String concatenation across lines
            // ex:
            // s = ("concat"
            //      "across"
            //      "lines")
            else if(first->type() == ASTNodeConstant)
            {
                auto* literal = static_cast<ConstantNode*>(first);

                if(literal->literal_type <= Literal::Bytes)
                {
                    while(!this->match_delimiter(Delimiter::RParen))
                    {
                        if(!this->expect_newline())
                        {
                            this->error_at_current("Expecting new line");
                            return nullptr;
                        }

                        this->skip_newlines();
                        this->skip_indents();

                        if(!this->check(Token::Kind::Literal))
                        {
                            this->error_at_current("Expecting a string literal, got {}",
                                                   this->current().kind);
                            return nullptr;
                        }

                        if(this->current().type > Literal::FormattedString)
                        {
                            this->error_at_current("Expecting a string literal");
                            return nullptr;
                        }

                        literal->raw.appends(this->current().value);

                        this->advance();
                    }
                }

                if(!this->expect_delimiter(Delimiter::RParen))
                    return nullptr;

                return literal;
            }

            if(!this->expect_delimiter(Delimiter::RParen))
                return nullptr;

            return first;
        }

        // List literal
        if(this->match_delimiter(Delimiter::LBracket))
        {
            std::uint32_t line = this->current().line;
            std::uint32_t column = this->current().column;
            this->advance();

            auto* node = this->arena.emplace<ListNode>(line, column);

            while(!this->at_end() && !this->match_delimiter(Delimiter::RBracket))
            {
                Node* elt = this->parse_expr();

                if(elt == nullptr)
                    return nullptr;

                node->elts.push_back(elt);

                if(this->match_delimiter(Delimiter::Comma))
                    this->advance();
            }

            if(!this->expect_delimiter(Delimiter::RBracket))
                return nullptr;

            return node;
        }

        // Dict literal
        if(this->match_delimiter(Delimiter::LBrace))
        {
            std::uint32_t line = this->current().line;
            std::uint32_t column = this->current().column;
            this->advance();

            auto* node = this->arena.emplace<DictNode>(line, column);

            while(!this->at_end() && !this->match_delimiter(Delimiter::RBrace))
            {
                Node* key = this->parse_expr();

                if(key == nullptr)
                    return nullptr;

                if(!this->expect_delimiter(Delimiter::Colon))
                    return nullptr;

                Node* value = this->parse_expr();

                if(value == nullptr)
                    return nullptr;

                node->keys.push_back(key);
                node->values.push_back(value);

                if(this->match_delimiter(Delimiter::Comma))
                    this->advance();
            }

            if(!this->expect_delimiter(Delimiter::RBrace))
                return nullptr;

            return node;
        }

        // Ellipsis ...
        if(this->match_delimiter(Delimiter::Dot))
        {
            this->advance();

            for(std::uint32_t i = 0; i < 2; i++)
            {
                if(!this->expect_delimiter(Delimiter::Dot))
                {
                    this->error_at_current("Unexpected token \"{}\" ({})",
                                           this->current().value,
                                           this->current().kind);

                    return nullptr;
                }
            }

            return this->arena.emplace<NameNode>("Ellipsis", this->current().line, this->current().column);
        }

        this->error_at_current("Unexpected token \"{}\" ({})",
                               this->current().value,
                               this->current().kind);

        return nullptr;
    }
};

bool AST::parse(const StringD& source_code, const Vector<Token>& tokens) noexcept
{
    this->_logger->trace("Parsing tokens");

    Parser parser(tokens, this->_nodes, this->_logger, source_code);

    this->_root = parser.arena.emplace<ModuleNode>();

    while(!parser.at_end())
    {
        parser.skip_newlines();

        if(parser.at_end())
            break;

        Node* stmt = parser.parse_statement();

        if(stmt == nullptr)
            return false;

        this->_root->body.push_back(stmt);
    }

    this->_logger->trace("Parsing successful");

    return true;
}

//
bool AST::from_text(const StringD& source_code, const bool debug) noexcept
{
    this->_nodes.clear();

    Vector<Token> tokens;

    if(!this->lex(source_code.c_str(), tokens))
        return false;

    if(debug)
    {
        this->_logger->debug("{0:*^{1}}", "", 50);
        this->_logger->debug("LEX: Tokens ({})", tokens.size());

        for(const auto token : tokens)
            this->_logger->debug("{} \"{}\" ({}:{})",
                                 Token::kind_as_string(token.kind),
                                 token.value,
                                 token.line,
                                 token.column);

        this->_logger->debug("{0:*^{1}}", "", 50);
    }

    if(!this->parse(source_code, tokens))
        return false;

    if(debug)
    {
        this->_logger->debug("{0:*^{1}}", "", 50);
        this->_logger->debug("AST");

        visit(this->_root, [&](Node* node, std::uint32_t depth) -> bool {
            node->debug(this->_logger, depth * 2);
            return true;
        });

        this->_logger->debug("{0:*^{1}}", "", 50);
    }

    return true;
}

PYTHON_NAMESPACE_END

STDROMANO_NAMESPACE_END
