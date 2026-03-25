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
    // { StringD::make_ref("match"), Keyword::Match}, // soft keyword
    // { StringD::make_ref("case"), Keyword::Case }, // soft keyword
    // { StringD::make_ref("type"), Keyword::Type }, // soft keyword
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
    { StringD::make_ref("@"), Operator::MatMul },
    { StringD::make_ref("="), Operator::Assign },
    { StringD::make_ref(":="), Operator::WalrusAssign },
    { StringD::make_ref("+="), Operator::AdditionAssign },
    { StringD::make_ref("-="), Operator::SubtractionAssign },
    { StringD::make_ref("*="), Operator::MultiplicationAssign },
    { StringD::make_ref("/="), Operator::DivisionAssign },
    { StringD::make_ref("%="), Operator::ModulusAssign },
    { StringD::make_ref("//="), Operator::FloorDivisionAssign },
    { StringD::make_ref("@="), Operator::MatMulAssign },
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

    if(*this->cursor == ':' && this->peek(1) == '=')
    {
        this->advance_n(2);

        out.emplace_back(StringD::make_ref(start, 2),
                         Token::Kind::Operator,
                         static_cast<std::uint32_t>(Operator::WalrusAssign),
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

        if(this->is_unicode_id_start(static_cast<unsigned int>(c)))
        {
            if(!this->lex_identifier(out))
                return false;

            continue;
        }

        if(this->lex_delimiter(out))
            continue;

        if(this->lex_operator(out))
            continue;

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

    void rewind(const std::uint32_t n = 1) noexcept { this->pos -= n; }

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

        this->logger->flush();

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

    void skip_whitespace_in_bracket() noexcept
    {
        while(!this->at_end() &&
              (this->check(Token::Kind::Newline) ||
               this->check(Token::Kind::Indent) ||
               this->check(Token::Kind::Dedent)))
            this->advance();
    }

    void skip_semicolons() noexcept
    {
        while(!this->at_end() && this->match_delimiter(Delimiter::Semicolon))
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

    bool match_soft_keyword(const char* name) const noexcept
    {
        return this->check(Token::Kind::Identifier) && this->current().value == name;
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

            this->skip_semicolons();
        }

        return true;
    }

    // Decorators

    Node* parse_decorated() noexcept
    {
        Vector<Node*> decorators;

        // Collect one or more @expr lines
        while(!this->at_end() && this->match_operator(Operator::MatMul))
        {
            this->advance(); // skip '@'

            // parse_expr() allows any expression like @buttons[0].clicked.connect
            Node* expr = this->parse_expr();

            if(expr == nullptr)
                return nullptr;

            decorators.push_back(expr);

            if(!this->expect_newline())
                return nullptr;

            this->skip_newlines();
        }

        // The decorated target must be def, class, or another decorator (already consumed)
        Node* target = nullptr;

        if(this->match_keyword(Keyword::Def))
            target = this->parse_funcdef();
        else if(this->match_keyword(Keyword::Class))
            target = this->parse_classdef();
        else
        {
            this->error_at_current("Expected \"def\" or \"class\" after decorator, got \"{}\"",
                                   this->current().value);

            return nullptr;
        }

        if(target == nullptr)
            return nullptr;

        // Wrap outermost decorator first
        // @a
        // @b
        // def f(): ...
        // becomes: DecoratorNode(a, DecoratorNode(b, f))
        for(std::int32_t i = static_cast<std::int32_t>(decorators.size()) - 1; i >= 0; i--)
        {
            target = this->arena.emplace<DecoratorNode>(decorators[i],
                                                        target,
                                                        decorators[i]->line(),
                                                        decorators[i]->column());
        }

        return target;
    }

    // Type params and aliases

    Node* parse_type_alias() noexcept
    {
        std::uint32_t line = this->current().line;
        std::uint32_t column = this->current().column;
        this->advance(); // skip 'type'

        if(!this->check(Token::Kind::Identifier))
        {
            this->error_at_current("Expected type alias name, got \"{}\"",
                                   this->current().value);

            return nullptr;
        }

        StringD name = std::move(StringD::make_from_c_str(this->current().value.c_str(),
                                                          this->current().value.size()));
        this->advance();

        auto* node = this->arena.emplace<TypeAliasNode>(std::move(name), nullptr, line, column);

        // Optional type parameters: type Name[T, U] = ...
        if(!this->parse_type_params(node->type_params))
            return nullptr;

        if(!this->match_operator(Operator::Assign))
        {
            this->error_at_current("Expected '=' in type alias, got \"{}\"",
                                   this->current().value);

            return nullptr;
        }

        this->advance(); // skip '='

        node->value = this->parse_expr();

        if(node->value == nullptr)
            return nullptr;

        return node;
    }

    bool parse_type_params(Vector<Node*>& type_params) noexcept
    {
        if(!this->match_delimiter(Delimiter::LBracket))
            return true; // no type params, not an error

        this->advance(); // skip '['

        while(!this->at_end() && !this->match_delimiter(Delimiter::RBracket))
        {
            std::uint32_t param_line = this->current().line;
            std::uint32_t param_column = this->current().column;

            bool is_paramspec = false;
            bool is_typevartuple = false;

            // **P (ParamSpec)
            if(this->match_operator(Operator::Exponentiation))
            {
                is_paramspec = true;
                this->advance();
            }
            // *Ts (TypeVarTuple)
            else if(this->match_operator(Operator::Multiplication))
            {
                is_typevartuple = true;
                this->advance();
            }

            if(!this->check(Token::Kind::Identifier))
            {
                this->error_at_current("Expected type parameter name, got \"{}\"",
                                       this->current().value);

                return false;
            }

            StringD name = std::move(StringD::make_from_c_str(this->current().value.c_str(),
                                                              this->current().value.size()));
            this->advance();

            auto* param = this->arena.emplace<TypeParamNode>(std::move(name), param_line, param_column);
            param->is_paramspec = is_paramspec;
            param->is_typevartuple = is_typevartuple;

            // Bound or constraint: T: int or T: (int, str)
            if(!is_paramspec && !is_typevartuple && this->match_delimiter(Delimiter::Colon))
            {
                this->advance();

                // Constraint tuple: T: (int, str)
                if(this->match_delimiter(Delimiter::LParen))
                {
                    this->advance();

                    auto* tup = this->arena.emplace<TupleNode>(this->current().line,
                                                               this->current().column);

                    while(!this->at_end() && !this->match_delimiter(Delimiter::RParen))
                    {
                        Node* constraint = this->parse_expr();

                        if(constraint == nullptr)
                            return false;

                        tup->elts.push_back(constraint);

                        if(this->match_delimiter(Delimiter::Comma))
                            this->advance();
                    }

                    if(!this->expect_delimiter(Delimiter::RParen))
                        return false;

                    param->constraint = tup;
                }
                else
                {
                    // Simple bound: T: int
                    Node* bound = this->parse_expr();

                    if(bound == nullptr)
                        return false;

                    param->bound = bound;
                }
            }

            // Default value: T = int (PEP 696)
            if(this->match_operator(Operator::Assign))
            {
                this->advance();

                Node* default_value = this->parse_expr();

                if(default_value == nullptr)
                    return false;

                param->default_value = default_value;
            }

            type_params.push_back(param);

            if(this->match_delimiter(Delimiter::Comma))
                this->advance();
        }

        if(!this->expect_delimiter(Delimiter::RBracket))
            return false;

        return true;
    }

    // Name Qualifier

    Node* parse_name_list_stmt(ASTNodeType node_type) noexcept
    {
        std::uint32_t line = this->current().line;
        std::uint32_t column = this->current().column;
        this->advance(); // skip 'global' / 'nonlocal' / 'del'

        Node* node = nullptr;

        std::function<void(Node*)> add_name = [&](Node*) {};

        switch(node_type)
        {
            case ASTNodeGlobal:
            {
                auto* global_node = this->arena.emplace<GlobalNode>(line, column);
                add_name = [&](Node* name) { global_node->names.emplace_back(std::move(name)); };
                node = global_node;
                break;
            }
            case ASTNodeNonLocal:
            {
                auto* non_local_node = this->arena.emplace<NonLocalNode>(line, column);
                add_name = [&](Node* name) { non_local_node->names.emplace_back(std::move(name)); };
                node = non_local_node;
                break;
            }
            case ASTNodeDel:
            {
                auto* del_node = this->arena.emplace<DelNode>(line, column);
                add_name = [&](Node* name) { del_node->names.emplace_back(std::move(name)); };
                node = del_node;
                break;
            }
            default:
            {
                this->error_at_current("Node type \"{}\" not handled in parse_name_list_stmt",
                                       static_cast<std::uint32_t>(node_type));
                return nullptr;
            }
        }

        do
        {
            Node* name = this->parse_expr();

            if(name == nullptr)
                return nullptr;

            add_name(name);
            this->advance();

        } while(this->match_delimiter(Delimiter::Comma) && (this->advance(), true));

        return node;
    }

    // Async statements

    Node* parse_async_statement() noexcept
    {
        this->advance(); // skip 'async'

        if(this->match_keyword(Keyword::Def))
        {
            this->rewind();

            return this->parse_async_funcdef();
        }

        if(this->match_keyword(Keyword::For))
        {
            this->rewind();

            return this->parse_async_for();
        }

        if(this->match_keyword(Keyword::With))
        {
            this->rewind();

            return this->parse_async_with();
        }

        this->error_at_current("Expected 'def', 'for', or 'with' after 'async', got \"{}\"",
            this->current().value);

        return nullptr;
    }

    // Statements

    Node* parse_statement() noexcept
    {
        this->skip_newlines();
        this->skip_dedents();

        if(this->at_end())
            return nullptr;

        // Decorators: @expr before def/class
        if(this->match_operator(Operator::MatMul))
            return this->parse_decorated();

        if(this->current().kind == Token::Kind::Keyword)
        {
            switch(static_cast<Keyword>(this->current().type))
            {
                case Keyword::Def: return this->parse_funcdef();
                case Keyword::Async: return this->parse_async_statement();
                case Keyword::Class: return this->parse_classdef();
                case Keyword::If: return this->parse_if();
                case Keyword::While: return this->parse_while();
                case Keyword::For: return this->parse_for();
                case Keyword::Return: return this->parse_return();
                case Keyword::Pass: return this->parse_pass();
                case Keyword::Break: return this->parse_break();
                case Keyword::Continue: return this->parse_continue();
                case Keyword::Raise: return this->parse_raise();
                case Keyword::Try: return this->parse_try();
                case Keyword::Import: return this->parse_import();
                case Keyword::From: return this->parse_import_from();
                case Keyword::Global: return this->parse_name_list_stmt(ASTNodeGlobal);
                case Keyword::Nonlocal: return this->parse_name_list_stmt(ASTNodeNonLocal);
                case Keyword::Del: return this->parse_name_list_stmt(ASTNodeDel);
                case Keyword::With: return this->parse_with();
                case Keyword::Assert: return this->parse_assertion();
                default: break;
            }
        }

        // Soft keyword 'type': type alias statement (PEP 695, Python 3.12+)
        // type Name = expr
        // type Name[T] = expr
        if(this->match_soft_keyword("type"))
        {
            // Peek ahead: must be followed by an identifier
            if(this->pos + 1 < this->tokens.size() &&
               this->tokens[this->pos + 1].kind == Token::Kind::Identifier)
                return this->parse_type_alias();
        }

        // Soft keyword 'match'
        // Use an heuristic to determine if we have a match case or just an identifier
        if(this->match_soft_keyword("match"))
        {
            if(this->pos + 1 < this->tokens.size())
            {
                const Token& next = this->tokens[this->pos + 1];

                bool is_match_stmt =
                    next.kind == Token::Kind::Identifier ||
                    next.kind == Token::Kind::Literal ||
                    (next.kind == Token::Kind::Keyword &&
                        (static_cast<Keyword>(next.type) == Keyword::True ||
                        static_cast<Keyword>(next.type) == Keyword::False ||
                        static_cast<Keyword>(next.type) == Keyword::None ||
                        static_cast<Keyword>(next.type) == Keyword::Not ||
                        static_cast<Keyword>(next.type) == Keyword::Lambda ||
                        static_cast<Keyword>(next.type) == Keyword::Await)) ||
                    (next.kind == Token::Kind::Delimiter &&
                        (static_cast<Delimiter>(next.type) == Delimiter::LParen ||
                        static_cast<Delimiter>(next.type) == Delimiter::LBracket ||
                        static_cast<Delimiter>(next.type) == Delimiter::LBrace)) ||
                    (next.kind == Token::Kind::Operator &&
                        (static_cast<Operator>(next.type) == Operator::Subtraction ||
                        static_cast<Operator>(next.type) == Operator::BitwiseNot ||
                        static_cast<Operator>(next.type) == Operator::Multiplication));

                if(is_match_stmt)
                    return this->parse_match();
            }
        }

        return this->parse_expr_or_assign();
    }

    bool parse_func_params(Vector<Node*>& args,
                           std::int32_t& posonly_index,
                           std::int32_t& kwonly_index,
                           Delimiter end_delim) noexcept
    {
        std::int32_t arg_count = 0;

        while(!this->at_end() && !this->match_delimiter(end_delim))
        {
            // Positional-only separator: /
            if(this->match_operator(Operator::Division))
            {
                posonly_index = arg_count;
                this->advance();

                if(this->match_delimiter(Delimiter::Comma))
                    this->advance();

                continue;
            }

            // Keyword-only separator: bare *
            if(this->match_operator(Operator::Multiplication))
            {
                if(this->pos + 1 < this->tokens.size() &&
                   (this->tokens[this->pos + 1].kind == Token::Kind::Delimiter))
                {
                    kwonly_index = arg_count;
                    this->advance();

                    if(this->match_delimiter(Delimiter::Comma))
                        this->advance();

                    continue;
                }

                // *args
                this->advance();

                if(this->check(Token::Kind::Identifier))
                {
                    Node* arg = this->parse_funcarg(end_delim == Delimiter::Colon);

                    if(arg == nullptr)
                        return false;

                    static_cast<FunctionArgNode*>(arg)->is_vararg = true;
                    args.push_back(arg);
                    arg_count++;
                }

                if(this->match_delimiter(Delimiter::Comma))
                    this->advance();

                continue;
            }

            // **kwargs
            if(this->match_operator(Operator::Exponentiation))
            {
                this->advance();

                if(this->check(Token::Kind::Identifier))
                {
                    Node* arg = this->parse_funcarg(end_delim == Delimiter::Colon);

                    if(arg == nullptr)
                        return false;

                    static_cast<FunctionArgNode*>(arg)->is_kwarg = true;
                    args.push_back(arg);
                    arg_count++;
                }

                if(this->match_delimiter(Delimiter::Comma))
                    this->advance();

                continue;
            }

            // Regular argument
            if(this->check(Token::Kind::Identifier))
            {
                Node* arg = this->parse_funcarg(end_delim == Delimiter::Colon);

                if(arg == nullptr)
                    return false;

                args.push_back(arg);
                arg_count++;
            }

            if(this->match_delimiter(Delimiter::Comma))
                this->advance();

            this->skip_whitespace_in_bracket();

            // Safety: if current token isn't anything we can handle, bail
            if(!this->match_delimiter(end_delim) &&
               !this->match_operator(Operator::Division) &&
               !this->match_operator(Operator::Multiplication) &&
               !this->match_operator(Operator::Exponentiation) &&
               !this->check(Token::Kind::Identifier) &&
               !this->match_delimiter(Delimiter::Comma) &&
               !this->check(Token::Kind::Newline) &&
               !this->check(Token::Kind::Dedent))
            {
                this->error_at_current("Unexpected token \"{}\" ({}) in parameter list",
                                       this->current().value,
                                       this->current().kind);

                return false;
            }
        }

        return true;
    }

    Node* parse_funcarg(bool skip_annotation = false) noexcept
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
                    if(!skip_annotation)
                    {
                        this->advance();
                        annotation = this->parse_expr();
                    }
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

            if(this->match_operator(Operator::Assign))
            {
                this->advance();

                Node* default_val = this->parse_expr();

                if(default_val == nullptr)
                    return nullptr;

                auto* farg = this->arena.emplace<FunctionArgNode>(std::move(name), annotation, name_line, name_column);
                farg->default_value = default_val;

                return farg;
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

        auto* node = this->arena.emplace<FunctionDefNode>(std::move(name), line, column);

        // Optional type parameters: def func[T, U](...) -> ...
        if(!this->parse_type_params(node->type_params))
            return nullptr;

        if(!this->expect_delimiter(Delimiter::LParen))
            return nullptr;

        // Parse arguments
        if(!this->parse_func_params(node->args, node->posonly_index, node->kwonly_index, Delimiter::RParen))
            return nullptr;

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

        // Inline body
        if(!this->check(Token::Kind::Newline))
            node->body.push_back(this->parse_statement());
        else if(!this->parse_block(node->body))
            return nullptr;

        return node;
    }

    Node* parse_async_funcdef() noexcept
    {
        std::uint32_t line = this->current().line;
        std::uint32_t column = this->current().column;
        this->advance(); // skip 'async'

        if(!this->expect_keyword(Keyword::Def))
            return nullptr;

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

        auto* node = this->arena.emplace<AsyncFunctionDefNode>(std::move(name), line, column);

        // Optional type parameters: def func[T, U](...) -> ...
        if(!this->parse_type_params(node->type_params))
            return nullptr;

        if(!this->expect_delimiter(Delimiter::LParen))
            return nullptr;

        // Parse arguments
        if(!this->parse_func_params(node->args, node->posonly_index, node->kwonly_index, Delimiter::RParen))
            return nullptr;

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

        // Inline body
        if(!this->check(Token::Kind::Newline))
            node->body.push_back(this->parse_statement());
        else if(!this->parse_block(node->body))
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

        // Optional type parameters: class Foo[T, U](Base):
        if(!this->parse_type_params(node->type_params))
            return nullptr;

        // Optional bases
        if(this->match_delimiter(Delimiter::LParen))
        {
            this->advance();

            while(!this->at_end() && !this->match_delimiter(Delimiter::RParen))
            {
                // **expr unpacking
                if(this->match_operator(Operator::Exponentiation))
                {
                    std::uint32_t kw_line = this->current().line;
                    std::uint32_t kw_column = this->current().column;
                    this->advance();

                    Node* value = this->parse_expr();

                    if(value == nullptr)
                        return nullptr;

                    node->bases.push_back(this->arena.emplace<KeywordArgNode>(StringD(), value, kw_line, kw_column));
                }
                else
                {
                    Node* base = this->parse_expr();

                    if(base == nullptr)
                        return nullptr;

                    // Keyword argument: metaclass=Meta, extra_kw=True
                    if(base->type() == ASTNodeName && this->match_operator(Operator::Assign))
                    {
                        std::uint32_t kw_line = base->line();
                        std::uint32_t kw_column = base->column();
                        StringD kw_name = std::move(static_cast<NameNode*>(base)->id);
                        this->advance();

                        Node* value = this->parse_expr();

                        if(value == nullptr)
                            return nullptr;

                        base = this->arena.emplace<KeywordArgNode>(std::move(kw_name), value, kw_line, kw_column);
                    }

                    node->bases.push_back(base);
                }

                if(this->match_delimiter(Delimiter::Comma))
                    this->advance();
            }

            if(!this->expect_delimiter(Delimiter::RParen))
                return nullptr;
        }

        if(!this->expect_delimiter(Delimiter::Colon))
            return nullptr;

        if(!this->check(Token::Kind::Newline))
            node->body.push_back(this->parse_statement());
        else if(!this->parse_block(node->body))
            return nullptr;

        return node;
    }

    Node* parse_await_stmt() noexcept
    {
        std::uint32_t line = this->current().line;
        std::uint32_t column = this->current().column;
        this->advance(); // skip 'await'

        Node* stmt = this->parse_statement();

        if(stmt == nullptr)
            return nullptr;

        return this->arena.emplace<AwaitNode>(stmt, line, column);
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

        if(!this->check(Token::Kind::Newline))
            node->body.push_back(this->parse_statement());
        else if(!this->parse_block(node->body))
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

        if(!this->check(Token::Kind::Newline))
            node->body.push_back(this->parse_statement());
        else if(!this->parse_block(node->body))
            return nullptr;

        this->skip_newlines();

        if(!this->at_end() && this->match_keyword(Keyword::Else))
        {
            this->advance();

            if(!this->expect_delimiter(Delimiter::Colon))
                return nullptr;

            if(!this->check(Token::Kind::Newline))
                node->orelse.push_back(this->parse_statement());
            else if(!this->parse_block(node->orelse))
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

        if(!this->check(Token::Kind::Newline))
            node->body.push_back(this->parse_statement());
        else if(!this->parse_block(node->body))
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

    Node* parse_async_for() noexcept
    {
        std::uint32_t line = this->current().line;
        std::uint32_t column = this->current().column;

        this->advance(); // skip 'async'

        if(!this->expect_keyword(Keyword::For))
            return nullptr;

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

        auto* node = this->arena.emplace<AsyncForNode>(target, iter, line, column);

        if(!this->check(Token::Kind::Newline))
            node->body.push_back(this->parse_statement());
        else if(!this->parse_block(node->body))
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

    // Context Manager

    Node* parse_with_item() noexcept
    {
        std::uint32_t line = this->current().line;
        std::uint32_t column = this->current().column;

        Node* context_expr = this->parse_expr();

        if(context_expr == nullptr)
            return nullptr;

        Node* optional_vars = nullptr;

        if(this->match_keyword(Keyword::As))
        {
            this->advance();

            optional_vars = this->parse_primary();

            if(optional_vars == nullptr)
                return nullptr;

            // Tuple target: with open(f) as (a, b):
            if(this->match_delimiter(Delimiter::Comma))
            {
                // Only build tuple if NOT followed by another with-item context expr
                // Peek: if next comma-separated thing has 'as' or ':', it's another item
                // Simple heuristic: if we're inside parens and next token after comma
                // could be a with_item, don't tuple-ify
            }
        }

        return this->arena.emplace<WithItemNode>(context_expr, optional_vars, line, column);
    }

    Node* parse_with() noexcept
    {
        std::uint32_t line = this->current().line;
        std::uint32_t column = this->current().column;
        this->advance(); // skip 'with'

        auto* node = this->arena.emplace<WithNode>(line, column);

        // Parenthesized items (Python 3.10+): with (A() as a, B() as b):
        bool parenthesized = false;

        if(this->match_delimiter(Delimiter::LParen))
        {
            parenthesized = true;
            this->advance();
            this->skip_whitespace_in_bracket();
        }

        // Parse first item
        Node* item = this->parse_with_item();

        if(item == nullptr)
            return nullptr;

        node->items.push_back(item);

        // Parse remaining items
        while(this->match_delimiter(Delimiter::Comma))
        {
            this->advance();

            if(parenthesized)
                this->skip_whitespace_in_bracket();

            if(parenthesized && this->match_delimiter(Delimiter::RParen))
                break;

            item = this->parse_with_item();

            if(item == nullptr)
                return nullptr;

            node->items.push_back(item);
        }

        if(parenthesized)
        {
            this->skip_whitespace_in_bracket();

            if(!this->expect_delimiter(Delimiter::RParen))
                return nullptr;
        }

        if(!this->expect_delimiter(Delimiter::Colon))
            return nullptr;

        if(!this->check(Token::Kind::Newline))
            node->body.push_back(this->parse_statement());
        else if(!this->parse_block(node->body))
            return nullptr;

        return node;
    }

    Node* parse_async_with() noexcept
    {
        std::uint32_t line = this->current().line;
        std::uint32_t column = this->current().column;
        this->advance(); // skip 'async'

        if(!this->expect_keyword(Keyword::With))
            return nullptr;

        auto* node = this->arena.emplace<AsyncWithNode>(line, column);

        // Parenthesized items (Python 3.10+): with (A() as a, B() as b):
        bool parenthesized = false;

        if(this->match_delimiter(Delimiter::LParen))
        {
            parenthesized = true;
            this->advance();
            this->skip_whitespace_in_bracket();
        }

        // Parse first item
        Node* item = this->parse_with_item();

        if(item == nullptr)
            return nullptr;

        node->items.push_back(item);

        // Parse remaining items
        while(this->match_delimiter(Delimiter::Comma))
        {
            this->advance();

            if(parenthesized)
                this->skip_whitespace_in_bracket();

            if(parenthesized && this->match_delimiter(Delimiter::RParen))
                break;

            item = this->parse_with_item();

            if(item == nullptr)
                return nullptr;

            node->items.push_back(item);
        }

        if(parenthesized)
        {
            this->skip_whitespace_in_bracket();

            if(!this->expect_delimiter(Delimiter::RParen))
                return nullptr;
        }

        if(!this->expect_delimiter(Delimiter::Colon))
            return nullptr;

        if(!this->check(Token::Kind::Newline))
            node->body.push_back(this->parse_statement());
        else if(!this->parse_block(node->body))
            return nullptr;

        return node;
    }

    // Assertion

    Node* parse_assertion() noexcept
    {
        std::uint32_t line = this->current().line;
        std::uint32_t column = this->current().column;
        this->advance(); // skip 'assert'

        Node* expr = this->parse_expr();

        if(expr == nullptr)
            return nullptr;

        if(this->match_delimiter(Delimiter::Comma))
            this->advance();
        else
            return this->arena.emplace<AssertionNode>(expr, nullptr, line, column);

        Node* message = this->parse_expr();

        if(message == nullptr)
            return nullptr;

        return this->arena.emplace<AssertionNode>(expr, message, line, column);
    }

    // Return

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

            // Implicit tuple: return 1, 2, 3
            if(this->match_delimiter(Delimiter::Comma))
            {
                auto* tup = this->arena.emplace<TupleNode>(value->line(), value->column());
                tup->elts.push_back(value);

                while(this->match_delimiter(Delimiter::Comma))
                {
                    this->advance();

                    if(this->at_end() || this->check(Token::Kind::Newline))
                        break;

                    Node* elt = this->parse_expr();

                    if(elt == nullptr)
                        return nullptr;

                    tup->elts.push_back(elt);
                }

                value = tup;
            }
        }

        return this->arena.emplace<ReturnNode>(value, line, column);
    }

    // Control flow

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

    // Exceptions

    Node* parse_raise() noexcept
    {
        std::uint32_t line = this->current().line;
        std::uint32_t column = this->current().column;
        this->advance();

        Node* exc = nullptr;
        Node* cause = nullptr;

        if(!this->at_end() && !this->check(Token::Kind::Newline))
        {
            exc = this->parse_expr();

            if(exc == nullptr)
                return nullptr;

            // raise exception from cause
            if(this->match_keyword(Keyword::From))
            {
                this->advance();

                cause = this->parse_expr();

                if(cause == nullptr)
                    return nullptr;
            }
        }

        return arena.emplace<RaiseNode>(exc, cause, line, column);
    }

    Node* parse_except() noexcept
    {
        std::uint32_t line = this->current().line;
        std::uint32_t column = this->current().column;
        this->advance(); // skip 'except'

        bool is_star = false;

        // except* for exception groups (PEP 654, Python 3.11+)
        if(this->match_operator(Operator::Multiplication))
        {
            is_star = true;
            this->advance();
        }

        Node* type = nullptr;
        StringD name;

        // Bare 'except:' (no type) — only valid without star
        if(!this->match_delimiter(Delimiter::Colon))
        {
            // Exception type expression
            type = this->parse_expr();

            if(type == nullptr)
                return nullptr;

            // 'as name'
            if(this->match_keyword(Keyword::As))
            {
                this->advance();

                if(!this->check(Token::Kind::Identifier))
                {
                    this->error_at_current("Expected name after 'as', got \"{}\"",
                                           this->current().value);

                    return nullptr;
                }

                name = std::move(StringD::make_from_c_str(this->current().value.c_str(),
                                                          this->current().value.size()));
                this->advance();
            }
        }

        if(!this->expect_delimiter(Delimiter::Colon))
            return nullptr;

        auto* handler = this->arena.emplace<ExceptNode>(type, std::move(name), is_star, line, column);

        if(!this->check(Token::Kind::Newline))
        {
            Node* stmt = this->parse_statement();

            if(stmt == nullptr)
                return nullptr;

            handler->body.push_back(stmt);
        }
        else if(!this->parse_block(handler->body))
        {
            return nullptr;
        }

        return handler;
    }

    Node* parse_try() noexcept
    {
        std::uint32_t line = this->current().line;
        std::uint32_t column = this->current().column;
        this->advance(); // skip 'try'

        if(!this->expect_delimiter(Delimiter::Colon))
            return nullptr;

        auto* node = this->arena.emplace<TryNode>(line, column);

        if(!this->check(Token::Kind::Newline))
        {
            Node* stmt = this->parse_statement();

            if(stmt == nullptr)
                return nullptr;

            node->body.push_back(stmt);
        }
        else if(!this->parse_block(node->body))
        {
            return nullptr;
        }

        this->skip_newlines();

        // except / except* handlers
        while(!this->at_end() && this->match_keyword(Keyword::Except))
        {
            Node* handler = this->parse_except();

            if(handler == nullptr)
                return nullptr;

            node->excepts.push_back(handler);

            this->skip_newlines();
        }

        // else block
        if(!this->at_end() && this->match_keyword(Keyword::Else))
        {
            this->advance();

            if(!this->expect_delimiter(Delimiter::Colon))
                return nullptr;

            if(!this->check(Token::Kind::Newline))
            {
                Node* stmt = this->parse_statement();

                if(stmt == nullptr)
                    return nullptr;

                node->orelse.push_back(stmt);
            }
            else if(!this->parse_block(node->orelse))
            {
                return nullptr;
            }

            this->skip_newlines();
        }

        // finally block
        if(!this->at_end() && this->match_keyword(Keyword::Finally))
        {
            this->advance();

            if(!this->expect_delimiter(Delimiter::Colon))
                return nullptr;

            if(!this->check(Token::Kind::Newline))
            {
                Node* stmt = this->parse_statement();

                if(stmt == nullptr)
                    return nullptr;

                node->finalbody.push_back(stmt);
            }
            else if(!this->parse_block(node->finalbody))
            {
                return nullptr;
            }
        }

        if(node->excepts.empty() && node->finalbody.empty())
        {
            this->error(line, column, 3, "try requires at least one except or finally clause");
            return nullptr;
        }

        return node;
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

        if(!this->check(Token::Kind::Identifier) && !this->match_delimiter(Delimiter::Dot))
        {
            this->error(this->current().line,
                        this->current().column,
                        static_cast<std::uint32_t>(this->current().value.size()),
                        "Expected module name or dotted import");

            return nullptr;
        }

        // concatenate import name, i.e from . | from .module | from os.path | from ...

        const char* start = this->current().value.c_str();
        std::size_t count = this->current().value.size();

        this->advance();

        while(!this->at_end() && (this->check(Token::Kind::Identifier) || this->match_delimiter(Delimiter::Dot)))
        {
            count += this->current().value.size();
            this->advance();
        }

        StringD module = std::move(StringD::make_from_c_str(start, count));

        if(!this->expect_keyword(Keyword::Import))
            return nullptr;

        auto* node = this->arena.emplace<ImportFromNode>(std::move(module), line, column);

        if(this->match_delimiter(Delimiter::LParen))
            this->advance();

        do
        {
            this->skip_whitespace_in_bracket();

            if(this->match_delimiter(Delimiter::RParen))
                break;

            // Identifier or *
            if(!this->check(Token::Kind::Identifier) && !this->match_operator(Operator::Multiplication))
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

        this->skip_whitespace_in_bracket();

        if(this->match_delimiter(Delimiter::RParen))
            this->advance();

        return node;
    }

    // Match

    Node* parse_match() noexcept
    {
        std::uint32_t line = this->current().line;
        std::uint32_t column = this->current().column;
        this->advance(); // skip 'match'

        Node* subject = this->parse_expr();

        if(subject == nullptr)
            return nullptr;

        if(!this->expect_delimiter(Delimiter::Colon))
            return nullptr;

        auto* node = this->arena.emplace<MatchNode>(subject, line, column);

        this->skip_newlines();

        if(!this->check(Token::Kind::Indent))
        {
            this->error_at_current("Expected indented block of case clauses");
            return nullptr;
        }

        this->advance(); // skip indent

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

            Node* case_node = this->parse_match_case();

            if(case_node == nullptr)
                return nullptr;

            node->cases.push_back(case_node);
        }

        return node;
    }

    Node* parse_match_case() noexcept
    {
        std::uint32_t line = this->current().line;
        std::uint32_t column = this->current().column;

        if(!this->match_soft_keyword("case"))
        {
            this->error_at_current("Expected 'case', got \"{}\"",
                                   this->current().value);

            return nullptr;
        }

        this->advance();

        Node* pattern = this->parse_match_pattern();

        if(pattern == nullptr)
            return nullptr;

        Node* guard = nullptr;

        if(this->match_keyword(Keyword::If))
        {
            this->advance();

            guard = this->parse_expr();

            if(guard == nullptr)
                return nullptr;
        }

        if(!this->expect_delimiter(Delimiter::Colon))
            return nullptr;

        auto* node = this->arena.emplace<MatchCaseNode>(pattern, guard, line, column);

        if(!this->check(Token::Kind::Newline))
            node->body.push_back(this->parse_statement());
        else if(!this->parse_block(node->body))
            return nullptr;

        return node;
    }

    // Entry point: OR patterns (lowest precedence)

    Node* parse_match_pattern() noexcept
    {
        Node* left = this->parse_match_as_pattern();

        if(left == nullptr)
            return nullptr;

        if(!this->match_operator(Operator::BitwiseOr))
            return left;

        auto* node = this->arena.emplace<MatchOrNode>(left->line(), left->column());
        node->patterns.push_back(left);

        while(this->match_operator(Operator::BitwiseOr))
        {
            this->advance();

            Node* alt = this->parse_match_as_pattern();

            if(alt == nullptr)
                return nullptr;

            node->patterns.push_back(alt);
        }

        return node;
    }

    // pattern as name

    Node* parse_match_as_pattern() noexcept
    {
        Node* pattern = this->parse_match_primary_pattern();

        if(pattern == nullptr)
            return nullptr;

        if(this->match_keyword(Keyword::As))
        {
            this->advance();

            if(!this->check(Token::Kind::Identifier))
            {
                this->error_at_current("Expected name after 'as', got \"{}\"",
                                       this->current().value);

                return nullptr;
            }

            StringD name = std::move(StringD::make_from_c_str(this->current().value.c_str(),
                                                              this->current().value.size()));
            this->advance();

            return this->arena.emplace<MatchAsNode>(pattern, std::move(name),
                                                    pattern->line(), pattern->column());
        }

        return pattern;
    }

    Node* parse_match_primary_pattern() noexcept
    {
        if(this->at_end())
        {
            this->error_at_current("Expected pattern");
            return nullptr;
        }

        std::uint32_t line = this->current().line;
        std::uint32_t column = this->current().column;

        // Wildcard: _
        if(this->check(Token::Kind::Identifier) && this->current().value == "_")
        {
            this->advance();
            return this->arena.emplace<MatchAsNode>(nullptr, StringD(), line, column);
        }

        // Star pattern: *name or *_ (only valid inside sequences, but parse it here)
        if(this->match_operator(Operator::Multiplication))
        {
            this->advance();

            StringD name;

            if(this->check(Token::Kind::Identifier))
            {
                if(this->current().value != "_")
                {
                    name = std::move(StringD::make_from_c_str(this->current().value.c_str(),
                                                              this->current().value.size()));
                }

                this->advance();
            }
            else
            {
                this->error_at_current("Expected name after '*' in pattern");
                return nullptr;
            }

            return this->arena.emplace<MatchStarNode>(std::move(name), line, column);
        }

        // Singletons: True, False, None
        if(this->match_keyword(Keyword::True) ||
           this->match_keyword(Keyword::False) ||
           this->match_keyword(Keyword::None))
        {
            Keyword kw = static_cast<Keyword>(this->current().type);
            this->advance();
            return this->arena.emplace<MatchSingletonNode>(kw, line, column);
        }

        // Numeric literals (including negative: -42, -3.14, complex: 1+2j)
        if(this->check(Token::Kind::Literal) ||
           ((this->match_operator(Operator::Subtraction)) && this->pos + 1 < this->tokens.size() &&
            this->tokens[this->pos + 1].kind == Token::Kind::Literal))
        {
            bool negate = false;

            if(this->match_operator(Operator::Subtraction))
            {
                negate = true;
                this->advance();
            }

            if(this->check(Token::Kind::Literal))
            {
                auto* val = this->arena.emplace<ConstantNode>(
                    std::move(StringD::make_from_c_str(this->current().value.c_str(),
                                                       this->current().value.size())),
                    static_cast<Literal>(this->current().type),
                    this->current().line,
                    this->current().column);

                this->advance();

                Node* result = val;

                if(negate)
                {
                    result = this->arena.emplace<UnaryOpNode>(Operator::Subtraction, val,
                                                              line, column);
                }

                // Complex literal pattern: 1+2j, 1-2j, -1+2j, -1-2j
                if(!this->at_end() &&
                   (this->match_operator(Operator::Addition) || this->match_operator(Operator::Subtraction)))
                {
                    Operator complex_op = static_cast<Operator>(this->current().type);
                    this->advance();

                    if(!this->check(Token::Kind::Literal))
                    {
                        this->error_at_current("Expected imaginary literal in complex pattern");
                        return nullptr;
                    }

                    auto* imag = this->arena.emplace<ConstantNode>(
                        std::move(StringD::make_from_c_str(this->current().value.c_str(),
                                                           this->current().value.size())),
                        static_cast<Literal>(this->current().type),
                        this->current().line,
                        this->current().column);

                    this->advance();

                    Node* imag_node = imag;

                    if(complex_op == Operator::Subtraction)
                    {
                        imag_node = this->arena.emplace<UnaryOpNode>(Operator::Subtraction, imag,
                                                                     imag->line(), imag->column());
                    }

                    result = this->arena.emplace<BinOpNode>(result, Operator::Addition, imag_node,
                                                            line, column);
                }

                return this->arena.emplace<MatchValueNode>(result, line, column);
            }
        }

        // Sequence pattern: [p1, p2, *rest, p3]
        if(this->match_delimiter(Delimiter::LBracket))
        {
            this->advance();

            auto* node = this->arena.emplace<MatchSequenceNode>(line, column);

            while(!this->at_end() && !this->match_delimiter(Delimiter::RBracket))
            {
                Node* pat = this->parse_match_pattern();

                if(pat == nullptr)
                    return nullptr;

                node->patterns.push_back(pat);

                if(this->match_delimiter(Delimiter::Comma))
                    this->advance();
            }

            if(!this->expect_delimiter(Delimiter::RBracket))
                return nullptr;

            return node;
        }

        // Mapping pattern: {key: pattern, **rest}
        if(this->match_delimiter(Delimiter::LBrace))
        {
            this->advance();

            auto* node = this->arena.emplace<MatchMappingNode>(line, column);

            while(!this->at_end() && !this->match_delimiter(Delimiter::RBrace))
            {
                // **rest capture
                if(this->match_operator(Operator::Exponentiation))
                {
                    this->advance();

                    if(!this->check(Token::Kind::Identifier))
                    {
                        this->error_at_current("Expected name after '**' in mapping pattern");
                        return nullptr;
                    }

                    node->rest = std::move(StringD::make_from_c_str(this->current().value.c_str(),
                                                                    this->current().value.size()));
                    this->advance();

                    if(this->match_delimiter(Delimiter::Comma))
                        this->advance();

                    continue;
                }

                Node* key = this->parse_match_primary_pattern();

                if(key == nullptr)
                    return nullptr;

                if(!this->expect_delimiter(Delimiter::Colon))
                    return nullptr;

                Node* pat = this->parse_match_pattern();

                if(pat == nullptr)
                    return nullptr;

                node->keys.push_back(key);
                node->patterns.push_back(pat);

                if(this->match_delimiter(Delimiter::Comma))
                    this->advance();
            }

            if(!this->expect_delimiter(Delimiter::RBrace))
                return nullptr;

            return node;
        }

        // Group pattern or sequence pattern: (pattern) or (p1, p2, ...)
        if(this->match_delimiter(Delimiter::LParen))
        {
            this->advance();

            if(this->match_delimiter(Delimiter::RParen))
            {
                this->advance();
                return this->arena.emplace<MatchSequenceNode>(line, column);
            }

            Node* first = this->parse_match_pattern();

            if(first == nullptr)
                return nullptr;

            // Sequence: (p1, p2, ...)
            if(this->match_delimiter(Delimiter::Comma))
            {
                auto* seq = this->arena.emplace<MatchSequenceNode>(line, column);
                seq->patterns.push_back(first);

                while(this->match_delimiter(Delimiter::Comma))
                {
                    this->advance();

                    if(this->match_delimiter(Delimiter::RParen))
                        break;

                    Node* pat = this->parse_match_pattern();

                    if(pat == nullptr)
                        return nullptr;

                    seq->patterns.push_back(pat);
                }

                if(!this->expect_delimiter(Delimiter::RParen))
                    return nullptr;

                return seq;
            }

            // Group: (pattern)
            if(!this->expect_delimiter(Delimiter::RParen))
                return nullptr;

            return first;
        }

        // Identifier: capture pattern, value pattern (dotted), or class pattern
        if(this->check(Token::Kind::Identifier))
        {
            StringD name = std::move(StringD::make_from_c_str(this->current().value.c_str(),
                                                              this->current().value.size()));
            this->advance();

            // Dotted name: Color.RED, module.Class.CONST
            if(this->match_delimiter(Delimiter::Dot))
            {
                auto* attr_node = this->arena.emplace<NameNode>(std::move(name), line, column);
                Node* dotted = attr_node;

                while(this->match_delimiter(Delimiter::Dot))
                {
                    this->advance();

                    if(!this->check(Token::Kind::Identifier))
                    {
                        this->error_at_current("Expected attribute name after '.'");
                        return nullptr;
                    }

                    StringD attr = std::move(StringD::make_from_c_str(this->current().value.c_str(),
                                                                      this->current().value.size()));
                    this->advance();

                    dotted = this->arena.emplace<AttributeNode>(dotted, std::move(attr),
                                                                dotted->line(), dotted->column());
                }

                // Class pattern with dotted name: module.ClassName(p1, attr=p2)
                if(this->match_delimiter(Delimiter::LParen))
                    return this->parse_match_class_pattern(dotted, line, column);

                return this->arena.emplace<MatchValueNode>(dotted, line, column);
            }

            // Class pattern: ClassName(p1, p2, attr=p3)
            if(this->match_delimiter(Delimiter::LParen))
            {
                auto* name_node = this->arena.emplace<NameNode>(std::move(name), line, column);
                return this->parse_match_class_pattern(name_node, line, column);
            }

            // Simple capture pattern: name
            return this->arena.emplace<MatchAsNode>(nullptr, std::move(name), line, column);
        }

        this->error_at_current("Unexpected token \"{}\" ({}) in pattern",
                               this->current().value,
                               this->current().kind);

        return nullptr;
    }

    Node* parse_match_class_pattern(Node* cls, std::uint32_t line, std::uint32_t column) noexcept
    {
        this->advance(); // skip '('

        auto* node = this->arena.emplace<MatchClassNode>(cls, line, column);

        while(!this->at_end() && !this->match_delimiter(Delimiter::RParen))
        {
            // Check for keyword pattern: attr=pattern
            if(this->check(Token::Kind::Identifier) &&
               this->pos + 1 < this->tokens.size() &&
               this->tokens[this->pos + 1].kind == Token::Kind::Operator &&
               static_cast<Operator>(this->tokens[this->pos + 1].type) == Operator::Assign)
            {
                StringD attr = std::move(StringD::make_from_c_str(this->current().value.c_str(),
                                                                  this->current().value.size()));
                this->advance(2); // skip name and '='

                Node* pat = this->parse_match_pattern();

                if(pat == nullptr)
                    return nullptr;

                node->kwd_attrs.push_back(std::move(attr));
                node->kwd_patterns.push_back(pat);
            }
            else
            {
                // Positional pattern
                Node* pat = this->parse_match_pattern();

                if(pat == nullptr)
                    return nullptr;

                node->patterns.push_back(pat);
            }

            if(this->match_delimiter(Delimiter::Comma))
                this->advance();
        }

        if(!this->expect_delimiter(Delimiter::RParen))
            return nullptr;

        return node;
    }

    // Comprehension clauses

    bool parse_comp_clauses(Vector<Node*>& generators) noexcept
    {
        while(!this->at_end() && (this->match_keyword(Keyword::For) || this->match_keyword(Keyword::Async)))
        {
            std::uint32_t comp_line = this->current().line;
            std::uint32_t comp_column = this->current().column;

            bool is_async = false;

            if(this->match_keyword(Keyword::Async))
            {
                this->advance(); // skip 'async' and expect 'for'

                if(!this->expect_keyword(Keyword::For))
                    return false;

                is_async = true;
            }
            else
            {
                this->advance(); // skip 'for'
            }

            Node* target = this->parse_primary();

            if(target == nullptr)
                return false;

            if(this->match_delimiter(Delimiter::Comma))
            {
                auto* tup = this->arena.emplace<TupleNode>(target->line(), target->column());
                tup->elts.push_back(target);

                while(this->match_delimiter(Delimiter::Comma))
                {
                    this->advance();

                    if(this->match_keyword(Keyword::In))
                        break;

                    Node* elt = this->parse_primary();

                    if(elt == nullptr)
                        return false;

                    tup->elts.push_back(elt);
                }

                target = tup;
            }

            if(!this->expect_keyword(Keyword::In))
                return false;

            // Use parse_or_expr() to avoid consuming 'if' or 'for' keywords
            Node* iter = this->parse_or_expr();

            if(iter == nullptr)
                return false;

            Vector<Node*> ifs;

            // Zero or more 'if' filters
            while(!this->at_end() && this->match_keyword(Keyword::If))
            {
                this->advance();

                Node* test = this->parse_or_expr();

                if(test == nullptr)
                    return false;

                ifs.push_back(test);
            }

            if(is_async)
            {
                auto* comp = this->arena.emplace<AsyncComprehensionNode>(target, iter, comp_line, comp_column);
                comp->ifs = std::move(ifs);
                generators.push_back(comp);
            }
            else
            {
                auto* comp = this->arena.emplace<ComprehensionNode>(target, iter, comp_line, comp_column);
                comp->ifs = std::move(ifs);
                generators.push_back(comp);
            }

        }

        return !generators.empty();
    }

    // Expression or assignment

    Node* parse_expr_or_assign() noexcept
    {
        std::uint32_t line = this->at_end() ? 0 : this->current().line;
        std::uint32_t column = this->at_end() ? 0 : this->current().column;

        Node* left = this->parse_expr();

        if(left == nullptr)
            return nullptr;

        // Type annotation
        if(this->match_delimiter(Delimiter::Colon) && !(this->peek().kind == Token::Kind::Newline))
        {
            this->advance();

            Node* annotation = this->parse_expr();

            if(annotation == nullptr)
                return nullptr;

            Node* value = nullptr;

            if(this->match_operator(Operator::Assign))
            {
                this->advance();

                value = this->parse_expr();

                if(value == nullptr)
                    return nullptr;

                // Implicit tuple: x: tuple = 1, 2, 3
                if(this->match_delimiter(Delimiter::Comma))
                {
                    auto* tup = this->arena.emplace<TupleNode>(value->line(), value->column());
                    tup->elts.push_back(value);

                    while(this->match_delimiter(Delimiter::Comma))
                    {
                        this->advance();

                        if(this->at_end() || this->check(Token::Kind::Newline))
                            break;

                        Node* elt = this->parse_expr();

                        if(elt == nullptr)
                            return nullptr;

                        tup->elts.push_back(elt);
                    }

                    value = tup;
                }
            }

            return  this->arena.emplace<AnnAssignNode>(left, value, annotation, line, column);
        }

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

            // Implicit tuple: x = 1, 2, 3
            if(this->match_delimiter(Delimiter::Comma))
            {
                auto* tup = this->arena.emplace<TupleNode>(value->line(), value->column());
                tup->elts.push_back(value);

                while(this->match_delimiter(Delimiter::Comma))
                {
                    this->advance();

                    if(this->at_end() || this->check(Token::Kind::Newline))
                        break;

                    Node* elt = this->parse_expr();

                    if(elt == nullptr)
                        return nullptr;

                    tup->elts.push_back(elt);
                }

                value = tup;
            }

            auto* node = this->arena.emplace<AssignNode>(value, line, column);

            node->targets = std::move(targets);

            return node;
        }

        // Multiple assignment: x, y, z = 1, 2, 3
        // Also handles: first, *rest = [1, 2, 3, 4]
        if(this->match_delimiter(Delimiter::Comma))
        {
            // Parse remaining targets
            while(this->match_delimiter(Delimiter::Comma))
            {
                this->advance(); // skip ','

                if(this->match_operator(Operator::Assign))
                    break;

                Node* target = this->parse_expr();

                if(target == nullptr)
                    return nullptr;

                targets.push_back(target);
            }

            // We must be at '=' now
            if(!this->match_operator(Operator::Assign))
            {
                this->error_at_current("Expected '=' in multi-assignment, got \"{}\"",
                                       this->current().value);

                return nullptr;
            }

            this->advance(); // skip '='

            // Parse values
            Vector<Node*> values;

            while(!this->at_end() && !this->check(Token::Kind::Newline))
            {
                Node* value = this->parse_expr();

                if(value == nullptr)
                    return nullptr;

                values.push_back(value);

                if(!this->match_delimiter(Delimiter::Comma))
                    break;

                this->advance(); // skip ','
            }

            if(values.empty())
            {
                this->error_at_current("Expected values in multi-assignment");
                return nullptr;
            }

            auto* assign = this->arena.emplace<MultiAssignNode>(line, column);
            assign->targets = std::move(targets);
            assign->values = std::move(values);

            return assign;
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

        // Standalone expression or implicit tuple
        if(this->match_delimiter(Delimiter::Comma))
        {
            auto* tup = this->arena.emplace<TupleNode>(left->line(), left->column());
            tup->elts.push_back(left);

            while(this->match_delimiter(Delimiter::Comma))
            {
                this->advance();

                if(this->at_end() || this->check(Token::Kind::Newline))
                    break;

                Node* elt = this->parse_expr();

                if(elt == nullptr)
                    return nullptr;

                tup->elts.push_back(elt);
            }

            return this->arena.emplace<ExprNode>(tup, line, column);
        }

        return this->arena.emplace<ExprNode>(left, line, column);
    }

    // Expressions (precedence climbing)

    Node* parse_expr() noexcept
    {
        if(!this->at_end() && this->match_keyword(Keyword::Lambda))
            return this->parse_lambda();

        if(!this->at_end() && this->match_keyword(Keyword::Yield))
              return this->parse_yield();

        Node* left = this->parse_or_expr();

        if(left == nullptr)
            return nullptr;

        // Ternary: body if test else orelse
        if(!this->at_end() && this->match_keyword(Keyword::If))
        {
            this->advance();

            // Condition is or_expr to avoid consuming a nested 'else'
            Node* test = this->parse_or_expr();

            if(test == nullptr)
                return nullptr;

            if(!this->expect_keyword(Keyword::Else))
                return nullptr;

            // Else branch is full parse_expr() to allow right-nesting:
            // a if b else c if d else e  →  a if b else (c if d else e)
            Node* orelse = this->parse_expr();

            if(orelse == nullptr)
                return nullptr;

            left = this->arena.emplace<TernaryOpNode>(left, test, orelse, left->line(), left->column());
        }

        // Walrus operator (:=)
        if(!this->at_end() && this->current().kind == Token::Kind::Operator)
        {
            Operator op = static_cast<Operator>(this->current().type);

            if(op == Operator::WalrusAssign)
            {
                this->advance();

                Node* value = this->parse_expr();

                if(value == nullptr)
                    return nullptr;

                return this->arena.emplace<WalrusAssignNode>(left, value, left->line(), left->column());
            }
        }

        return left;
    }

    // Lambda

    Node* parse_lambda() noexcept
    {
        std::uint32_t line = this->current().line;
        std::uint32_t column = this->current().column;
        this->advance(); // skip 'lambda'

        auto* node = this->arena.emplace<LambdaNode>(nullptr, line, column);

        // No-argument lambda: lambda: expr
        if(!this->match_delimiter(Delimiter::Colon))
        {
            if(!this->parse_func_params(node->args, node->posonly_index, node->kwonly_index, Delimiter::Colon))
                return nullptr;
        }

        if(!this->expect_delimiter(Delimiter::Colon))
            return nullptr;

        node->body = this->parse_expr();

        if(node->body == nullptr)
            return nullptr;

        return node;
    }

    // Yield

    Node* parse_yield() noexcept
    {
        std::uint32_t line = this->current().line;
        std::uint32_t column = this->current().column;
        this->advance(); // skip 'yield'

        // yield from expr
        if(!this->at_end() && this->match_keyword(Keyword::From))
        {
            this->advance();

            Node* value = this->parse_expr();

            if(value == nullptr)
                return nullptr;

            return this->arena.emplace<YieldFromNode>(value, line, column);
        }

        // Bare yield (no value)
        if(this->at_end() ||
           this->check(Token::Kind::Newline) ||
           this->match_delimiter(Delimiter::RParen) ||
           this->match_delimiter(Delimiter::RBracket) ||
           this->match_delimiter(Delimiter::RBrace))
        {
            return this->arena.emplace<YieldNode>(nullptr, line, column);
        }

        // yield expr
        Node* value = this->parse_expr();

        if(value == nullptr)
            return nullptr;

        // Implicit tuple: yield 1, 2, 3
        if(this->match_delimiter(Delimiter::Comma))
        {
            auto* tup = this->arena.emplace<TupleNode>(value->line(), value->column());
            tup->elts.push_back(value);

            while(this->match_delimiter(Delimiter::Comma))
            {
                this->advance();

                if(this->at_end() ||
                   this->check(Token::Kind::Newline) ||
                   this->match_delimiter(Delimiter::RParen) ||
                   this->match_delimiter(Delimiter::RBracket) ||
                   this->match_delimiter(Delimiter::RBrace))
                {
                    break;
                }

                Node* elt = this->parse_expr();

                if(elt == nullptr)
                    return nullptr;

                tup->elts.push_back(elt);
            }

            value = tup;
        }

        return this->arena.emplace<YieldNode>(value, line, column);
    }

    Node* parse_or_expr() noexcept
    {
        Node* left = this->parse_and_expr();

        if(left == nullptr)
            return nullptr;

        while(!this->at_end() && this->match_keyword(Keyword::Or))
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

        while(!this->at_end() && this->match_keyword(Keyword::And))
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
        if(!this->at_end() && this->match_keyword(Keyword::Not))
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

        // Helper lambda: check if current position is a comparison op,
        // return the Operator enum and how many tokens to consume
        auto match_cmp_op = [this](Operator& out_op, std::uint32_t& out_advance) -> bool
        {
            if(this->at_end())
                return false;

            // Operator-based comparators: ==, !=, <, >, <=, >=
            if(this->current().kind == Token::Kind::Operator)
            {
                Operator op = static_cast<Operator>(this->current().type);

                if(op >= Operator::ComparatorEquals && op <= Operator::ComparatorLessEqualsThan)
                {
                    out_op = op;
                    out_advance = 1;
                    return true;
                }
            }

            // Keyword-based: is, is not, in, not in
            if(this->match_keyword(Keyword::Is))
            {
                // 'is not'
                if(this->pos + 1 < this->tokens.size() &&
                   this->tokens[this->pos + 1].kind == Token::Kind::Keyword &&
                   static_cast<Keyword>(this->tokens[this->pos + 1].type) == Keyword::Not)
                {
                    out_op = Operator::IdentityIsNot;
                    out_advance = 2;
                    return true;
                }

                out_op = Operator::IdentityIs;
                out_advance = 1;
                return true;
            }

            if(this->match_keyword(Keyword::In))
            {
                out_op = Operator::MembershipIn;
                out_advance = 1;
                return true;
            }

            if(this->match_keyword(Keyword::Not))
            {
                // 'not in'
                if(this->pos + 1 < this->tokens.size() &&
                   this->tokens[this->pos + 1].kind == Token::Kind::Keyword &&
                   static_cast<Keyword>(this->tokens[this->pos + 1].type) == Keyword::In)
                {
                    out_op = Operator::MembershipNotIn;
                    out_advance = 2;
                    return true;
                }

                return false;
            }

            return false;
        };

        Operator op;
        std::uint32_t skip;

        if(!match_cmp_op(op, skip))
            return left;

        auto* node = arena.emplace<CompareOp>(left, left->line(), left->column());

        while(match_cmp_op(op, skip))
        {
            node->ops.push_back(op);
            this->advance(skip);

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
               this->match_operator(Operator::FloorDivision) ||
               this->match_operator(Operator::MatMul)))
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
        Node* left = this->parse_await();

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

    // Await

    Node* parse_await() noexcept
    {
        if(this->match_keyword(Keyword::Await))
        {
            std::uint32_t line = this->current().line;
            std::uint32_t column = this->current().column;

            this->advance(); // skip 'await'

            // Recursive call as python allows await await <expr>
            Node* expr = this->parse_await();

            return this->arena.emplace<AwaitNode>(expr, line, column);
        }
        else
        {
            return this->parse_postfix();
        }
    }

    // List slice

    Node* parse_slice_expr() noexcept
    {
        std::uint32_t line = this->current().line;
        std::uint32_t column = this->current().column;

        Node* lower = nullptr;
        Node* upper = nullptr;
        Node* step = nullptr;

        // Lower bound (optional)
        if(!this->match_delimiter(Delimiter::Colon))
        {
            lower = this->parse_expr();

            if(lower == nullptr)
                return nullptr;
        }

        // If no colon follows, it's a regular subscript, not a slice
        if(!this->match_delimiter(Delimiter::Colon))
            return lower;

        this->advance(); // skip first ':'

        // Upper bound (optional)
        if(!this->match_delimiter(Delimiter::Colon) &&
           !this->match_delimiter(Delimiter::RBracket) &&
           !this->match_delimiter(Delimiter::Comma))
        {
            upper = this->parse_expr();

            if(upper == nullptr)
                return nullptr;
        }

        // Step (optional, requires second ':')
        if(this->match_delimiter(Delimiter::Colon))
        {
            this->advance(); // skip second ':'

            if(!this->match_delimiter(Delimiter::RBracket) &&
               !this->match_delimiter(Delimiter::Comma))
            {
                step = this->parse_expr();

                if(step == nullptr)
                    return nullptr;
            }
        }

        return this->arena.emplace<SliceNode>(lower, upper, step,
                                              lower ? lower->line() : line,
                                              lower ? lower->column() : column);
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
                    this->skip_whitespace_in_bracket();

                    if(this->match_delimiter(Delimiter::RParen))
                        break;

                    // **expr unpacking in call
                    if(this->match_operator(Operator::Exponentiation))
                    {
                        std::uint32_t kw_line = this->current().line;
                        std::uint32_t kw_column = this->current().column;
                        this->advance();

                        Node* value = this->parse_expr();

                        if(value == nullptr)
                            return nullptr;

                        auto* kwarg = this->arena.emplace<KeywordArgNode>(StringD(), value, kw_line, kw_column);
                        call->args.push_back(kwarg);
                    }
                    else
                    {
                        Node* arg = this->parse_expr();

                        if(arg == nullptr)
                            return nullptr;

                        // keyword argument: name=value
                        if(arg->type() == ASTNodeName && this->match_operator(Operator::Assign))
                        {
                            std::uint32_t kw_line = arg->line();
                            std::uint32_t kw_column = arg->column();
                            StringD kw_name = std::move(static_cast<NameNode*>(arg)->id);
                            this->advance();

                            Node* value = this->parse_expr();

                            if(value == nullptr)
                                return nullptr;

                            arg = this->arena.emplace<KeywordArgNode>(std::move(kw_name), value, kw_line, kw_column);
                        }

                        // Generator expression as call argument
                        if(this->match_keyword(Keyword::For) || this->match_keyword(Keyword::For))
                        {
                            auto* genexpr = this->arena.emplace<GeneratorExprNode>(arg, arg->line(), arg->column());

                            if(!this->parse_comp_clauses(genexpr->generators))
                                return nullptr;

                            arg = genexpr;
                        }

                        call->args.push_back(arg);
                    }

                    if(this->match_delimiter(Delimiter::Comma))
                        this->advance();

                    this->skip_whitespace_in_bracket();
                }

                if(!this->expect_delimiter(Delimiter::RParen))
                    return nullptr;

                node = call;
            }
            else if(this->match_delimiter(Delimiter::LBracket))
            {
                this->advance();
                this->skip_whitespace_in_bracket();

                Node* first = this->parse_slice_expr();

                if(first == nullptr)
                    return nullptr;

                // Multiple subscript items: dict[str, int], tuple[int, ...]
                if(this->match_delimiter(Delimiter::Comma))
                {
                    auto* tup = this->arena.emplace<TupleNode>(first->line(), first->column());
                    tup->elts.push_back(first);

                    while(this->match_delimiter(Delimiter::Comma))
                    {
                        this->advance();
                        this->skip_whitespace_in_bracket();

                        if(this->match_delimiter(Delimiter::RBracket))
                            break;

                        Node* elt = this->parse_slice_expr();

                        if(elt == nullptr)
                            return nullptr;

                        tup->elts.push_back(elt);
                    }

                    this->skip_whitespace_in_bracket();

                    if(!this->expect_delimiter(Delimiter::RBracket))
                        return nullptr;

                    node = this->arena.emplace<SubscriptNode>(node, tup, node->line(), node->column());
                }
                else
                {
                    this->skip_whitespace_in_bracket();

                    if(!this->expect_delimiter(Delimiter::RBracket))
                        return nullptr;

                    node = this->arena.emplace<SubscriptNode>(node, first, node->line(), node->column());
                }
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

    // Primary: atoms and literals

    Node* parse_paren_expr() noexcept
    {
        this->advance(); // skip '('

        this->skip_whitespace_in_bracket();

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

        // Generator expression
        if(this->match_keyword(Keyword::For) || this->match_keyword(Keyword::Async))
        {
            auto* node = this->arena.emplace<GeneratorExprNode>(first, first->line(), first->column());

            if(!this->parse_comp_clauses(node->generators))
                return nullptr;

            if(!this->expect_delimiter(Delimiter::RParen))
                return nullptr;

            return node;
        }

        // Tuple with comma
        if(this->match_delimiter(Delimiter::Comma))
        {
            auto* tup = this->arena.emplace<TupleNode>(first->line(), first->column());
            tup->elts.push_back(first);

            while(this->match_delimiter(Delimiter::Comma))
            {
                this->advance();
                this->skip_whitespace_in_bracket();

                if(this->match_delimiter(Delimiter::RParen))
                    break;

                Node* elt = this->parse_expr();

                if(elt == nullptr)
                    return nullptr;

                tup->elts.push_back(elt);
            }

            this->skip_whitespace_in_bracket();

            if(!this->expect_delimiter(Delimiter::RParen))
                return nullptr;

            return tup;
        }

        // String concatenation across lines
        // s = ("concat"
        //      "across"
        //      "lines")
        else if(first->type() == ASTNodeConstant)
        {
            auto* literal = static_cast<ConstantNode*>(first);

            if(literal->literal_type <= Literal::Bytes)
            {
                // Peek ahead past whitespace to see if next token is a string literal
                std::uint32_t saved_pos = this->pos;
                this->skip_whitespace_in_bracket();
                bool is_concat = this->check(Token::Kind::Literal) &&
                                 static_cast<Literal>(this->current().type) <= Literal::FormattedString;
                this->pos = saved_pos;

                if(is_concat)
                {
                    while(!this->match_delimiter(Delimiter::RParen))
                    {
                        this->skip_whitespace_in_bracket();

                        if(this->match_delimiter(Delimiter::RParen))
                            break;

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

                    if(!this->expect_delimiter(Delimiter::RParen))
                        return nullptr;

                    return literal;
                }
            }

            // Not string concatenation, fall through to normal close
            this->skip_whitespace_in_bracket();

            // Allow postfix chaining inside parens (multiline method chaining)
            while(!this->at_end())
            {
                this->skip_whitespace_in_bracket();

                if(this->match_delimiter(Delimiter::RParen))
                    break;

                if(this->match_delimiter(Delimiter::Dot))
                {
                    this->advance();

                    if(!this->check(Token::Kind::Identifier))
                    {
                        this->error_at_current("Expected attribute name");
                        return nullptr;
                    }

                    StringD attr = std::move(StringD::make_from_c_str(this->current().value.c_str(),
                                                                      this->current().value.size()));
                    this->advance();

                    first = this->arena.emplace<AttributeNode>(first, std::move(attr), first->line(), first->column());
                }
                else if(this->match_delimiter(Delimiter::LParen))
                {
                    this->advance();

                    auto* call = this->arena.emplace<CallNode>(first, first->line(), first->column());

                    while(!this->at_end() && !this->match_delimiter(Delimiter::RParen))
                    {
                        this->skip_whitespace_in_bracket();

                        if(this->match_delimiter(Delimiter::RParen))
                            break;

                        if(this->match_operator(Operator::Exponentiation))
                        {
                            std::uint32_t kw_line = this->current().line;
                            std::uint32_t kw_column = this->current().column;
                            this->advance();

                            Node* value = this->parse_expr();

                            if(value == nullptr)
                                return nullptr;

                            call->args.push_back(this->arena.emplace<KeywordArgNode>(StringD(), value, kw_line, kw_column));
                        }
                        else
                        {
                            Node* arg = this->parse_expr();

                            if(arg == nullptr)
                                return nullptr;

                            if(arg->type() == ASTNodeName && this->match_operator(Operator::Assign))
                            {
                                std::uint32_t kw_line = arg->line();
                                std::uint32_t kw_column = arg->column();
                                StringD kw_name = std::move(static_cast<NameNode*>(arg)->id);
                                this->advance();

                                Node* value = this->parse_expr();

                                if(value == nullptr)
                                    return nullptr;

                                arg = this->arena.emplace<KeywordArgNode>(std::move(kw_name), value, kw_line, kw_column);
                            }

                            call->args.push_back(arg);
                        }

                        if(this->match_delimiter(Delimiter::Comma))
                            this->advance();

                        this->skip_whitespace_in_bracket();
                    }

                    if(!this->expect_delimiter(Delimiter::RParen))
                        return nullptr;

                    first = call;
                }
                else if(this->match_delimiter(Delimiter::LBracket))
                {
                    this->advance();
                    this->skip_whitespace_in_bracket();

                    Node* slice = this->parse_slice_expr();

                    if(slice == nullptr)
                        return nullptr;

                    this->skip_whitespace_in_bracket();

                    if(!this->expect_delimiter(Delimiter::RBracket))
                        return nullptr;

                    first = this->arena.emplace<SubscriptNode>(first, slice, first->line(), first->column());
                }
                else
                {
                    break;
                }
            }

            if(!this->expect_delimiter(Delimiter::RParen))
                return nullptr;

            return first;
        }

        this->skip_whitespace_in_bracket();

        if(!this->expect_delimiter(Delimiter::RParen))
            return nullptr;

        return first;
    }

    Node* parse_list_literal() noexcept
    {
        std::uint32_t line = this->current().line;
        std::uint32_t column = this->current().column;
        this->advance(); // skip '['

        this->skip_whitespace_in_bracket();

        // Empty list
        if(this->match_delimiter(Delimiter::RBracket))
        {
            auto* node = this->arena.emplace<ListNode>(line, column);
            this->advance();
            return node;
        }

        Node* first = this->parse_expr();

        if(first == nullptr)
            return nullptr;

        // List comprehension
        if(this->match_keyword(Keyword::For) || this->match_keyword(Keyword::Async))
        {
            auto* node = this->arena.emplace<ListCompNode>(first, line, column);

            if(!this->parse_comp_clauses(node->generators))
                return nullptr;

            if(!this->expect_delimiter(Delimiter::RBracket))
                return nullptr;

            return node;
        }

        // Regular list literal
        auto* node = this->arena.emplace<ListNode>(line, column);
        node->elts.push_back(first);

        while(this->match_delimiter(Delimiter::Comma))
        {
            this->advance();
            this->skip_whitespace_in_bracket();

            if(this->match_delimiter(Delimiter::RBracket))
                break;

            Node* elt = this->parse_expr();

            if(elt == nullptr)
                return nullptr;

            node->elts.push_back(elt);
        }

        this->skip_whitespace_in_bracket();

        if(!this->expect_delimiter(Delimiter::RBracket))
            return nullptr;

        return node;
    }

    Node* parse_dict_or_set_literal() noexcept
    {
        std::uint32_t line = this->current().line;
        std::uint32_t column = this->current().column;
        this->advance(); // skip '{'

        this->skip_whitespace_in_bracket();

        // Empty dict: {}
        if(this->match_delimiter(Delimiter::RBrace))
        {
            auto* node = this->arena.emplace<DictNode>(line, column);
            this->advance();
            return node;
        }

        // Dict starting with **expr unpacking
        if(this->match_operator(Operator::Exponentiation))
            return this->parse_dict_unpack_first(line, column);

        Node* first = this->parse_expr();

        if(first == nullptr)
            return nullptr;

        // Set comprehension
        if(this->match_keyword(Keyword::For) || this->match_keyword(Keyword::Async))
            return this->parse_set_comp(first, line, column);

        // Dict path
        if(this->match_delimiter(Delimiter::Colon))
            return this->parse_dict_after_first_key(first, line, column);

        // Set literal
        return this->parse_set_literal(first, line, column);
    }

    Node* parse_dict_unpack_first(std::uint32_t line, std::uint32_t column) noexcept
    {
        this->advance(); // skip '**'

        Node* val = this->parse_expr();

        if(val == nullptr)
            return nullptr;

        auto* node = this->arena.emplace<DictNode>(line, column);
        node->keys.push_back(nullptr);
        node->values.push_back(val);

        return this->parse_dict_remaining(node);
    }

    Node* parse_dict_after_first_key(Node* first_key, std::uint32_t line, std::uint32_t column) noexcept
    {
        this->advance(); // skip ':'

        Node* value = this->parse_expr();

        if(value == nullptr)
            return nullptr;

        // Dict comprehension
        if(this->match_keyword(Keyword::For) || this->match_keyword(Keyword::Async))
        {
            auto* node = this->arena.emplace<DictCompNode>(first_key, value, line, column);

            if(!this->parse_comp_clauses(node->generators))
                return nullptr;

            this->skip_whitespace_in_bracket();

            if(!this->expect_delimiter(Delimiter::RBrace))
                return nullptr;

            return node;
        }

        // Regular dict literal
        auto* node = this->arena.emplace<DictNode>(line, column);
        node->keys.push_back(first_key);
        node->values.push_back(value);

        return this->parse_dict_remaining(node);
    }

    Node* parse_dict_remaining(DictNode* node) noexcept
    {
        while(this->match_delimiter(Delimiter::Comma))
        {
            this->advance();
            this->skip_whitespace_in_bracket();

            if(this->match_delimiter(Delimiter::RBrace))
                break;

            // Dict unpacking: **expr
            if(this->match_operator(Operator::Exponentiation))
            {
                this->advance();

                Node* val = this->parse_expr();

                if(val == nullptr)
                    return nullptr;

                node->keys.push_back(nullptr);
                node->values.push_back(val);
                continue;
            }

            Node* key = this->parse_expr();

            if(key == nullptr)
                return nullptr;

            if(!this->expect_delimiter(Delimiter::Colon))
                return nullptr;

            Node* val = this->parse_expr();

            if(val == nullptr)
                return nullptr;

            node->keys.push_back(key);
            node->values.push_back(val);
        }

        this->skip_whitespace_in_bracket();

        if(!this->expect_delimiter(Delimiter::RBrace))
            return nullptr;

        return node;
    }

    Node* parse_set_comp(Node* first, std::uint32_t line, std::uint32_t column) noexcept
    {
        auto* node = this->arena.emplace<SetCompNode>(first, line, column);

        if(!this->parse_comp_clauses(node->generators))
            return nullptr;

        this->skip_whitespace_in_bracket();

        if(!this->expect_delimiter(Delimiter::RBrace))
            return nullptr;

        return node;
    }

    Node* parse_set_literal(Node* first, std::uint32_t line, std::uint32_t column) noexcept
    {
        auto* node = this->arena.emplace<SetNode>(line, column);
        node->elts.push_back(first);

        while(this->match_delimiter(Delimiter::Comma))
        {
            this->advance();
            this->skip_whitespace_in_bracket();

            if(this->match_delimiter(Delimiter::RBrace))
                break;

            Node* elt = this->parse_expr();

            if(elt == nullptr)
                return nullptr;

            node->elts.push_back(elt);
        }

        this->skip_whitespace_in_bracket();

        if(!this->expect_delimiter(Delimiter::RBrace))
            return nullptr;

        return node;
    }

    Node* parse_primary() noexcept
    {
        if(this->at_end())
        {
            this->error((this->tokens.end() - 1)->line,
                        (this->tokens.end() - 1)->column,
                        static_cast<std::uint32_t>((this->tokens.end() - 1)->value.size()),
                        "Unexpected end of input");

            return nullptr;
        }

        // Star expression: *expr
        if(this->match_operator(Operator::Multiplication))
        {
            std::uint32_t star_line = this->current().line;
            std::uint32_t star_column = this->current().column;
            this->advance();

            Node* value = this->parse_expr();

            if(value == nullptr)
                return nullptr;

            return this->arena.emplace<StarredNode>(value, star_line, star_column);
        }

        // Identifier
        if(this->check(Token::Kind::Identifier))
        {
            auto* node = this->arena.emplace<NameNode>(
                std::move(StringD::make_from_c_str(this->current().value.c_str(),
                                                   this->current().value.size())),
                this->current().line,
                this->current().column);

            this->advance();

            return node;
        }

        // Literals
        if(this->check(Token::Kind::Literal))
        {
            Literal lit = static_cast<Literal>(this->current().type);

            auto* node = this->arena.emplace<ConstantNode>(
                std::move(StringD::make_from_c_str(this->current().value.c_str(),
                                                   this->current().value.size())),
                lit,
                this->current().line,
                this->current().column);

            this->advance();

            return node;
        }

        // True / False / None
        if(this->match_keyword(Keyword::True) ||
           this->match_keyword(Keyword::False) ||
           this->match_keyword(Keyword::None))
        {
            auto* node = this->arena.emplace<ConstantNode>(
                std::move(StringD::make_from_c_str(this->current().value.c_str(),
                                                   this->current().value.size())),
                Literal::String,
                this->current().line,
                this->current().column);

            this->advance();

            return node;
        }

        // Parenthesized expression / tuple / generator / string concat
        if(this->match_delimiter(Delimiter::LParen))
            return this->parse_paren_expr();

        // List literal / list comprehension
        if(this->match_delimiter(Delimiter::LBracket))
            return this->parse_list_literal();

        // Dict / set literal / comprehension
        if(this->match_delimiter(Delimiter::LBrace))
            return this->parse_dict_or_set_literal();

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

bool AST::parse(const StringD& source_code, const Vector<Token>& tokens, bool debug) noexcept
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

        if(debug)
        {
            visit(stmt, [&](Node* node, std::uint32_t depth) -> bool {
                node->debug(this->_logger, depth * 2);
                return true;
            });
        }

        this->_root->body.push_back(stmt);

        parser.skip_semicolons();
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

    if(!this->parse(source_code, tokens, debug))
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

void node_children(Node* node, Vector<Node*>& out) noexcept
{
    if(node == nullptr)
        return;

    switch(node->type())
    {
        case ASTNodeModule:
        {
            auto* n = static_cast<ModuleNode*>(node);
            for(auto* c : n->body) out.push_back(c);
            break;
        }
        case ASTNodeDecorator:
        {
            auto* n = static_cast<DecoratorNode*>(node);
            if(n->expr) out.push_back(n->expr);
            if(n->target) out.push_back(n->target);
            break;
        }
        case ASTNodeFunctionArg:
        {
            auto* n = static_cast<FunctionArgNode*>(node);
            if(n->annotation != nullptr) out.push_back(n->annotation);
            if(n->default_value != nullptr) out.push_back(n->default_value);
            break;
        }
        case ASTNodeFunctionDef:
        {
            auto* n = static_cast<FunctionDefNode*>(node);
            for(auto* c : n->args) out.push_back(c);
            for(auto* c : n->body) out.push_back(c);
            if(n->return_annotation) out.push_back(n->return_annotation);
            for(auto* c : n->type_params) out.push_back(c);
            break;
        }
        case ASTNodeAsyncFunctionDef:
        {
            auto* n = static_cast<AsyncFunctionDefNode*>(node);
            for(auto* c : n->args) out.push_back(c);
            for(auto* c : n->body) out.push_back(c);
            if(n->return_annotation) out.push_back(n->return_annotation);
            for(auto* c : n->type_params) out.push_back(c);
            break;
        }
        case ASTNodeClassDef:
        {
            auto* n = static_cast<ClassDefNode*>(node);
            for(auto* c : n->bases) out.push_back(c);
            for(auto* c : n->body) out.push_back(c);
            for(auto* c : n->type_params) out.push_back(c);
            break;
        }
        case ASTNodeReturn:
        {
            auto* n = static_cast<ReturnNode*>(node);
            if(n->value) out.push_back(n->value);
            break;
        }
        case ASTNodeAssign:
        {
            auto* n = static_cast<AssignNode*>(node);
            for(auto* c : n->targets) out.push_back(c);
            if(n->value) out.push_back(n->value);
            break;
        }
        case ASTNodeAnnAssign:
        {
            auto* n = static_cast<AnnAssignNode*>(node);
            if(n->target) out.push_back(n->target);
            if(n->value) out.push_back(n->value);
            if(n->ann) out.push_back(n->ann);
            break;
        }
        case ASTNodeMultiAssign:
        {
            auto* n = static_cast<MultiAssignNode*>(node);
            for(auto* t : n->targets) out.push_back(t);
            for(auto* v : n->values) out.push_back(v);
            break;
        }
        case ASTNodeAugAssign:
        {
            auto* n = static_cast<AugAssignNode*>(node);
            if(n->target) out.push_back(n->target);
            if(n->value) out.push_back(n->value);
            break;
        }
        case ASTNodeWalrusAssign:
        {
            auto* n = static_cast<WalrusAssignNode*>(node);
            if(n->target) out.push_back(n->target);
            if(n->value) out.push_back(n->value);
        }
        case ASTNodeFor:
        {
            auto* n = static_cast<ForNode*>(node);
            if(n->target) out.push_back(n->target);
            if(n->iter) out.push_back(n->iter);
            for(auto* c : n->body) out.push_back(c);
            for(auto* c : n->orelse) out.push_back(c);
            break;
        }
        case ASTNodeAsyncFor:
        {
            auto* n = static_cast<AsyncForNode*>(node);
            if(n->target) out.push_back(n->target);
            if(n->iter) out.push_back(n->iter);
            for(auto* c : n->body) out.push_back(c);
            for(auto* c : n->orelse) out.push_back(c);
            break;
        }
        case ASTNodeWhile:
        {
            auto* n = static_cast<WhileNode*>(node);
            if(n->test) out.push_back(n->test);
            for(auto* c : n->body) out.push_back(c);
            for(auto* c : n->orelse) out.push_back(c);
            break;
        }
        case ASTNodeIf:
        {
            auto* n = static_cast<IfNode*>(node);
            if(n->test) out.push_back(n->test);
            for(auto* c : n->body) out.push_back(c);
            for(auto* c : n->orelse) out.push_back(c);
            break;
        }
        case ASTNodeExpr:
        {
            auto* n = static_cast<ExprNode*>(node);
            if(n->value) out.push_back(n->value);
            break;
        }
        case ASTNodeRaise:
        {
            auto* n = static_cast<RaiseNode*>(node);
            if(n->exc) out.push_back(n->exc);
            if(n->cause) out.push_back(n->cause);
            break;
        }
        case ASTNodeTry:
        {
            auto* n = static_cast<TryNode*>(node);
            for(auto* c : n->body) out.push_back(c);
            for(auto* c : n->excepts) out.push_back(c);
            for(auto* c : n->orelse) out.push_back(c);
            for(auto* c : n->finalbody) out.push_back(c);
            break;
        }
        case ASTNodeExcept:
        {
            auto* n = static_cast<ExceptNode*>(node);
            if(n->type) out.push_back(n->type);
            for(auto* c : n->body) out.push_back(c);
            break;
        }
        case ASTNodeBinOp:
        {
            auto* n = static_cast<BinOpNode*>(node);
            if(n->left) out.push_back(n->left);
            if(n->right) out.push_back(n->right);
            break;
        }
        case ASTNodeUnaryOp:
        {
            auto* n = static_cast<UnaryOpNode*>(node);
            if(n->operand) out.push_back(n->operand);
            break;
        }
        case ASTNodeTernaryOp:
        {
            auto* n = static_cast<TernaryOpNode*>(node);
            if(n->body) out.push_back(n->body);
            if(n->test) out.push_back(n->test);
            if(n->orelse) out.push_back(n->orelse);
            break;
        }
        case ASTNodeBoolOp:
        {
            auto* n = static_cast<BoolOpNode*>(node);
            for(auto* c : n->values) out.push_back(c);
            break;
        }
        case ASTNodeCompareOp:
        {
            auto* n = static_cast<CompareOp*>(node);
            if(n->left) out.push_back(n->left);
            for(auto* c : n->comparators) out.push_back(c);
            break;
        }
        case ASTNodeKeywordArg:
        {
            auto* n = static_cast<KeywordArgNode*>(node);
            if(n->value) out.push_back(n->value);
            break;
        }
        case ASTNodeCall:
        {
            auto* n = static_cast<CallNode*>(node);
            if(n->func) out.push_back(n->func);
            for(auto* c : n->args) out.push_back(c);
            break;
        }
        case ASTNodeAttribute:
        {
            auto* n = static_cast<AttributeNode*>(node);
            if(n->value) out.push_back(n->value);
            break;
        }
        case ASTNodeSubscript:
        {
            auto* n = static_cast<SubscriptNode*>(node);
            if(n->value) out.push_back(n->value);
            if(n->slice) out.push_back(n->slice);
            break;
        }
        case ASTNodeStarred:
        {
            auto* n = static_cast<StarredNode*>(node);
            if(n->value) out.push_back(n->value);
            break;
        }
        case ASTNodeList:
        {
            auto* n = static_cast<ListNode*>(node);
            for(auto* c : n->elts) out.push_back(c);
            break;
        }
        case ASTNodeSet:
        {
            auto* n = static_cast<SetNode*>(node);
            for(auto* c : n->elts) out.push_back(c);
            break;
        }
        case ASTNodeTuple:
        {
            auto* n = static_cast<TupleNode*>(node);
            for(auto* c : n->elts) out.push_back(c);
            break;
        }
        case ASTNodeDict:
        {
            auto* n = static_cast<DictNode*>(node);
            for(auto* c : n->keys) out.push_back(c);
            for(auto* c : n->values) out.push_back(c);
            break;
        }
        case ASTNodeComprehension:
        {
            auto* n = static_cast<ComprehensionNode*>(node);
            if(n->target) out.push_back(n->target);
            if(n->iter) out.push_back(n->iter);
            for(auto* c : n->ifs) out.push_back(c);
            break;
        }
        case ASTNodeAsyncComprehension:
        {
            auto* n = static_cast<AsyncComprehensionNode*>(node);
            if(n->target) out.push_back(n->target);
            if(n->iter) out.push_back(n->iter);
            for(auto* c : n->ifs) out.push_back(c);
            break;
        }
        case ASTNodeListComp:
        {
            auto* n = static_cast<ListCompNode*>(node);
            if(n->elt) out.push_back(n->elt);
            for(auto* c : n->generators) out.push_back(c);
            break;
        }
        case ASTNodeSetComp:
        {
            auto* n = static_cast<SetCompNode*>(node);
            if(n->elt) out.push_back(n->elt);
            for(auto* c : n->generators) out.push_back(c);
            break;
        }
        case ASTNodeDictComp:
        {
            auto* n = static_cast<DictCompNode*>(node);
            if(n->key) out.push_back(n->key);
            if(n->value) out.push_back(n->value);
            for(auto* c : n->generators) out.push_back(c);
            break;
        }
        case ASTNodeGeneratorExpr:
        {
            auto* n = static_cast<GeneratorExprNode*>(node);
            if(n->elt) out.push_back(n->elt);
            for(auto* c : n->generators) out.push_back(c);
            break;
        }
        case ASTNodeLambda:
        {
            auto* n = static_cast<LambdaNode*>(node);
            for(auto* c : n->args) out.push_back(c);
            if(n->body) out.push_back(n->body);
            break;
        }
        case ASTNodeMatch:
        {
            auto* n = static_cast<MatchNode*>(node);
            if(n->subject) out.push_back(n->subject);
            for(auto* c : n->cases) out.push_back(c);
            break;
        }
        case ASTNodeMatchCase:
        {
            auto* n = static_cast<MatchCaseNode*>(node);
            if(n->pattern) out.push_back(n->pattern);
            if(n->guard) out.push_back(n->guard);
            for(auto* c : n->body) out.push_back(c);
            break;
        }
        case ASTNodeMatchOr:
        {
            auto* n = static_cast<MatchOrNode*>(node);
            for(auto* c : n->patterns) out.push_back(c);
            break;
        }
        case ASTNodeMatchAs:
        {
            auto* n = static_cast<MatchAsNode*>(node);
            if(n->pattern) out.push_back(n->pattern);
            break;
        }
        case ASTNodeMatchValue:
        {
            auto* n = static_cast<MatchValueNode*>(node);
            if(n->value) out.push_back(n->value);
            break;
        }
        case ASTNodeMatchSingleton:
        {
            auto* n = static_cast<MatchSingletonNode*>(node);
            STDROMANO_UNUSED(n);
            break;
        }
        case ASTNodeMatchSequence:
        {
            auto* n = static_cast<MatchSequenceNode*>(node);
            for(auto* c : n->patterns) out.push_back(c);
            break;
        }
        case ASTNodeMatchMapping:
        {
            auto* n = static_cast<MatchMappingNode*>(node);
            for(auto* c : n->keys) out.push_back(c);
            for(auto* c : n->patterns) out.push_back(c);
        }
        case ASTNodeMatchClass:
        {
            auto* n = static_cast<MatchClassNode*>(node);
            for(auto* c : n->patterns) out.push_back(c);
            for(auto* c : n->kwd_patterns) out.push_back(c);
            break;
        }
        case ASTNodeMatchStar:
        {
            auto* n = static_cast<MatchStarNode*>(node);
            STDROMANO_UNUSED(n);
            break;
        }
        case ASTNodeYield:
        {
            auto* n = static_cast<YieldNode*>(node);
            if(n->value) out.push_back(n->value);
            break;
        }
        case ASTNodeYieldFrom:
        {
            auto* n = static_cast<YieldFromNode*>(node);
            if(n->value) out.push_back(n->value);
            break;
        }
        case ASTNodeTypeParam:
        {
            auto* n = static_cast<TypeParamNode*>(node);
            if(n->bound) out.push_back(n->bound);
            if(n->constraint) out.push_back(n->constraint);
            break;
        }
        case ASTNodeTypeAlias:
        {
            auto* n = static_cast<TypeAliasNode*>(node);
            for(auto* c : n->type_params) out.push_back(c);
            if(n->value) out.push_back(n->value);
            break;
        }
        case ASTNodeGlobal:
        {
            auto* n = static_cast<GlobalNode*>(node);
            for(auto* c : n->names) out.push_back(c);
            break;
        }
        case ASTNodeNonLocal:
        {
            auto* n = static_cast<NonLocalNode*>(node);
            for(auto* c : n->names) out.push_back(c);
            break;
        }
        case ASTNodeDel:
        {
            auto* n = static_cast<DelNode*>(node);
            for(auto* c : n->names) out.push_back(c);
            break;
        }
        case ASTNodeAwait:
        {
            auto* n = static_cast<AwaitNode*>(node);
            if(n->expr != nullptr) out.push_back(n->expr);
            break;
        }
        case ASTNodeWithItem:
        {
            auto* n = static_cast<WithItemNode*>(node);
            if(n->context_expr) out.push_back(n->context_expr);
            if(n->optional_vars) out.push_back(n->optional_vars);
            break;
        }
        case ASTNodeWith:
        {
            auto* n = static_cast<WithNode*>(node);
            for(auto* c : n->items) out.push_back(c);
            for(auto* c : n->body) out.push_back(c);
            break;
        }
        case ASTNodeAsyncWith:
        {
            auto* n = static_cast<AsyncWithNode*>(node);
            for(auto* c : n->items) out.push_back(c);
            for(auto* c : n->body) out.push_back(c);
            break;
        }
        case ASTNodeAssertion:
        {
            auto* n = static_cast<AssertionNode*>(node);
            if(n->expr) out.push_back(n->expr);
            if(n->message) out.push_back(n->message);
            break;
        }
        default:
            break;
    }
}

PYTHON_NAMESPACE_END

STDROMANO_NAMESPACE_END
