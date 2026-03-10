// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - Present Romain Augier
// All rights reserved.

#include "stdromano/hashmap.hpp"
#include "stdromano/python.hpp"

STDROMANO_NAMESPACE_BEGIN

PYTHON_NAMESPACE_BEGIN

static constexpr std::uint32_t LEX_ERROR = std::numeric_limits<std::uint32_t>::max();

struct Token
{
    enum Kind : std::uint32_t
    {
        Identifier = 0,
        Keyword = 1,
        Literal = 2,
        Operator = 3,
        Delimiter = 4,
        Newline = 5,
        Indent = 6,
        Dedent = 7,
    };

    StringD value;

    Kind kind;

    std::uint32_t type;

    std::uint32_t column;
    std::uint32_t line;

    Token(StringD value, Kind kind, std::uint32_t type, std::uint32_t column, std::uint32_t line) : value(std::move(value)),
                                                                                                    kind(kind),
                                                                                                    type(type),
                                                                                                    column(column),
                                                                                                    line(line) {}
};

bool is_identifier_start(unsigned int c) noexcept
{
    return is_letter(c) | c == '_';
}

bool is_identifier(unsigned int c) noexcept
{
    return is_letter(c) | is_digit(c) | c == '_';
}

const HashMap<StringD, Keyword> g_keywords = {
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

const HashMap<StringD, Delimiter> g_delimiters = {
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

bool is_delimiter(unsigned int c) noexcept
{
    return c == '(' |
           c == ')' |
           c == '[' |
           c == ']' |
           c == '{' |
           c == '}' |
           c == ',' |
           c == ':' |
           c == '.' |
           c == ';' |
           c == '@' |
           c == '-' |
           c == '>';
}

const HashMap<StringD, Operator> g_operators = {
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

bool is_operator_start(unsigned int c) noexcept
{
    return c == '+' |
           c == '-' |
           c == '*' |
           c == '/' |
           c == '&' |
           c == '|' |
           c == '^' |
           c == '>' |
           c == '<' |
           c == '=' |
           c == '!' |
           c == '%' |
           c == 'a' |
           c == 'i' |
           c == 'n' |
           c == 'o';
}

bool is_binary_operator(unsigned int c) noexcept
{
    return c == '+' |
           c == '-' |
           c == '*' |
           c == '/' |
           c == '&' |
           c == '|' |
           c == '^' |
           c == '>' |
           c == '<' |
           c == '%';
}

bool is_unary_operator(unsigned int c) noexcept
{
    return c == '!';
}

bool is_assignment_operator(unsigned int c)
{
    return c == '=';
}

std::uint32_t is_logical_operator(char* c) noexcept
{
    if(std::strncmp(c, "or", 2) == 0)
    {
        return 2;
    }
    else if(std::strncmp(c, "and", 3) == 0 || std::strncmp(c, "not", 3) == 0)
    {
        return 3;
    }

    return 0;
}

std::uint32_t is_identity_operator(char* c) noexcept
{
    if(std::strncmp(c, "is", 2) == 0)
        return std::strncmp(c + 2, " not", 4) == 0 ? 6 : 2;

    return 0;
}

std::uint32_t is_membership_operator(char* c) noexcept
{
    if(std::strncmp(c, "in", 2) == 0)
        return 2;
    else if(std::strncmp(c, "not in", 6))
        return 6;
    else
        return 0;
}

std::uint32_t is_operator(char* c) noexcept
{
    if(is_binary_operator(*c))
    {
        std::uint32_t length = 1;
        c++;

        while(is_binary_operator(*c) || is_assignment_operator(*c))
        {
            c++;
            length++;
        }

        return length;
    }
    else if(is_assignment_operator(*c))
    {
        return 1;
    }
    else if(is_unary_operator(*c))
    {
        if(is_assignment_operator(*(c + 1)))
        {
            return 2;
        }

        return LEX_ERROR;
    }

    return 0;
}

bool is_string_literal_prefix(unsigned int c) noexcept
{
    return c == 'r' |
           c == 'u' |
           c == 'R' |
           c == 'U' |
           c == 'f' |
           c == 'F';
}

std::uint32_t is_string_literal_start(char* c) noexcept
{
    if(is_string_literal_prefix(*c) && *(c + 1) != '\0')
    {
        if(*(c + 1) == '"' || *(c + 1) == '\'')
        {
            return 2;
        }
        else if(is_string_literal_prefix(*(c + 1)))
        {
            if(*(c + 2) != '\0' && (*(c + 2) == '"' || *(c + 2) == '\''))
                return 3;
            else
                return 0;
        }

        return 0;
    }
    else if(*c == '"' || *c == '\'')
    {
        if(*(c + 1) != '\0' && (*(c + 1) == '"' || *(c + 1) == '\''))
            if(*(c + 2) != '\0' && (*(c + 2) == '"' || *(c + 2) == '\''))
                return 3;

        return 1;
    }

    return 0;
}

std::uint32_t consume_string_literal(char* c, std::uint32_t& position, std::uint32_t& line) noexcept
{
    std::uint32_t length = 0;

    while(*c != '\0' && (*c != '"' && *c != '\''))
    {
        position++;
        c++;
        length++;

        if(*c == '\n')
        {
            position = 0;
            line++;
        }
    }

    return length;
}

bool is_numeric_literal_start(unsigned int c) noexcept
{
    return is_digit(c);
}

bool is_int_literal_start(unsigned int c) noexcept
{
    return is_digit(c);
}

bool is_int_literal(unsigned int c) noexcept
{
    return is_digit(c);
}

std::uint32_t consume_int_literal(char* c) noexcept
{
    std::uint32_t length = 0;

    while(*c != '\0' && is_digit(*c))
    {
        c++;
        length++;
    }

    return length;
}

bool is_float_literal_start(unsigned int c) noexcept
{
    return is_digit(c);
}

bool is_float_literal(unsigned int c) noexcept
{
    return is_digit(c) | c == '.';
}

uint32_t consume_float_literal(char* c) noexcept
{
    std::uint32_t length = 0;

    bool found_dot = false;

    while(*c != '\0')
    {
        if(*c == '.')
        {
            if(found_dot)
                break;

            found_dot = true;

            c++;
            length++;
        }
        else if(is_digit(*c))
        {
            c++;
            length++;
        }
        else
        {
            break;
        }
    }

    return found_dot ? length : 0;
}

static constexpr std::uint32_t INDENT_SZ_UNDEFINED = std::numeric_limits<std::uint32_t>::max();

bool lex(const char* buffer, Vector<Token>& tokens) noexcept
{
    char* s = const_cast<char*>(buffer);

    std::uint32_t indent_size = INDENT_SZ_UNDEFINED;
    std::uint32_t indent_level = 0;
    std::uint32_t position = 1;
    std::uint32_t line = 1;

    while(*s != '\0')
    {
        std::uint32_t string_literal_prefix = is_string_literal_start(s);

        if(string_literal_prefix != 0)
        {
            s += string_literal_prefix;
            position += string_literal_prefix;

            char* start = s;
            std::uint32_t length = consume_string_literal(start, position, line);

            s += length;

            tokens.emplace_back(StringD::make_ref(start, length),
                                Token::Kind::Literal,
                                Literal::String,
                                position,
                                line);

            while(*s != '\0' && (*s == '"' || *s == '\''))
            {
                s++;
                position++;
            }
        }
        else if(is_numeric_literal_start(*s))
        {
            char* start = s;
            std::uint32_t length = consume_float_literal(s);

            if(length > 0)
            {
                tokens.emplace_back(StringD::make_ref(start, length),
                                    Token::Kind::Literal,
                                    Literal::Float,
                                    position,
                                    line);

                s += length;
                position += length;
                continue;
            }

            length = consume_int_literal(s);

            if(length > 0)
            {
                tokens.emplace_back(StringD::make_ref(start, length),
                                    Token::Kind::Literal,
                                    Literal::Integer,
                                    position,
                                    line);

                s += length;
                position += length;
                continue;
            }
        }
        else if(is_identifier_start(*s))
        {
            char* start = s;
            std::uint32_t length = 1;

            s++;
            position++;

            while(*s != '\0' && is_identifier(*s))
            {
                s++;
                length++;
            }

            position += length;

            StringD identifier = std::move(StringD::make_from_c_str(start, length));

            auto keyword_it = g_keywords.find(identifier);

            if(keyword_it != g_keywords.end())
            {
                tokens.emplace_back(identifier,
                                    Token::Kind::Keyword,
                                    keyword_it->second,
                                    position - length,
                                    line);
                s++;
                position++;
                continue;
            }

            auto operator_it = g_operators.find(identifier);

            if(operator_it != g_operators.end())
            {
                tokens.emplace_back(identifier,
                                    Token::Kind::Operator,
                                    operator_it->second,
                                    position - length,
                                    line);
                s++;
                position++;
                continue;
            }

            tokens.emplace_back(identifier,
                                Token::Kind::Identifier,
                                0,
                                position - length,
                                line);
        }
        else if(is_delimiter(*s))
        {
            char* start = s;
            std::uint32_t length = 1;

            s++;
            position++;

            if(*start == '-')
            {
                while(*s != '\0' && is_delimiter(*s))
                {
                    s++;
                    length++;
                }
            }

            position += length;

            StringD delimiter = std::move(StringD::make_ref(start, length));

            auto delimiter_it = g_delimiters.find(delimiter);

            if(delimiter_it == g_delimiters.end())
            {
                s -= length;
                position -= length;
                goto is_operator;
            }

            tokens.emplace_back(delimiter,
                                Token::Kind::Delimiter,
                                delimiter_it->second,
                                position,
                                line);
        }
        else if(is_operator_start(*s))
        {
            is_operator:
            char* start = s;
            std::uint32_t length = is_operator(s);

            if(length > 0)
            {
                position += length;
                StringD op = std::move(StringD::make_ref(start, length));

                auto operator_it = g_operators.find(op);

                if(operator_it == g_operators.end() || length == LEX_ERROR)
                {
                    // log_error("Syntax Error: Invalid operator : {}", op);
                    // log_error("Line {}, Position {}", line, position);
                    return false;
                }

                tokens.emplace_back(op,
                                    Token::Kind::Operator,
                                    operator_it->second,
                                    position,
                                    line);

                s += length;
                continue;
            }
        }
        else if(*s == '\n')
        {
            tokens.emplace_back(StringD(),
                                Token::Kind::Newline,
                                0,
                                position,
                                line);
            s++;

            line++;
            position = 0;

            while(*s != '\0' && *s == '\n')
            {
                tokens.emplace_back(StringD(),
                                    Token::Kind::Newline,
                                    0,
                                    position,
                                    line);
                s++;
                line++;
            }

            std::uint32_t line_indent = 0;

            while(*s != '\0' && *s == ' ')
            {
                line_indent++;
                s++;
                position++;
            }

            if(indent_size == INDENT_SZ_UNDEFINED || indent_size == 0)
            {
                indent_size = line_indent;
            }
            // else if(indent_size != 0 && line_indent % indent_size != 0)
            // {
            //     log_error("Indent Error: Inconsistent indent size {} (Found indent size is {})", line_indent, indent_size);
            //     log_error("Line {}, Position {}", line, position);
            //     return false;
            // }

            if(line_indent > indent_level)
            {
                tokens.emplace_back(StringD(),
                                    Token::Kind::Indent,
                                    line_indent,
                                    position,
                                    line);
            }
            else if(line_indent < indent_level)
            {
                tokens.emplace_back(StringD(),
                                    Token::Kind::Dedent,
                                    line_indent,
                                    position,
                                    line);
            }

            indent_level = line_indent;
        }
        else
        {
            s++;
            position++;
        }
    }

    return true;
}

//
bool AST::from_text(const stdromano::StringD& buffer, const bool debug) noexcept
{
    this->_nodes.clear();

    this->_logger->trace("Parsing text");

    Vector<Token> tokens;

    if(!lex(buffer.c_str(), tokens))
        return false;

    if(debug)
    {
        this->_logger->debug("{0:*^{1}}", "", 50);
        this->_logger->debug("Tokens ({})", tokens.size());

        for(const auto token : tokens)
            this->_logger->debug("{} ({}:{})", token.value, token.column, token.line);

        this->_logger->debug("{0:*^{1}}", "", 50);
    }

    this->_logger->trace("Parse successful");

    return true;
}

PYTHON_NAMESPACE_END

STDROMANO_NAMESPACE_END
