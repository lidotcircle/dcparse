#include "c_parser.h"
#include <memory>
using namespace std;


#define NONTERMS \
    TENTRY(PRIMARY_EXPRESSION) \
    TENTRY(POSTFIX_EXPRESSION) \
    TENTRY(ARGUMENT_EXPRESSION_LIST) \
    TENTRY(UNARY_EXPRESSION) \
    TENTRY(CAST_EXPRESSION) \
    TENTRY(MULTIPLICATIVE_EXPRESSION) \
    TENTRY(ADDITIVE_EXPRESSION) \
    TENTRY(SHIFT_EXPRESSION) \
    TENTRY(RELATIONAL_EXPRESSION) \
    TENTRY(EQUALITY_EXPRESSION) \
    TENTRY(AND_EXPRESSION) \
    TENTRY(EXCLUSIVE_OR_EXPRESSION) \
    TENTRY(INCLUSIVE_OR_EXPRESSION) \
    TENTRY(LOGICAL_AND_EXPRESSION) \
    TENTRY(LOGICAL_OR_EXPRESSION) \
    TENTRY(CONDITIONAL_EXPRESSION) \
    TENTRY(ASSIGNMENT_EXPRESSION) \
    TENTRY(EXPRESSION) \
    TENTRY(CONSTANT_EXPRESSION) \
    \
    TENTRY(DECLARATION) \
    TENTRY(DECLARATION_SPECIFIERS) \
    TENTRY(INIT_DECLARATOR_LIST) \
    TENTRY(INIT_DECLARATOR) \
    TENTRY(STORAGE_CLASS_SPECIFIER) \
    TENTRY(TYPE_SPECIFIER) \
    TENTRY(STRUCT_OR_UNION_SPECIFIER) \
    TENTRY(STRUCT_OR_UNIOIN) \
    TENTRY(STRUCT_DECLARATION_LIST) \
    TENTRY(STRUCT_DECLARATION) \
    TENTRY(SPECIFIER_QUALIFIER_LIST) \
    TENTRY(STRUCT_DECLARATOR) \
    TENTRY(ENUM_SPECIFIER) \
    TENTRY(ENUMERATOR_LIST) \
    TENTRY(ENUMERATOR) \
    TENTRY(TYPE_QUALIFIER) \
    TENTRY(FUNCTION_SPECIFIER) \
    TENTRY(DECLARATOR) \
    TENTRY(DIRECT_DECLARATOR) \
    TENTRY(POINTER) \
    TENTRY(TYPE_QUALIFIER_LIST) \
    TENTRY(PARAMETER_TYPE_LIST) \
    TENTRY(PARAMETER_LIST) \
    TENTRY(PARAMETER_DECLARATION) \
    TENTRY(IDENTIFIER_LIST) \
    TENTRY(ABSTRACT_DECLARATOR) \
    TENTRY(DIRECT_ABSTRACT_DECLARATOR) \
    TENTRY(TYPEDEF_NAME) \
    TENTRY(INITIALIZER) \
    TENTRY(INITIALIZER_LIST) \
    TENTRY(DESIGNATION) \
    TENTRY(DESIGNATOR_LIST) \
    TENTRY(DESIGNATOR) \
    \
    TENTRY(STATEMENT) \
    TENTRY(LABELED_STATEMENT) \
    TENTRY(COMPOUND_STATEMENT) \
    TENTRY(BLOCK_ITEM_LIST) \
    TENTRY(BLOCK_ITEM) \
    TENTRY(EXPRESSION_STATEMENT) \
    TENTRY(SELECTION_STATEMENT) \
    TENTRY(ITERATION_STATEMENT) \
    TENTRY(JUMP_STATEMENT) \
    TENTRY(TRANSLATION_UNIT) \
    TENTRY(EXTERNAL_DECLARATION) \
    TENTRY(FUNCTION_DEFINITION) \
    TENTRY(DECLARATION_LIST) \

#define TENTRY(n) \
    struct NonTerm##n: public NonTerminal { \
        std::shared_ptr<cparser::ASTNode> astnode; \
        inline NonTerm##n(std::shared_ptr<cparser::ASTNode> node): astnode(node) {} \
    };
NONTERMS
#undef TENTRY

#define NI(t) CharInfo<NonTerm##t>()
#define TI(t) CharInfo<Token##t>()
#define KW(t) CharInfo<TokenKeyword_##t>()
#define PT(t) CharInfo<TokenPunc##t>()
using ParserChar = DCParser::ParserChar;


#define add_binary_rule(op, astop, assoc) \
    parser( NI(EXPRESSION), \
        { NI(EXPRESSION), PT(op), NI(EXPRESSION) }, \
        [](auto c, auto ts) { \
            assert(ts.size() == 3); \
            auto left  = dynamic_pointer_cast<NonTermEXPRESSION>(ts[0]); \
            auto right = dynamic_pointer_cast<NonTermEXPRESSION>(ts[2]); \
            assert(left && right); \
            auto astleft = dynamic_pointer_cast<ASTNodeExpr>(left->astnode); \
            auto astright = dynamic_pointer_cast<ASTNodeExpr>(right->astnode); \
            auto ast = make_shared<ASTNodeExprBinaryOp>( \
                    c, \
                    ASTNodeExprBinaryOp::astop, \
                    astleft, \
                    astright); \
            return make_shared<NonTermEXPRESSION>(ast); \
        }, assoc);

#define add_prefix_unary_rule(op, astop, assoc) \
    parser( NI(EXPRESSION), \
        { PT(op), NI(EXPRESSION) }, \
        [](auto c, auto ts) { \
            assert(ts.size() == 2); \
            auto right = dynamic_pointer_cast<NonTermEXPRESSION>(ts[1]); \
            assert(right); \
            auto astright = dynamic_pointer_cast<ASTNodeExpr>(right->astnode); \
            auto ast = make_shared<ASTNodeExprUnaryOp>( \
                    c, \
                    ASTNodeExprUnaryOp::astop, \
                    astright); \
            return make_shared<NonTermEXPRESSION>(ast); \
        }, \
        assoc);

#define add_postfix_unary_rule(op, astop, assoc) \
    parser( NI(EXPRESSION), \
        { NI(EXPRESSION), PT(op) }, \
        [](auto c, auto ts) { \
            assert(ts.size() == 2); \
            auto left = dynamic_pointer_cast<NonTermEXPRESSION>(ts[0]); \
            assert(left); \
            auto leftast = dynamic_pointer_cast<ASTNodeExpr>(left->astnode); \
            auto ast = make_shared<ASTNodeExprUnaryOp>( \
                    c, \
                    ASTNodeExprUnaryOp::astop, \
                    leftast); \
            return make_shared<NonTermEXPRESSION>(ast); \
        }, \
        assoc);


namespace cparser {

void CParser::expression_rules()
{
    DCParser& parser = *this;


    parser( NI(EXPRESSION),
        { TI(ID) },
        [](auto c, auto ts) {
            assert(ts.size() == 1);
            auto id = dynamic_pointer_cast<TokenID>(ts[0]);
            assert(id);
            auto ast = make_shared<ASTNodeExprIdentifier>(c, id);
            return make_shared<NonTermEXPRESSION>(ast);
        }, RuleAssocitiveLeft);

    parser( NI(EXPRESSION),
        { TI(ConstantInteger) },
        [](auto c, auto ts) {
            assert(ts.size() == 1);
            auto integer = dynamic_pointer_cast<TokenConstantInteger>(ts[0]);
            assert(integer);
            auto ast = make_shared<ASTNodeExprInteger>(c, integer);
            return make_shared<NonTermEXPRESSION>(ast);
        }, RuleAssocitiveLeft);

    parser( NI(EXPRESSION),
        { TI(ConstantFloat) },
        [](auto c, auto ts) {
            assert(ts.size() == 1);
            auto float_ = dynamic_pointer_cast<TokenConstantFloat>(ts[0]);
            assert(float_);
            auto ast = make_shared<ASTNodeExprFloat>(c, float_);
            return make_shared<NonTermEXPRESSION>(ast);
        }, RuleAssocitiveLeft);

    parser( NI(EXPRESSION),
        { TI(StringLiteral) },
        [] (auto c, auto ts) {
            assert(ts.size() == 1);
            auto str = dynamic_pointer_cast<TokenStringLiteral>(ts[0]);
            assert(str);
            auto ast = make_shared<ASTNodeExprString>(c, str);
            return make_shared<NonTermEXPRESSION>(ast);
        }, RuleAssocitiveLeft);

    parser( NI(EXPRESSION),
        { PT(LPAREN), NI(EXPRESSION), PT(RPAREN) },
        [](auto c, auto ts) {
            assert(ts.size() == 3);
            auto expr = dynamic_pointer_cast<NonTermEXPRESSION>(ts[1]);
            assert(expr);
            return expr;
        }, RuleAssocitiveLeft);


    parser.dec_priority();


    parser( NI(EXPRESSION),
        { NI(EXPRESSION), PT(LBRACKET), NI(EXPRESSION), PT(RBRACKET) },
        [](auto c, auto ts) {
            assert(ts.size() == 4);
            auto expr = dynamic_pointer_cast<NonTermEXPRESSION>(ts[0]);
            assert(expr);
            auto index = dynamic_pointer_cast<NonTermEXPRESSION>(ts[2]);
            assert(index);
            auto ast = make_shared<ASTNodeExprIndexing>(c, expr, index);
            return make_shared<NonTermEXPRESSION>(ast);
        }, RuleAssocitiveLeft);

    parser( NI(EXPRESSION),
        { NI(EXPRESSION), PT(LPAREN), ParserChar::beOptional(NI(ARGUMENT_EXPRESSION_LIST)), PT(RPAREN) },
        [](auto c, auto ts) {
            assert(ts.size() == 4);
            auto func = dynamic_pointer_cast<NonTermEXPRESSION>(ts[0]);
            assert(func);
            auto args = dynamic_pointer_cast<NonTermARGUMENT_EXPRESSION_LIST>(ts[2]);
            auto argsast = make_shared<ASTNodeArgList>(c, args);
            if (args) {
                auto ax = dynamic_pointer_cast<ASTNodeArgList>(args->astnode);
                assert(ax);
                argsast = ax;
            }
            auto ast = make_shared<ASTNodeExprFunctionCall>(c, func, argsast);
            return make_shared<NonTermEXPRESSION>(ast);
        }, RuleAssocitiveLeft);

    parser( NI(EXPRESSION),
        { NI(EXPRESSION), PT(DOT), TI(ID) },
        [](auto c, auto ts) {
            assert(ts.size() == 3);
            auto expr = dynamic_pointer_cast<NonTermEXPRESSION>(ts[0]);
            assert(expr);
            auto id = dynamic_pointer_cast<TokenID>(ts[2]);
            assert(id);
            auto ast = make_shared<ASTNodeExprMemberAccess>(c, expr, id);
            return make_shared<NonTermEXPRESSION>(ast);
        }, RuleAssocitiveLeft);

    parser( NI(EXPRESSION),
        { NI(EXPRESSION), PT(PTRACCESS), TI(ID) },
        [](auto c, auto ts) {
            assert(ts.size() == 3);
            auto expr = dynamic_pointer_cast<NonTermEXPRESSION>(ts[0]);
            assert(expr);
            auto id = dynamic_pointer_cast<TokenID>(ts[2]);
            assert(id);
            auto ast = make_shared<ASTNodeExprPointerMemberAccess>(c, expr, id);
            return make_shared<NonTermEXPRESSION>(ast);
        }, RuleAssocitiveLeft);

    add_postfix_unary_rule(PLUSPLUS, POSFIX_INC, RuleAssocitiveLeft);
    add_postfix_unary_rule(MINUSMINUS, POSFIX_DEC, RuleAssocitiveLeft);

    add_prefix_unary_rule(REF, REFERENCE, RuleAssocitiveRight);

    add_binary_rule(MULTIPLY, MULTIPLY, RuleAssocitiveLeft);
}

CParser::CParser()
{
    this->expression_rules();
    this->__________();
    this->declaration_rules();
    this->__________();
    this->statement_rules  ();
    this->__________();
    this->function_rules   ();
    this->__________();
    this->translation_unit_rules();

    this->add_start_symbol(NI(TRANSLATION_UNIT).id);

    this->generate_table();
}

}
