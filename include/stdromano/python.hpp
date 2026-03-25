// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - Present Romain Augier
// All rights reserved.

#pragma once

#if !defined(__STDROMANO_PYTHON)
#define __STDROMANO_PYTHON

#include "stdromano/string.hpp"
#include "stdromano/vector.hpp"

#include "spdlog/spdlog.h"

STDROMANO_NAMESPACE_BEGIN

#define PYTHON_NAMESPACE_BEGIN namespace Python {
#define PYTHON_NAMESPACE_END }

// Very simple and fast Python parser
//
// For reference:
//   - https://docs.python.org/3.8/reference/lexical_analysis.html
//   - https://docs.python.org/3/library/ast.html

PYTHON_NAMESPACE_BEGIN

// Helps debugging the parser
#define STDROMANO_PYTHON_PARSER_ASSERT_ON_ERROR

// Lexer Token

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

    static const char* kind_as_string(Kind kind) noexcept;
};

enum Keyword : std::uint32_t
{
    False = 0,
    Await,
    Else,
    Import,
    Pass,
    None,
    Break,
    Except,
    In,
    Raise,
    True,
    Class,
    Finally,
    Is,
    Return,
    And,
    Continue,
    For,
    Lambda,
    Try,
    As,
    Def,
    From,
    Nonlocal,
    While,
    Assert,
    Del,
    Global,
    Not,
    With,
    Async,
    Elif,
    If,
    Or,
    Yield,
    Match,
    Case,
    Type,
};

enum Delimiter : std::uint32_t
{
    LParen = 1,
    RParen,
    LBracket,
    RBracket,
    LBrace,
    RBrace,
    Comma,
    Colon,
    Dot,
    Semicolon,
    RightArrow,
};

enum Operator : std::uint32_t
{
    Addition = 1,
    Subtraction,
    Multiplication,
    Division,
    Modulus,
    Exponentiation,
    FloorDivision,
    MatMul,
    Assign,
    WalrusAssign,
    AdditionAssign,
    SubtractionAssign,
    MultiplicationAssign,
    DivisionAssign,
    ModulusAssign,
    FloorDivisionAssign,
    MatMulAssign,
    ExponentiationAssign,
    BitwiseAndAssign,
    BitwiseOrAssign,
    BitwiseXorAssign,
    BitwiseLShiftAssign,
    BitwiseRShiftAssign,
    BitwiseAnd,
    BitwiseOr,
    BitwiseXor,
    BitwiseNot,
    BitwiseLShift,
    BitwiseRShift,
    ComparatorEquals,
    ComparatorNotEquals,
    ComparatorGreaterThan,
    ComparatorLessThan,
    ComparatorGreaterEqualsThan,
    ComparatorLessEqualsThan,
    LogicalAnd,
    LogicalOr,
    LogicalNot,
    IdentityIs,
    IdentityIsNot,
    MembershipIn,
    MembershipNotIn,
    Bang,
};

enum Literal : std::uint32_t
{
    String = 1,
    UnicodeString,
    RawString,
    FormattedString,
    Bytes,
    Integer,
    Float,
    Complex,
};

// AST Node

enum ASTNodeType : std::uint32_t
{
    ASTNodeModule = 0,

    ASTNodeDecorator,

    ASTNodeFunctionArg,
    ASTNodeFunctionDef,
    ASTNodeAsyncFunctionDef,
    ASTNodeClassDef,

    ASTNodeReturn,

    ASTNodeAssign,
    ASTNodeAnnAssign,
    ASTNodeMultiAssign,
    ASTNodeAugAssign,
    ASTNodeWalrusAssign,

    ASTNodeFor,
    ASTNodeAsyncFor,
    ASTNodeWhile,
    ASTNodeIf,
    ASTNodePass,
    ASTNodeBreak,
    ASTNodeContinue,

    ASTNodeImport,
    ASTNodeImportFrom,

    ASTNodeExpr,

    ASTNodeRaise,
    ASTNodeTry,
    ASTNodeExcept,

    ASTNodeBinOp,
    ASTNodeUnaryOp,
    ASTNodeTernaryOp,
    ASTNodeBoolOp,
    ASTNodeCompareOp,

    ASTNodeKeywordArg,
    ASTNodeCall,

    ASTNodeName,
    ASTNodeConstant,

    ASTNodeAttribute,
    ASTNodeSubscript,

    ASTNodeStarred,

    ASTNodeList,
    ASTNodeSet,
    ASTNodeTuple,
    ASTNodeDict,

    ASTNodeComprehension,
    ASTNodeAsyncComprehension,
    ASTNodeListComp,
    ASTNodeSetComp,
    ASTNodeDictComp,
    ASTNodeGeneratorExpr,

    ASTNodeLambda,

    ASTNodeMatch,
    ASTNodeMatchCase,
    ASTNodeMatchOr,
    ASTNodeMatchAs,
    ASTNodeMatchValue,
    ASTNodeMatchSingleton,
    ASTNodeMatchSequence,
    ASTNodeMatchMapping,
    ASTNodeMatchClass,
    ASTNodeMatchStar,

    ASTNodeYield,
    ASTNodeYieldFrom,

    ASTNodeTypeParam,
    ASTNodeTypeAlias,

    ASTNodeGlobal,
    ASTNodeNonLocal,

    ASTNodeDel,

    ASTNodeAwait,

    ASTNodeWithItem,
    ASTNodeWith,
    ASTNodeAsyncWith,

    ASTNodeAssertion,

    ASTNodeCount,
};

struct Node
{
    std::uint32_t _type;
    std::uint32_t _line;
    std::uint32_t _column;

    Node(std::uint32_t type, std::uint32_t line, std::uint32_t column) : _type(type),
                                                                         _line(line),
                                                                         _column(column) {}

    std::uint32_t type() const noexcept { return _type; }
    std::uint32_t line() const noexcept { return _line; }
    std::uint32_t column() const noexcept { return _column; }

    virtual const char* type_str() const noexcept { return "NODE"; }

    virtual void debug(std::shared_ptr<spdlog::logger> logger, std::uint32_t indent) const noexcept
    {
        logger->debug("{0: ^{1}}NODE", "", indent);
    }
};

// Module (top-level node for files)

struct ModuleNode : Node
{
    Vector<Node*> body;

    ModuleNode() : Node(ASTNodeModule, 0, 0) {}

    virtual const char* type_str() const noexcept override { return "MODULE"; }

    virtual void debug(std::shared_ptr<spdlog::logger> logger, std::uint32_t indent) const noexcept override
    {
        logger->debug("{0: ^{1}}MODULE", "", indent);
    }
};

// Function/Class Decorator

struct DecoratorNode : Node
{
    Node* expr;
    Node* target; // the FunctionDefNode or ClassDefNode being decorated

    DecoratorNode(Node* expr,
                  Node* target,
                  std::uint32_t line,
                  std::uint32_t column) : Node(ASTNodeDecorator, line, column),
                                          expr(expr),
                                          target(target) {}

        virtual const char* type_str() const noexcept override { return "DECORATOR"; }

        virtual void debug(std::shared_ptr<spdlog::logger> logger, std::uint32_t indent) const noexcept override
        {
            logger->debug("{0: ^{1}}DECORATOR", "", indent);
        }
};

// Function definition

struct FunctionArgNode : Node
{
    StringD name;
    Node* annotation;
    Node* default_value;
    bool is_vararg; // *args
    bool is_kwarg; // **kwargs

    FunctionArgNode(StringD name, Node* annotation, std::uint32_t line, std::uint32_t column) : Node(ASTNodeFunctionArg, line, column),
                                                                                                name(std::move(name)),
                                                                                                annotation(annotation),
                                                                                                default_value(nullptr),
                                                                                                is_vararg(false),
                                                                                                is_kwarg(false) {}

    virtual const char* type_str() const noexcept override { return "FUNC_ARG"; }

    virtual void debug(std::shared_ptr<spdlog::logger> logger, std::uint32_t indent) const noexcept override
    {
        logger->debug("{0: ^{1}}FUNCARG ({2})", "", indent, this->name);
    }
};

struct FunctionDefNode : Node
{
    StringD name;
    Vector<Node*> args;
    Vector<Node*> body;
    Node* return_annotation = nullptr;
    Vector<Node*> type_params;
    std::int32_t posonly_index = -1; // index of '/' separator, -1 if absent
    std::int32_t kwonly_index = -1; // index of bare '*' separator, -1 if absent

    FunctionDefNode(StringD name, std::uint32_t line, std::uint32_t column) : Node(ASTNodeFunctionDef, line, column),
                                                                              name(std::move(name)),
                                                                              return_annotation(nullptr) {}

    virtual const char* type_str() const noexcept override { return "FUNC_DEF"; }

    virtual void debug(std::shared_ptr<spdlog::logger> logger, std::uint32_t indent) const noexcept override
    {
        logger->debug("{0: ^{1}}FUNCDEF ({2})", "", indent, this->name);
    }
};

struct AsyncFunctionDefNode : Node
{
    StringD name;
    Vector<Node*> args;
    Vector<Node*> body;
    Node* return_annotation = nullptr;
    Vector<Node*> type_params;
    std::int32_t posonly_index = -1; // index of '/' separator, -1 if absent
    std::int32_t kwonly_index = -1; // index of bare '*' separator, -1 if absent

    AsyncFunctionDefNode(StringD name, std::uint32_t line, std::uint32_t column) : Node(ASTNodeAsyncFunctionDef, line, column),
                                                                                   name(std::move(name)) {}

    virtual const char* type_str() const noexcept override { return "ASYNC_FUNC_DEF"; }

    virtual void debug(std::shared_ptr<spdlog::logger> logger, std::uint32_t indent) const noexcept override
    {
        logger->debug("{0: ^{1}}ASYNC_FUNC_DEF ({2})", "", indent, this->name);
    }
};

// Class Definition

struct ClassDefNode : Node
{
    StringD name;
    Vector<Node*> bases;
    Vector<Node*> body;
    Vector<Node*> type_params;

    ClassDefNode(StringD name, std::uint32_t line, std::uint32_t column) : Node(ASTNodeClassDef, line, column),
                                                                           name(std::move(name)) {}

    virtual const char* type_str() const noexcept override { return "CLASS_DEF"; }

    virtual void debug(std::shared_ptr<spdlog::logger> logger, std::uint32_t indent) const noexcept override
    {
        logger->debug("{0: ^{1}}CLASSDEF ({2})", "", indent, this->name);
    }
};

// Return

struct ReturnNode : Node
{
    Node* value;

    ReturnNode(Node* value, std::uint32_t line, std::uint32_t column) : Node(ASTNodeReturn, line, column),
                                                                        value(value) {}

    virtual const char* type_str() const noexcept override { return "RETURN"; }

    virtual void debug(std::shared_ptr<spdlog::logger> logger, std::uint32_t indent) const noexcept override
    {
        logger->debug("{0: ^{1}}RETURN", "", indent);
    }
};

// Assignment

struct AssignNode : Node
{
    Vector<Node*> targets;
    Node* value;

    AssignNode(Node* value, std::uint32_t line, std::uint32_t column) : Node(ASTNodeAssign, line, column),
                                                                        value(value) {}

    virtual const char* type_str() const noexcept override { return "ASSIGN"; };

    virtual void debug(std::shared_ptr<spdlog::logger> logger, std::uint32_t indent) const noexcept override
    {
        logger->debug("{0: ^{1}}ASSIGN", "", indent);
    }
};

struct AnnAssignNode : Node
{
    Node* target;
    Node* value;
    Node* ann;

    AnnAssignNode(Node* target, Node* value, Node* ann, std::uint32_t line, std::uint32_t column) : Node(ASTNodeAnnAssign, line, column),
                                                                                                    target(target),
                                                                                                    value(value),
                                                                                                    ann(ann) {}

    virtual const char* type_str() const noexcept override { return "ANN_ASSIGN"; }

    virtual void debug(std::shared_ptr<spdlog::logger> logger, std::uint32_t indent) const noexcept override
    {
        logger->debug("{0: ^{1}}ANNASSIGN", "", indent);
    }
};

struct MultiAssignNode : Node
{
    Vector<Node*> targets;
    Vector<Node*> values;

    MultiAssignNode(std::uint32_t line, std::uint32_t column) : Node(ASTNodeMultiAssign, line, column) {}

    virtual const char* type_str() const noexcept override { return "MULTI_ASSIGN"; };

    virtual void debug(std::shared_ptr<spdlog::logger> logger, std::uint32_t indent) const noexcept override
    {
        logger->debug("{0: ^{1}}MULTIASSIGN", "", indent);
    }
};

struct AugAssignNode : Node
{
    Node* target;
    Operator op;
    Node* value;

    AugAssignNode(Node* target, Operator op, Node* value, std::uint32_t line, std::uint32_t column) : Node(ASTNodeAugAssign, line, column),
                                                                                                      target(target),
                                                                                                      op(op),
                                                                                                      value(value) {}

    virtual const char* type_str() const noexcept override { return "AUG_ASSIGN"; }

    virtual void debug(std::shared_ptr<spdlog::logger> logger, std::uint32_t indent) const noexcept override
    {
        logger->debug("{0: ^{1}}AUGASSIGN ({2})", "", indent, this->op);
    }
};

struct WalrusAssignNode : Node
{
    Node* target;
    Node* value;

    WalrusAssignNode(Node* target, Node* value, std::uint32_t line, std::uint32_t column) : Node(ASTNodeWalrusAssign, line, column),
                                                                                            target(target),
                                                                                            value(value) {}

    virtual const char* type_str() const noexcept override { return "WALRUS_ASSIGN"; }

    virtual void debug(std::shared_ptr<spdlog::logger> logger, std::uint32_t indent) const noexcept override
    {
        logger->debug("{0: ^{1}}WALRUSASSIGN", "", indent);
    }
};

// Control Flow

struct ForNode : Node
{
    Node* target;
    Node* iter;
    Vector<Node*> body;
    Vector<Node*> orelse;

    ForNode(Node* target, Node* iter, std::uint32_t line, std::uint32_t column) : Node(ASTNodeFor, line, column),
                                                                                  target(target),
                                                                                  iter(iter) {}

    virtual const char* type_str() const noexcept override { return "FOR"; }

    virtual void debug(std::shared_ptr<spdlog::logger> logger, std::uint32_t indent) const noexcept override
    {
        logger->debug("{0: ^{1}}FOR", "", indent);
    }
};

struct AsyncForNode : Node
{
    Node* target;
    Node* iter;
    Vector<Node*> body;
    Vector<Node*> orelse;

    AsyncForNode(Node* target, Node* iter, std::uint32_t line, std::uint32_t column) : Node(ASTNodeAsyncFor, line, column),
                                                                                       target(target),
                                                                                       iter(iter) {}

    virtual const char* type_str() const noexcept override { return "ASYNC_FOR"; }

    virtual void debug(std::shared_ptr<spdlog::logger> logger, std::uint32_t indent) const noexcept override
    {
        logger->debug("{0: ^{1}}ASYNC_FOR", "", indent);
    }
};

struct WhileNode : Node
{
    Node* test;
    Vector<Node*> body;
    Vector<Node*> orelse;

    WhileNode(Node* test, std::uint32_t line, std::uint32_t column) : Node(ASTNodeWhile, line, column),
                                                                      test(test) {}

    virtual const char* type_str() const noexcept override { return "WHILE"; }

    virtual void debug(std::shared_ptr<spdlog::logger> logger, std::uint32_t indent) const noexcept override
    {
        logger->debug("{0: ^{1}}WHILE", "", indent);
    }
};

struct IfNode : Node
{
    Node* test;
    Vector<Node*> body;
    Vector<Node*> orelse;

    IfNode(Node* test, std::uint32_t line, std::uint32_t column) : Node(ASTNodeIf, line, column),
                                                                   test(test) {}

    virtual const char* type_str() const noexcept override { return "IF"; }

    virtual void debug(std::shared_ptr<spdlog::logger> logger, std::uint32_t indent) const noexcept override
    {
        logger->debug("{0: ^{1}}IF","", indent);
    }
};

struct PassNode : Node
{
    PassNode(std::uint32_t line, std::uint32_t column) : Node(ASTNodePass, line, column) {}

    virtual const char* type_str() const noexcept override { return "PASS"; }

    virtual void debug(std::shared_ptr<spdlog::logger> logger, std::uint32_t indent) const noexcept override
    {
        logger->debug("{0: ^{1}}PASS", "", indent);
    }
};

struct BreakNode : Node
{
    BreakNode(std::uint32_t line, std::uint32_t column) : Node(ASTNodeBreak, line, column) {}

    virtual const char* type_str() const noexcept override { return "BREAK"; }

    virtual void debug(std::shared_ptr<spdlog::logger> logger, std::uint32_t indent) const noexcept override
    {
        logger->debug("{0: ^{1}}BREAK", "", indent);
    }
};

struct ContinueNode : Node
{
    ContinueNode(std::uint32_t line, std::uint32_t column) : Node(ASTNodeContinue, line, column) {}

    virtual const char* type_str() const noexcept override { return "CONTINUE"; }

    virtual void debug(std::shared_ptr<spdlog::logger> logger, std::uint32_t indent) const noexcept override
    {
        logger->debug("{0: ^{1}}CONTINUE", "", indent);
    }
};

// Imports

struct ImportNode : Node
{
    Vector<StringD> names;
    Vector<StringD> aliases;

    ImportNode(std::uint32_t line, std::uint32_t column) : Node(ASTNodeImport, line, column) {}

    virtual const char* type_str() const noexcept override { return "IMPORT"; }

    virtual void debug(std::shared_ptr<spdlog::logger> logger, std::uint32_t indent) const noexcept override
    {
        const StringD import_names = join(this->names, [](StringD name) { return name; }, ", ");

        logger->debug("{0: ^{1}}IMPORT ({2})", "", indent, import_names);
    }
};

struct ImportFromNode : Node
{
    StringD module;
    Vector<StringD> names;
    Vector<StringD> aliases;

    ImportFromNode(StringD module, std::uint32_t line, std::uint32_t column) : Node(ASTNodeImportFrom, line, column),
                                                                               module(std::move(module)) {}

    virtual const char* type_str() const noexcept override { return "IMPORT_FROM"; }

    virtual void debug(std::shared_ptr<spdlog::logger> logger, std::uint32_t indent) const noexcept override
    {
        logger->debug("{0: ^{1}}IMPORT FROM ({2})", "", indent, this->module);
    }
};

// Standalone expression

struct ExprNode : Node
{
    Node* value;

    ExprNode(Node* value, std::uint32_t line, std::uint32_t column) : Node(ASTNodeExpr, line, column),
                                                                      value(value) {}

    virtual const char* type_str() const noexcept override { return "EXPR"; }

    virtual void debug(std::shared_ptr<spdlog::logger> logger, std::uint32_t indent) const noexcept override
    {
        logger->debug("{0: ^{1}}EXPR", "", indent);
    }
};

// Raise, try, except (Exceptions)

struct RaiseNode : Node
{
    Node* exc;
    Node* cause;

    RaiseNode(Node* exc, Node* cause, std::uint32_t line, std::uint32_t column) : Node(ASTNodeRaise, line, column),
                                                                                  exc(exc),
                                                                                  cause(cause) {}

    virtual const char* type_str() const noexcept override { return "RAISE"; }

    virtual void debug(std::shared_ptr<spdlog::logger> logger, std::uint32_t indent) const noexcept override
    {
        logger->debug("{0: ^{1}}RAISE", "", indent);
    }
};

struct TryNode : Node
{
    Vector<Node*> body;
    Vector<Node*> excepts;
    Vector<Node*> orelse; // else block
    Vector<Node*> finalbody; // finally block

    TryNode(std::uint32_t line, std::uint32_t column) : Node(ASTNodeTry, line, column) {}

    virtual const char* type_str() const noexcept override { return "TRY"; }

    virtual void debug(std::shared_ptr<spdlog::logger> logger, std::uint32_t indent) const noexcept override
    {
        logger->debug("{0: ^{1}}TRY", "", indent);
    }
};

struct ExceptNode : Node
{
    Node* type; // exception type, nullptr for 'except:'
    StringD name;
    Vector<Node*> body;
    bool is_star; // except* for exception groups

    ExceptNode(Node* type, StringD name, bool is_star, std::uint32_t line, std::uint32_t column) : Node(ASTNodeExcept, line, column),
                                                                                                   type(type),
                                                                                                   name(std::move(name)),
                                                                                                   is_star(is_star) {}

    virtual const char* type_str() const noexcept override { return "EXCEPT"; }

    virtual void debug(std::shared_ptr<spdlog::logger> logger, std::uint32_t indent) const noexcept override
    {
        logger->debug("{0: ^{1}}EXCEPT", "", indent);
    }
};

// Ops

struct UnaryOpNode : Node
{
    Operator op;
    Node* operand;

    UnaryOpNode(Operator op, Node* operand, std::uint32_t line, std::uint32_t column) : Node(ASTNodeUnaryOp, line, column),
                                                                                        op(op),
                                                                                        operand(operand) {}

    virtual const char* type_str() const noexcept override { return "UNARY_OP"; }

    virtual void debug(std::shared_ptr<spdlog::logger> logger, std::uint32_t indent) const noexcept override
    {
        logger->debug("{0: ^{1}}UNOP ({2})", "", indent, this->op);
    }
};

struct BinOpNode : Node
{
    Node* left;
    Operator op;
    Node* right;

    BinOpNode(Node* left, Operator op, Node* right, std::uint32_t line, std::uint32_t column) : Node(ASTNodeBinOp, line, column),
                                                                                                left(left),
                                                                                                op(op),
                                                                                                right(right) {}

    virtual const char* type_str() const noexcept override { return "BINARY_OP"; }

    virtual void debug(std::shared_ptr<spdlog::logger> logger, std::uint32_t indent) const noexcept override
    {
        logger->debug("{0: ^{1}}BINOP ({2})", "", indent, this->op);
    }
};

struct TernaryOpNode : Node
{
    Node* body; // x
    Node* test; // y
    Node* orelse; // z

    TernaryOpNode(Node* body,
                  Node* test,
                  Node* orelse,
                  std::uint32_t line,
                  std::uint32_t column) : Node(ASTNodeTernaryOp, line, column),
                                          body(body),
                                          test(test),
                                          orelse(orelse) {}

    virtual const char* type_str() const noexcept override { return "TERNARY_OP"; }

    virtual void debug(std::shared_ptr<spdlog::logger> logger, std::uint32_t indent) const noexcept override
    {
        logger->debug("{0: ^{1}}TERNARYOP", "", indent);
    }
};

struct BoolOpNode : Node
{
    Operator op;
    Vector<Node*> values;

    BoolOpNode(Operator op, std::uint32_t line, std::uint32_t column) : Node(ASTNodeBoolOp, line, column),
                                                                        op(op) {}

    virtual const char* type_str() const noexcept override { return "BOOL_OP"; }

    virtual void debug(std::shared_ptr<spdlog::logger> logger, std::uint32_t indent) const noexcept override
    {
        logger->debug("{0: ^{1}}BOOLOP ({2})", "", indent, this->op);
    }
};

struct CompareOp : Node
{
    Node* left;
    Vector<Operator> ops;
    Vector<Node*> comparators;

    CompareOp(Node* left, std::uint32_t line, std::uint32_t column) : Node(ASTNodeCompareOp, line, column),
                                                                        left(left) {}

    virtual const char* type_str() const noexcept override { return "COMPARE"; }

    virtual void debug(std::shared_ptr<spdlog::logger> logger, std::uint32_t indent) const noexcept override
    {
        logger->debug("{0: ^{1}}COMPARE", "", indent);
    }
};

// Function calls and args/kwarfs

struct KeywordArgNode : Node
{
    StringD name; // empty when **expr unpacking
    Node* value;

    KeywordArgNode(StringD name,
                   Node* value,
                   std::uint32_t line,
                   std::uint32_t column) : Node(ASTNodeKeywordArg, line, column),
                                           name(std::move(name)),
                                           value(value) {}

    virtual const char* type_str() const noexcept override { return "KEYWORD_ARG"; }

    virtual void debug(std::shared_ptr<spdlog::logger> logger, std::uint32_t indent) const noexcept override
    {
        logger->debug("{0: ^{1}}KEYWORDARG", "", indent);
    }
};

struct CallNode : Node
{
    Node* func;
    Vector<Node*> args;

    CallNode(Node* func, std::uint32_t line, std::uint32_t column) : Node(ASTNodeCall, line, column),
                                                                     func(func) {}

    virtual const char* type_str() const noexcept override { return "CALL"; }

    virtual void debug(std::shared_ptr<spdlog::logger> logger, std::uint32_t indent) const noexcept override
    {
        logger->debug("{0: ^{1}}CALL", "", indent);
    }
};

// Values

struct NameNode : Node
{
    StringD id;

    Node* type = nullptr;

    NameNode(StringD id, std::uint32_t line, std::uint32_t column) : Node(ASTNodeName, line, column),
                                                                     id(std::move(id)) {}

    virtual const char* type_str() const noexcept override { return "NAME"; }

    virtual void debug(std::shared_ptr<spdlog::logger> logger, std::uint32_t indent) const noexcept override
    {
        logger->debug("{0: ^{1}}NAME (\"{2}\")", "", indent, this->id);
    }
};

struct ConstantNode : Node
{
    StringD raw;
    Literal literal_type;

    ConstantNode(StringD raw, Literal literal_type, std::uint32_t line, std::uint32_t column) : Node(ASTNodeConstant, line, column),
                                                                                                raw(std::move(raw)),
                                                                                                literal_type(literal_type) {}

    virtual const char* type_str() const noexcept override { return "CONSTANT"; }

    virtual void debug(std::shared_ptr<spdlog::logger> logger, std::uint32_t indent) const noexcept override
    {
        logger->debug("{0: ^{1}}CONSTANT ({2}: {3})", "", indent, this->literal_type, this->raw);
    }
};

// Postfix

struct AttributeNode : Node
{
    Node* value;
    Node* type_annotation = nullptr;
    StringD attr;

    AttributeNode(Node* value, StringD attr, std::uint32_t line, std::uint32_t column) : Node(ASTNodeAttribute, line, column),
                                                                                         value(value),
                                                                                         attr(std::move(attr)) {}

    virtual const char* type_str() const noexcept override { return "ATTRIBUTE"; }

    virtual void debug(std::shared_ptr<spdlog::logger> logger, std::uint32_t indent) const noexcept override
    {
        logger->debug("{0: ^{1}}ATTRIBUTE ({2})", "", indent, this->attr);
    }
};

struct SubscriptNode : Node
{
    Node* value;
    Node* slice;

    SubscriptNode(Node* value, Node* slice, std::uint32_t line, std::uint32_t column) : Node(ASTNodeSubscript, line, column),
                                                                                        value(value),
                                                                                        slice(slice) {}

    virtual const char* type_str() const noexcept override { return "SUBSCRIPT"; }

    virtual void debug(std::shared_ptr<spdlog::logger> logger, std::uint32_t indent) const noexcept override
    {
        logger->debug("{0: ^{1}}SUBSCRIPT", "", indent);
    }
};

// Star

struct StarredNode : Node
{
    Node* value;

    StarredNode(Node* value, std::uint32_t line, std::uint32_t column) : Node(ASTNodeStarred, line, column),
                                                                         value(value) {}

    virtual const char* type_str() const noexcept override { return "STARRED"; }

    virtual void debug(std::shared_ptr<spdlog::logger> logger, std::uint32_t indent) const noexcept override
    {
        logger->debug("{0: ^{1}}STARRED", "", indent);
    }
};

// Builtin data structures

struct ListNode : Node
{
    Vector<Node*> elts;

    ListNode(std::uint32_t line, std::uint32_t column) : Node(ASTNodeList, line, column) {}

    virtual const char* type_str() const noexcept override { return "LIST"; }

    virtual void debug(std::shared_ptr<spdlog::logger> logger, std::uint32_t indent) const noexcept override
    {
        logger->debug("{0: ^{1}}LIST ({2} elements)", "", indent, this->elts.size());
    }
};

struct SetNode : Node
{
    Vector<Node*> elts;

    SetNode(std::uint32_t line, std::uint32_t column) : Node(ASTNodeSet, line, column) {}

    virtual const char* type_str() const noexcept override { return "SET"; }

    virtual void debug(std::shared_ptr<spdlog::logger> logger, std::uint32_t indent) const noexcept override
    {
        logger->debug("{0: ^{1}}SET ({2} elements)", "", indent, this->elts.size());
    }
};

struct TupleNode : Node
{
    Vector<Node*> elts;

    TupleNode(std::uint32_t line, std::uint32_t column) : Node(ASTNodeTuple, line, column) {}

    virtual const char* type_str() const noexcept override { return "TUPLE"; }

    virtual void debug(std::shared_ptr<spdlog::logger> logger, std::uint32_t indent) const noexcept override
    {
        logger->debug("{0: ^{1}}TUPLE ({2} elements)", "", indent, this->elts.size());
    }
};

struct DictNode : Node
{
    Vector<Node*> keys;
    Vector<Node*> values;

    DictNode(std::uint32_t line, std::uint32_t column) : Node(ASTNodeDict, line, column) {}

    virtual const char* type_str() const noexcept override { return "DICT"; }

    virtual void debug(std::shared_ptr<spdlog::logger> logger, std::uint32_t indent) const noexcept override
    {
        logger->debug("{0: ^{1}}DICT ({2} elements)", "", indent, this->keys.size());
    }
};

// Comprehension and Data Structures comprehensions

struct ComprehensionNode : Node
{
    Node* target;
    Node* iter;
    Vector<Node*> ifs;

    ComprehensionNode(Node* target,
                      Node* iter,
                      std::uint32_t line,
                      std::uint32_t column) : Node(ASTNodeComprehension, line, column),
                                              target(target),
                                              iter(iter) {}

    virtual const char* type_str() const noexcept override { return "COMP"; }

    virtual void debug(std::shared_ptr<spdlog::logger> logger, std::uint32_t indent) const noexcept override
    {
        logger->debug("{0: ^{1}}COMP", "", indent);
    }
};

struct AsyncComprehensionNode : Node
{
    Node* target;
    Node* iter;
    Vector<Node*> ifs;

    AsyncComprehensionNode(Node* target,
                           Node* iter,
                           std::uint32_t line,
                           std::uint32_t column) : Node(ASTNodeAsyncComprehension, line, column),
                                                   target(target),
                                                   iter(iter) {}

    virtual const char* type_str() const noexcept override { return "ASYNC_COMP"; }

    virtual void debug(std::shared_ptr<spdlog::logger> logger, std::uint32_t indent) const noexcept override
    {
        logger->debug("{0: ^{1}}ASYNC_COMP", "", indent);
    }
};

struct ListCompNode : Node
{
    Node* elt;
    Vector<Node*> generators;

    ListCompNode(Node* elt, std::uint32_t line, std::uint32_t column) : Node(ASTNodeListComp, line, column),
                                                                        elt(elt) {}

    virtual const char* type_str() const noexcept override { return "LIST_COMP"; }

    virtual void debug(std::shared_ptr<spdlog::logger> logger, std::uint32_t indent) const noexcept override
    {
        logger->debug("{0: ^{1}}LISTCOMP", "", indent);
    }
};

struct SetCompNode : Node
{
    Node* elt;
    Vector<Node*> generators;

    SetCompNode(Node* elt, std::uint32_t line, std::uint32_t column) : Node(ASTNodeSetComp, line, column),
                                                                       elt(elt) {}

    virtual const char* type_str() const noexcept override { return "SET_COMP"; }

    virtual void debug(std::shared_ptr<spdlog::logger> logger, std::uint32_t indent) const noexcept override
    {
        logger->debug("{0: ^{1}}SETCOMP", "", indent);
    }
};

struct DictCompNode : Node
{
    Node* key;
    Node* value;
    Vector<Node*> generators;

    DictCompNode(Node* key, Node* value, std::uint32_t line, std::uint32_t column) : Node(ASTNodeDictComp, line, column),
                                                                                     key(key),
                                                                                     value(value) {}

    virtual const char* type_str() const noexcept override { return "DICT_COMP"; }

    virtual void debug(std::shared_ptr<spdlog::logger> logger, std::uint32_t indent) const noexcept override
    {
        logger->debug("{0: ^{1}}DICTCOMP", "", indent);
    }
};

struct GeneratorExprNode : Node
{
    Node* elt;
    Vector<Node*> generators;

    GeneratorExprNode(Node* elt, std::uint32_t line, std::uint32_t column) : Node(ASTNodeGeneratorExpr, line, column),
                                                                             elt(elt) {}

    virtual const char* type_str() const noexcept override { return "GENERATOR_EXPR"; }

    virtual void debug(std::shared_ptr<spdlog::logger> logger, std::uint32_t indent) const noexcept override
    {
        logger->debug("{0: ^{1}}GENERATOREXPR", "", indent);
    }
};

// Lambda

struct LambdaNode : Node
{
    Vector<Node*> args;
    Node* body;
    std::int32_t posonly_index = -1;
    std::int32_t kwonly_index = -1;

    LambdaNode(Node* body, std::uint32_t line, std::uint32_t column) : Node(ASTNodeLambda, line, column),
                                                                       body(body) {}

    virtual const char* type_str() const noexcept override { return "LAMBDA"; }

    virtual void debug(std::shared_ptr<spdlog::logger> logger, std::uint32_t indent) const noexcept override
    {
        logger->debug("{0: ^{1}}LAMBDA ({2} args)", "", indent, this->args.size());
    }
};

// Match/Case

struct MatchNode : Node
{
    Node* subject;
    Vector<Node*> cases;

    MatchNode(Node* subject,
              std::uint32_t line,
              std::uint32_t column) : Node(ASTNodeMatch, line, column),
                                      subject(subject) {}

    virtual const char* type_str() const noexcept override { return "MATCH"; }

    virtual void debug(std::shared_ptr<spdlog::logger> logger, std::uint32_t indent) const noexcept override
    {
        logger->debug("{0: ^{1}}MATCH", "", indent);
    }
};

struct MatchCaseNode : Node
{
    Node* pattern;
    Node* guard; // optional 'if' condition, nullptr if absent
    Vector<Node*> body;

    MatchCaseNode(Node* pattern,
                  Node* guard,
                  std::uint32_t line,
                  std::uint32_t column) : Node(ASTNodeMatchCase, line, column),
                                          pattern(pattern),
                                          guard(guard) {}

    virtual const char* type_str() const noexcept override { return "MATCH_CASE"; }

    virtual void debug(std::shared_ptr<spdlog::logger> logger, std::uint32_t indent) const noexcept override
    {
        logger->debug("{0: ^{1}}MATCHCASE", "", indent);
    }
};

// pattern1 | pattern2 | ...
struct MatchOrNode : Node
{
    Vector<Node*> patterns;

    MatchOrNode(std::uint32_t line,
                std::uint32_t column) : Node(ASTNodeMatchOr, line, column) {}

    virtual const char* type_str() const noexcept override { return "MATCH_OR"; }

    virtual void debug(std::shared_ptr<spdlog::logger> logger, std::uint32_t indent) const noexcept override
    {
        logger->debug("{0: ^{1}}MATCHOR", "", indent);
    }
};

// pattern as name (also used for wildcard _ and simple capture)
struct MatchAsNode : Node
{
    Node* pattern; // nullptr for wildcard '_' or simple capture
    StringD name; // empty for wildcard '_'

    MatchAsNode(Node* pattern,
                StringD name,
                std::uint32_t line,
                std::uint32_t column) : Node(ASTNodeMatchAs, line, column),
                                        pattern(pattern),
                                        name(std::move(name)) {}

    virtual const char* type_str() const noexcept override { return "MATCH_AS"; }

    virtual void debug(std::shared_ptr<spdlog::logger> logger, std::uint32_t indent) const noexcept override
    {
        logger->debug("{0: ^{1}}MATCHAS", "", indent);
    }
};

// Literal or dotted name: 42, "hello", Color.RED
struct MatchValueNode : Node
{
    Node* value;

    MatchValueNode(Node* value,
                   std::uint32_t line,
                   std::uint32_t column) : Node(ASTNodeMatchValue, line, column),
                                           value(value) {}

    virtual const char* type_str() const noexcept override { return "MATCH_VALUE"; }

    virtual void debug(std::shared_ptr<spdlog::logger> logger, std::uint32_t indent) const noexcept override
    {
        logger->debug("{0: ^{1}}MATCHVALUE", "", indent);
    }
};

// True, False, None
struct MatchSingletonNode : Node
{
    Keyword value;

    MatchSingletonNode(Keyword value,
                       std::uint32_t line,
                       std::uint32_t column) : Node(ASTNodeMatchSingleton, line, column),
                                               value(value) {}

    virtual const char* type_str() const noexcept override { return "MATCH_SINGLETON"; }

    virtual void debug(std::shared_ptr<spdlog::logger> logger, std::uint32_t indent) const noexcept override
    {
        logger->debug("{0: ^{1}}MATCHSINGLETON", "", indent);
    }
};

// [p1, p2, *rest, p3]
struct MatchSequenceNode : Node
{
    Vector<Node*> patterns;

    MatchSequenceNode(std::uint32_t line,
                      std::uint32_t column) : Node(ASTNodeMatchSequence, line, column) {}

    virtual const char* type_str() const noexcept override { return "MATCH_SEQUENCE"; }

    virtual void debug(std::shared_ptr<spdlog::logger> logger, std::uint32_t indent) const noexcept override
    {
        logger->debug("{0: ^{1}}MATCHSEQUENCE", "", indent);
    }
};

// {"key": pattern, **rest}
struct MatchMappingNode : Node
{
    Vector<Node*> keys;
    Vector<Node*> patterns;
    StringD rest; // **rest name, empty if absent

    MatchMappingNode(std::uint32_t line,
                     std::uint32_t column) : Node(ASTNodeMatchMapping, line, column) {}

    virtual const char* type_str() const noexcept override { return "MATCH_MAPPING"; }

    virtual void debug(std::shared_ptr<spdlog::logger> logger, std::uint32_t indent) const noexcept override
    {
        logger->debug("{0: ^{1}}MATCHMAPPING", "", indent);
    }
};

// ClassName(p1, p2, attr=p3)
struct MatchClassNode : Node
{
    Node* cls;
    Vector<Node*> patterns;
    Vector<StringD> kwd_attrs;
    Vector<Node*> kwd_patterns;

    MatchClassNode(Node* cls,
                   std::uint32_t line,
                   std::uint32_t column) : Node(ASTNodeMatchClass, line, column),
                                           cls(cls) {}

    virtual const char* type_str() const noexcept override { return "MATCH_CLASS"; }

    virtual void debug(std::shared_ptr<spdlog::logger> logger, std::uint32_t indent) const noexcept override
    {
        logger->debug("{0: ^{1}}MATCHCLASS", "", indent);
    }
};

// *name in sequence patterns
struct MatchStarNode : Node
{
    StringD name; // empty for *_ (wildcard star)

    MatchStarNode(StringD name,
                  std::uint32_t line,
                  std::uint32_t column) : Node(ASTNodeMatchStar, line, column),
                                          name(std::move(name)) {}

    virtual const char* type_str() const noexcept override { return "MATCH_STAR"; }

    virtual void debug(std::shared_ptr<spdlog::logger> logger, std::uint32_t indent) const noexcept override
    {
        logger->debug("{0: ^{1}}MATCHSTAR", "", indent);
    }
};

// Yield

struct YieldNode : Node
{
    Node* value; // nullptr for bare 'yield'

    YieldNode(Node* value,
              std::uint32_t line,
              std::uint32_t column) : Node(ASTNodeYield, line, column),
                                      value(value) {}

    virtual const char* type_str() const noexcept override { return "YIELD"; }

    virtual void debug(std::shared_ptr<spdlog::logger> logger, std::uint32_t indent) const noexcept override
    {
        logger->debug("{0: ^{1}}YIELD", "", indent);
    }
};

struct YieldFromNode : Node
{
    Node* value;

    YieldFromNode(Node* value,
                  std::uint32_t line,
                  std::uint32_t column) : Node(ASTNodeYieldFrom, line, column),
                                          value(value) {}

    virtual const char* type_str() const noexcept override { return "YIELD_FROM"; }

    virtual void debug(std::shared_ptr<spdlog::logger> logger, std::uint32_t indent) const noexcept override
    {
        logger->debug("{0: ^{1}}YIELDFROM", "", indent);
    }
};

// Types

struct TypeParamNode : Node
{
    StringD name;
    Node* bound = nullptr; // T: int -> bound is int, nullptr if absent
    Node* constraint = nullptr; // T: (int, str) -> TupleNode of constraints, nullptr if absent
    Node* default_value = nullptr;
    bool is_paramspec = false;  // **P
    bool is_typevartuple = false; // *Ts

    TypeParamNode(StringD name,
                  std::uint32_t line,
                  std::uint32_t column) : Node(ASTNodeTypeParam, line, column),
                                          name(std::move(name)) {}

    virtual const char* type_str() const noexcept override { return "TYPE_PARAM"; }

    virtual void debug(std::shared_ptr<spdlog::logger> logger, std::uint32_t indent) const noexcept override
    {
        logger->debug("{0: ^{1}}TYPEPARAM", "", indent);
    }
};

struct TypeAliasNode : Node
{
    StringD name;
    Vector<Node*> type_params;
    Node* value;

    TypeAliasNode(StringD name,
                  Node* value,
                  std::uint32_t line,
                  std::uint32_t column) : Node(ASTNodeTypeAlias, line, column),
                                          name(std::move(name)),
                                          value(value) {}

    virtual const char* type_str() const noexcept override { return "TYPE_ALIAS"; }

    virtual void debug(std::shared_ptr<spdlog::logger> logger, std::uint32_t indent) const noexcept override
    {
        logger->debug("{0: ^{1}}TYPEPALIAS", "", indent);
    }
};

// Name Qualifier

struct GlobalNode : Node
{
    Vector<Node*> names;

    GlobalNode(std::uint32_t line, std::uint32_t column) : Node(ASTNodeGlobal, line, column) {}

    virtual const char* type_str() const noexcept override { return "GLOBAL"; }

    virtual void debug(std::shared_ptr<spdlog::logger> logger, std::uint32_t indent) const noexcept override
    {
        logger->debug("{0: ^{1}}GLOBAL", "", indent);
    }
};

struct NonLocalNode : Node
{
    Vector<Node*> names;

    NonLocalNode(std::uint32_t line, std::uint32_t column) : Node(ASTNodeNonLocal, line, column) {}

    virtual const char* type_str() const noexcept override { return "NONLOCAL"; }

    virtual void debug(std::shared_ptr<spdlog::logger> logger, std::uint32_t indent) const noexcept override
    {
        logger->debug("{0: ^{1}}NONLOCAL", "", indent);
    }
};

// Del

struct DelNode : Node
{
    Vector<Node*> names;

    DelNode(std::uint32_t line, std::uint32_t column) : Node(ASTNodeDel, line, column) {}

    virtual const char* type_str() const noexcept override { return "DEL"; }

    virtual void debug(std::shared_ptr<spdlog::logger> logger, std::uint32_t indent) const noexcept override
    {
        logger->debug("{0: ^{1}}DEL", "", indent);
    }
};

// Await Expression/Statement

struct AwaitNode : Node
{
    Node* expr = nullptr;

    AwaitNode(Node* expr, std::uint32_t line, std::uint32_t column) : Node(ASTNodeAwait, line, column),
                                                                      expr(expr) {}

    virtual const char* type_str() const noexcept override { return "AWAIT"; }

    virtual void debug(std::shared_ptr<spdlog::logger> logger, std::uint32_t indent) const noexcept override
    {
        logger->debug("{0: ^{1}}AWAIT", "", indent);
    }
};

// Context managers

struct WithItemNode : Node
{
    Node* context_expr;
    Node* optional_vars; // 'as name'

    WithItemNode(Node* context_expr, Node* optional_vars, std::uint32_t line, std::uint32_t column) : Node(ASTNodeWithItem, line, column),
                                                                                                      context_expr(context_expr),
                                                                                                      optional_vars(optional_vars) {}

    virtual const char* type_str() const noexcept override { return "WITH_ITEM"; }

    virtual void debug(std::shared_ptr<spdlog::logger> logger, std::uint32_t indent) const noexcept override
    {
        logger->debug("{0: ^{1}}WITH_ITEM", "", indent);
    }
};

struct WithNode : Node
{
    Vector<Node*> items;
    Vector<Node*> body;

    WithNode(std::uint32_t line, std::uint32_t column) : Node(ASTNodeWith, line, column) {}

    virtual const char* type_str() const noexcept override { return "WITH"; }

    virtual void debug(std::shared_ptr<spdlog::logger> logger, std::uint32_t indent) const noexcept override
    {
        logger->debug("{0: ^{1}}WITH", "", indent);
    }
};

struct AsyncWithNode : Node
{
    Vector<Node*> items;
    Vector<Node*> body;

    AsyncWithNode(std::uint32_t line, std::uint32_t column) : Node(ASTNodeAsyncWith, line, column) {}

    virtual const char* type_str() const noexcept override { return "ASYNC_WITH"; }

    virtual void debug(std::shared_ptr<spdlog::logger> logger, std::uint32_t indent) const noexcept override
    {
        logger->debug("{0: ^{1}}ASYNC_WITH", "", indent);
    }
};

// Assertion

struct AssertionNode : Node
{
    Node* expr;
    Node* message;

    AssertionNode(Node* expr, Node* message, std::uint32_t line, std::uint32_t column) : Node(ASTNodeAssertion, line, column) {}

    virtual const char* type_str() const noexcept override { return "ASSERTION"; }

    virtual void debug(std::shared_ptr<spdlog::logger> logger, std::uint32_t indent) const noexcept override
    {
        logger->debug("{0: ^{1}}ASSERTION", "", indent);
    }
};

// Visitor

// Collects the immediate children of a node into a flat vector.
// Used by the generic visitor to recurse automatically.
STDROMANO_API void node_children(Node* node, Vector<Node*>& out) noexcept;

// Visit all nodes depth-first. The visitor callable receives Node* and returns
// true to recurse into children, false to skip them.
template<typename F>
void visit(Node* node, F&& visitor, std::uint32_t depth = 0) noexcept
{
    if(node == nullptr)
        return;

    if(!visitor(node, depth))
        return;

    Vector<Node*> children;

    node_children(node, children);

    for(auto* child : children)
        visit(child, visitor, depth + 1);
}

class STDROMANO_API AST
{
    Arena _nodes;

    ModuleNode* _root;

    std::shared_ptr<spdlog::logger> _logger = nullptr;

    bool lex(const char* buffer, Vector<Token>& tokens) noexcept;

    bool parse(const StringD& source_code, const Vector<Token>& tokens) noexcept;

public:
    AST(std::shared_ptr<spdlog::logger> logger = nullptr) : _root(nullptr),
                                                            _logger(logger == nullptr ? spdlog::default_logger() : logger) {}

    ModuleNode* root() noexcept { return this->_root; }
    const ModuleNode* root() const noexcept { return this->_root; }

    bool from_text(const StringD& buffer, const bool debug = false) noexcept;
};

PYTHON_NAMESPACE_END

STDROMANO_NAMESPACE_END

template<>
struct fmt::formatter<stdromano::Python::Token::Kind>
{
    constexpr auto parse(format_parse_context& ctx)
    {
        return ctx.begin();
    }

    auto format(const stdromano::Python::Token::Kind& kind, format_context& ctx) const
    {
        return format_to(ctx.out(), stdromano::Python::Token::kind_as_string(kind));
    }
};

template<>
struct fmt::formatter<stdromano::Python::Operator>
{
    constexpr auto parse(format_parse_context& ctx)
    {
        return ctx.begin();
    }

    auto format(const stdromano::Python::Operator& op, format_context& ctx) const
    {
        switch(op)
        {
            case stdromano::Python::Addition:
                return format_to(ctx.out(), "+");
            case stdromano::Python::Subtraction:
                return format_to(ctx.out(), "-");
            case stdromano::Python::Multiplication:
                return format_to(ctx.out(), "*");
            case stdromano::Python::Division:
                return format_to(ctx.out(), "/");
            case stdromano::Python::Modulus:
                return format_to(ctx.out(), "%");
            case stdromano::Python::Exponentiation:
                return format_to(ctx.out(), "**");
            case stdromano::Python::FloorDivision:
                return format_to(ctx.out(), "//");
            case stdromano::Python::MatMul:
                return format_to(ctx.out(), "@");
            case stdromano::Python::Assign:
                return format_to(ctx.out(), "=");
            case stdromano::Python::AdditionAssign:
                return format_to(ctx.out(), "+=");
            case stdromano::Python::SubtractionAssign:
                return format_to(ctx.out(), "-=");
            case stdromano::Python::MultiplicationAssign:
                return format_to(ctx.out(), "*=");
            case stdromano::Python::DivisionAssign:
                return format_to(ctx.out(), "/=");
            case stdromano::Python::ModulusAssign:
                return format_to(ctx.out(), "%=");
            case stdromano::Python::FloorDivisionAssign:
                return format_to(ctx.out(), "//=");
            case stdromano::Python::MatMulAssign:
                return format_to(ctx.out(), "@=");
            case stdromano::Python::ExponentiationAssign:
                return format_to(ctx.out(), "**=");
            case stdromano::Python::BitwiseAndAssign:
                return format_to(ctx.out(), "&=");
            case stdromano::Python::BitwiseOrAssign:
                return format_to(ctx.out(), "|=");
            case stdromano::Python::BitwiseXorAssign:
                return format_to(ctx.out(), "^=");
            case stdromano::Python::BitwiseLShiftAssign:
                return format_to(ctx.out(), "<<=");
            case stdromano::Python::BitwiseRShiftAssign:
                return format_to(ctx.out(), ">>=");
            case stdromano::Python::BitwiseAnd:
                return format_to(ctx.out(), "&");
            case stdromano::Python::BitwiseOr:
                return format_to(ctx.out(), "|");
            case stdromano::Python::BitwiseXor:
                return format_to(ctx.out(), "^");
            case stdromano::Python::BitwiseNot:
                return format_to(ctx.out(), "~");
            case stdromano::Python::BitwiseLShift:
                return format_to(ctx.out(), "<<");
            case stdromano::Python::BitwiseRShift:
                return format_to(ctx.out(), ">>");
            case stdromano::Python::ComparatorEquals:
                return format_to(ctx.out(), "==");
            case stdromano::Python::ComparatorNotEquals:
                return format_to(ctx.out(), "!=");
            case stdromano::Python::ComparatorGreaterThan:
                return format_to(ctx.out(), ">");
            case stdromano::Python::ComparatorLessThan:
                return format_to(ctx.out(), "<");
            case stdromano::Python::ComparatorGreaterEqualsThan:
                return format_to(ctx.out(), ">=");
            case stdromano::Python::ComparatorLessEqualsThan:
                return format_to(ctx.out(), "<=");
            case stdromano::Python::LogicalAnd:
                return format_to(ctx.out(), "and");
            case stdromano::Python::LogicalOr:
                return format_to(ctx.out(), "or");
            case stdromano::Python::LogicalNot:
                return format_to(ctx.out(), "not");
            case stdromano::Python::IdentityIs:
                return format_to(ctx.out(), "is");
            case stdromano::Python::MembershipIn:
                return format_to(ctx.out(), "in");
            case stdromano::Python::MembershipNotIn:
                return format_to(ctx.out(), "not in");
            default:
                return format_to(ctx.out(), "?");
        }
    }
};

template<>
struct fmt::formatter<stdromano::Python::Literal>
{
    constexpr auto parse(format_parse_context& ctx)
    {
        return ctx.begin();
    }

    auto format(const stdromano::Python::Literal& lit, format_context& ctx) const
    {
        switch(lit)
        {
            case stdromano::Python::String:
                return format_to(ctx.out(), "String");
            case stdromano::Python::UnicodeString:
                return format_to(ctx.out(), "UnicodeString");
            case stdromano::Python::RawString:
                return format_to(ctx.out(), "RawString");
            case stdromano::Python::FormattedString:
                return format_to(ctx.out(), "FormattedString");
            case stdromano::Python::Bytes:
                return format_to(ctx.out(), "Bytes");
            case stdromano::Python::Integer:
                return format_to(ctx.out(), "Integer");
            case stdromano::Python::Float:
                return format_to(ctx.out(), "Float");
            default:
                return format_to(ctx.out(), "Unknown");
        }
    }
};

#endif // !defined(__STDROMANO_PYTHON)
