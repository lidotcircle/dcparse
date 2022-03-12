#ifndef _C_PARSER_AST_H_
#define _C_PARSER_AST_H_

#include "parser/parser.h"
#include "./c_token.h"
#include "./c_reporter.h"
#include <string>
#include <memory>
#include <variant>
#include <set>

namespace cparser {


using ASTNodeParserContext = std::weak_ptr<DCParser::DCParserContext>;

class CParserContext;
class ASTNode {
private:
    ASTNodeParserContext m_parser_context;
    size_t m_start_pos, m_end_pos;

public:
    size_t& start_pos() { return m_start_pos; }
    size_t& end_pos() { return m_end_pos; }
    inline ASTNode(ASTNodeParserContext p): m_parser_context(p), m_start_pos(0), m_end_pos(0) {}
    std::shared_ptr<CParserContext> context() const;
    inline ASTNodeParserContext lcontext() const { return m_parser_context; }

    virtual void check_constraints(std::shared_ptr<SemanticReporter> reporter) = 0;
    virtual std::string to_string() const;
    virtual ~ASTNode() = default;
};


// forward declaration
class ASTNodeVariableType;
class ASTNodeInitializer;
class ASTNodeInitializerList;

class ASTNodeBlockItem : public ASTNode {
public:
    ASTNodeBlockItem(ASTNodeParserContext p): ASTNode(p) {}
};

class ASTNodeExternalDeclaration : public ASTNodeBlockItem {
public:
    ASTNodeExternalDeclaration(ASTNodeParserContext p): ASTNodeBlockItem(p) {}
};


/******************************************************************************
 *                                                                            *
 *                               Expression                                   *
 *                                                                            *
 ******************************************************************************/
class ASTNodeVariableType;
class ASTNodeExpr : public ASTNode {
private:
    std::shared_ptr<ASTNodeVariableType> m_type;

public:
    inline ASTNodeExpr(ASTNodeParserContext p): ASTNode(p) {}
    inline void resolve_type(std::shared_ptr<ASTNodeVariableType> type)
    {
        assert(this->m_type == nullptr);
        assert(type != nullptr);
        this->m_type = type;
    }
    inline std::shared_ptr<ASTNodeVariableType> type() const { return m_type; }

    virtual bool is_constexpr() const;
    virtual std::optional<long long>   get_integer_constant() const = 0;
    virtual std::optional<long double> get_float_constant() const = 0;

    virtual bool is_lvalue() const = 0;
};

class ASTNodeExprIdentifier: public ASTNodeExpr {
private:
    std::shared_ptr<TokenID> m_id;

public:
    inline ASTNodeExprIdentifier(ASTNodeParserContext p, std::shared_ptr<TokenID> id): ASTNodeExpr(p), m_id(id) {}
    inline std::shared_ptr<TokenID> id() { return this->m_id; }
    
    virtual void check_constraints(std::shared_ptr<SemanticReporter> reporter) override;
    virtual std::optional<long long> get_integer_constant() const override;
    virtual std::optional<long double> get_float_constant() const override;
    virtual bool is_lvalue() const override;
};

class ASTNodeExprString: public ASTNodeExpr {
private:
    std::shared_ptr<TokenStringLiteral> m_token;

public:
    inline ASTNodeExprString(ASTNodeParserContext p, std::shared_ptr<TokenStringLiteral> str): ASTNodeExpr(p), m_token(str) {}
    inline std::shared_ptr<TokenStringLiteral> token() { return this->m_token; }

    virtual void check_constraints(std::shared_ptr<SemanticReporter> reporter) override;
    virtual std::optional<long long> get_integer_constant() const override;
    virtual std::optional<long double> get_float_constant() const override;
    virtual bool is_lvalue() const override;
};

class ASTNodeExprInteger: public ASTNodeExpr {
private:
    std::shared_ptr<TokenConstantInteger> m_token;

public:
    inline ASTNodeExprInteger(ASTNodeParserContext p, std::shared_ptr<TokenConstantInteger> val): ASTNodeExpr(p), m_token(val) {}
    inline std::shared_ptr<TokenConstantInteger> token() { return this->m_token; }

    virtual void check_constraints(std::shared_ptr<SemanticReporter> reporter) override;
    virtual std::optional<long long> get_integer_constant() const override;
    virtual std::optional<long double> get_float_constant() const override;
    virtual bool is_lvalue() const override;
};

class ASTNodeExprFloat: public ASTNodeExpr {
private:
    std::shared_ptr<TokenConstantFloat> m_token;

public:
    inline ASTNodeExprFloat(ASTNodeParserContext p, std::shared_ptr<TokenConstantFloat> val): ASTNodeExpr(p), m_token(val) {}
    inline std::shared_ptr<TokenConstantFloat> token() { return this->m_token; }

    virtual void check_constraints(std::shared_ptr<SemanticReporter> reporter) override;
    virtual std::optional<long long> get_integer_constant() const override;
    virtual std::optional<long double> get_float_constant() const override;
    virtual bool is_lvalue() const override;
};

class ASTNodeExprIndexing: public ASTNodeExpr {
private:
    std::shared_ptr<ASTNodeExpr> m_array, m_idx;

public:
    inline ASTNodeExprIndexing(ASTNodeParserContext p, std::shared_ptr<ASTNodeExpr> array, std::shared_ptr<ASTNodeExpr> idx): ASTNodeExpr(p), m_array(array), m_idx(idx) {}
    inline std::shared_ptr<ASTNodeExpr> array() { return this->m_array; }
    inline std::shared_ptr<ASTNodeExpr> idx() { return this->m_idx; }

    virtual void check_constraints(std::shared_ptr<SemanticReporter> reporter) override;
    virtual std::optional<long long> get_integer_constant() const override;
    virtual std::optional<long double> get_float_constant() const override;
    virtual bool is_lvalue() const override;
};

class ASTNodeArgList: public ASTNode, private std::vector<std::shared_ptr<ASTNodeExpr>>
{
private:
    using container_t = std::vector<std::shared_ptr<ASTNodeExpr>>;

public:
    using reference = container_t::reference;
    using const_reference = container_t::const_reference;
    using iterator = container_t::iterator;
    using const_iterator = container_t::const_iterator;

    inline ASTNodeArgList(
            ASTNodeParserContext c): ASTNode(c) {}

    using container_t::begin;
    using container_t::end;
    using container_t::empty;
    using container_t::size;
    using container_t::operator[];
    using container_t::push_back;

    virtual void check_constraints(std::shared_ptr<SemanticReporter> reporter) override;
};

class ASTNodeExprFunctionCall: public ASTNodeExpr {
private:
    std::shared_ptr<ASTNodeExpr> m_func;
    std::shared_ptr<ASTNodeArgList> m_args;

public:
    inline ASTNodeExprFunctionCall(
            ASTNodeParserContext c,
            std::shared_ptr<ASTNodeExpr> func,
            std::shared_ptr<ASTNodeArgList> args): ASTNodeExpr(c), m_func(func), m_args(args) {}
    inline std::shared_ptr<ASTNodeExpr> func() { return this->m_func; }
    inline std::shared_ptr<ASTNodeArgList> args() { return this->m_args; }

    virtual void check_constraints(std::shared_ptr<SemanticReporter> reporter) override;
    virtual std::optional<long long> get_integer_constant() const override;
    virtual std::optional<long double> get_float_constant() const override;
    virtual bool is_lvalue() const override;
};

class ASTNodeExprMemberAccess: public ASTNodeExpr {
private:
    std::shared_ptr<ASTNodeExpr> m_obj;
    std::shared_ptr<TokenID> m_member;
    std::shared_ptr<ASTNodeVariableType> m_type;

public:
    inline ASTNodeExprMemberAccess(
            ASTNodeParserContext c,
            std::shared_ptr<ASTNodeExpr> obj,
            std::shared_ptr<TokenID> member): ASTNodeExpr(c), m_obj(obj), m_member(member) {}
    inline std::shared_ptr<ASTNodeExpr> obj() { return this->m_obj; }
    inline std::shared_ptr<TokenID> member() { return this->m_member; }

    virtual void check_constraints(std::shared_ptr<SemanticReporter> reporter) override;
    virtual std::optional<long long> get_integer_constant() const override;
    virtual std::optional<long double> get_float_constant() const override;
    virtual bool is_lvalue() const override;
};

class ASTNodeExprPointerMemberAccess: public ASTNodeExpr {
private:
    std::shared_ptr<ASTNodeExpr> m_obj;
    std::shared_ptr<TokenID> m_member;

public:
    inline ASTNodeExprPointerMemberAccess(
            ASTNodeParserContext c,
            std::shared_ptr<ASTNodeExpr> obj,
            std::shared_ptr<TokenID> member): ASTNodeExpr(c), m_obj(obj), m_member(member) {}
    inline std::shared_ptr<ASTNodeExpr> obj() { return this->m_obj; }
    inline std::shared_ptr<TokenID> member() { return this->m_member; }

    virtual void check_constraints(std::shared_ptr<SemanticReporter> reporter) override;
    virtual std::optional<long long> get_integer_constant() const override;
    virtual std::optional<long double> get_float_constant() const override;
    virtual bool is_lvalue() const override;
};

class ASTNodeExprInitializer: public ASTNodeExpr {
private:
    std::shared_ptr<ASTNodeInitializerList> m_init;
    std::shared_ptr<ASTNodeVariableType> m_type;

public:
    inline ASTNodeExprInitializer(
            ASTNodeParserContext c,
            std::shared_ptr<ASTNodeVariableType> type,
            std::shared_ptr<ASTNodeInitializerList> init
            ): ASTNodeExpr(c), m_init(init), m_type(type) {}
    inline std::shared_ptr<ASTNodeInitializerList> init() { return this->m_init; }
    inline std::shared_ptr<ASTNodeVariableType> type() { return this->m_type; }

    virtual void check_constraints(std::shared_ptr<SemanticReporter> reporter) override;
    virtual std::optional<long long> get_integer_constant() const override;
    virtual std::optional<long double> get_float_constant() const override;
    virtual bool is_lvalue() const override;
};

class ASTNodeExprUnaryOp: public ASTNodeExpr {
public:
    enum UnaryOperatorType {
        POSFIX_INC, POSFIX_DEC,
        PREFIX_INC, PREFIX_DEC,
        REFERENCE, DEREFERENCE,
        PLUS, MINUS,
        BITWISE_NOT,
        LOGICAL_NOT,
        SIZEOF,
    };

private:
    UnaryOperatorType m_operator;
    std::shared_ptr<ASTNodeExpr> m_expr;

public:
    inline ASTNodeExprUnaryOp(
            ASTNodeParserContext c,
            UnaryOperatorType optype,
            std::shared_ptr<ASTNodeExpr> expr
            ):
        ASTNodeExpr(c), m_operator(optype),
        m_expr(expr) {}

    virtual void check_constraints(std::shared_ptr<SemanticReporter> reporter) override;
    virtual std::optional<long long> get_integer_constant() const override;
    virtual std::optional<long double> get_float_constant() const override;
    virtual bool is_lvalue() const override;
};

class ASTNodeExprSizeof: public ASTNodeExpr {
private:
    std::shared_ptr<ASTNodeVariableType> m_type;

public:
    inline ASTNodeExprSizeof(
            ASTNodeParserContext c,
            std::shared_ptr<ASTNodeVariableType> type
            ):
        ASTNodeExpr(c), m_type(type) {}

    virtual void check_constraints(std::shared_ptr<SemanticReporter> reporter) override;
    virtual std::optional<long long> get_integer_constant() const override;
    virtual std::optional<long double> get_float_constant() const override;
    virtual bool is_lvalue() const override;
};

class ASTNodeExprCast: public ASTNodeExpr {
private:
    std::shared_ptr<ASTNodeVariableType> m_type;
    std::shared_ptr<ASTNodeExpr> m_expr;

public:
    inline ASTNodeExprCast(
            ASTNodeParserContext c,
            std::shared_ptr<ASTNodeVariableType> type,
            std::shared_ptr<ASTNodeExpr> expr
            ):
        ASTNodeExpr(c), m_type(type),
        m_expr(expr) {}

    virtual void check_constraints(std::shared_ptr<SemanticReporter> reporter) override;
    virtual std::optional<long long> get_integer_constant() const override;
    virtual std::optional<long double> get_float_constant() const override;
    virtual bool is_lvalue() const override;
};

class ASTNodeExprBinaryOp: public ASTNodeExpr {
public:
    enum BinaryOperatorType {
        MULTIPLY, DIVISION,
        REMAINDER, 
        PLUS, MINUS,
        LEFT_SHIFT, RIGHT_SHIFT,
        LESS_THAN, GREATER_THAN,
        LESS_THAN_EQUAL, GREATER_THAN_EQUAL,
        EQUAL, NOT_EQUAL,
        BITWISE_AND, BITWISE_XOR, BITWISE_OR,
        LOGICAL_AND, LOGICAL_OR,
        ASSIGNMENT,
        ASSIGNMENT_MULTIPLY, ASSIGNMENT_DIVISION,
        ASSIGNMENT_REMAINDER,
        ASSIGNMENT_PLUS, ASSIGNMENT_MINUS,
        ASSIGNMENT_LEFT_SHIFT, ASSIGNMENT_RIGHT_SHIFT,
        ASSIGNMENT_BITWISE_AND, ASSIGNMENT_BITWISE_XOR, ASSIGNMENT_BITWISE_OR,
    };

private:
    BinaryOperatorType m_operator;
    std::shared_ptr<ASTNodeExpr> m_left, m_right;

public:
    inline ASTNodeExprBinaryOp(
            ASTNodeParserContext c,
            BinaryOperatorType optype,
            std::shared_ptr<ASTNodeExpr> left,
            std::shared_ptr<ASTNodeExpr> right
            ):
        ASTNodeExpr(c), m_operator(optype),
        m_left(std::move(left)), m_right(std::move(right)) {}

    virtual void check_constraints(std::shared_ptr<SemanticReporter> reporter) override;
    virtual std::optional<long long> get_integer_constant() const override;
    virtual std::optional<long double> get_float_constant() const override;
    virtual bool is_lvalue() const override;
};

class ASTNodeExprConditional: public ASTNodeExpr {
private:
    std::shared_ptr<ASTNodeExpr> m_cond, m_true, m_false;

public:
    inline ASTNodeExprConditional(
            ASTNodeParserContext c,
            std::shared_ptr<ASTNodeExpr> cond,
            std::shared_ptr<ASTNodeExpr> true_expr,
            std::shared_ptr<ASTNodeExpr> false_expr
            ):
        ASTNodeExpr(c), m_cond(std::move(cond)),
        m_true(std::move(true_expr)), m_false(std::move(false_expr)) {}

    virtual void check_constraints(std::shared_ptr<SemanticReporter> reporter) override;
    virtual std::optional<long long> get_integer_constant() const override;
    virtual std::optional<long double> get_float_constant() const override;
    virtual bool is_lvalue() const override;
};

class ASTNodeExprList: public ASTNodeExpr, private std::vector<std::shared_ptr<ASTNodeExpr>>
{
private:
    using container_t = std::vector<std::shared_ptr<ASTNodeExpr>>;

public:
    using reference = container_t::reference;
    using const_reference = container_t::const_reference;
    using iterator = container_t::iterator;
    using const_iterator = container_t::const_iterator;

    inline ASTNodeExprList(
            ASTNodeParserContext c): ASTNodeExpr(c) {}

    using container_t::begin;
    using container_t::end;
    using container_t::empty;
    using container_t::size;
    using container_t::operator[];
    using container_t::push_back;

    virtual void check_constraints(std::shared_ptr<SemanticReporter> reporter) override;
    virtual std::optional<long long> get_integer_constant() const override;
    virtual std::optional<long double> get_float_constant() const override;
    virtual bool is_lvalue() const override;
};


/******************************************************************************
 *                                                                            *
 *                              Declaration                                   *
 *                                                                            *
 ******************************************************************************/
class ASTNodeInitDeclarator: public ASTNode {
private:
    std::shared_ptr<TokenID> m_id;
    std::shared_ptr<ASTNodeVariableType> m_type;
    std::shared_ptr<ASTNodeInitializer> m_initializer;

public:
    inline ASTNodeInitDeclarator(
            ASTNodeParserContext c,
            std::shared_ptr<TokenID> id,
            std::shared_ptr<ASTNodeVariableType> type,
            std::shared_ptr<ASTNodeInitializer> initializer
            ):
        ASTNode(c),
        m_id(id),
        m_type(type),
        m_initializer(initializer) {}

    inline std::shared_ptr<TokenID>& id() { return this->m_id; }
    inline std::shared_ptr<ASTNodeVariableType>& type() { return this->m_type; }
    inline std::shared_ptr<ASTNodeInitializer>& initializer() { return this->m_initializer; }

    void set_leaf_type(std::shared_ptr<ASTNodeVariableType> type);

    virtual void check_constraints(std::shared_ptr<SemanticReporter> reporter) override;
};
using ASTNodeDeclaration = ASTNodeInitDeclarator;

class ASTNodeStructUnionDeclaration: public ASTNode
{
private:
    std::shared_ptr<ASTNodeInitDeclarator> m_decl;
    std::shared_ptr<ASTNodeExpr> m_bit_width;
    std::optional<size_t> m_offset;

public:
    inline ASTNodeStructUnionDeclaration(
            ASTNodeParserContext c,
            std::shared_ptr<ASTNodeInitDeclarator> decl,
            std::shared_ptr<ASTNodeExpr> bit_width):
        ASTNode(c), m_decl(decl), m_bit_width(bit_width) {}

    inline std::shared_ptr<TokenID>& id() { return this->m_decl->id(); }
    inline std::shared_ptr<ASTNodeVariableType>& type() { return this->m_decl->type(); }
    inline std::optional<size_t>& offset() { return this->m_offset; }

    void set_leaf_type(std::shared_ptr<ASTNodeVariableType> type);
 
    virtual void check_constraints(std::shared_ptr<SemanticReporter> reporter) override;
};

class ASTNodeInitDeclaratorList: public ASTNode, private std::vector<std::shared_ptr<ASTNodeInitDeclarator>>
{
private:
    using container_t = std::vector<std::shared_ptr<ASTNodeInitDeclarator>>;

public:
    using reference = container_t::reference;
    using const_reference = container_t::const_reference;
    using iterator = container_t::iterator;
    using const_iterator = container_t::const_iterator;

    inline ASTNodeInitDeclaratorList(ASTNodeParserContext c): ASTNode(c) {}

    using container_t::begin;
    using container_t::end;
    using container_t::empty;
    using container_t::size;
    using container_t::operator[];
    using container_t::push_back;

    virtual void check_constraints(std::shared_ptr<SemanticReporter> reporter) override;
};

class ASTNodeDeclarationList: public ASTNodeExternalDeclaration, private std::vector<std::shared_ptr<ASTNodeDeclaration>>
{
private:
    using container_t = std::vector<std::shared_ptr<ASTNodeDeclaration>>;

public:
    using reference = container_t::reference;
    using const_reference = container_t::const_reference;
    using iterator = container_t::iterator;
    using const_iterator = container_t::const_iterator;

    inline ASTNodeDeclarationList(ASTNodeParserContext c): ASTNodeExternalDeclaration(c) {}

    using container_t::begin;
    using container_t::end;
    using container_t::empty;
    using container_t::size;
    using container_t::operator[];
    using container_t::push_back;

    virtual void check_constraints(std::shared_ptr<SemanticReporter> reporter) override;
};

class ASTNodeStructUnionDeclarationList: public ASTNode, private std::vector<std::shared_ptr<ASTNodeStructUnionDeclaration>>
{
private:
    using container_t = std::vector<std::shared_ptr<ASTNodeStructUnionDeclaration>>;
    bool _is_struct;
    std::shared_ptr<TokenID> _id;
    std::optional<size_t> _alignment;
    std::optional<size_t> _sizeof;

public:
    using reference = container_t::reference;
    using const_reference = container_t::const_reference;
    using iterator = container_t::iterator;
    using const_iterator = container_t::const_iterator;

    inline ASTNodeStructUnionDeclarationList(ASTNodeParserContext c): ASTNode(c) {}
    inline bool& is_struct() { return this->_is_struct; }
    inline std::shared_ptr<TokenID>& id() { return this->_id; }
    inline std::optional<size_t>& alignment() { return this->_alignment; }
    inline std::optional<size_t>& sizeof_() { return this->_sizeof; }

    using container_t::begin;
    using container_t::end;
    using container_t::empty;
    using container_t::size;
    using container_t::operator[];
    using container_t::push_back;

    virtual void check_constraints(std::shared_ptr<SemanticReporter> reporter) override;
};

class ASTNodeEnumerationConstant: public ASTNode
{
private:
    std::shared_ptr<TokenID> m_id;

public:
    inline ASTNodeEnumerationConstant(ASTNodeParserContext c, std::shared_ptr<TokenID> id): ASTNode(c), m_id(id) {}

    inline std::shared_ptr<TokenID> id() { return this->m_id; }

    virtual void check_constraints(std::shared_ptr<SemanticReporter> reporter) override;
};

class ASTNodeEnumerator: public ASTNode
{
private:
    std::shared_ptr<TokenID> m_id;
    std::shared_ptr<ASTNodeExpr> m_value;

public:
    inline ASTNodeEnumerator(ASTNodeParserContext c, std::shared_ptr<TokenID> id, std::shared_ptr<ASTNodeExpr> value):
        ASTNode(c), m_id(id), m_value(value) {}

    inline std::shared_ptr<TokenID> id() { return this->m_id; }
    inline std::shared_ptr<ASTNodeExpr> value() { return this->m_value; }

    virtual void check_constraints(std::shared_ptr<SemanticReporter> reporter) override;
};

class ASTNodeEnumeratorList: public ASTNode, private std::vector<std::shared_ptr<ASTNodeEnumerator>>
{
private:
    using container_t = std::vector<std::shared_ptr<ASTNodeEnumerator>>;

public:
    using reference = container_t::reference;
    using const_reference = container_t::const_reference;
    using iterator = container_t::iterator;
    using const_iterator = container_t::const_iterator;

    inline ASTNodeEnumeratorList(ASTNodeParserContext c): ASTNode(c) {}

    using container_t::begin;
    using container_t::end;
    using container_t::empty;
    using container_t::size;
    using container_t::operator[];
    using container_t::push_back;

    virtual void check_constraints(std::shared_ptr<SemanticReporter> reporter) override;
};

class ASTNodeVariableType: public ASTNode {
private:
    bool m_const, m_volatile, m_restrict;

public:
    enum storage_class_t { SC_Typedef, SC_Extern, SC_Static, SC_Auto, SC_Register, SC_Default, };
    enum variable_basic_type { DUMMY, INT, FLOAT, VOID, STRUCT, UNION, ENUM, POINTER, ARRAY, FUNCTION, };

    inline ASTNodeVariableType(ASTNodeParserContext c): ASTNode(c), m_const(false), m_volatile(false), m_restrict(false) {}

    virtual variable_basic_type basic_type() const = 0;
    virtual void set_leaf_type(std::shared_ptr<ASTNodeVariableType> type) = 0;

    virtual std::optional<size_t> opsizeof() const = 0;
    virtual std::optional<size_t> opalignof() const = 0;

    virtual bool is_object_type() const;
    virtual bool is_incomplete_type() const;
    virtual bool is_function_type() const;

    inline bool is_arithmatic_type() const
    {
        const auto bt = this->basic_type();
        return bt == variable_basic_type::INT || bt == variable_basic_type::FLOAT;
    }
    inline bool is_scalar_type() const
    {
        const auto bt = this->basic_type();
        return this->is_arithmatic_type() || bt == variable_basic_type::POINTER;
    }
    virtual std::shared_ptr<ASTNodeVariableType> 
    compatible_with(std::shared_ptr<ASTNodeVariableType> type) const = 0;

    virtual std::shared_ptr<ASTNodeVariableType>
    implicit_cast_to(std::shared_ptr<ASTNodeVariableType> type) const = 0;
    virtual std::shared_ptr<ASTNodeVariableType>
    explicit_cast_to(std::shared_ptr<ASTNodeVariableType> type) const = 0;
    virtual bool equal_to(std::shared_ptr<ASTNodeVariableType> type) const = 0;

    virtual bool& const_ref();
    virtual bool& volatile_ref();
    virtual bool& restrict_ref();
    inline void reset_qualifies()
    {
        this->m_const = false;
        this->m_volatile = false;
        this->m_restrict = false;
    }
    inline void reset_storage_class()
    {
        this->storage_class() = storage_class_t::SC_Default;
    }
    bool  const_ref() const { return this->m_const; }
    bool  volatile_ref() const { return this->m_volatile; }
    bool  restrict_ref() const { return this->m_restrict; }
    virtual storage_class_t& storage_class() = 0;
    inline  storage_class_t  storage_class() const { auto _this = const_cast<ASTNodeVariableType*>(this); return _this->storage_class(); }
    virtual bool& inlined() = 0;
    inline bool inlined() const { auto _this = const_cast<ASTNodeVariableType*>(this); return _this->inlined(); }

    virtual std::shared_ptr<ASTNodeVariableType> copy() const = 0;
};

class ASTNodeVariableTypePlain: public ASTNodeVariableType {
private:
    std::multiset<std::string> m_typenames;
    using storage_class_t = ASTNodeVariableType::storage_class_t;
    storage_class_t m_storage_class;
    bool m_inlined;

public:
    inline ASTNodeVariableTypePlain(ASTNodeParserContext c): 
        ASTNodeVariableType(c), m_storage_class(storage_class_t::SC_Default), m_inlined(false) {}

    inline const std::multiset<std::string>& type_names() const { return this->m_typenames; }
    inline void add_name (const std::string& name) { this->m_typenames.insert(name); }

    virtual storage_class_t & storage_class() override;
    virtual bool& inlined() override;
    virtual void set_leaf_type(std::shared_ptr<ASTNodeVariableType> type) override;
};

class ASTNodeVariableTypeDummy: public ASTNodeVariableTypePlain {
public:
    inline ASTNodeVariableTypeDummy(
            ASTNodeParserContext c): ASTNodeVariableTypePlain(c) {}

    virtual variable_basic_type basic_type() const override;
    virtual std::optional<size_t> opsizeof() const override;
    virtual std::optional<size_t> opalignof() const override;
    virtual std::shared_ptr<ASTNodeVariableType> copy() const override;

    virtual std::shared_ptr<ASTNodeVariableType> 
    compatible_with(std::shared_ptr<ASTNodeVariableType> type) const override;

    virtual std::shared_ptr<ASTNodeVariableType>
    implicit_cast_to(std::shared_ptr<ASTNodeVariableType> type) const override;
    virtual std::shared_ptr<ASTNodeVariableType>
    explicit_cast_to(std::shared_ptr<ASTNodeVariableType> type) const override;
    virtual bool equal_to(std::shared_ptr<ASTNodeVariableType> type) const override;

    virtual void check_constraints(std::shared_ptr<SemanticReporter> reporter) override;
};

class ASTNodeVariableTypeStruct: public ASTNodeVariableTypePlain {
private:
    std::optional<size_t> m_type_id;
    std::shared_ptr<TokenID> m_name;
    std::shared_ptr<ASTNodeStructUnionDeclarationList> m_definition;

public:
    inline ASTNodeVariableTypeStruct(
            ASTNodeParserContext c,
            std::shared_ptr<TokenID> name
            ): ASTNodeVariableTypePlain(c), m_name(name)
    {
        this->add_name("struct " + m_name->id);
    }

    inline std::shared_ptr<TokenID> name() { return this->m_name; }
    inline std::shared_ptr<ASTNodeStructUnionDeclarationList>& definition() { return this->m_definition; }
    inline void resolve_type_id(size_t id) { assert(!this->m_type_id.has_value()); this->m_type_id = id; }

    virtual variable_basic_type basic_type() const override;

    virtual bool is_object_type() const override;

    virtual std::shared_ptr<ASTNodeVariableType> 
    compatible_with(std::shared_ptr<ASTNodeVariableType> type) const override;

    virtual std::shared_ptr<ASTNodeVariableType>
    implicit_cast_to(std::shared_ptr<ASTNodeVariableType> type) const override;
    virtual std::shared_ptr<ASTNodeVariableType>
    explicit_cast_to(std::shared_ptr<ASTNodeVariableType> type) const override;
    virtual bool equal_to(std::shared_ptr<ASTNodeVariableType> type) const override;

    virtual std::optional<size_t> opsizeof() const override;
    virtual std::optional<size_t> opalignof() const override;
    virtual std::shared_ptr<ASTNodeVariableType> copy() const override;

    virtual void check_constraints(std::shared_ptr<SemanticReporter> reporter) override;
};

class ASTNodeVariableTypeUnion: public ASTNodeVariableTypePlain {
private:
    std::optional<size_t> m_type_id;
    std::shared_ptr<TokenID> m_name;
    std::shared_ptr<ASTNodeStructUnionDeclarationList> m_definition;

public:
    inline ASTNodeVariableTypeUnion(
            ASTNodeParserContext c,
            std::shared_ptr<TokenID> name
            ): ASTNodeVariableTypePlain(c), m_name(name)
    {
        this->add_name("union " + m_name->id);
    }

    inline std::shared_ptr<TokenID> name() { return this->m_name; }
    inline std::shared_ptr<ASTNodeStructUnionDeclarationList>& definition() { return this->m_definition; }
    inline void resolve_type_id(size_t id) { assert(!this->m_type_id.has_value()); this->m_type_id = id; }

    virtual variable_basic_type basic_type() const override;

    virtual bool is_object_type() const override;

    virtual std::shared_ptr<ASTNodeVariableType> 
    compatible_with(std::shared_ptr<ASTNodeVariableType> type) const override;

    virtual std::shared_ptr<ASTNodeVariableType>
    implicit_cast_to(std::shared_ptr<ASTNodeVariableType> type) const override;
    virtual std::shared_ptr<ASTNodeVariableType>
    explicit_cast_to(std::shared_ptr<ASTNodeVariableType> type) const override;
    virtual bool equal_to(std::shared_ptr<ASTNodeVariableType> type) const override;

    virtual std::optional<size_t> opsizeof() const override;
    virtual std::optional<size_t> opalignof() const override;
    virtual std::shared_ptr<ASTNodeVariableType> copy() const override;

    virtual void check_constraints(std::shared_ptr<SemanticReporter> reporter) override;
};

class ASTNodeEnumeratorList;
class ASTNodeVariableTypeEnum: public ASTNodeVariableTypePlain {
private:
    std::optional<size_t> m_type_id;
    std::shared_ptr<TokenID> m_name;
    std::shared_ptr<ASTNodeEnumeratorList> m_definition;

public:
    inline ASTNodeVariableTypeEnum(
            ASTNodeParserContext c,
            std::shared_ptr<TokenID> name
            ): ASTNodeVariableTypePlain(c), m_name(name)
    {
        this->add_name("enum " + m_name->id);
    }

    inline std::shared_ptr<TokenID> name() { return this->m_name; }
    inline std::shared_ptr<ASTNodeEnumeratorList>& definition() { return this->m_definition; }
    inline void resolve_type_id(size_t id) { assert(!this->m_type_id.has_value()); this->m_type_id = id; }

    virtual variable_basic_type basic_type() const override;

    virtual bool is_object_type() const override;

    virtual std::shared_ptr<ASTNodeVariableType> 
    compatible_with(std::shared_ptr<ASTNodeVariableType> type) const override;

    virtual std::shared_ptr<ASTNodeVariableType>
    implicit_cast_to(std::shared_ptr<ASTNodeVariableType> type) const override;
    virtual std::shared_ptr<ASTNodeVariableType>
    explicit_cast_to(std::shared_ptr<ASTNodeVariableType> type) const override;
    virtual bool equal_to(std::shared_ptr<ASTNodeVariableType> type) const override;

    virtual std::optional<size_t> opsizeof() const override;
    virtual std::optional<size_t> opalignof() const override;
    virtual std::shared_ptr<ASTNodeVariableType> copy() const override;

    virtual void check_constraints(std::shared_ptr<SemanticReporter> reporter) override;
};

class ASTNodeVariableTypeTypedef: public ASTNodeVariableTypePlain {
private:
    std::shared_ptr<TokenID> m_name;
    std::shared_ptr<ASTNodeVariableType> m_type;

public:
    inline ASTNodeVariableTypeTypedef(
            ASTNodeParserContext c,
            std::shared_ptr<TokenID> name
            ): ASTNodeVariableTypePlain(c), m_name(name)
    {
        this->add_name(m_name->id);
    }

    inline std::shared_ptr<TokenID> name() { return this->m_name; }
    inline std::shared_ptr<ASTNodeVariableType> type() const { return this->m_type; }
    inline void set_type(std::shared_ptr<ASTNodeVariableType> type) { this->m_type = type; }

    virtual variable_basic_type basic_type() const override;

    virtual bool is_object_type() const override;

    virtual std::shared_ptr<ASTNodeVariableType> 
    compatible_with(std::shared_ptr<ASTNodeVariableType> type) const override;

    virtual std::shared_ptr<ASTNodeVariableType>
    implicit_cast_to(std::shared_ptr<ASTNodeVariableType> type) const override;
    virtual std::shared_ptr<ASTNodeVariableType>
    explicit_cast_to(std::shared_ptr<ASTNodeVariableType> type) const override;
    virtual bool equal_to(std::shared_ptr<ASTNodeVariableType> type) const override;

    virtual std::optional<size_t> opsizeof() const override;
    virtual std::optional<size_t> opalignof() const override;
    virtual std::shared_ptr<ASTNodeVariableType> copy() const override;

    virtual void check_constraints(std::shared_ptr<SemanticReporter> reporter) override;
};

class ASTNodeVariableTypeVoid: public ASTNodeVariableTypePlain {
public:
    inline ASTNodeVariableTypeVoid(
            ASTNodeParserContext c
            ): ASTNodeVariableTypePlain(c)
    {
        this->add_name("void");
    }

    virtual variable_basic_type basic_type() const override;

    virtual std::shared_ptr<ASTNodeVariableType> 
    compatible_with(std::shared_ptr<ASTNodeVariableType> type) const override;

    virtual std::shared_ptr<ASTNodeVariableType>
    implicit_cast_to(std::shared_ptr<ASTNodeVariableType> type) const override;
    virtual std::shared_ptr<ASTNodeVariableType>
    explicit_cast_to(std::shared_ptr<ASTNodeVariableType> type) const override;
    virtual bool equal_to(std::shared_ptr<ASTNodeVariableType> type) const override;

    virtual std::optional<size_t> opsizeof() const override;
    virtual std::optional<size_t> opalignof() const override;
    virtual std::shared_ptr<ASTNodeVariableType> copy() const override;

    virtual void check_constraints(std::shared_ptr<SemanticReporter> reporter) override;
};

class ASTNodeVariableTypeInt: public ASTNodeVariableTypePlain {
private:
    size_t m_byte_length;;
    bool   m_is_unsigned;

public:
    inline ASTNodeVariableTypeInt(
            ASTNodeParserContext c,
            size_t byte_length,
            bool is_unsigned
            ): ASTNodeVariableTypePlain(c), m_byte_length(byte_length),
        m_is_unsigned(is_unsigned) {}

    inline size_t byte_length() { return this->m_byte_length; }
    inline bool is_unsigned() { return this->m_is_unsigned; }

    virtual variable_basic_type basic_type() const override;

    virtual std::shared_ptr<ASTNodeVariableType> 
    compatible_with(std::shared_ptr<ASTNodeVariableType> type) const override;

    virtual std::shared_ptr<ASTNodeVariableType>
    implicit_cast_to(std::shared_ptr<ASTNodeVariableType> type) const override;
    virtual std::shared_ptr<ASTNodeVariableType>
    explicit_cast_to(std::shared_ptr<ASTNodeVariableType> type) const override;
    virtual bool equal_to(std::shared_ptr<ASTNodeVariableType> type) const override;

    virtual std::optional<size_t> opsizeof() const override;
    virtual std::optional<size_t> opalignof() const override;
    virtual std::shared_ptr<ASTNodeVariableType> copy() const override;

    virtual void check_constraints(std::shared_ptr<SemanticReporter> reporter) override;
};

class ASTNodeVariableTypeFloat: public ASTNodeVariableTypePlain {
private:
    size_t m_byte_length;

public:
    inline ASTNodeVariableTypeFloat(
            ASTNodeParserContext c,
            size_t byte_length
            ): ASTNodeVariableTypePlain(c), m_byte_length(byte_length) {}

    inline size_t byte_length() { return this->m_byte_length; }

    virtual variable_basic_type basic_type() const override;

    virtual std::shared_ptr<ASTNodeVariableType> 
    compatible_with(std::shared_ptr<ASTNodeVariableType> type) const override;

    virtual std::shared_ptr<ASTNodeVariableType>
    implicit_cast_to(std::shared_ptr<ASTNodeVariableType> type) const override;
    virtual std::shared_ptr<ASTNodeVariableType>
    explicit_cast_to(std::shared_ptr<ASTNodeVariableType> type) const override;
    virtual bool equal_to(std::shared_ptr<ASTNodeVariableType> type) const override;

    virtual std::optional<size_t> opsizeof() const override;
    virtual std::optional<size_t> opalignof() const override;
    virtual std::shared_ptr<ASTNodeVariableType> copy() const override;

    virtual void check_constraints(std::shared_ptr<SemanticReporter> reporter) override;
};

class ASTNodeVariableTypePointer : public ASTNodeVariableType {
private:
    std::shared_ptr<ASTNodeVariableType> m_type;

public:
    inline ASTNodeVariableTypePointer(
            ASTNodeParserContext c,
            std::shared_ptr<ASTNodeVariableType> type
            ):
        ASTNodeVariableType(c),
        m_type(std::move(type)) {}

    inline std::shared_ptr<ASTNodeVariableType> deref() const { return this->m_type; }

    virtual variable_basic_type basic_type() const override;
    virtual void set_leaf_type(std::shared_ptr<ASTNodeVariableType> type) override;

    virtual storage_class_t& storage_class() override;
    virtual std::optional<size_t> opsizeof() const override;
    virtual std::shared_ptr<ASTNodeVariableType> 
    compatible_with(std::shared_ptr<ASTNodeVariableType> type) const override;

    virtual std::shared_ptr<ASTNodeVariableType>
    implicit_cast_to(std::shared_ptr<ASTNodeVariableType> type) const override;
    virtual std::shared_ptr<ASTNodeVariableType>
    explicit_cast_to(std::shared_ptr<ASTNodeVariableType> type) const override;
    virtual bool equal_to(std::shared_ptr<ASTNodeVariableType> type) const override;

    virtual std::optional<size_t> opalignof() const override;
    virtual std::shared_ptr<ASTNodeVariableType> copy() const override;
    virtual bool& inlined() override;

    virtual void check_constraints(std::shared_ptr<SemanticReporter> reporter) override;
};

class ASTNodeVariableTypeArray : public ASTNodeVariableType {
private:
    std::shared_ptr<ASTNodeVariableType> m_type;
    std::shared_ptr<ASTNodeExpr> m_size;
    bool m_static;
    bool m_unspecified_size_vla;

public:
    inline ASTNodeVariableTypeArray(
            ASTNodeParserContext c,
            std::shared_ptr<ASTNodeVariableType> type,
            std::shared_ptr<ASTNodeExpr> size,
            bool static_
            ):
        ASTNodeVariableType(c),
        m_type(std::move(type)),
        m_size(std::move(size)),
        m_static(static_),
        m_unspecified_size_vla(false) {}

    virtual variable_basic_type basic_type() const override;

    virtual bool is_object_type() const override;

    virtual void set_leaf_type(std::shared_ptr<ASTNodeVariableType> type) override;
    inline std::shared_ptr<ASTNodeVariableType> elemtype() const { return this->m_type; }
    inline std::shared_ptr<ASTNodeExpr> array_size() const { return this->m_size; }
    inline bool& unspecified_size_vla() { return this->m_unspecified_size_vla; }

    virtual storage_class_t& storage_class() override;
    virtual std::optional<size_t> opsizeof() const override;
    virtual std::shared_ptr<ASTNodeVariableType> 
    compatible_with(std::shared_ptr<ASTNodeVariableType> type) const override;

    virtual std::shared_ptr<ASTNodeVariableType>
    implicit_cast_to(std::shared_ptr<ASTNodeVariableType> type) const override;
    virtual std::shared_ptr<ASTNodeVariableType>
    explicit_cast_to(std::shared_ptr<ASTNodeVariableType> type) const override;
    virtual bool equal_to(std::shared_ptr<ASTNodeVariableType> type) const override;

    virtual std::optional<size_t> opalignof() const override;
    virtual std::shared_ptr<ASTNodeVariableType> copy() const override;
    virtual bool& inlined() override;

    virtual void check_constraints(std::shared_ptr<SemanticReporter> reporter) override;
};

class ASTNodeParameterDeclaration: public ASTNode {
private:
    std::shared_ptr<TokenID> m_id;
    std::shared_ptr<ASTNodeVariableType> m_type;

public:
    inline ASTNodeParameterDeclaration(
            ASTNodeParserContext c,
            std::shared_ptr<TokenID> id,
            std::shared_ptr<ASTNodeVariableType> type
            ):
        ASTNode(c),
        m_id(id),
        m_type(std::move(type)) {}

    inline std::shared_ptr<TokenID>& id() { return this->m_id; }
    inline std::shared_ptr<ASTNodeVariableType>& type() { return this->m_type; }
    
    virtual void check_constraints(std::shared_ptr<SemanticReporter> reporter) override;
};

class ASTNodeParameterDeclarationList: public ASTNode, private std::vector<std::shared_ptr<ASTNodeParameterDeclaration>>
{
private:
    using container_t = std::vector<std::shared_ptr<ASTNodeParameterDeclaration>>;
    bool m_variadic;

public:
    using reference = container_t::reference;
    using const_reference = container_t::const_reference;
    using iterator = container_t::iterator;
    using const_iterator = container_t::const_iterator;

    inline ASTNodeParameterDeclarationList(ASTNodeParserContext c): ASTNode(c), m_variadic(false) {}

    inline bool& variadic() { return this->m_variadic; }

    using container_t::begin;
    using container_t::end;
    using container_t::empty;
    using container_t::size;
    using container_t::operator[];
    using container_t::push_back;

    virtual void check_constraints(std::shared_ptr<SemanticReporter> reporter) override;
};

class ASTNodeVariableTypeFunction : public ASTNodeVariableType {
private:
    std::shared_ptr<ASTNodeParameterDeclarationList> m_parameter_declaration_list;
    std::shared_ptr<ASTNodeVariableType> m_return_type;

public:
    inline ASTNodeVariableTypeFunction(
            ASTNodeParserContext c,
            std::shared_ptr<ASTNodeParameterDeclarationList> parameter_declaration_list,
            std::shared_ptr<ASTNodeVariableType> return_type
            ):
        ASTNodeVariableType(c),
        m_parameter_declaration_list(std::move(parameter_declaration_list)),
        m_return_type(std::move(return_type)) {}

    inline std::shared_ptr<ASTNodeParameterDeclarationList>& parameter_declaration_list() { return this->m_parameter_declaration_list; }
    inline std::shared_ptr<ASTNodeVariableType>& return_type() { return this->m_return_type; }
    inline std::shared_ptr<ASTNodeParameterDeclarationList> parameter_declaration_list() const { return this->m_parameter_declaration_list; }
    inline std::shared_ptr<ASTNodeVariableType> return_type() const { return this->m_return_type; }

    virtual variable_basic_type basic_type() const override;
    virtual void set_leaf_type(std::shared_ptr<ASTNodeVariableType> type) override;

    virtual storage_class_t& storage_class() override;
    virtual std::optional<size_t> opsizeof() const override;
    virtual std::shared_ptr<ASTNodeVariableType> 
    compatible_with(std::shared_ptr<ASTNodeVariableType> type) const override;

    virtual std::shared_ptr<ASTNodeVariableType>
    implicit_cast_to(std::shared_ptr<ASTNodeVariableType> type) const override;
    virtual std::shared_ptr<ASTNodeVariableType>
    explicit_cast_to(std::shared_ptr<ASTNodeVariableType> type) const override;
    virtual bool equal_to(std::shared_ptr<ASTNodeVariableType> type) const override;

    virtual std::optional<size_t> opalignof() const override;
    virtual std::shared_ptr<ASTNodeVariableType> copy() const override;
    virtual bool& inlined() override;

    virtual void check_constraints(std::shared_ptr<SemanticReporter> reporter) override;
};

class ASTNodeInitializer: public ASTNode {
private:
    std::variant<
        std::shared_ptr<ASTNodeExpr>,
        std::shared_ptr<ASTNodeInitializerList>> m_value;

public:
    ASTNodeInitializer(ASTNodeParserContext c,
                       std::shared_ptr<ASTNodeExpr> expr): 
        ASTNode(c), m_value(expr) {}

    ASTNodeInitializer(ASTNodeParserContext c,
                       std::shared_ptr<ASTNodeInitializerList> list):
        ASTNode(c), m_value(list) {}

    bool is_constexpr() const;
    
    virtual void check_constraints(std::shared_ptr<SemanticReporter> reporter) override;
};

class ASTNodeDesignation: public ASTNode {
private:
    using array_designator_t = std::shared_ptr<ASTNodeExpr>;
    using member_designator_t = std::shared_ptr<TokenID>;
    std::vector<std::variant<array_designator_t,member_designator_t>> m_designators;
    std::shared_ptr<ASTNodeInitializer> m_initializer;

public:
    ASTNodeDesignation(ASTNodeParserContext c,
                      std::shared_ptr<ASTNodeExpr> pos):
        ASTNode(c), m_designators({ pos }) {}

    ASTNodeDesignation(ASTNodeParserContext c, 
                      std::shared_ptr<TokenID> member):
        ASTNode(c), m_designators({ member }) {}

    ASTNodeDesignation(ASTNodeParserContext c):
        ASTNode(c), m_designators() {}

    inline std::shared_ptr<ASTNodeInitializer>& initializer() { return this->m_initializer; }
    inline const std::vector<std::variant<array_designator_t,member_designator_t>>& designators() const { return this->m_designators; }
    inline void add_designator(std::variant<array_designator_t,member_designator_t> designator) { this->m_designators.push_back(designator); }
    inline bool is_constexpr() const { return this->m_initializer->is_constexpr(); }

    virtual void check_constraints(std::shared_ptr<SemanticReporter> reporter) override;
};

class ASTNodeInitializerList: public ASTNode, public std::vector<std::shared_ptr<ASTNodeDesignation>>
{
private:
    using container_t = std::vector<std::shared_ptr<ASTNodeDesignation>>;

public:
    using reference = container_t::reference;
    using const_reference = container_t::const_reference;
    using iterator = container_t::iterator;
    using const_iterator = container_t::const_iterator;

    inline ASTNodeInitializerList(ASTNodeParserContext c): ASTNode(c) {}

    using container_t::begin;
    using container_t::end;
    using container_t::empty;
    using container_t::size;
    using container_t::operator[];
    using container_t::push_back;

    inline bool is_constexpr() const 
    {
        for (auto& d: *this)
            if (!d->is_constexpr())
                return false;
        return true;
    }

    virtual void check_constraints(std::shared_ptr<SemanticReporter> reporter) override;
};


/******************************************************************************
 *                                                                            *
 *                               Statement                                    *
 *                                                                            *
 ******************************************************************************/
class ASTNodeStat: public ASTNodeBlockItem
{
public:
    inline ASTNodeStat(ASTNodeParserContext c): ASTNodeBlockItem(c) {}
};

class ASTNodeStatLabel: public ASTNodeStat {
private:
    using id_label_t = std::shared_ptr<TokenID>;
    using case_label_t = std::shared_ptr<ASTNodeExpr>;
    using default_label_t = std::nullptr_t;
    std::variant<id_label_t,case_label_t,default_label_t> m_label;
    std::shared_ptr<ASTNodeStat> m_stat;

public:
    ASTNodeStatLabel(
            ASTNodeParserContext c,
            std::shared_ptr<TokenID> label,
            std::shared_ptr<ASTNodeStat> stat):
        ASTNodeStat(c), m_label(label), m_stat(stat) {}

    ASTNodeStatLabel(
            ASTNodeParserContext c,
            std::shared_ptr<ASTNodeExpr> label,
            std::shared_ptr<ASTNodeStat> stat):
        ASTNodeStat(c), m_label(label), m_stat(stat) {}

    ASTNodeStatLabel(
            ASTNodeParserContext c,
            std::shared_ptr<ASTNodeStat> stat):
        ASTNodeStat(c), m_label(default_label_t(nullptr)), m_stat(stat) {}

    inline std::shared_ptr<ASTNodeStat>& stat() { return this->m_stat; }

    inline id_label_t id_label() const { return std::get<id_label_t>(this->m_label); }
    inline case_label_t case_label() const { return std::get<case_label_t>(this->m_label); }
    inline default_label_t default_label() const { return std::get<default_label_t>(this->m_label); }

    inline bool is_id_label() const { return std::holds_alternative<id_label_t>(this->m_label); }
    inline bool is_case_label() const { return std::holds_alternative<case_label_t>(this->m_label); }
    inline bool is_default_label() const { return std::holds_alternative<default_label_t>(this->m_label); }

    virtual void check_constraints(std::shared_ptr<SemanticReporter> reporter) override;
};

class ASTNodeStatCase: public ASTNodeStat {
private:
    std::shared_ptr<ASTNodeExpr> m_expr;
    std::shared_ptr<ASTNodeStat> m_stat;

public:
    ASTNodeStatCase(
            ASTNodeParserContext c,
            std::shared_ptr<ASTNodeExpr> expr,
            std::shared_ptr<ASTNodeStat> stat):
        ASTNodeStat(c), m_expr(expr), m_stat(stat) {}

    virtual void check_constraints(std::shared_ptr<SemanticReporter> reporter) override;
};

class ASTNodeStatDefault: public ASTNodeStat {
private:
    std::shared_ptr<ASTNodeStat> m_stat;

public:
    ASTNodeStatDefault(
            ASTNodeParserContext c,
            std::shared_ptr<ASTNodeStat> stat):
        ASTNodeStat(c), m_stat(stat) {}

    virtual void check_constraints(std::shared_ptr<SemanticReporter> reporter) override;
};

class ASTNodeBlockItemList: public ASTNode, private std::vector<std::shared_ptr<ASTNodeBlockItem>>
{
private:
    using container_t = std::vector<std::shared_ptr<ASTNodeBlockItem>>;

public:
    using reference = container_t::reference;
    using const_reference = container_t::const_reference;
    using iterator = container_t::iterator;
    using const_iterator = container_t::const_iterator;

    inline ASTNodeBlockItemList(
            ASTNodeParserContext c): ASTNode(c) {}

    using container_t::begin;
    using container_t::end;
    using container_t::empty;
    using container_t::size;
    using container_t::operator[];
    using container_t::push_back;

    virtual void check_constraints(std::shared_ptr<SemanticReporter> reporter) override;
};

class ASTNodeStatCompound: public ASTNodeStat
{
private:
    std::shared_ptr<ASTNodeBlockItemList> _statlist;

public:
    inline ASTNodeStatCompound(
            ASTNodeParserContext c,
            std::shared_ptr<ASTNodeBlockItemList> statlist):
        ASTNodeStat(c), _statlist(statlist) {}

    inline std::shared_ptr<ASTNodeBlockItemList>       StatementList()       { return this->_statlist; }
    inline const std::shared_ptr<ASTNodeBlockItemList> StatementList() const { return this->_statlist; }

    virtual void check_constraints(std::shared_ptr<SemanticReporter> reporter) override;
};

class ASTNodeStatExpr: public ASTNodeStat
{ 
private:
    std::shared_ptr<ASTNodeExpr> _expr;

public:
    inline ASTNodeStatExpr(
            ASTNodeParserContext c,
            std::shared_ptr<ASTNodeExpr> expr): ASTNodeStat(c), _expr(expr) {}

    inline       std::shared_ptr<ASTNodeExpr> expr()       { return this->_expr; }
    inline const std::shared_ptr<ASTNodeExpr> expr() const { return this->_expr; }

    virtual void check_constraints(std::shared_ptr<SemanticReporter> reporter) override;
};

class ASTNodeStatIF: public ASTNodeStat
{
private:
    std::shared_ptr<ASTNodeStat> _truestat, _falsestat;
    std::shared_ptr<ASTNodeExpr> _cond;

public:
    inline ASTNodeStatIF(
            ASTNodeParserContext c,
            std::shared_ptr<ASTNodeExpr> condition,
            std::shared_ptr<ASTNodeStat> truestat,
            std::shared_ptr<ASTNodeStat> falsestat):
        ASTNodeStat(c), _cond(condition),
        _truestat(truestat), _falsestat(falsestat) {}

    inline std::shared_ptr<ASTNodeStat> trueStat()  { return this->_truestat; }
    inline std::shared_ptr<ASTNodeStat> falseStat() { return this->_falsestat; }
    inline std::shared_ptr<ASTNodeExpr> condition() { return this->_cond; }

    virtual void check_constraints(std::shared_ptr<SemanticReporter> reporter) override;
};

class ASTNodeStatSwitch: public ASTNodeStat
{
private:
    std::shared_ptr<ASTNodeExpr> _expr;
    std::shared_ptr<ASTNodeStat> _stat;

public:
    inline ASTNodeStatSwitch(
            ASTNodeParserContext c,
            std::shared_ptr<ASTNodeExpr> expr,
            std::shared_ptr<ASTNodeStat> stat):
        ASTNodeStat(c), _expr(expr), _stat(stat) {}

    inline std::shared_ptr<ASTNodeExpr> expr()       { return this->_expr; }
    inline std::shared_ptr<ASTNodeStat> stat()       { return this->_stat; }

    virtual void check_constraints(std::shared_ptr<SemanticReporter> reporter) override;
};

class ASTNodeStatDoWhile: public ASTNodeStat
{
private:
    std::shared_ptr<ASTNodeExpr> _expr;
    std::shared_ptr<ASTNodeStat> _stat;
public:
    inline ASTNodeStatDoWhile(
            ASTNodeParserContext c,
            std::shared_ptr<ASTNodeExpr> expr,
            std::shared_ptr<ASTNodeStat> stat):
        ASTNodeStat(c), _expr(expr), _stat(stat) {}

    inline std::shared_ptr<ASTNodeExpr> expr()       { return this->_expr; }
    inline std::shared_ptr<ASTNodeStat> stat()       { return this->_stat; }

    virtual void check_constraints(std::shared_ptr<SemanticReporter> reporter) override;
};

class ASTNodeStatFor: public ASTNodeStat
{
private:
    std::shared_ptr<ASTNodeExpr> _init, _cond, _post;
    std::shared_ptr<ASTNodeStat> _stat;

public:
    inline ASTNodeStatFor(
            ASTNodeParserContext c,
            std::shared_ptr<ASTNodeExpr> init,
            std::shared_ptr<ASTNodeExpr> cond,
            std::shared_ptr<ASTNodeExpr> post,
            std::shared_ptr<ASTNodeStat> stat):
        ASTNodeStat(c), _init(init), _cond(cond), _post(post), _stat(stat) {}

    inline std::shared_ptr<ASTNodeExpr> init()       { return this->_init; }
    inline std::shared_ptr<ASTNodeExpr> cond()       { return this->_cond; }
    inline std::shared_ptr<ASTNodeExpr> post()       { return this->_post; }
    inline std::shared_ptr<ASTNodeStat> stat()       { return this->_stat; }

    virtual void check_constraints(std::shared_ptr<SemanticReporter> reporter) override;
};

class ASTNodeStatForDecl: public ASTNodeStat
{
private:
    std::shared_ptr<ASTNodeDeclarationList> _decls;
    std::shared_ptr<ASTNodeExpr> _cond, _post;
    std::shared_ptr<ASTNodeStat> _stat;

public:
    inline ASTNodeStatForDecl(
            ASTNodeParserContext c,
            std::shared_ptr<ASTNodeDeclarationList> decls,
            std::shared_ptr<ASTNodeExpr> cond,
            std::shared_ptr<ASTNodeExpr> post,
            std::shared_ptr<ASTNodeStat> stat):
        ASTNodeStat(c), _decls(decls), _cond(cond), _post(post), _stat(stat) {}

    inline std::shared_ptr<ASTNodeDeclarationList> decls() { return this->_decls; }
    inline std::shared_ptr<ASTNodeExpr>               cond()  { return this->_cond; }
    inline std::shared_ptr<ASTNodeExpr>               post()  { return this->_post; }
    inline std::shared_ptr<ASTNodeStat>               stat()  { return this->_stat; }

    virtual void check_constraints(std::shared_ptr<SemanticReporter> reporter) override;
};

class ASTNodeStatGoto: public ASTNodeStat
{
private:
    std::shared_ptr<TokenID> _label;

public:
    inline ASTNodeStatGoto(
            ASTNodeParserContext c,
            std::shared_ptr<TokenID> label):
        ASTNodeStat(c), _label(label) {}

    virtual void check_constraints(std::shared_ptr<SemanticReporter> reporter) override;
};

class ASTNodeStatContinue: public ASTNodeStat
{
public:
    inline ASTNodeStatContinue(
            ASTNodeParserContext c):
        ASTNodeStat(c) {}

    virtual void check_constraints(std::shared_ptr<SemanticReporter> reporter) override;
};

class ASTNodeStatBreak: public ASTNodeStat
{
public:
    inline ASTNodeStatBreak(
            ASTNodeParserContext c):
        ASTNodeStat(c) {}

    virtual void check_constraints(std::shared_ptr<SemanticReporter> reporter) override;
};

class ASTNodeStatReturn: public ASTNodeStat
{ 
private:
    std::shared_ptr<ASTNodeExpr> _expr;

public:
    inline ASTNodeStatReturn(
            ASTNodeParserContext c,
            std::shared_ptr<ASTNodeExpr> expr): ASTNodeStat(c), _expr(expr) {}

    inline       std::shared_ptr<ASTNodeExpr> expr()       { return this->_expr; }
    inline const std::shared_ptr<ASTNodeExpr> expr() const { return this->_expr; }

    virtual void check_constraints(std::shared_ptr<SemanticReporter> reporter) override;
};


class ASTNodeFunctionDefinition: public ASTNodeExternalDeclaration
{
private:
    std::shared_ptr<ASTNodeVariableTypeFunction> _decl;
    std::shared_ptr<ASTNodeStatCompound> _body;

public:
    inline ASTNodeFunctionDefinition(
            ASTNodeParserContext c,
            std::shared_ptr<ASTNodeVariableTypeFunction> decl,
            std::shared_ptr<ASTNodeStatCompound> body):
        ASTNodeExternalDeclaration(c), _decl(decl), _body(body) {}

    inline std::shared_ptr<ASTNodeVariableTypeFunction> decl() { return this->_decl; }
    inline std::shared_ptr<ASTNodeStatCompound>         body() { return this->_body; }

    virtual void check_constraints(std::shared_ptr<SemanticReporter> reporter) override;
};

class ASTNodeTranslationUnit: public ASTNode, private std::vector<std::shared_ptr<ASTNodeExternalDeclaration>>
{
private:
    using container_t = std::vector<std::shared_ptr<ASTNodeExternalDeclaration>>;

public:
    using reference = container_t::reference;
    using const_reference = container_t::const_reference;
    using iterator = container_t::iterator;
    using const_iterator = container_t::const_iterator;

    inline ASTNodeTranslationUnit(
            ASTNodeParserContext c): ASTNode(c) {}

    using container_t::begin;
    using container_t::end;
    using container_t::empty;
    using container_t::size;
    using container_t::operator[];
    using container_t::push_back;

    virtual void check_constraints(std::shared_ptr<SemanticReporter> reporter) override;
};

}
#endif // _C_PARSER_AST_H_
