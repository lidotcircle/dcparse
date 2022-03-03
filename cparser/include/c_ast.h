#ifndef _C_PARSER_AST_H_
#define _C_PARSER_AST_H_

#include "parser/parser.h"
#include "./c_token.h"
#include <string>
#include <memory>
#include <variant>

namespace cparser {


using ASTNodeParserContext = std::weak_ptr<DCParser::DCParserContext>;

class ASTNode {
private:
    ASTNodeParserContext m_parser_context;

public:
    inline ASTNode(ASTNodeParserContext p): m_parser_context(p) {}
    inline ASTNodeParserContext context() { return this->m_parser_context; }
    virtual ~ASTNode() = default;
};


// forward declaration
class ASTNodeType;
class ASTNodeDeclaration;
class ASTNodeInitializer;
class ASTNodeDeclarator;

class ASTNodeBlockItem : public ASTNode {
public:
    ASTNodeBlockItem(ASTNodeParserContext p): ASTNode(p) {}
};

class ASTNodeExternalDeclaration : public ASTNode {
public:
    ASTNodeExternalDeclaration(ASTNodeParserContext p): ASTNode(p) {}
};


/******************************************************************************
 *                                                                            *
 *                               Expression                                   *
 *                                                                            *
 ******************************************************************************/
class ASTNodeExpr : public ASTNode {
public:
    inline ASTNodeExpr(ASTNodeParserContext p): ASTNode(p) {}
};

class ASTNodeExprIdentifier: public ASTNodeExpr {
private:
    TokenID m_id;

public:
    inline ASTNodeExprIdentifier(ASTNodeParserContext p, TokenID id): ASTNodeExpr(p), m_id(id) {}
    inline TokenID id() { return this->m_id; }
};

class ASTNodeExprString: public ASTNodeExpr {
private:
    TokenStringLiteral m_token;

public:
    inline ASTNodeExprString(ASTNodeParserContext p, TokenStringLiteral str): ASTNodeExpr(p), m_token(str) {}
    inline TokenStringLiteral token() { return this->m_token; }
};

class ASTNodeExprInteger: public ASTNodeExpr {
private:
    TokenConstantInteger m_token;

public:
    inline ASTNodeExprInteger(ASTNodeParserContext p, TokenConstantInteger val): ASTNodeExpr(p), m_token(val) {}
    inline TokenConstantInteger token() { return this->m_token; }
};

class ASTNodeExprFloat: public ASTNodeExpr {
private:
    TokenConstantFloat m_token;

public:
    inline ASTNodeExprFloat(ASTNodeParserContext p, TokenConstantFloat val): ASTNodeExpr(p), m_token(val) {}
    inline TokenConstantFloat token() { return this->m_token; }
};

class ASTNodeExprIndexing: public ASTNodeExpr {
private:
    std::shared_ptr<ASTNodeExpr> m_array, m_idx;

public:
    inline ASTNodeExprIndexing(ASTNodeParserContext p, std::shared_ptr<ASTNodeExpr> array, std::shared_ptr<ASTNodeExpr> idx): ASTNodeExpr(p), m_array(array), m_idx(idx) {}
    inline std::shared_ptr<ASTNodeExpr> array() { return this->m_array; }
    inline std::shared_ptr<ASTNodeExpr> idx() { return this->m_idx; }
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
};

class ASTNodeExprMemberAccess: public ASTNodeExpr {
private:
    std::shared_ptr<ASTNodeExpr> m_obj;
    TokenID m_member;

public:
    inline ASTNodeExprMemberAccess(
            ASTNodeParserContext c,
            std::shared_ptr<ASTNodeExpr> obj,
            TokenID member): ASTNodeExpr(c), m_obj(obj), m_member(member) {}
    inline std::shared_ptr<ASTNodeExpr> obj() { return this->m_obj; }
    inline TokenID member() { return this->m_member; }
};

class ASTNodeExprPointerMemberAccess: public ASTNodeExpr {
private:
    std::shared_ptr<ASTNodeExpr> m_obj;
    TokenID m_member;

public:
    inline ASTNodeExprPointerMemberAccess(
            ASTNodeParserContext c,
            std::shared_ptr<ASTNodeExpr> obj,
            TokenID member): ASTNodeExpr(c), m_obj(obj), m_member(member) {}
    inline std::shared_ptr<ASTNodeExpr> obj() { return this->m_obj; }
    inline TokenID member() { return this->m_member; }
};

class ASTNodeExprInitializer: public ASTNodeExpr {
private:
    std::shared_ptr<ASTNodeInitializer> m_init;
    std::shared_ptr<ASTNodeType> m_type;

public:
    inline ASTNodeExprInitializer(
            ASTNodeParserContext c,
            std::shared_ptr<ASTNodeInitializer> init,
            std::shared_ptr<ASTNodeType> type): ASTNodeExpr(c), m_init(init), m_type(type) {}
    inline std::shared_ptr<ASTNodeInitializer> init() { return this->m_init; }
    inline std::shared_ptr<ASTNodeType> type() { return this->m_type; }
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
};

class ASTNodeExprSizeof: public ASTNodeExpr {
private:
    std::shared_ptr<ASTNodeType> m_type;

public:
    inline ASTNodeExprSizeof(
            ASTNodeParserContext c,
            std::shared_ptr<ASTNodeType> type
            ):
        ASTNodeExpr(c), m_type(type) {}
};

class ASTNodeExprCast: public ASTNodeExpr {
private:
    std::shared_ptr<ASTNodeType> m_type;
    std::shared_ptr<ASTNodeExpr> m_expr;

public:
    inline ASTNodeExprCast(
            ASTNodeParserContext c,
            std::shared_ptr<ASTNodeType> type,
            std::shared_ptr<ASTNodeExpr> expr
            ):
        ASTNodeExpr(c), m_type(type),
        m_expr(expr) {}
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
};


/******************************************************************************
 *                                                                            *
 *                              Declaration                                   *
 *                                                                            *
 ******************************************************************************/
class ASTNodeTypeSpecifier: public ASTNode {
public:
    inline ASTNodeTypeSpecifier(ASTNodeParserContext c): ASTNode(c) {}
};

class Qualifiable {
public:
    enum TypeQualifier { CONST, VOLATILE, RESTRICT, };

private:
    bool m_const, m_volatile, m_restrict;

public:
    inline Qualifiable(): m_const(false), m_volatile(false), m_restrict(false) {}

    inline bool& const_ref()    { return this->m_const; }
    inline bool& volatile_ref() { return this->m_volatile; }
    inline bool& restrict_ref() { return this->m_restrict; }
};

class ASTNodeDeclarationSpecifier: public ASTNode, public Qualifiable
{
public:
    enum StorageClass {
        SC_Typedef, SC_Extern, SC_Static, SC_Auto, SC_Register,
        SC_Default,
    };

private:
    StorageClass m_storage_class;
    std::shared_ptr<ASTNodeTypeSpecifier> m_type_specifier;
    bool m_inlined;

public:
    inline ASTNodeDeclarationSpecifier(ASTNodeParserContext c): ASTNode(c), m_storage_class(SC_Default), m_inlined(false) {}

    inline std::shared_ptr<ASTNodeTypeSpecifier>& type_specifier() { return this->m_type_specifier; }
    inline bool&                                  inlined() { return this->m_inlined; }
    inline StorageClass&                          storage_class() { return this->m_storage_class; }
};

class ASTNodeVariableType: public ASTNode {
public:
    inline ASTNodeVariableType(ASTNodeParserContext c): ASTNode(c) {}

    bool convert_to_type(std::shared_ptr<ASTNodeVariableType>& type);
};

class ASTNodeVariableTypePlain : public ASTNodeVariableType {
private:
    std::shared_ptr<ASTNodeDeclarationSpecifier> m_declaration_specifier;

public:
    inline ASTNodeVariableTypePlain(
            ASTNodeParserContext c,
            std::shared_ptr<ASTNodeDeclarationSpecifier> declaration_specifier
            ):
        ASTNodeVariableType(c),
        m_declaration_specifier(std::move(declaration_specifier)) {}
};

class ASTNodeVariableTypePointer : public ASTNodeVariableType, public Qualifiable
{
private:
    std::shared_ptr<ASTNodeVariableType> m_type;

public:
    inline ASTNodeVariableTypePointer(
            ASTNodeParserContext c,
            std::shared_ptr<ASTNodeVariableType> type
            ):
        ASTNodeVariableType(c),
        m_type(std::move(type)) {}
};

class ASTNodeVariableTypeArray : public ASTNodeVariableType, public Qualifiable
{
private:
    std::shared_ptr<ASTNodeVariableType> m_type;
    std::shared_ptr<ASTNodeExpr> m_size;
    bool m_static;

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
        m_static(static_) {}
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
};

class ASTNodeParameterDeclarationList: public ASTNode, private std::vector<std::shared_ptr<ASTNodeParameterDeclaration>>
{
private:
    using container_t = std::vector<std::shared_ptr<ASTNodeParameterDeclaration>>;

public:
    using reference = container_t::reference;
    using const_reference = container_t::const_reference;
    using iterator = container_t::iterator;
    using const_iterator = container_t::const_iterator;

    inline ASTNodeParameterDeclarationList(ASTNodeParserContext c): ASTNode(c) {}

    using container_t::begin;
    using container_t::end;
    using container_t::empty;
    using container_t::size;
    using container_t::operator[];
    using container_t::push_back;
};

class ASTNodeVariableTypeFunction : public ASTNodeVariableType
{
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
};

class ASTNodeTypeSpecifierBuiltin: public ASTNodeTypeSpecifier {
private:
    TokenID m_type;

public:
    inline ASTNodeTypeSpecifierBuiltin(
            ASTNodeParserContext c,
            TokenID type): ASTNodeTypeSpecifier(c), m_type(type) {}
    inline TokenID type() { return this->m_type; }
};

class ASTNodeTypeSpecifierStruct: public ASTNodeTypeSpecifier {
private:
    std::shared_ptr<TokenID> m_name;

public:
    inline ASTNodeTypeSpecifierStruct(
            ASTNodeParserContext c,
            std::shared_ptr<TokenID> name): ASTNodeTypeSpecifier(c), m_name(name) {}
    inline std::shared_ptr<TokenID> name() { return this->m_name; }
};

class ASTNodeTypeSpecifierUnion: public ASTNodeTypeSpecifier {
private:
    std::shared_ptr<TokenID> m_name;

public:
    inline ASTNodeTypeSpecifierUnion(
            ASTNodeParserContext c,
            std::shared_ptr<TokenID> name): ASTNodeTypeSpecifier(c), m_name(name) {}
    inline std::shared_ptr<TokenID> name() { return this->m_name; }
};

class ASTNodeTypeSpecifierEnum: public ASTNodeTypeSpecifier {
private:
    std::shared_ptr<TokenID> m_name;

public:
    inline ASTNodeTypeSpecifierEnum(
            ASTNodeParserContext c,
            std::shared_ptr<TokenID> name): ASTNodeTypeSpecifier(c), m_name(name) {}
    inline std::shared_ptr<TokenID> name() { return this->m_name; }
};

class ASTNodeTypeSpecifierTypedef: public ASTNodeTypeSpecifier {
private:
    std::shared_ptr<TokenID> m_name;

public:
    inline ASTNodeTypeSpecifierTypedef(
            ASTNodeParserContext c,
            std::shared_ptr<TokenID> name): ASTNodeTypeSpecifier(c), m_name(name) {}
    inline std::shared_ptr<TokenID> name() { return this->m_name; }
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
    TokenID m_label;
    std::shared_ptr<ASTNodeStat> m_stat;

public:
    ASTNodeStatLabel(
            ASTNodeParserContext c,
            TokenID label,
            std::shared_ptr<ASTNodeStat> stat):
        ASTNodeStat(c), m_label(label), m_stat(stat) {}
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
};

class ASTNodeStatDefault: public ASTNodeStat {
private:
    std::shared_ptr<ASTNodeStat> m_stat;

public:
    ASTNodeStatDefault(
            ASTNodeParserContext c,
            std::shared_ptr<ASTNodeStat> stat):
        ASTNodeStat(c), m_stat(stat) {}
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
};

class ASTNodeStatForDecl: public ASTNodeStat
{
private:
    std::shared_ptr<ASTNodeDeclaration> _decl;
    std::shared_ptr<ASTNodeExpr> _cond, _post;
    std::shared_ptr<ASTNodeStat> _stat;

public:
    inline ASTNodeStatForDecl(
            ASTNodeParserContext c,
            std::shared_ptr<ASTNodeDeclaration> decl,
            std::shared_ptr<ASTNodeExpr> cond,
            std::shared_ptr<ASTNodeExpr> post,
            std::shared_ptr<ASTNodeStat> stat):
        ASTNodeStat(c), _decl(decl), _cond(cond), _post(post), _stat(stat) {}

    inline std::shared_ptr<ASTNodeDeclaration> decl()       { return this->_decl; }
    inline std::shared_ptr<ASTNodeExpr> cond()       { return this->_cond; }
    inline std::shared_ptr<ASTNodeExpr> post()       { return this->_post; }
    inline std::shared_ptr<ASTNodeStat> stat()       { return this->_stat; }
};

class ASTNodeStatGoto: public ASTNodeStat
{
private:
    TokenID _label;

public:
    inline ASTNodeStatGoto(
            ASTNodeParserContext c,
            TokenID label):
        ASTNodeStat(c), _label(label) {}
};

class ASTNodeStatContinue: public ASTNodeStat
{
public:
    inline ASTNodeStatContinue(
            ASTNodeParserContext c):
        ASTNodeStat(c) {}
};

class ASTNodeStatBreak: public ASTNodeStat
{
public:
    inline ASTNodeStatBreak(
            ASTNodeParserContext c):
        ASTNodeStat(c) {}
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
};

}
#endif // _C_PARSER_AST_H_
