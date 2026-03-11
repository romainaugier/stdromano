// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - Present Romain Augier
// All rights reserved.

#include "stdromano/hashmap.hpp"
#include "stdromano/python.hpp"

STDROMANO_NAMESPACE_BEGIN

PYTHON_NAMESPACE_BEGIN

// Lexer

static constexpr std::uint32_t LEX_ERROR = std::numeric_limits<std::uint32_t>::max();

static const char* const g_token_kind_as_string[8] = {
    "IDENTIFIER",
    "KEYWORD",
    "LITERAL",
    "OPERATOR",
    "DELIMITER",
    "NEWLINE",
    "INDENT",
    "DEDENT",
};

const char* token_kind_as_string(Token::Kind kind) noexcept
{
    const std::uint32_t k = static_cast<std::uint32_t>(kind);

    if(k > 7)
        return nullptr;

    return g_token_kind_as_string[k];
}

bool is_identifier_start(unsigned int c) noexcept
{
    return is_letter(c) | (c == '_');
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
    return (c == '(') |
           (c == ')') |
           (c == '[') |
           (c == ']') |
           (c == '{') |
           (c == '}') |
           (c == ',') |
           (c == ':') |
           (c == '.') |
           (c == ';') |
           (c == '@') |
           (c == '-') |
           (c == '>');
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
    return (c == '+') |
           (c == '-') |
           (c == '*') |
           (c == '/') |
           (c == '&') |
           (c == '|') |
           (c == '^') |
           (c == '>') |
           (c == '<') |
           (c == '=') |
           (c == '!') |
           (c == '%') |
           (c == 'a') |
           (c == 'i') |
           (c == 'n') |
           (c == 'o');
}

bool is_binary_operator(unsigned int c) noexcept
{
    return (c == '+') |
           (c == '-') |
           (c == '*') |
           (c == '/') |
           (c == '&') |
           (c == '|') |
           (c == '^') |
           (c == '>') |
           (c == '<') |
           (c == '%');
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
    return (c == 'r') |
           (c == 'u') |
           (c == 'R') |
           (c == 'U') |
           (c == 'f') |
           (c == 'F');
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
    return is_digit(c) | (c == '.');
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

bool AST::lex(const char* buffer, Vector<Token>& tokens) noexcept
{
    this->_logger->trace("Lexing source code");

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
                    this->_logger->error("Syntax: Invalid operator {}", op);
                    this->_logger->error("Line {}, Position {}", line, position);
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

    this->_logger->trace("Lex successful");

    return true;
}

// Parser

struct Parser
{
    const Vector<Token>& tokens;
    Arena& arena;
    std::shared_ptr<spdlog::logger>& logger;
    std::uint32_t pos;

    Parser(const Vector<Token>& tokens, Arena& arena, std::shared_ptr<spdlog::logger>& logger) : tokens(tokens),
                                                                                                 arena(arena),
                                                                                                 logger(logger),
                                                                                                 pos(0) {}

    bool at_end() const noexcept { return pos >= tokens.size(); }

    const Token& current() const noexcept { return tokens[pos]; }

    const Token& peek(std::uint32_t offset = 1) const noexcept { return tokens[pos + offset]; }

    void advance() noexcept { pos++; }

    void skip_newlines() noexcept
    {
        while(!this->at_end() && this->current().kind == Token::Kind::Newline)
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
            this->logger->error("Expected keyword, got \"{}\" ({}:{})",
                                this->current().value,
                                this->current().line,
                                this->current().column);

            return false;
        }

        this->advance();

        return true;
    }

    bool expect_delimiter(Delimiter d) noexcept
    {
        if(!match_delimiter(d))
        {
            this->logger->error("Expected delimiter, got \"{}\" ({}:{})",
                                this->current().value,
                                this->current().line,
                                this->current().column);

            return false;
        }

        this->advance();

        return true;
    }

    bool expect_newline() noexcept
    {
        if(!this->check(Token::Kind::Newline) && !this->at_end())
        {
            this->logger->error("Expected newline, got \"{}\" ({}:{})",
                                this->current().value,
                                this->current().line,
                                this->current().column);
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
            this->logger->error("Expected indented block ({}:{})", this->current().line, this->current().column);
            return false;
        }

        std::uint32_t block_indent = this->current().type;
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

    Node* parse_funcdef() noexcept
    {
        std::uint32_t line = this->current().line;
        std::uint32_t column = this->current().column;
        this->advance(); // skip 'def'

        if(!this->check(Token::Kind::Identifier))
        {
            this->logger->error("Expected function name ({}:{})", this->current().line, this->current().column);
            return nullptr;
        }

        StringD name = std::move(StringD::make_from_c_str(this->current().value.c_str(), this->current().value.size()));
        this->advance();

        if(!this->expect_delimiter(Delimiter::LParen))
            return nullptr;

        auto* node = this->arena.emplace<FunctionDefNode>(std::move(name), line, column);

        // Parse parameters as simple names
        while(!this->at_end() && !this->match_delimiter(Delimiter::RParen))
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
            logger->error("Expected class name ({}:{})", this->current().line, this->current().column);
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
                logger->error("Expected module name ({}:{})", this->current().line, this->current().column);
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
                    this->logger->error("Expected name after dot ({}:{})", this->current().line, this->current().column);
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
                    this->logger->error("Expected alias ({}:{})", this->current().line, this->current().column);
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
            this->logger->error("Expected module name ({}:{})", this->current().line, this->current().column);
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
                this->logger->error("Expected name ({}:{})", this->current().line, this->current().column);
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
                    this->logger->error("Expected alias ({}:{})", this->current().line, this->current().column);
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

        // Simple assignment: target = value
        if(this->match_operator(Operator::Assign))
        {
            this->advance();

            Node* value = this->parse_expr();

            if(value == nullptr)
                return nullptr;

            auto* node = this->arena.emplace<AssignNode>(value, line, column);
            node->targets.push_back(left);
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
                    this->logger->error("Expected attribute name ({}:{})", this->current().line, this->current().column);
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
            this->logger->error("Unexpected end of input");
            return nullptr;
        }

        // Identifier / Name
        if(this->check(Token::Kind::Identifier))
        {
            auto* node = this->arena.emplace<NameNode>(
                std::move(StringD::make_from_c_str(this->current().value.c_str(), this->current().value.size())),
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
                std::move(StringD::make_from_c_str(this->current().value.c_str(), this->current().value.size())),
                lit,
                this->current().line,
                this->current().column);

            this->advance();

            return node;
        }

        // True / False / None as constants
        if(this->match_keyword(Keyword::True) || this->match_keyword(Keyword::False) || this->match_keyword(Keyword::None))
        {
            auto* node = this->arena.emplace<ConstantNode>(
                std::move(StringD::make_from_c_str(this->current().value.c_str(), this->current().value.size())),
                Literal::String, // reuse; the raw value distinguishes them
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

            auto* node = arena.emplace<DictNode>(line, column);

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

        this->logger->error("Unexpected token \"{}\" ({}:{})",
                            this->current().value,
                            this->current().line,
                            this->current().column);

        return nullptr;
    }
};

bool AST::parse(const Vector<Token>& tokens) noexcept
{
    this->_logger->trace("Parsing tokens");

    Parser parser(tokens, this->_nodes, this->_logger);

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
bool AST::from_text(const stdromano::StringD& buffer, const bool debug) noexcept
{
    this->_nodes.clear();

    Vector<Token> tokens;

    if(!this->lex(buffer.c_str(), tokens))
        return false;

    if(debug)
    {
        this->_logger->debug("{0:*^{1}}", "", 50);
        this->_logger->debug("LEX: Tokens ({})", tokens.size());

        for(const auto token : tokens)
            this->_logger->debug("{} \"{}\" ({}:{})",
                                 token_kind_as_string(token.kind),
                                 token.value,
                                 token.line,
                                 token.column);

        this->_logger->debug("{0:*^{1}}", "", 50);
    }

    if(!this->parse(tokens))
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
