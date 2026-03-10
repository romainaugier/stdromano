// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - Present Romain Augier
// All rights reserved.

#pragma once

#if !defined(__STDROMANO_PYTHON)
#define __STDROMANO_PYTHON

#include "stdromano/string.hpp"
#include "stdromano/vector.hpp"

#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"

STDROMANO_NAMESPACE_BEGIN

#define PYTHON_NAMESPACE_BEGIN namespace Python {
#define PYTHON_NAMESPACE_END }

// Very simple and fast Python parser
//
// For reference:
//   - https://docs.python.org/3.8/reference/lexical_analysis.html
//   - https://docs.python.org/3/library/ast.html

PYTHON_NAMESPACE_BEGIN

enum Keyword : std::uint32_t
{
    False = 0,
    Await = 1,
    Else = 2,
    Import = 3,
    Pass = 4,
    None = 5,
    Break = 6,
    Except = 7,
    In = 8,
    Raise = 9,
    True = 10,
    Class = 11,
    Finally = 12,
    Is = 13,
    Return = 14,
    And = 15,
    Continue = 16,
    For = 17,
    Lambda = 18,
    Try = 19,
    As = 20,
    Def = 21,
    From = 22,
    Nonlocal = 23,
    While = 24,
    Assert = 25,
    Del = 26,
    Global = 27,
    Not = 28,
    With = 29,
    Async = 30,
    Elif = 30,
    If = 31,
    Or = 32,
    Yield = 33
};

enum Delimiter : std::uint32_t
{
    LParen = 1,
    RParen = 2,
    LBracket = 3,
    RBracket = 4,
    LBrace = 5,
    RBrace = 6,
    Comma = 7,
    Colon = 8,
    Dot = 9,
    Semicolon = 10,
    At = 11,
    RightArrow = 12
};

enum Operator : std::uint32_t
{
    Addition = 1,
    Subtraction = 2,
    Multiplication = 3,
    Division = 4,
    Modulus = 5,
    Exponentiation = 6,
    FloorDivision = 7,
    Assign = 8,
    AdditionAssign = 9,
    SubtractionAssign = 10,
    MultiplicationAssign = 11,
    DivisionAssign = 12,
    ModulusAssign = 13,
    FloorDivisionAssign = 14,
    ExponentiationAssign = 15,
    BitwiseAndAssign = 16,
    BitwiseOrAssign = 17,
    BitwiseXorAssign = 18,
    BitwiseLShiftAssign = 19,
    BitwiseRShiftAssign = 20,
    BitwiseAnd = 21,
    BitwiseOr = 22,
    BitwiseXor = 23,
    BitwiseNot = 24,
    BitwiseLShift = 25,
    BitwiseRShift = 26,
    ComparatorEquals = 27,
    ComparatorNotEquals = 28,
    ComparatorGreaterThan = 29,
    ComparatorLessThan = 30,
    ComparatorGreaterEqualsThan = 31,
    ComparatorLessEqualsThan = 32,
    LogicalAnd = 33,
    LogicalOr = 34,
    LogicalNot = 35,
    IdentityIs = 36,
    IdentityIsNot = 37,
    MembershipIn = 38,
    MembershipNotIn = 39
};

enum Literal : std::uint32_t
{
    String = 1,
    UnicodeString = 2,
    RawString = 3,
    FormattedString = 4,
    Bytes = 5,
    Integer = 6,
    Float = 7
};

class STDROMANO_API AST
{
    Arena _nodes;

    std::shared_ptr<spdlog::logger> _logger = nullptr;

public:
    AST(std::shared_ptr<spdlog::logger> logger = nullptr) : _logger(logger)
    {
        if(this->_logger == nullptr)
        {
            auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
            console_sink->set_pattern("[%T] [%^%l%$] [python] %v");

            auto py_logger = std::make_shared<spdlog::logger>("python", spdlog::sinks_init_list{ console_sink });
            py_logger->set_level(spdlog::get_level());
            spdlog::register_logger(py_logger);

            this->_logger = py_logger;
        }
    }

    bool from_text(const stdromano::StringD& buffer, const bool debug = false) noexcept;
};

PYTHON_NAMESPACE_END

STDROMANO_NAMESPACE_END

#endif // !defined(__STDROMANO_PYTHON)
