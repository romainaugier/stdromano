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
};

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
    Elif = 31,
    If = 32,
    Or = 33,
    Yield = 34,
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
    RightArrow = 12,
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
    MembershipNotIn = 39,
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

// AST Node

enum NodeType : std::uint32_t
{
    NodeModule = 0,
    NodeFunctionDef,
    NodeClassDef,
    NodeReturn,
    NodeAssign,
    NodeAugAssign,
    NodeFor,
    NodeWhile,
    NodeIf,
    NodeImport,
    NodeImportFrom,
    NodeExpr,
    NodePass,
    NodeBreak,
    NodeContinue,
    NodeRaise,
    NodeBinOp,
    NodeUnaryOp,
    NodeBoolOp,
    NodeCompare,
    NodeCall,
    NodeName,
    NodeConstant,
    NodeAttribute,
    NodeSubscript,
    NodeList,
    NodeTuple,
    NodeDict,
    NodeLambda,
    NodeCount,
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

    virtual const char* type_str() const noexcept { return "Node"; }

    virtual void debug(std::shared_ptr<spdlog::logger> logger, std::uint32_t indent) const noexcept
    {
        logger->debug("{0: ^{1}}NODE", "", indent);
    }
};

struct ModuleNode : Node
{
    Vector<Node*> body;

    ModuleNode() : Node(NodeModule, 0, 0) {}

    virtual const char* type_str() const noexcept override { return "MODULE"; }

    virtual void debug(std::shared_ptr<spdlog::logger> logger, std::uint32_t indent) const noexcept override
    {
        logger->debug("{0: ^{1}}MODULE", "", indent);
    }
};

struct FunctionDefNode : Node
{
    StringD name;
    Vector<Node*> args;
    Vector<Node*> body;
    Node* return_annotation;

    FunctionDefNode(StringD name, std::uint32_t line, std::uint32_t column) : Node(NodeFunctionDef, line, column),
                                                                              name(std::move(name)),
                                                                              return_annotation(nullptr) {}

    virtual const char* type_str() const noexcept override { return "FUNCDEF"; }

    virtual void debug(std::shared_ptr<spdlog::logger> logger, std::uint32_t indent) const noexcept override
    {
        logger->debug("{0: ^{1}}FUNCDEF ({2})", "", indent, this->name);
    }
};

struct ClassDefNode : Node
{
    StringD name;
    Vector<Node*> bases;
    Vector<Node*> body;

    ClassDefNode(StringD name, std::uint32_t line, std::uint32_t column) : Node(NodeClassDef, line, column),
                                                                           name(std::move(name)) {}

    virtual const char* type_str() const noexcept override { return "CLASSDEF"; }

    virtual void debug(std::shared_ptr<spdlog::logger> logger, std::uint32_t indent) const noexcept override
    {
        logger->debug("{0: ^{1}}CLASSDEF ({2})", "", indent, this->name);
    }
};

struct ReturnNode : Node
{
    Node* value;

    ReturnNode(Node* value, std::uint32_t line, std::uint32_t column) : Node(NodeReturn, line, column),
                                                                        value(value) {}

    virtual const char* type_str() const noexcept override { return "RETURN"; }

    virtual void debug(std::shared_ptr<spdlog::logger> logger, std::uint32_t indent) const noexcept override
    {
        logger->debug("{0: ^{1}}RETURN", "", indent);
    }
};

struct AssignNode : Node
{
    Vector<Node*> targets;
    Node* value;

    AssignNode(Node* value, std::uint32_t line, std::uint32_t column) : Node(NodeAssign, line, column),
                                                                        value(value) {}

    virtual const char* type_str() const noexcept override { return "ASSIGN"; };

    virtual void debug(std::shared_ptr<spdlog::logger> logger, std::uint32_t indent) const noexcept override
    {
        logger->debug("{0: ^{1}}ASSIGN", "", indent);
    }
};

struct AugAssignNode : Node
{
    Node* target;
    Operator op;
    Node* value;

    AugAssignNode(Node* target, Operator op, Node* value, std::uint32_t line, std::uint32_t column) : Node(NodeAugAssign, line, column),
                                                                                                      target(target),
                                                                                                      op(op),
                                                                                                      value(value) {}

    virtual const char* type_str() const noexcept override { return "AUGASSIGN"; }

    virtual void debug(std::shared_ptr<spdlog::logger> logger, std::uint32_t indent) const noexcept override
    {
        logger->debug("{0: ^{1}}AUGASSIGN ({2})", "", indent, this->op);
    }
};

struct ForNode : Node
{
    Node* target;
    Node* iter;
    Vector<Node*> body;
    Vector<Node*> orelse;

    ForNode(Node* target, Node* iter, std::uint32_t line, std::uint32_t column) : Node(NodeFor, line, column),
                                                                                  target(target),
                                                                                  iter(iter) {}

    virtual const char* type_str() const noexcept override { return "FOR"; }

    virtual void debug(std::shared_ptr<spdlog::logger> logger, std::uint32_t indent) const noexcept override
    {
        logger->debug("{0: ^{1}}FOR", "", indent);
    }
};

struct WhileNode : Node
{
    Node* test;
    Vector<Node*> body;
    Vector<Node*> orelse;

    WhileNode(Node* test, std::uint32_t line, std::uint32_t column) : Node(NodeWhile, line, column),
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

    IfNode(Node* test, std::uint32_t line, std::uint32_t column) : Node(NodeIf, line, column),
                                                                   test(test) {}

    virtual const char* type_str() const noexcept override { return "IF"; }

    virtual void debug(std::shared_ptr<spdlog::logger> logger, std::uint32_t indent) const noexcept override
    {
        logger->debug("{0: ^{1}}IF","", indent);
    }
};

struct ImportNode : Node
{
    Vector<StringD> names;
    Vector<StringD> aliases;

    ImportNode(std::uint32_t line, std::uint32_t column) : Node(NodeImport, line, column) {}

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

    ImportFromNode(StringD module, std::uint32_t line, std::uint32_t column) : Node(NodeImportFrom, line, column),
                                                                               module(std::move(module)) {}

    virtual const char* type_str() const noexcept override { return "IMPORT FROM"; }

    virtual void debug(std::shared_ptr<spdlog::logger> logger, std::uint32_t indent) const noexcept override
    {
        logger->debug("{0: ^{1}}IMPORT FROM ({2})", "", indent, this->module);
    }
};

struct ExprNode : Node
{
    Node* value;

    ExprNode(Node* value, std::uint32_t line, std::uint32_t column) : Node(NodeExpr, line, column),
                                                                      value(value) {}

    virtual const char* type_str() const noexcept override { return "EXPR"; }

    virtual void debug(std::shared_ptr<spdlog::logger> logger, std::uint32_t indent) const noexcept override
    {
        logger->debug("{0: ^{1}}EXPR", "", indent);
    }
};

struct PassNode : Node
{
    PassNode(std::uint32_t line, std::uint32_t column) : Node(NodePass, line, column) {}

    virtual const char* type_str() const noexcept override { return "PASS"; }

    virtual void debug(std::shared_ptr<spdlog::logger> logger, std::uint32_t indent) const noexcept override
    {
        logger->debug("{0: ^{1}}PASS", "", indent);
    }
};

struct BreakNode : Node
{
    BreakNode(std::uint32_t line, std::uint32_t column) : Node(NodeBreak, line, column) {}

    virtual const char* type_str() const noexcept override { return "BREAK"; }

    virtual void debug(std::shared_ptr<spdlog::logger> logger, std::uint32_t indent) const noexcept override
    {
        logger->debug("{0: ^{1}}BREAK", "", indent);
    }
};

struct ContinueNode : Node
{
    ContinueNode(std::uint32_t line, std::uint32_t column) : Node(NodeContinue, line, column) {}

    virtual const char* type_str() const noexcept override { return "CONTINUE"; }

    virtual void debug(std::shared_ptr<spdlog::logger> logger, std::uint32_t indent) const noexcept override
    {
        logger->debug("{0: ^{1}}CONTINUE", "", indent);
    }
};

struct RaiseNode : Node
{
    Node* exc;

    RaiseNode(Node* exc, std::uint32_t line, std::uint32_t column) : Node(NodeRaise, line, column),
                                                                     exc(exc) {}

    virtual const char* type_str() const noexcept override { return "RAISE"; }

    virtual void debug(std::shared_ptr<spdlog::logger> logger, std::uint32_t indent) const noexcept override
    {
        logger->debug("{0: ^{1}}RAISE", "", indent);
    }
};

struct BinOpNode : Node
{
    Node* left;
    Operator op;
    Node* right;

    BinOpNode(Node* left, Operator op, Node* right, std::uint32_t line, std::uint32_t column) : Node(NodeBinOp, line, column),
                                                                                                left(left),
                                                                                                op(op),
                                                                                                right(right) {}

    virtual const char* type_str() const noexcept override { return "BINOP"; }

    virtual void debug(std::shared_ptr<spdlog::logger> logger, std::uint32_t indent) const noexcept override
    {
        logger->debug("{0: ^{1}}BINOP ({2})", "", indent, this->op);
    }
};

struct UnaryOpNode : Node
{
    Operator op;
    Node* operand;

    UnaryOpNode(Operator op, Node* operand, std::uint32_t line, std::uint32_t column) : Node(NodeUnaryOp, line, column),
                                                                                        op(op),
                                                                                        operand(operand) {}

    virtual const char* type_str() const noexcept override { return "UNOP"; }

    virtual void debug(std::shared_ptr<spdlog::logger> logger, std::uint32_t indent) const noexcept override
    {
        logger->debug("{0: ^{1}}UNOP ({2})", "", indent, this->op);
    }
};

struct BoolOpNode : Node
{
    Operator op;
    Vector<Node*> values;

    BoolOpNode(Operator op, std::uint32_t line, std::uint32_t column) : Node(NodeBoolOp, line, column),
                                                                        op(op) {}

    virtual const char* type_str() const noexcept override { return "BOOLOP"; }

    virtual void debug(std::shared_ptr<spdlog::logger> logger, std::uint32_t indent) const noexcept override
    {
        logger->debug("{0: ^{1}}BOOLOP ({2})", "", indent, this->op);
    }
};

struct CompareNode : Node
{
    Node* left;
    Vector<Operator> ops;
    Vector<Node*> comparators;

    CompareNode(Node* left, std::uint32_t line, std::uint32_t column) : Node(NodeCompare, line, column),
                                                                        left(left) {}

    virtual const char* type_str() const noexcept override { return "COMPARE"; }

    virtual void debug(std::shared_ptr<spdlog::logger> logger, std::uint32_t indent) const noexcept override
    {
        logger->debug("{0: ^{1}}COMPARE", "", indent);
    }
};

struct CallNode : Node
{
    Node* func;
    Vector<Node*> args;

    CallNode(Node* func, std::uint32_t line, std::uint32_t column) : Node(NodeCall, line, column),
                                                                     func(func) {}

    virtual const char* type_str() const noexcept override { return "CALL"; }

    virtual void debug(std::shared_ptr<spdlog::logger> logger, std::uint32_t indent) const noexcept override
    {
        logger->debug("{0: ^{1}}CALL", "", indent);
    }
};

struct NameNode : Node
{
    StringD id;

    NameNode(StringD id, std::uint32_t line, std::uint32_t column) : Node(NodeName, line, column),
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

    ConstantNode(StringD raw, Literal literal_type, std::uint32_t line, std::uint32_t column) : Node(NodeConstant, line, column),
                                                                                                raw(std::move(raw)),
                                                                                                literal_type(literal_type) {}

    virtual const char* type_str() const noexcept override { return "CONSTANT"; }

    virtual void debug(std::shared_ptr<spdlog::logger> logger, std::uint32_t indent) const noexcept override
    {
        logger->debug("{0: ^{1}}CONSTANT ({2}: {3})", "", indent, this->literal_type, this->raw);
    }
};

struct AttributeNode : Node
{
    Node* value;
    StringD attr;

    AttributeNode(Node* value, StringD attr, std::uint32_t line, std::uint32_t column) : Node(NodeAttribute, line, column),
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

    SubscriptNode(Node* value, Node* slice, std::uint32_t line, std::uint32_t column) : Node(NodeSubscript, line, column),
                                                                                        value(value),
                                                                                        slice(slice) {}

    virtual const char* type_str() const noexcept override { return "SUBSCRIPT"; }

    virtual void debug(std::shared_ptr<spdlog::logger> logger, std::uint32_t indent) const noexcept override
    {
        logger->debug("{0: ^{1}}SUBSCRIPT", "", indent);
    }
};

struct ListNode : Node
{
    Vector<Node*> elts;

    ListNode(std::uint32_t line, std::uint32_t column) : Node(NodeList, line, column) {}

    virtual const char* type_str() const noexcept override { return "LIST"; }

    virtual void debug(std::shared_ptr<spdlog::logger> logger, std::uint32_t indent) const noexcept override
    {
        logger->debug("{0: ^{1}}LIST ({2} elements)", "", indent, this->elts.size());
    }
};

struct TupleNode : Node
{
    Vector<Node*> elts;

    TupleNode(std::uint32_t line, std::uint32_t column) : Node(NodeTuple, line, column) {}

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

    DictNode(std::uint32_t line, std::uint32_t column) : Node(NodeDict, line, column) {}

    virtual const char* type_str() const noexcept override { return "DICT"; }

    virtual void debug(std::shared_ptr<spdlog::logger> logger, std::uint32_t indent) const noexcept override
    {
        logger->debug("{0: ^{1}}DICT ({2} elements)", "", indent, this->keys.size());
    }
};

struct LambdaNode : Node
{
    Vector<Node*> args;
    Node* body;

    LambdaNode(Node* body, std::uint32_t line, std::uint32_t column) : Node(NodeLambda, line, column),
                                                                       body(body) {}

    virtual const char* type_str() const noexcept override { return "LAMBDA"; }

    virtual void debug(std::shared_ptr<spdlog::logger> logger, std::uint32_t indent) const noexcept override
    {
        logger->debug("{0: ^{1}}LAMBDA ({2} args)", "", indent, this->args.size());
    }
};

// Visitor

// Collects the immediate children of a node into a flat vector.
// Used by the generic visitor to recurse automatically.
inline void node_children(Node* node, Vector<Node*>& out) noexcept
{
    if(node == nullptr)
        return;

    switch(node->type())
    {
        case NodeModule:
        {
            auto* n = static_cast<ModuleNode*>(node);
            for(auto* c : n->body) out.push_back(c);
            break;
        }
        case NodeFunctionDef:
        {
            auto* n = static_cast<FunctionDefNode*>(node);
            for(auto* c : n->args) out.push_back(c);
            for(auto* c : n->body) out.push_back(c);
            if(n->return_annotation) out.push_back(n->return_annotation);
            break;
        }
        case NodeClassDef:
        {
            auto* n = static_cast<ClassDefNode*>(node);
            for(auto* c : n->bases) out.push_back(c);
            for(auto* c : n->body) out.push_back(c);
            break;
        }
        case NodeReturn:
        {
            auto* n = static_cast<ReturnNode*>(node);
            if(n->value) out.push_back(n->value);
            break;
        }
        case NodeAssign:
        {
            auto* n = static_cast<AssignNode*>(node);
            for(auto* c : n->targets) out.push_back(c);
            if(n->value) out.push_back(n->value);
            break;
        }
        case NodeAugAssign:
        {
            auto* n = static_cast<AugAssignNode*>(node);
            if(n->target) out.push_back(n->target);
            if(n->value) out.push_back(n->value);
            break;
        }
        case NodeFor:
        {
            auto* n = static_cast<ForNode*>(node);
            if(n->target) out.push_back(n->target);
            if(n->iter) out.push_back(n->iter);
            for(auto* c : n->body) out.push_back(c);
            for(auto* c : n->orelse) out.push_back(c);
            break;
        }
        case NodeWhile:
        {
            auto* n = static_cast<WhileNode*>(node);
            if(n->test) out.push_back(n->test);
            for(auto* c : n->body) out.push_back(c);
            for(auto* c : n->orelse) out.push_back(c);
            break;
        }
        case NodeIf:
        {
            auto* n = static_cast<IfNode*>(node);
            if(n->test) out.push_back(n->test);
            for(auto* c : n->body) out.push_back(c);
            for(auto* c : n->orelse) out.push_back(c);
            break;
        }
        case NodeExpr:
        {
            auto* n = static_cast<ExprNode*>(node);
            if(n->value) out.push_back(n->value);
            break;
        }
        case NodeRaise:
        {
            auto* n = static_cast<RaiseNode*>(node);
            if(n->exc) out.push_back(n->exc);
            break;
        }
        case NodeBinOp:
        {
            auto* n = static_cast<BinOpNode*>(node);
            if(n->left) out.push_back(n->left);
            if(n->right) out.push_back(n->right);
            break;
        }
        case NodeUnaryOp:
        {
            auto* n = static_cast<UnaryOpNode*>(node);
            if(n->operand) out.push_back(n->operand);
            break;
        }
        case NodeBoolOp:
        {
            auto* n = static_cast<BoolOpNode*>(node);
            for(auto* c : n->values) out.push_back(c);
            break;
        }
        case NodeCompare:
        {
            auto* n = static_cast<CompareNode*>(node);
            if(n->left) out.push_back(n->left);
            for(auto* c : n->comparators) out.push_back(c);
            break;
        }
        case NodeCall:
        {
            auto* n = static_cast<CallNode*>(node);
            if(n->func) out.push_back(n->func);
            for(auto* c : n->args) out.push_back(c);
            break;
        }
        case NodeAttribute:
        {
            auto* n = static_cast<AttributeNode*>(node);
            if(n->value) out.push_back(n->value);
            break;
        }
        case NodeSubscript:
        {
            auto* n = static_cast<SubscriptNode*>(node);
            if(n->value) out.push_back(n->value);
            if(n->slice) out.push_back(n->slice);
            break;
        }
        case NodeList:
        {
            auto* n = static_cast<ListNode*>(node);
            for(auto* c : n->elts) out.push_back(c);
            break;
        }
        case NodeTuple:
        {
            auto* n = static_cast<TupleNode*>(node);
            for(auto* c : n->elts) out.push_back(c);
            break;
        }
        case NodeDict:
        {
            auto* n = static_cast<DictNode*>(node);
            for(auto* c : n->keys) out.push_back(c);
            for(auto* c : n->values) out.push_back(c);
            break;
        }
        case NodeLambda:
        {
            auto* n = static_cast<LambdaNode*>(node);
            for(auto* c : n->args) out.push_back(c);
            if(n->body) out.push_back(n->body);
            break;
        }
        default:
            break;
    }
}

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
            case stdromano::Python::IdentityIsNot:
                return format_to(ctx.out(), "is not");
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
