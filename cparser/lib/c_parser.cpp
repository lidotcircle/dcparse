#include "c_parser.h"
#include "c_error.h"
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
    TENTRY(STRUCT_OR_UNION) \
    TENTRY(STRUCT_DECLARATION_LIST) \
    TENTRY(STRUCT_DECLARATION) \
    TENTRY(SPECIFIER_QUALIFIER_LIST) \
    TENTRY(STRUCT_DECLARATOR_LIST) \
    TENTRY(STRUCT_DECLARATOR) \
    TENTRY(ENUM_SPECIFIER) \
    TENTRY(ENUMERATOR_LIST) \
    TENTRY(ENUMERATOR) \
    TENTRY(ENUMERATION_CONSTANT) \
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
    TENTRY(TYPE_NAME) \
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


#define get_ast(varn, nonterm, nodetype, idx) \
    auto varn = dynamic_pointer_cast<NonTerm##nonterm>(ts[idx]); \
    assert(varn && varn->astnode); \
    auto varn##ast = dynamic_pointer_cast<nodetype>(varn->astnode); \
    assert(varn##ast);

#define get_ast_if_presents(varn, nonterm, nodetype, idx) \
    auto varn = dynamic_pointer_cast<NonTerm##nonterm>(ts[idx]); \
    std::shared_ptr<nodetype> varn##ast = nullptr; \
    if (varn) {\
        assert(varn->astnode); \
        varn##ast = dynamic_pointer_cast<nodetype>(varn->astnode); \
        assert(varn##ast);\
    }

#define add_binary_rule(nonterm, leftnonterm, rightnonterm, op, astop, assoc) \
    parser( NI(nonterm), \
        { NI(leftnonterm), PT(op), NI(rightnonterm) }, \
        [](auto c, auto ts) { \
            assert(ts.size() == 3); \
            get_ast(lhs, leftnonterm, ASTNodeExpr, 0); \
            get_ast(rhs, rightnonterm, ASTNodeExpr, 2); \
            auto ast = make_shared<ASTNodeExprBinaryOp>( \
                    c, \
                    ASTNodeExprBinaryOp::astop, \
                    lhsast, \
                    rhsast); \
            return make_shared<NonTerm##nonterm>(ast); \
        }, assoc);

#define add_prefix_unary_rule(lhsnonterm, rhsnonterm, op, astop, assoc) \
    parser( NI(lhsnonterm), \
        { PT(op), NI(rhsnonterm) }, \
        [](auto c, auto ts) { \
            assert(ts.size() == 2); \
            get_ast(rhs, rhsnonterm, ASTNodeExpr, 1); \
            auto ast = make_shared<ASTNodeExprUnaryOp>( \
                    c, \
                    ASTNodeExprUnaryOp::astop, \
                    rhsast); \
            return make_shared<NonTerm##lhsnonterm>(ast); \
        }, \
        assoc);

#define add_postfix_unary_rule(lhsnonterm, rhsnonterm, op, astop, assoc) \
    parser( NI(lhsnonterm), \
        { NI(rhsnonterm), PT(op) }, \
        [](auto c, auto ts) { \
            assert(ts.size() == 2); \
            get_ast(lhs, rhsnonterm, ASTNodeExpr, 0); \
            auto ast = make_shared<ASTNodeExprUnaryOp>( \
                    c, \
                    ASTNodeExprUnaryOp::astop, \
                    lhsast); \
            return make_shared<NonTerm##rhsnonterm>(ast); \
        }, \
        assoc);


namespace cparser {


CParserContext::CParserContext(CParser* parser):
    DCParser::DCParserContext(), m_cparser(parser) {}


void CParser::typedef_rule()
{
    auto& parser = *this;


    parser( NI(TYPEDEF_NAME),
        { TI(ID) },
        [](auto c, auto ts) {
            assert(ts.size() == 1);
            auto id = dynamic_pointer_cast<TokenID>(ts[0]);
            assert(id);
            return make_shared<NonTermTYPEDEF_NAME>(make_shared<ASTNodeTypeSpecifierTypedef>(c, id));
        }, RuleAssocitiveLeft, 
        make_shared<RuleDecisionFunction>([](auto ctx, auto& rhs, auto& stack) {
            assert(rhs.size() == 1);
            auto id = dynamic_pointer_cast<TokenID>(rhs[0]);
            assert(id);
            auto _id = id->id;
            auto sctx = ctx.lock();
            assert(sctx);
            auto cctx = dynamic_pointer_cast<CParserContext>(sctx);
            assert(cctx);
            auto _parser = cctx->cparser();
            assert(_parser);
            auto& typedefs = _parser->m_typedefs;

            if (typedefs.find(_id) == typedefs.end())
                return false;

            if (stack.size() > 0) {
                auto& b = stack.back();
                const auto m = dynamic_pointer_cast<NonTermDECLARATION_SPECIFIERS>(b);
                if (!m) return true;

                auto dsast = dynamic_pointer_cast<ASTNodeDeclarationSpecifier>(m->astnode);
                assert(dsast);
                if (dsast->type_specifier())
                    return false;
            }

            return true;
        }));
}

#define expr_reduce(to, from) \
    parser.__________(); \
    parser( NI(to), \
        { NI(from) }, \
        [](auto c, auto ts) { \
            assert(ts.size() == 1); \
            get_ast(expr, from, ASTNodeExpr, 0); \
            return make_shared<NonTerm##to>(exprast); \
        });

void CParser::expression_rules()
{
    DCParser& parser = *this;


    parser( NI(PRIMARY_EXPRESSION),
        { TI(ID) },
        [](auto c, auto ts) {
            assert(ts.size() == 1);
            auto id = dynamic_pointer_cast<TokenID>(ts[0]);
            assert(id);
            auto ast = make_shared<ASTNodeExprIdentifier>(c, id);
            return make_shared<NonTermPRIMARY_EXPRESSION>(ast);
        }, RuleAssocitiveLeft);

    parser( NI(PRIMARY_EXPRESSION),
        { TI(ConstantInteger) },
        [](auto c, auto ts) {
            assert(ts.size() == 1);
            auto integer = dynamic_pointer_cast<TokenConstantInteger>(ts[0]);
            assert(integer);
            auto ast = make_shared<ASTNodeExprInteger>(c, integer);
            return make_shared<NonTermPRIMARY_EXPRESSION>(ast);
        }, RuleAssocitiveLeft);

    parser( NI(PRIMARY_EXPRESSION),
        { TI(ConstantFloat) },
        [](auto c, auto ts) {
            assert(ts.size() == 1);
            auto float_ = dynamic_pointer_cast<TokenConstantFloat>(ts[0]);
            assert(float_);
            auto ast = make_shared<ASTNodeExprFloat>(c, float_);
            return make_shared<NonTermPRIMARY_EXPRESSION>(ast);
        }, RuleAssocitiveLeft);

    parser( NI(PRIMARY_EXPRESSION),
        { TI(StringLiteral) },
        [] (auto c, auto ts) {
            assert(ts.size() == 1);
            auto str = dynamic_pointer_cast<TokenStringLiteral>(ts[0]);
            assert(str);
            auto ast = make_shared<ASTNodeExprString>(c, str);
            return make_shared<NonTermPRIMARY_EXPRESSION>(ast);
        }, RuleAssocitiveLeft);

    parser( NI(PRIMARY_EXPRESSION),
        { PT(LPAREN), NI(EXPRESSION), PT(RPAREN) },
        [](auto c, auto ts) {
            assert(ts.size() == 3);
            get_ast(expr, EXPRESSION, ASTNodeExpr, 1);
            return make_shared<NonTermPRIMARY_EXPRESSION>(exprast);
        }, RuleAssocitiveLeft);


    expr_reduce(POSTFIX_EXPRESSION, PRIMARY_EXPRESSION);

    parser( NI(POSTFIX_EXPRESSION),
        { NI(POSTFIX_EXPRESSION), PT(LBRACKET), NI(EXPRESSION), PT(RBRACKET) },
        [](auto c, auto ts) {
            assert(ts.size() == 4);
            get_ast(expr, POSTFIX_EXPRESSION, ASTNodeExpr, 0);
            get_ast(index, EXPRESSION, ASTNodeExpr, 2);
            auto ast = make_shared<ASTNodeExprIndexing>(c, exprast, indexast);
            return make_shared<NonTermPOSTFIX_EXPRESSION>(ast);
        }, RuleAssocitiveLeft);

    parser( NI(POSTFIX_EXPRESSION),
        { NI(POSTFIX_EXPRESSION), PT(LPAREN), ParserChar::beOptional(NI(ARGUMENT_EXPRESSION_LIST)), PT(RPAREN) },
        [](auto c, auto ts) {
            assert(ts.size() == 4);
            get_ast(func, POSTFIX_EXPRESSION, ASTNodeExpr, 0);
            auto args = dynamic_pointer_cast<NonTermARGUMENT_EXPRESSION_LIST>(ts[2]);
            auto argsast = make_shared<ASTNodeArgList>(c);
            if (args) {
                auto ax = dynamic_pointer_cast<ASTNodeArgList>(args->astnode);
                assert(ax);
                argsast = ax;
            }
            auto ast = make_shared<ASTNodeExprFunctionCall>(c, funcast, argsast);
            return make_shared<NonTermPOSTFIX_EXPRESSION>(ast);
        }, RuleAssocitiveLeft);

    parser( NI(POSTFIX_EXPRESSION),
        { NI(POSTFIX_EXPRESSION), PT(DOT), TI(ID) },
        [](auto c, auto ts) {
            assert(ts.size() == 3);
            get_ast(expr, POSTFIX_EXPRESSION, ASTNodeExpr, 0);
            auto id = dynamic_pointer_cast<TokenID>(ts[2]);
            assert(id);
            auto ast = make_shared<ASTNodeExprMemberAccess>(c, exprast, id);
            return make_shared<NonTermPOSTFIX_EXPRESSION>(ast);
        }, RuleAssocitiveLeft);

    parser( NI(POSTFIX_EXPRESSION),
        { NI(POSTFIX_EXPRESSION), PT(PTRACCESS), TI(ID) },
        [](auto c, auto ts) {
            assert(ts.size() == 3);
            get_ast(expr, POSTFIX_EXPRESSION, ASTNodeExpr, 0);
            auto id = dynamic_pointer_cast<TokenID>(ts[2]);
            assert(id);
            auto ast = make_shared<ASTNodeExprPointerMemberAccess>(c, exprast, id);
            return make_shared<NonTermPOSTFIX_EXPRESSION>(ast);
        }, RuleAssocitiveLeft);

    add_postfix_unary_rule(POSTFIX_EXPRESSION, POSTFIX_EXPRESSION, PLUSPLUS, POSFIX_INC, RuleAssocitiveLeft);
    add_postfix_unary_rule(POSTFIX_EXPRESSION, POSTFIX_EXPRESSION, MINUSMINUS, POSFIX_DEC, RuleAssocitiveLeft);

    parser( NI(POSTFIX_EXPRESSION),
        { PT(LPAREN), NI(TYPE_NAME), PT(RPAREN), PT(LBRACE), NI(INITIALIZER_LIST), ParserChar::beOptional(PT(COMMA)), PT(RBRACE) },
        [](auto c, auto ts) {
            assert(ts.size() == 7);
            get_ast(type, TYPE_NAME, ASTNodeVariableType, 1);
            get_ast(init, INITIALIZER_LIST, ASTNodeInitializerList, 4);
            auto ast = make_shared<ASTNodeExprInitializer>(c, typeast, initast);
            return make_shared<NonTermPOSTFIX_EXPRESSION>(ast);
        }, RuleAssocitiveLeft);

    parser( NI(ARGUMENT_EXPRESSION_LIST),
        { ParserChar::beOptional(NI(ARGUMENT_EXPRESSION_LIST)), NI(ASSIGNMENT_EXPRESSION) },
        [](auto c, auto ts) {
            assert(ts.size() == 2);
            auto args = dynamic_pointer_cast<NonTermARGUMENT_EXPRESSION_LIST>(ts[0]);
            get_ast(expr, ASSIGNMENT_EXPRESSION, ASTNodeExpr, 1);
            auto argsast = make_shared<ASTNodeArgList>(c);
            if (args) {
                auto ax = dynamic_pointer_cast<ASTNodeArgList>(args->astnode);
                assert(ax);
                argsast = ax;
            }
            argsast->push_back(exprast);
            return make_shared<NonTermARGUMENT_EXPRESSION_LIST>(argsast);
        }, RuleAssocitiveLeft);


    expr_reduce(UNARY_EXPRESSION, POSTFIX_EXPRESSION);
    add_prefix_unary_rule(UNARY_EXPRESSION, UNARY_EXPRESSION, PLUSPLUS, PREFIX_INC, RuleAssocitiveRight);
    add_prefix_unary_rule(UNARY_EXPRESSION, UNARY_EXPRESSION, MINUSMINUS, PREFIX_DEC, RuleAssocitiveRight);

    add_prefix_unary_rule(UNARY_EXPRESSION, CAST_EXPRESSION,  REF, REFERENCE, RuleAssocitiveRight);
    add_prefix_unary_rule(UNARY_EXPRESSION, CAST_EXPRESSION,  MULTIPLY, DEREFERENCE, RuleAssocitiveRight);
    add_prefix_unary_rule(UNARY_EXPRESSION, CAST_EXPRESSION,  PLUS, PLUS, RuleAssocitiveRight);
    add_prefix_unary_rule(UNARY_EXPRESSION, CAST_EXPRESSION,  MINUS, MINUS, RuleAssocitiveRight);
    add_prefix_unary_rule(UNARY_EXPRESSION, CAST_EXPRESSION,  BIT_NOT, BITWISE_NOT, RuleAssocitiveRight);
    add_prefix_unary_rule(UNARY_EXPRESSION, CAST_EXPRESSION,  LOGIC_NOT, LOGICAL_NOT, RuleAssocitiveRight);

    parser( NI(UNARY_EXPRESSION),
        { KW(sizeof), NI(UNARY_EXPRESSION) },
        [](auto c, auto ts) {
            assert(ts.size() == 2);
            get_ast(rhs, UNARY_EXPRESSION, ASTNodeExpr, 1);
            auto ast = make_shared<ASTNodeExprUnaryOp>(
                    c,
                    ASTNodeExprUnaryOp::SIZEOF,
                    rhsast);
            return make_shared<NonTermUNARY_EXPRESSION>(ast);
        },
        RuleAssocitiveLeft);

    parser( NI(UNARY_EXPRESSION),
        { KW(sizeof), PT(LPAREN), NI(TYPE_NAME), PT(RPAREN) },
        [](auto c, auto ts) {
            assert(ts.size() == 4);
            get_ast(rhs, TYPE_NAME, ASTNodeVariableType, 2);
            auto ast = make_shared<ASTNodeExprSizeof>(
                    c,
                    rhsast);
            return make_shared<NonTermUNARY_EXPRESSION>(ast);
        },
        RuleAssocitiveLeft);


    expr_reduce(CAST_EXPRESSION, UNARY_EXPRESSION);
    parser( NI(CAST_EXPRESSION),
        { PT(LPAREN), NI(TYPE_NAME), PT(RPAREN), NI(CAST_EXPRESSION) },
        [](auto c, auto ts) {
            assert(ts.size() == 4);
            get_ast(type, TYPE_NAME, ASTNodeVariableType, 2);
            get_ast(rhs, CAST_EXPRESSION, ASTNodeExpr, 3);
            auto ast = make_shared<ASTNodeExprCast>(c, typeast, rhsast);
            return make_shared<NonTermCAST_EXPRESSION>(ast);
        }, RuleAssocitiveLeft);


    expr_reduce(MULTIPLICATIVE_EXPRESSION, CAST_EXPRESSION);
    add_binary_rule(MULTIPLICATIVE_EXPRESSION, MULTIPLICATIVE_EXPRESSION, CAST_EXPRESSION, MULTIPLY, MULTIPLY, RuleAssocitiveLeft);
    add_binary_rule(MULTIPLICATIVE_EXPRESSION, MULTIPLICATIVE_EXPRESSION, CAST_EXPRESSION, DIVISION, DIVISION, RuleAssocitiveLeft);
    add_binary_rule(MULTIPLICATIVE_EXPRESSION, MULTIPLICATIVE_EXPRESSION, CAST_EXPRESSION, REMAINDER, REMAINDER, RuleAssocitiveLeft);


    expr_reduce(ADDITIVE_EXPRESSION, MULTIPLICATIVE_EXPRESSION);
    add_binary_rule(ADDITIVE_EXPRESSION, ADDITIVE_EXPRESSION, MULTIPLICATIVE_EXPRESSION, PLUS, PLUS, RuleAssocitiveLeft);
    add_binary_rule(ADDITIVE_EXPRESSION, ADDITIVE_EXPRESSION, MULTIPLICATIVE_EXPRESSION, MINUS, MINUS, RuleAssocitiveLeft);


    expr_reduce(SHIFT_EXPRESSION, ADDITIVE_EXPRESSION);
    add_binary_rule(SHIFT_EXPRESSION, SHIFT_EXPRESSION, ADDITIVE_EXPRESSION, LEFT_SHIFT, LEFT_SHIFT, RuleAssocitiveLeft);
    add_binary_rule(SHIFT_EXPRESSION, SHIFT_EXPRESSION, ADDITIVE_EXPRESSION, RIGHT_SHIFT, RIGHT_SHIFT, RuleAssocitiveLeft);


    expr_reduce(RELATIONAL_EXPRESSION, SHIFT_EXPRESSION);
    add_binary_rule(RELATIONAL_EXPRESSION, RELATIONAL_EXPRESSION, SHIFT_EXPRESSION, LESS_THAN, LESS_THAN, RuleAssocitiveLeft);
    add_binary_rule(RELATIONAL_EXPRESSION, RELATIONAL_EXPRESSION, SHIFT_EXPRESSION, GREATER_THAN, GREATER_THAN, RuleAssocitiveLeft);
    add_binary_rule(RELATIONAL_EXPRESSION, RELATIONAL_EXPRESSION, SHIFT_EXPRESSION, LESS_EQUAL, LESS_THAN_EQUAL, RuleAssocitiveLeft);
    add_binary_rule(RELATIONAL_EXPRESSION, RELATIONAL_EXPRESSION, SHIFT_EXPRESSION, GREATER_EQUAL, GREATER_THAN_EQUAL, RuleAssocitiveLeft);


    expr_reduce(EQUALITY_EXPRESSION, RELATIONAL_EXPRESSION);
    add_binary_rule(EQUALITY_EXPRESSION, EQUALITY_EXPRESSION, RELATIONAL_EXPRESSION, EQUAL, EQUAL, RuleAssocitiveLeft);
    add_binary_rule(EQUALITY_EXPRESSION, EQUALITY_EXPRESSION, RELATIONAL_EXPRESSION, NOT_EQUAL, NOT_EQUAL, RuleAssocitiveLeft);


    expr_reduce(AND_EXPRESSION, EQUALITY_EXPRESSION);
    add_binary_rule(AND_EXPRESSION, AND_EXPRESSION, EQUALITY_EXPRESSION, REF, BITWISE_AND, RuleAssocitiveLeft);


    expr_reduce(EXCLUSIVE_OR_EXPRESSION, AND_EXPRESSION);
    add_binary_rule(EXCLUSIVE_OR_EXPRESSION, EXCLUSIVE_OR_EXPRESSION, AND_EXPRESSION, BIT_XOR , BITWISE_XOR, RuleAssocitiveLeft);


    expr_reduce(INCLUSIVE_OR_EXPRESSION, EXCLUSIVE_OR_EXPRESSION);
    add_binary_rule(INCLUSIVE_OR_EXPRESSION, INCLUSIVE_OR_EXPRESSION, EXCLUSIVE_OR_EXPRESSION, BIT_OR, BITWISE_OR, RuleAssocitiveLeft);


    expr_reduce(LOGICAL_AND_EXPRESSION, INCLUSIVE_OR_EXPRESSION);
    add_binary_rule(LOGICAL_AND_EXPRESSION, LOGICAL_AND_EXPRESSION, INCLUSIVE_OR_EXPRESSION, LOGIC_AND, LOGICAL_AND, RuleAssocitiveLeft);


    expr_reduce(LOGICAL_OR_EXPRESSION, LOGICAL_AND_EXPRESSION);
    add_binary_rule(LOGICAL_OR_EXPRESSION, LOGICAL_OR_EXPRESSION, LOGICAL_AND_EXPRESSION, LOGIC_OR, LOGICAL_OR, RuleAssocitiveLeft);


    expr_reduce(CONDITIONAL_EXPRESSION, LOGICAL_OR_EXPRESSION);
    parser( NI(CONDITIONAL_EXPRESSION),
        { NI(LOGICAL_OR_EXPRESSION), PT(QUESTION), NI(EXPRESSION), PT(COLON), NI(CONDITIONAL_EXPRESSION) },
        [](auto c, auto ts) {
            assert(ts.size() == 4);
            get_ast(cond, LOGICAL_OR_EXPRESSION, ASTNodeExpr, 0);
            get_ast(true_expr, EXPRESSION, ASTNodeExpr, 2);
            get_ast(false_expr, CONDITIONAL_EXPRESSION, ASTNodeExpr, 4);
            auto ast = make_shared<ASTNodeExprConditional>(c, condast, true_exprast, false_exprast);
            return make_shared<NonTermCONDITIONAL_EXPRESSION>(ast);
        }, RuleAssocitiveRight);


    expr_reduce(ASSIGNMENT_EXPRESSION, CONDITIONAL_EXPRESSION);
    add_binary_rule(ASSIGNMENT_EXPRESSION, UNARY_EXPRESSION, ASSIGNMENT_EXPRESSION, ASSIGN, ASSIGNMENT, RuleAssocitiveRight);
    add_binary_rule(ASSIGNMENT_EXPRESSION, UNARY_EXPRESSION, ASSIGNMENT_EXPRESSION, MULTIPLY_ASSIGN, ASSIGNMENT_MULTIPLY, RuleAssocitiveRight);
    add_binary_rule(ASSIGNMENT_EXPRESSION, UNARY_EXPRESSION, ASSIGNMENT_EXPRESSION, DIVISION_ASSIGN, ASSIGNMENT_DIVISION, RuleAssocitiveRight);
    add_binary_rule(ASSIGNMENT_EXPRESSION, UNARY_EXPRESSION, ASSIGNMENT_EXPRESSION, REMAINDER_ASSIGN, ASSIGNMENT_REMAINDER, RuleAssocitiveRight);
    add_binary_rule(ASSIGNMENT_EXPRESSION, UNARY_EXPRESSION, ASSIGNMENT_EXPRESSION, PLUS_ASSIGN, ASSIGNMENT_PLUS, RuleAssocitiveRight);
    add_binary_rule(ASSIGNMENT_EXPRESSION, UNARY_EXPRESSION, ASSIGNMENT_EXPRESSION, MINUS_ASSIGN, ASSIGNMENT_MINUS, RuleAssocitiveRight);
    add_binary_rule(ASSIGNMENT_EXPRESSION, UNARY_EXPRESSION, ASSIGNMENT_EXPRESSION, LEFT_SHIFT_ASSIGN, ASSIGNMENT_LEFT_SHIFT, RuleAssocitiveRight);
    add_binary_rule(ASSIGNMENT_EXPRESSION, UNARY_EXPRESSION, ASSIGNMENT_EXPRESSION, RIGHT_SHIFT_ASSIGN, ASSIGNMENT_RIGHT_SHIFT, RuleAssocitiveRight);
    add_binary_rule(ASSIGNMENT_EXPRESSION, UNARY_EXPRESSION, ASSIGNMENT_EXPRESSION, BIT_AND_ASSIGN, ASSIGNMENT_BITWISE_AND, RuleAssocitiveRight);
    add_binary_rule(ASSIGNMENT_EXPRESSION, UNARY_EXPRESSION, ASSIGNMENT_EXPRESSION, BIT_XOR_ASSIGN, ASSIGNMENT_BITWISE_XOR, RuleAssocitiveRight);
    add_binary_rule(ASSIGNMENT_EXPRESSION, UNARY_EXPRESSION, ASSIGNMENT_EXPRESSION, BIT_OR_ASSIGN, ASSIGNMENT_BITWISE_OR, RuleAssocitiveRight);

    parser( NI(EXPRESSION),
        { NI(EXPRESSION), PT(COMMA), NI(ASSIGNMENT_EXPRESSION) },
        [](auto c, auto ts) {
            assert(ts.size() == 3);
            get_ast(expr, EXPRESSION, ASTNodeExprList, 0);
            get_ast(assign_expr, ASSIGNMENT_EXPRESSION, ASTNodeExpr, 2);
            exprast->push_back(assign_exprast);
            return make_shared<NonTermEXPRESSION>(exprast);
        }, RuleAssocitiveLeft);

    parser( NI(EXPRESSION),
        { NI(ASSIGNMENT_EXPRESSION) },
        [](auto c, auto ts) {
            assert(ts.size() == 1);
            get_ast(expr, ASSIGNMENT_EXPRESSION, ASTNodeExpr, 0);
            auto ast = make_shared<ASTNodeExprList>(c);
            return make_shared<NonTermEXPRESSION>(ast);
        });


    parser.dec_priority();
    expr_reduce(CONSTANT_EXPRESSION, EXPRESSION);
}

void CParser::declaration_rules()
{
    DCParser& parser = *this;


    parser( NI(DECLARATION),
        { NI(DECLARATION_SPECIFIERS), ParserChar::beOptional(NI(INIT_DECLARATOR_LIST)), PT(SEMICOLON) },
        [](auto c, auto ts) {
            assert(ts.size() == 3);
            auto ctx = c.lock();
            auto cctx = dynamic_pointer_cast<CParserContext>(ctx);
            auto _p = cctx->cparser();
            get_ast(decl_spec, DECLARATION_SPECIFIERS, ASTNodeDeclarationSpecifier, 0);

            auto init_decl_list_ast = make_shared<ASTNodeInitDeclaratorList>(c);
            if (ts[1]) {
                get_ast(init_decl_listx, INIT_DECLARATOR_LIST, ASTNodeInitDeclaratorList, 1);
                init_decl_list_ast = init_decl_listxast;
            }

            auto ast = make_shared<ASTNodeDeclarationList>(c);

            for (auto decl: *init_decl_list_ast) {
                decl->set_leaf_type(make_shared<ASTNodeVariableTypePlain>(c, decl_specast));
                ast->push_back(decl);

                auto type = decl->type();
                auto spec = type->declspec();
                if (spec->storage_class() == ASTNodeDeclarationSpecifier::StorageClass::SC_Typedef) {
                    auto typedefid = decl->id();
                    assert(typedefid && !typedefid->id.empty());
                    _p->m_typedefs.insert(typedefid->id);
                }
            }

            return make_shared<NonTermDECLARATION>(ast);
        });

    // exchage RHS order
    parser( NI(DECLARATION_SPECIFIERS),
        { ParserChar::beOptional(NI(DECLARATION_SPECIFIERS)), NI(STORAGE_CLASS_SPECIFIER) },
        [](auto c, auto ts) {
            assert(ts.size() == 2);
            get_ast(sspec, STORAGE_CLASS_SPECIFIER, ASTNodeDeclarationSpecifier, 1);
            if (ts[0]) {
                get_ast(ds, DECLARATION_SPECIFIERS, ASTNodeDeclarationSpecifier, 0);
                if (dsast->storage_class() != ASTNodeDeclarationSpecifier::StorageClass::SC_Default) {
                    // TODO warnning or error
                }
                dsast->storage_class() = sspecast->storage_class();
                std::swap(sspecast, dsast);
            }
            return make_shared<NonTermDECLARATION_SPECIFIERS>(sspecast);
        }, RuleAssocitiveLeft);

    // exchage RHS order
    parser( NI(DECLARATION_SPECIFIERS),
        { ParserChar::beOptional(NI(DECLARATION_SPECIFIERS)), NI(TYPE_SPECIFIER) },
        [](auto c, auto ts) {
            assert(ts.size() == 2);
            get_ast(tspec, TYPE_SPECIFIER, ASTNodeTypeSpecifier, 1);
            auto ast = make_shared<ASTNodeDeclarationSpecifier>(c);
            ast->type_specifier() = tspecast;
            if (ts[0]) {
                get_ast(ds, DECLARATION_SPECIFIERS, ASTNodeDeclarationSpecifier, 0);
                auto old_type = dsast->type_specifier();
                if (old_type != nullptr) {
                    switch (old_type->dtype()) {
                    case ASTNodeTypeSpecifier::data_type::STRUCT:
                    case ASTNodeTypeSpecifier::data_type::UNION:
                    case ASTNodeTypeSpecifier::data_type::ENUM:
                    case ASTNodeTypeSpecifier::data_type::VOID:
                    case ASTNodeTypeSpecifier::data_type::TYPEDEF:
                        throw CErrorparserMixedType("mixed type specifier only allowed in integer and float");
                        break;
                    case ASTNodeTypeSpecifier::data_type::INT:
                    case ASTNodeTypeSpecifier::data_type::FLOAT:
                        // TODO mixed type, long signed ..., and warnning or error
                        break;
                    default:
                        assert(false && "unreachable");
                        break;
                    }
                }
                dsast->type_specifier() = tspecast;
                ast = dsast;
            }
            return make_shared<NonTermDECLARATION_SPECIFIERS>(ast);
        }, RuleAssocitiveLeft);

    // exchage RHS order
    parser( NI(DECLARATION_SPECIFIERS),
        { ParserChar::beOptional(NI(DECLARATION_SPECIFIERS)), NI(TYPE_QUALIFIER) },
        [](auto c, auto ts) {
            assert(ts.size() == 2);
            get_ast(tqual, TYPE_QUALIFIER, ASTNodeDeclarationSpecifier, 1);
            if (ts[0]) {
                get_ast(ds, DECLARATION_SPECIFIERS, ASTNodeDeclarationSpecifier, 0);
                dsast->const_ref() = dsast->const_ref() || tqualast->const_ref();;
                dsast->restrict_ref() = dsast->restrict_ref() || tqualast->restrict_ref();;
                dsast->volatile_ref() = dsast->volatile_ref() || tqualast->volatile_ref();;
                std::swap(tqualast, dsast);
            }
            return make_shared<NonTermDECLARATION_SPECIFIERS>(tqualast);
        }, RuleAssocitiveLeft);

    // exchage RHS order
    parser( NI(DECLARATION_SPECIFIERS),
        { ParserChar::beOptional(NI(DECLARATION_SPECIFIERS)), NI(FUNCTION_SPECIFIER) },
        [](auto c, auto ts) {
            assert(ts.size() == 2);
            get_ast(tqual, FUNCTION_SPECIFIER, ASTNodeDeclarationSpecifier, 1);
            if (ts[0]) {
                get_ast(ds, DECLARATION_SPECIFIERS, ASTNodeDeclarationSpecifier, 0);
                dsast->inlined() = tqualast->inlined();;
                std::swap(tqualast, dsast);
            }
            return make_shared<NonTermDECLARATION_SPECIFIERS>(tqualast);
        }, RuleAssocitiveLeft);

    parser( NI(INIT_DECLARATOR_LIST),
        { ParserChar::beOptional(NI(INIT_DECLARATOR_LIST)), NI(INIT_DECLARATOR) },
        [](auto c, auto ts) {
            assert(ts.size() == 2);
            auto ast = make_shared<ASTNodeInitDeclaratorList>(c);
            if (ts[0]) {
                get_ast(init_decl_listx, INIT_DECLARATOR_LIST, ASTNodeInitDeclaratorList, 0);
                ast = init_decl_listxast;
            }
            get_ast(init_decl, INIT_DECLARATOR, ASTNodeInitDeclarator, 1);
            ast->push_back(init_declast);
            return make_shared<NonTermINIT_DECLARATOR_LIST>(ast);
        }, RuleAssocitiveLeft);

    parser( NI(INIT_DECLARATOR),
        { NI(DECLARATOR) },
        [](auto c, auto ts) {
            assert(ts.size() == 1);
            get_ast(decl, DECLARATOR, ASTNodeInitDeclarator, 0);
            return make_shared<NonTermINIT_DECLARATOR>(declast);
        }, RuleAssocitiveRight);

    parser( NI(INIT_DECLARATOR),
        { NI(DECLARATOR), PT(ASSIGN), NI(INITIALIZER) },
        [](auto c, auto ts) {
            assert(ts.size() == 3);
            get_ast(decl, DECLARATOR, ASTNodeInitDeclarator, 0);
            get_ast(init, INITIALIZER, ASTNodeInitializer, 2);
            declast->initializer() = initast;
            return make_shared<NonTermINIT_DECLARATOR>(declast);
        }, RuleAssocitiveRight);

#define to_storage_specifier(kw, en) \
    parser( NI(STORAGE_CLASS_SPECIFIER), \
        { KW(kw) }, \
        [](auto c, auto ts) { \
            assert(ts.size() == 1); \
            auto ast = make_shared<ASTNodeDeclarationSpecifier>(c); \
            ast->storage_class() = ASTNodeDeclarationSpecifier::StorageClass::en; \
            return make_shared<NonTermSTORAGE_CLASS_SPECIFIER>(ast); \
        });
    to_storage_specifier(typedef,  SC_Typedef);
    to_storage_specifier(extern,   SC_Extern);
    to_storage_specifier(static,   SC_Static);
    to_storage_specifier(auto,     SC_Auto);
    to_storage_specifier(register, SC_Register);

#define to_type_specifier(kw, en, ...) \
    parser( NI(TYPE_SPECIFIER), \
        { KW(kw) }, \
        [](auto c, auto ts) { \
            assert(ts.size() == 1); \
            auto ast = make_shared<ASTNodeTypeSpecifier##en>(c, ##__VA_ARGS__); \
            return make_shared<NonTermTYPE_SPECIFIER>(ast); \
        });
    to_type_specifier(void,     Void);
    to_type_specifier(char,     Int,   sizeof(char), false);
    to_type_specifier(short,    Int,   sizeof(short), false);
    to_type_specifier(int,      Int,   sizeof(int), false);
    to_type_specifier(long,     Int,   sizeof(long), false);
    to_type_specifier(float,    Float, sizeof(float));
    to_type_specifier(double,   Float, sizeof(double));
    to_type_specifier(signed,   Int,   sizeof(signed), false);
    to_type_specifier(unsigned, Int,   sizeof(signed), true);
    to_type_specifier(_Bool,    Int,   sizeof(bool), false);
    // TODO _Complex

    parser( NI(TYPE_SPECIFIER),
        { NI(STRUCT_OR_UNION_SPECIFIER) },
        [](auto c, auto ts) {
            assert(ts.size() == 1);
            get_ast(su, STRUCT_OR_UNION_SPECIFIER, ASTNodeTypeSpecifier, 0);
            return make_shared<NonTermTYPE_SPECIFIER>(suast);
        });

    parser( NI(TYPE_SPECIFIER),
        { NI(ENUM_SPECIFIER) },
        [](auto c, auto ts) {
            assert(ts.size() == 1);
            get_ast(es, ENUM_SPECIFIER, ASTNodeTypeSpecifier, 0);
            return make_shared<NonTermTYPE_SPECIFIER>(esast);
        });

    parser( NI(TYPE_SPECIFIER),
        { NI(TYPEDEF_NAME) },
        [](auto c, auto ts) {
            assert(ts.size() == 1);
            get_ast(tn, TYPEDEF_NAME, ASTNodeTypeSpecifier, 0);
            return make_shared<NonTermTYPE_SPECIFIER>(tnast);
        });

struct struct_pesudo: public ASTNode { struct_pesudo(ASTNodeParserContext c): ASTNode(c) {} };
struct union_pesudo:  public ASTNode { union_pesudo (ASTNodeParserContext c): ASTNode(c) {} };

static int anonymous_struct_union_counter = 0;
    parser( NI(STRUCT_OR_UNION_SPECIFIER),
        { NI(STRUCT_OR_UNION), ParserChar::beOptional(TI(ID)), PT(LBRACE), NI(STRUCT_DECLARATION_LIST), PT(RBRACE) },
        [](auto c, auto ts) {
            assert(ts.size() == 5);
            assert(ts[0]);
            auto struct_or_union = dynamic_pointer_cast<NonTermSTRUCT_OR_UNION>(ts[0]);
            assert(struct_or_union && struct_or_union->astnode);
            const auto is_struct = dynamic_pointer_cast<struct_pesudo>(struct_or_union->astnode) == nullptr;

            shared_ptr<TokenID> id = nullptr;
            if (ts[1]) {
                id = dynamic_pointer_cast<TokenID>(ts[1]);
                assert(id);
            } else {
                id = make_shared<TokenID>(
                        "#anonymous_struct_union_" + std::to_string(++anonymous_struct_union_counter),
                        LexerToken::TokenInfo());
            }
            get_ast(dc, STRUCT_OR_UNION_SPECIFIER, ASTNodeStructUnionDeclarationList, 3);
            // TODO add declaration to global context

            shared_ptr<ASTNodeTypeSpecifier> ast = nullptr;
            if (is_struct) {
                ast = make_shared<ASTNodeTypeSpecifierStruct>(c, id);
            } else {
                ast = make_shared<ASTNodeTypeSpecifierUnion>(c, id);
            }
            return make_shared<NonTermSTRUCT_OR_UNION_SPECIFIER>(ast);
        }, RuleAssocitiveRight);

    parser( NI(STRUCT_OR_UNION_SPECIFIER),
        { NI(STRUCT_OR_UNION), TI(ID) },
        [](auto c, auto ts) {
            assert(ts.size() == 2);
            assert(ts[0]);
            auto struct_or_union = dynamic_pointer_cast<NonTermSTRUCT_OR_UNION>(ts[0]);
            assert(struct_or_union && struct_or_union->astnode);
            const auto is_struct = dynamic_pointer_cast<struct_pesudo>(struct_or_union->astnode) == nullptr;

            auto id = dynamic_pointer_cast<TokenID>(ts[1]);
            assert(id);

            shared_ptr<ASTNodeTypeSpecifier> ast = nullptr;
            if (is_struct) {
                ast = make_shared<ASTNodeTypeSpecifierStruct>(c, id);
            } else {
                ast = make_shared<ASTNodeTypeSpecifierUnion>(c, id);
            }
            return make_shared<NonTermSTRUCT_OR_UNION_SPECIFIER>(ast);
        }, RuleAssocitiveRight);

    parser( NI(STRUCT_OR_UNION),
        { KW(struct) },
        [](auto c, auto ts) {
            assert(ts.size() == 1);
            return make_shared<NonTermSTRUCT_OR_UNION>(make_shared<struct_pesudo>(c));
        });

    parser( NI(STRUCT_OR_UNION),
        { KW(union) },
        [](auto c, auto ts) {
            assert(ts.size() == 1);
            return make_shared<NonTermSTRUCT_OR_UNION>(make_shared<union_pesudo>(c));
        });

    parser( NI(STRUCT_DECLARATION_LIST),
        { ParserChar::beOptional(NI(STRUCT_DECLARATION_LIST)), NI(STRUCT_DECLARATION) },
        [](auto c, auto ts) {
            assert(ts.size() == 2);
            auto sdlast = make_shared<ASTNodeStructUnionDeclarationList>(c);
            if (ts[0]) {
                get_ast(xsdl, STRUCT_DECLARATION_LIST, ASTNodeStructUnionDeclarationList, 0);
                sdlast = xsdlast;
            }
            get_ast(sd,  STRUCT_DECLARATION, ASTNodeStructUnionDeclarationList, 1);
            std::copy(sdast->begin(), sdast->end(), sdlast->begin());
            return make_shared<NonTermSTRUCT_DECLARATION_LIST>(sdlast);
        });

    parser( NI(STRUCT_DECLARATION),
        { NI(SPECIFIER_QUALIFIER_LIST), NI(STRUCT_DECLARATOR_LIST), PT(SEMICOLON) },
        [](auto c, auto ts) {
            get_ast(sql, SPECIFIER_QUALIFIER_LIST, ASTNodeDeclarationSpecifier, 0);
            get_ast(sdl, STRUCT_DECLARATOR_LIST,  ASTNodeStructUnionDeclarationList, 1);

            auto vt = make_shared<ASTNodeVariableTypePlain>(c, sqlast);
            for (auto sd: *sdlast)
                sd->set_leaf_type(vt);

            return make_shared<NonTermSTRUCT_DECLARATION>(sdlast);
        });

    // exchange RHS order
    parser( NI(SPECIFIER_QUALIFIER_LIST),
        { ParserChar::beOptional(NI(SPECIFIER_QUALIFIER_LIST)), NI(TYPE_SPECIFIER) },
        [](auto c, auto ts) {
            assert(ts.size() == 2);
            get_ast(tspec, TYPE_SPECIFIER, ASTNodeTypeSpecifier, 1);
            auto ast = make_shared<ASTNodeDeclarationSpecifier>(c);
            ast->type_specifier() = tspecast;
            if (ts[0]) {
                get_ast(ds, SPECIFIER_QUALIFIER_LIST, ASTNodeDeclarationSpecifier, 0);
                if (dsast->type_specifier() != nullptr) {
                    // TODO mixed type, long signed ..., and warnning or error
                }
                dsast->type_specifier() = tspecast;
                ast = dsast;
            }
            return make_shared<NonTermSPECIFIER_QUALIFIER_LIST>(ast);
        }, RuleAssocitiveLeft);

    // exchange RHS order
    parser( NI(SPECIFIER_QUALIFIER_LIST),
        { ParserChar::beOptional(NI(SPECIFIER_QUALIFIER_LIST)), NI(TYPE_QUALIFIER) },
        [](auto c, auto ts) {
            assert(ts.size() == 2);
            get_ast(tqual, TYPE_QUALIFIER, ASTNodeDeclarationSpecifier, 1);
            if (ts[0]) {
                get_ast(ds, SPECIFIER_QUALIFIER_LIST, ASTNodeDeclarationSpecifier, 0);
                dsast->const_ref() = dsast->const_ref() || tqualast->const_ref();;
                dsast->restrict_ref() = dsast->restrict_ref() || tqualast->restrict_ref();;
                dsast->volatile_ref() = dsast->volatile_ref() || tqualast->volatile_ref();;
                std::swap(tqualast, dsast);
            }
            return make_shared<NonTermSPECIFIER_QUALIFIER_LIST>(tqualast);
        }, RuleAssocitiveLeft);

    parser( NI(STRUCT_DECLARATOR_LIST),
        { ParserChar::beOptional(NI(STRUCT_DECLARATOR_LIST)), NI(STRUCT_DECLARATOR) },
        [](auto c, auto ts) {
            assert(ts.size() == 2);
            auto sdlast = make_shared<ASTNodeStructUnionDeclarationList>(c);
            if (ts[0]) {
                get_ast(xsdl, STRUCT_DECLARATOR_LIST, ASTNodeStructUnionDeclarationList, 0);
                sdlast = xsdlast;
            }
            get_ast(sd,  STRUCT_DECLARATOR, ASTNodeStructUnionDeclaration, 1);
            sdlast->push_back(sdast);
            return make_shared<NonTermSTRUCT_DECLARATOR_LIST>(sdlast);
        });

    parser( NI(STRUCT_DECLARATOR),
        { NI(DECLARATOR) },
        [](auto c, auto ts) {
            assert(ts.size() == 1);
            get_ast(d, DECLARATOR, ASTNodeInitDeclarator, 0);
            auto ast = make_shared<ASTNodeStructUnionDeclaration>(c, dast, nullptr);
            return make_shared<NonTermSTRUCT_DECLARATOR>(ast);
        });

    parser( NI(STRUCT_DECLARATOR),
        { ParserChar::beOptional(NI(DECLARATOR)), PT(COLON), NI(CONSTANT_EXPRESSION) },
        [](auto c, auto ts) {
            assert(ts.size() == 3);
            shared_ptr<ASTNodeInitDeclarator> dast = nullptr;
            if (ts[0]) {
                get_ast(xd, DECLARATOR, ASTNodeInitDeclarator, 0);
                dast = xdast;
                assert(dast);
            }
            get_ast(ce, CONSTANT_EXPRESSION, ASTNodeExpr, 2);
            auto ast = make_shared<ASTNodeStructUnionDeclaration>(c, dast, ceast);
            return make_shared<NonTermSTRUCT_DECLARATOR>(ast);
        });

static size_t anonymous_enum_count = 0;
    parser( NI(ENUM_SPECIFIER),
        { KW(enum), ParserChar::beOptional(TI(ID)), PT(LBRACE), NI(ENUMERATOR_LIST), ParserChar::beOptional(PT(COMMA)), PT(RBRACE) },
        [](auto c, auto ts) {
            assert(ts.size() == 6);
            shared_ptr<TokenID> id = nullptr;

            if (ts[1]) {
                id = dynamic_pointer_cast<TokenID>(ts[1]);
                assert(id);
            } else {
                id = make_shared<TokenID>(
                        "anonymous_enum_" + std::to_string(++anonymous_enum_count),
                        LexerToken::TokenInfo());
            }

            get_ast(el, ENUMERATOR_LIST, ASTNodeEnumeratorList, 3);
            // TODO add enum specifier to context

            auto ast = make_shared<ASTNodeTypeSpecifierEnum>(c, id);
            return make_shared<NonTermENUM_SPECIFIER>(ast);
        }, RuleAssocitiveRight);

    parser( NI(ENUM_SPECIFIER),
        { KW(enum), TI(ID) },
        [](auto c, auto ts) {
            assert(ts.size() == 2);
            auto id = dynamic_pointer_cast<TokenID>(ts[1]);
            auto ast = make_shared<ASTNodeTypeSpecifierEnum>(c, id);
            return make_shared<NonTermENUM_SPECIFIER>(ast);
        }, RuleAssocitiveRight);

    parser( NI(ENUMERATOR_LIST),
        { ParserChar::beOptional(NI(ENUMERATOR_LIST)), NI(ENUMERATOR) },
        [] (auto c, auto ts) {
            assert(ts.size() == 2);
            auto elast = make_shared<ASTNodeEnumeratorList>(c);
            if (ts[0]) {
                get_ast(xel, ENUMERATOR_LIST, ASTNodeEnumeratorList, 0);
                elast = xelast;
            }
            get_ast(en, ENUMERATOR, ASTNodeEnumerator, 1);
            elast->push_back(enast);
            return make_shared<NonTermENUMERATOR_LIST>(elast);
        });

    parser( NI(ENUMERATOR),
        { NI(ENUMERATION_CONSTANT) },
        [](auto c, auto ts) {
            assert(ts.size() == 1);
            get_ast(ec, ENUMERATION_CONSTANT, ASTNodeEnumerationConstant, 0);
            auto ast = make_shared<ASTNodeEnumerator>(c, ecast->id(), nullptr);
            return make_shared<NonTermENUMERATOR>(ast);
        }, RuleAssocitiveRight);

    parser( NI(ENUMERATOR),
        { NI(ENUMERATION_CONSTANT), PT(ASSIGN), NI(CONSTANT_EXPRESSION) },
        [](auto c, auto ts) {
            assert(ts.size() == 1);
            get_ast(ec, ENUMERATION_CONSTANT, ASTNodeEnumerationConstant, 0);
            get_ast(constant, CONSTANT_EXPRESSION, ASTNodeExpr, 2);
            auto ast = make_shared<ASTNodeEnumerator>(c, ecast->id(), constantast);
            return make_shared<NonTermENUMERATOR>(ast);
        }, RuleAssocitiveRight);

    parser( NI(ENUMERATION_CONSTANT),
        { TI(ID) },
        [](auto c, auto ts) {
            assert(ts.size() == 1);
            auto id = dynamic_pointer_cast<TokenID>(ts[0]);
            auto ast = make_shared<ASTNodeEnumerationConstant>(c, id);
            return make_shared<NonTermENUMERATION_CONSTANT>(ast);
        });

#define add_to_qualifier(kw) \
    parser( NI(TYPE_QUALIFIER), \
        { KW(kw) }, \
        [](auto c, auto ts) { \
            assert(ts.size() == 1); \
            auto ast = make_shared<ASTNodeDeclarationSpecifier>(c); \
            ast->kw##_ref() = true; \
            return make_shared<NonTermTYPE_QUALIFIER>(ast); \
        })
    add_to_qualifier(const);
    add_to_qualifier(restrict);
    add_to_qualifier(volatile);

#define add_to_function_specifier(kw, kw_ref) \
    parser( NI(FUNCTION_SPECIFIER), \
        { KW(kw) }, \
        [](auto c, auto ts) { \
            assert(ts.size() == 1); \
            auto ast = make_shared<ASTNodeDeclarationSpecifier>(c); \
            ast->kw_ref() = true; \
            return make_shared<NonTermFUNCTION_SPECIFIER>(ast); \
        });
    add_to_function_specifier(inline, inlined);

    parser( NI(DECLARATOR),
        { ParserChar::beOptional(NI(POINTER)), NI(DIRECT_DECLARATOR) },
        [](auto c, auto ts) {
            assert(ts.size() == 2);
            shared_ptr<ASTNodeVariableTypePointer> vtype = nullptr;
            if (ts[0]) {
                get_ast(xv, POINTER, ASTNodeVariableTypePointer, 0);
                vtype = xvast;
            }
            get_ast(dd, DIRECT_DECLARATOR, ASTNodeInitDeclarator, 1);
            if (vtype)
                ddast->set_leaf_type(vtype);

            return make_shared<NonTermDECLARATOR>(ddast);
        }, RuleAssocitiveRight);

    parser( NI(DIRECT_DECLARATOR),
        { TI(ID) },
        [](auto c, auto ts) {
            assert(ts.size() == 1);
            auto id = dynamic_pointer_cast<TokenID>(ts[0]);
            auto ast = make_shared<ASTNodeInitDeclarator>(c, id, nullptr, nullptr);
            return make_shared<NonTermDIRECT_DECLARATOR>(ast);
        });

    parser( NI(DIRECT_DECLARATOR),
        { PT(RPAREN), NI(DECLARATOR), PT(RPAREN) },
        [](auto c, auto ts) {
            assert(ts.size() == 3);
            get_ast(dd, DECLARATOR, ASTNodeInitDeclarator, 1);
            return dynamic_pointer_cast<NonTermDIRECT_DECLARATOR>(ddast);
        }, RuleAssocitiveRight);

    parser( NI(DIRECT_DECLARATOR),
        { NI(DIRECT_DECLARATOR), PT(LBRACKET), ParserChar::beOptional(NI(TYPE_QUALIFIER_LIST)), ParserChar::beOptional(NI(ASSIGNMENT_EXPRESSION)), PT(RBRACKET) },
        [](auto c, auto ts) {
            assert(ts.size() == 5);
            get_ast(dd, DIRECT_DECLARATOR, ASTNodeInitDeclarator, 0);
            get_ast_if_presents(ql, TYPE_QUALIFIER_LIST, ASTNodeDeclarationSpecifier, 2);
            get_ast_if_presents(size, ASSIGNMENT_EXPRESSION, ASTNodeExpr, 3);
            auto array = make_shared<ASTNodeVariableTypeArray>(c, nullptr, sizeast, false);
            if (qlast) {
                array->const_ref() = qlast->const_ref();
                array->restrict_ref() = qlast->restrict_ref();
                array->volatile_ref() = qlast->volatile_ref();
            }
            ddast->set_leaf_type(array);

            return make_shared<NonTermDIRECT_DECLARATOR>(ddast);
        }, RuleAssocitiveRight);

    parser( NI(DIRECT_DECLARATOR),
        { NI(DIRECT_DECLARATOR), PT(LBRACKET), KW(static), ParserChar::beOptional(NI(TYPE_QUALIFIER_LIST)), NI(ASSIGNMENT_EXPRESSION), PT(RBRACKET) },
        [](auto c, auto ts) {
            assert(ts.size() == 6);
            get_ast(dd, DIRECT_DECLARATOR, ASTNodeInitDeclarator, 0);
            get_ast_if_presents(ql, TYPE_QUALIFIER_LIST, ASTNodeDeclarationSpecifier, 3);
            get_ast(size, ASSIGNMENT_EXPRESSION, ASTNodeExpr, 4);
            auto array = make_shared<ASTNodeVariableTypeArray>(c, nullptr, sizeast, true);
            if (qlast) {
                array->const_ref() = qlast->const_ref();
                array->restrict_ref() = qlast->restrict_ref();
                array->volatile_ref() = qlast->volatile_ref();
            }
            ddast->set_leaf_type(array);

            return make_shared<NonTermDIRECT_DECLARATOR>(ddast);
        }, RuleAssocitiveRight);

    parser( NI(DIRECT_DECLARATOR),
        { NI(DIRECT_DECLARATOR), PT(LBRACKET), NI(TYPE_QUALIFIER_LIST), KW(static), NI(ASSIGNMENT_EXPRESSION), PT(RBRACKET) },
        [](auto c, auto ts) {
            assert(ts.size() == 6);
            get_ast(dd, DIRECT_DECLARATOR, ASTNodeInitDeclarator, 0);
            get_ast(ql, TYPE_QUALIFIER_LIST, ASTNodeDeclarationSpecifier, 2);
            get_ast(size, ASSIGNMENT_EXPRESSION, ASTNodeExpr, 4);
            auto array = make_shared<ASTNodeVariableTypeArray>(c, nullptr, sizeast, true);
            if (qlast) {
                array->const_ref() = qlast->const_ref();
                array->restrict_ref() = qlast->restrict_ref();
                array->volatile_ref() = qlast->volatile_ref();
            }
            ddast->set_leaf_type(array);

            return make_shared<NonTermDIRECT_DECLARATOR>(ddast);
        }, RuleAssocitiveRight);

    parser( NI(DIRECT_DECLARATOR),
        { NI(DIRECT_DECLARATOR), PT(LBRACKET), ParserChar::beOptional(NI(TYPE_QUALIFIER_LIST)), PT(MULTIPLY), PT(RBRACKET) },
        [](auto c, auto ts) {
            assert(ts.size() == 6);
            get_ast(dd, DIRECT_DECLARATOR, ASTNodeInitDeclarator, 0);
            get_ast_if_presents(ql, TYPE_QUALIFIER_LIST, ASTNodeDeclarationSpecifier, 2);
            auto array = make_shared<ASTNodeVariableTypeArray>(c, nullptr, nullptr, false);
            array->unspecified_size_vla() = true;
            if (qlast) {
                array->const_ref() = qlast->const_ref();
                array->restrict_ref() = qlast->restrict_ref();
                array->volatile_ref() = qlast->volatile_ref();
            }
            ddast->set_leaf_type(array);

            return make_shared<NonTermDIRECT_DECLARATOR>(ddast);
        }, RuleAssocitiveRight);

    parser( NI(DIRECT_DECLARATOR),
        { NI(DIRECT_DECLARATOR), PT(LPAREN), NI(PARAMETER_TYPE_LIST), PT(RPAREN) },
        [](auto c, auto ts) {
            assert(ts.size() == 4);
            get_ast(dd, DIRECT_DECLARATOR, ASTNodeInitDeclarator, 0);
            get_ast(pl, PARAMETER_TYPE_LIST, ASTNodeParameterDeclarationList, 2);
            auto func = make_shared<ASTNodeVariableTypeFunction>(c, plast, nullptr);
            ddast->set_leaf_type(func);

            return make_shared<NonTermDIRECT_DECLARATOR>(ddast);
        }, RuleAssocitiveRight);

    parser( NI(DIRECT_DECLARATOR),
        { NI(DIRECT_DECLARATOR), PT(LPAREN), ParserChar::beOptional(NI(IDENTIFIER_LIST)), PT(RPAREN) },
        [](auto c, auto ts) {
            assert(ts.size() == 4);
            get_ast(dd, DIRECT_DECLARATOR, ASTNodeInitDeclarator, 0);
            auto plast = make_shared<ASTNodeParameterDeclarationList>(c);
            get_ast_if_presents(xpl, IDENTIFIER_LIST, ASTNodeParameterDeclarationList, 2);
            if (xplast) plast = xplast;
            auto func = make_shared<ASTNodeVariableTypeFunction>(c, plast, nullptr);
            ddast->set_leaf_type(func);

            return make_shared<NonTermDIRECT_DECLARATOR>(ddast);
        }, RuleAssocitiveRight);

    // exchange RHS order
    parser( NI(POINTER),
        {  ParserChar::beOptional(NI(POINTER)), PT(MULTIPLY), ParserChar::beOptional(NI(TYPE_QUALIFIER_LIST)) },
        [](auto c, auto ts) {
            assert(ts.size() == 3);
            auto ptr = make_shared<ASTNodeVariableTypePointer>(c, nullptr);
            get_ast_if_presents(ql, TYPE_QUALIFIER_LIST, ASTNodeDeclarationSpecifier, 2);
            get_ast_if_presents(optr, POINTER, ASTNodeVariableTypePointer, 0);
            if (qlast) {
                ptr->const_ref() = qlast->const_ref();
                ptr->restrict_ref() = qlast->restrict_ref();
                ptr->volatile_ref() = qlast->volatile_ref();
            }
            if (optrast)
                ptr->set_leaf_type(optrast);

            return make_shared<NonTermPOINTER>(ptr);
        }, RuleAssocitiveRight);

    parser( NI(TYPE_QUALIFIER_LIST),
        { ParserChar::beOptional(NI(TYPE_QUALIFIER_LIST)), NI(TYPE_QUALIFIER) },
        [](auto c, auto ts) {
            assert(ts.size() == 2);

            auto ql = make_shared<ASTNodeDeclarationSpecifier>(c);
            get_ast_if_presents(xql, TYPE_QUALIFIER_LIST, ASTNodeDeclarationSpecifier, 0);
            get_ast(q, TYPE_QUALIFIER, ASTNodeDeclarationSpecifier, 1);

            if (xql) ql = xqlast;

            ql->const_ref() = ql->const_ref() || qast->const_ref();
            ql->restrict_ref() = ql->restrict_ref() || qast->restrict_ref();
            ql->volatile_ref() = ql->volatile_ref() || qast->volatile_ref();

            return make_shared<NonTermTYPE_QUALIFIER_LIST>(ql);
        }, RuleAssocitiveRight);

    parser( NI(PARAMETER_TYPE_LIST),
        { NI(PARAMETER_LIST) },
        [](auto c, auto ts) {
            assert(ts.size() == 1);
            get_ast(pl, PARAMETER_LIST, ASTNodeParameterDeclarationList, 0);
            return make_shared<NonTermPARAMETER_TYPE_LIST>(plast);
        });

    parser( NI(PARAMETER_TYPE_LIST),
        { NI(PARAMETER_LIST), PT(COMMA), PT(DOTS) },
        [](auto c, auto ts) {
            assert(ts.size() == 3);
            get_ast(pl, PARAMETER_LIST, ASTNodeParameterDeclarationList, 0);
            plast->variadic() = true;
            return make_shared<NonTermPARAMETER_TYPE_LIST>(plast);
        });

    parser( NI(PARAMETER_LIST),
        { NI(PARAMETER_DECLARATION) },
        [](auto c, auto ts) {
            assert(ts.size() == 1);
            auto pl = make_shared<ASTNodeParameterDeclarationList>(c);
            get_ast(pd, PARAMETER_DECLARATION, ASTNodeParameterDeclaration, 0);
            pl->push_back(pdast);
            return make_shared<NonTermPARAMETER_LIST>(pl);
        });

    parser( NI(PARAMETER_LIST),
        { NI(PARAMETER_LIST), PT(COMMA), NI(PARAMETER_DECLARATION) },
        [](auto c, auto ts) {
            assert(ts.size() == 3);
            get_ast(pl, PARAMETER_LIST, ASTNodeParameterDeclarationList, 0);
            get_ast(pd, PARAMETER_DECLARATION, ASTNodeParameterDeclaration, 2);
            plast->push_back(pdast);
            return make_shared<NonTermPARAMETER_LIST>(plast);
        });

    parser( NI(PARAMETER_DECLARATION),
        { NI(DECLARATION_SPECIFIERS), NI(DECLARATOR) },
        [](auto c, auto ts) {
            assert(ts.size() == 2);
            get_ast(ds, DECLARATION_SPECIFIERS, ASTNodeDeclarationSpecifier, 0);
            get_ast(d, DECLARATOR, ASTNodeInitDeclarator, 1);
            auto pd = make_shared<ASTNodeVariableTypePlain>(c, dsast);
            dast->set_leaf_type(pd);
            auto ast = make_shared<ASTNodeParameterDeclaration>(c, dast->id(), dast->type());
            return make_shared<NonTermPARAMETER_DECLARATION>(ast);
        });

    parser( NI(PARAMETER_DECLARATION),
        { NI(DECLARATION_SPECIFIERS), ParserChar::beOptional(NI(ABSTRACT_DECLARATOR)) },
        [](auto c, auto ts) {
            assert(ts.size() == 2);
            get_ast(ds, DECLARATION_SPECIFIERS, ASTNodeDeclarationSpecifier, 0);
            get_ast_if_presents(d, ABSTRACT_DECLARATOR, ASTNodeInitDeclarator, 1);
            shared_ptr<ASTNodeVariableType> pd;
            pd = make_shared<ASTNodeVariableTypePlain>(c, dsast);
            if (dast) {
                dast->set_leaf_type(pd);
                pd = dast->type();
            }
            auto ast = make_shared<ASTNodeParameterDeclaration>(c, nullptr, pd);
            return make_shared<NonTermPARAMETER_DECLARATION>(ast);
        });

    parser( NI(IDENTIFIER_LIST),
        { TI(ID) },
        [](auto c, auto ts) {
            assert(ts.size() == 1);
            auto il = make_shared<ASTNodeParameterDeclarationList>(c);
            auto id = dynamic_pointer_cast<TokenID>(ts[0]);
            assert(id);
            il->push_back(make_shared<ASTNodeParameterDeclaration>(c, id, nullptr));
            return make_shared<NonTermIDENTIFIER_LIST>(il);
        });

    parser( NI(IDENTIFIER_LIST),
        { NI(IDENTIFIER_LIST), PT(COMMA), TI(ID) },
        [](auto c, auto ts) {
            assert(ts.size() == 3);
            get_ast(il, IDENTIFIER_LIST, ASTNodeParameterDeclarationList, 0);
            auto id = dynamic_pointer_cast<TokenID>(ts[2]);
            assert(id);
            ilast->push_back(make_shared<ASTNodeParameterDeclaration>(c, id, nullptr));
            return make_shared<NonTermIDENTIFIER_LIST>(ilast);
        });

    parser( NI(TYPE_NAME),
        { NI(SPECIFIER_QUALIFIER_LIST), ParserChar::beOptional(NI(ABSTRACT_DECLARATOR)) },
        [](auto c, auto ts) {
            assert(ts.size() == 2);
            get_ast(sq, SPECIFIER_QUALIFIER_LIST, ASTNodeDeclarationSpecifier, 0);
            get_ast_if_presents(d, ABSTRACT_DECLARATOR, ASTNodeInitDeclarator, 1);

            shared_ptr<ASTNodeVariableType> kt;
            kt = make_shared<ASTNodeVariableTypePlain>(c, sqast);
            if (dast) {
                dast->set_leaf_type(kt);
                kt = dast->type();
            }

            return make_shared<NonTermTYPE_NAME>(kt);
        }, RuleAssocitiveRight);

    parser( NI(ABSTRACT_DECLARATOR),
        { NI(POINTER) },
        [](auto c, auto ts) {
            assert(ts.size() == 1);
            get_ast(sq, POINTER, ASTNodeVariableTypePointer, 0);
            auto ast = make_shared<ASTNodeInitDeclarator>(c, nullptr, sqast, nullptr);
            return make_shared<NonTermABSTRACT_DECLARATOR>(ast);
        }, RuleAssocitiveRight);

    parser( NI(ABSTRACT_DECLARATOR),
        { ParserChar::beOptional(NI(POINTER)), NI(DIRECT_ABSTRACT_DECLARATOR) },
        [](auto c, auto ts) {
            assert(ts.size() == 2);
            get_ast_if_presents(sq, POINTER, ASTNodeVariableTypePointer, 0);
            get_ast(d, DIRECT_ABSTRACT_DECLARATOR, ASTNodeInitDeclarator, 1);

            if (sqast) dast->set_leaf_type(sqast);

            return make_shared<NonTermABSTRACT_DECLARATOR>(dast);
        }, RuleAssocitiveRight);

    parser( NI(DIRECT_ABSTRACT_DECLARATOR),
        { PT(LPAREN), NI(ABSTRACT_DECLARATOR), PT(RPAREN) },
        [](auto c, auto ts) {
            assert(ts.size() == 3);
            get_ast(d, ABSTRACT_DECLARATOR, ASTNodeInitDeclarator, 1);
            auto ast = make_shared<ASTNodeInitDeclarator>(c, nullptr, dast->type(), nullptr);
            return make_shared<NonTermDIRECT_ABSTRACT_DECLARATOR>(ast);
        });

    parser( NI(DIRECT_ABSTRACT_DECLARATOR),
        { ParserChar::beOptional(NI(DIRECT_ABSTRACT_DECLARATOR)), PT(LBRACKET), ParserChar::beOptional(NI(TYPE_QUALIFIER_LIST)), ParserChar::beOptional(NI(ASSIGNMENT_EXPRESSION)), PT(RBRACKET) },
        [](auto c, auto ts) {
            assert(ts.size() == 5);
            get_ast_if_presents(dd, DIRECT_ABSTRACT_DECLARATOR, ASTNodeInitDeclarator, 0);
            get_ast_if_presents(ql, TYPE_QUALIFIER_LIST, ASTNodeDeclarationSpecifier, 2);
            get_ast_if_presents(size, ASSIGNMENT_EXPRESSION, ASTNodeExpr, 3);
            auto array = make_shared<ASTNodeVariableTypeArray>(c, nullptr, sizeast, false);
            if (qlast) {
                array->const_ref() = qlast->const_ref();
                array->restrict_ref() = qlast->restrict_ref();
                array->volatile_ref() = qlast->volatile_ref();
            }

            if (!ddast) ddast = make_shared<ASTNodeInitDeclarator>(c, nullptr, nullptr, nullptr);
            ddast->set_leaf_type(array);

            return make_shared<NonTermDIRECT_ABSTRACT_DECLARATOR>(ddast);
        }, RuleAssocitiveRight);

    parser( NI(DIRECT_ABSTRACT_DECLARATOR),
        { ParserChar::beOptional(NI(DIRECT_ABSTRACT_DECLARATOR)), PT(LBRACKET), KW(static), ParserChar::beOptional(NI(TYPE_QUALIFIER_LIST)), NI(ASSIGNMENT_EXPRESSION), PT(RBRACKET) },
        [](auto c, auto ts) {
            assert(ts.size() == 6);
            get_ast_if_presents(dd, DIRECT_ABSTRACT_DECLARATOR, ASTNodeInitDeclarator, 0);
            get_ast_if_presents(ql, TYPE_QUALIFIER_LIST, ASTNodeDeclarationSpecifier, 3);
            get_ast(size, ASSIGNMENT_EXPRESSION, ASTNodeExpr, 4);
            auto array = make_shared<ASTNodeVariableTypeArray>(c, nullptr, sizeast, true);
            if (qlast) {
                array->const_ref() = qlast->const_ref();
                array->restrict_ref() = qlast->restrict_ref();
                array->volatile_ref() = qlast->volatile_ref();
            }

            if (!ddast) ddast = make_shared<ASTNodeInitDeclarator>(c, nullptr, nullptr, nullptr);
            ddast->set_leaf_type(array);

            return make_shared<NonTermDIRECT_ABSTRACT_DECLARATOR>(ddast);
        }, RuleAssocitiveRight);

    parser( NI(DIRECT_ABSTRACT_DECLARATOR),
        { ParserChar::beOptional(NI(DIRECT_ABSTRACT_DECLARATOR)), PT(LBRACKET), NI(TYPE_QUALIFIER_LIST), KW(static), NI(ASSIGNMENT_EXPRESSION), PT(RBRACKET) },
        [](auto c, auto ts) {
            assert(ts.size() == 6);
            get_ast_if_presents(dd, DIRECT_ABSTRACT_DECLARATOR, ASTNodeInitDeclarator, 0);
            get_ast(ql, TYPE_QUALIFIER_LIST, ASTNodeDeclarationSpecifier, 2);
            get_ast(size, ASSIGNMENT_EXPRESSION, ASTNodeExpr, 4);
            auto array = make_shared<ASTNodeVariableTypeArray>(c, nullptr, sizeast, true);
            if (qlast) {
                array->const_ref() = qlast->const_ref();
                array->restrict_ref() = qlast->restrict_ref();
                array->volatile_ref() = qlast->volatile_ref();
            }

            if (!ddast) ddast = make_shared<ASTNodeInitDeclarator>(c, nullptr, nullptr, nullptr);
            ddast->set_leaf_type(array);

            return make_shared<NonTermDIRECT_ABSTRACT_DECLARATOR>(ddast);
        }, RuleAssocitiveRight);

    parser( NI(DIRECT_ABSTRACT_DECLARATOR),
        { ParserChar::beOptional(NI(DIRECT_ABSTRACT_DECLARATOR)), PT(LBRACKET), ParserChar::beOptional(NI(TYPE_QUALIFIER_LIST)), PT(MULTIPLY), PT(RBRACKET) },
        [](auto c, auto ts) {
            assert(ts.size() == 6);
            get_ast_if_presents(dd, DIRECT_ABSTRACT_DECLARATOR, ASTNodeInitDeclarator, 0);
            get_ast_if_presents(ql, TYPE_QUALIFIER_LIST, ASTNodeDeclarationSpecifier, 2);
            auto array = make_shared<ASTNodeVariableTypeArray>(c, nullptr, nullptr, false);
            array->unspecified_size_vla() = true;
            if (qlast) {
                array->const_ref() = qlast->const_ref();
                array->restrict_ref() = qlast->restrict_ref();
                array->volatile_ref() = qlast->volatile_ref();
            }

            if (!ddast) ddast = make_shared<ASTNodeInitDeclarator>(c, nullptr, nullptr, nullptr);
            ddast->set_leaf_type(array);

            return make_shared<NonTermDIRECT_ABSTRACT_DECLARATOR>(ddast);
        }, RuleAssocitiveRight);

    parser( NI(DIRECT_ABSTRACT_DECLARATOR),
        { ParserChar::beOptional(NI(DIRECT_ABSTRACT_DECLARATOR)), PT(LPAREN), ParserChar::beOptional(NI(PARAMETER_TYPE_LIST)), PT(RPAREN) },
        [](auto c, auto ts) {
            assert(ts.size() == 4);
            get_ast_if_presents(dd, DIRECT_ABSTRACT_DECLARATOR, ASTNodeInitDeclarator, 0);
            get_ast_if_presents(pl, PARAMETER_TYPE_LIST, ASTNodeParameterDeclarationList, 2);

            if (!plast) plast = make_shared<ASTNodeParameterDeclarationList>(c);

            auto func = make_shared<ASTNodeVariableTypeFunction>(c, plast, nullptr);
            ddast->set_leaf_type(func);

            return make_shared<NonTermDIRECT_ABSTRACT_DECLARATOR>(ddast);
        }, RuleAssocitiveRight);

    // TYPEDEF_NAME typedef_rule()

    parser( NI(INITIALIZER),
        { NI(ASSIGNMENT_EXPRESSION) },
        [](auto c, auto ts) {
            assert(ts.size() == 1);
            get_ast(expr, ASSIGNMENT_EXPRESSION, ASTNodeExpr, 0);
            return make_shared<NonTermINITIALIZER>(make_shared<ASTNodeInitializer>(c, exprast));
        });

    parser( NI(INITIALIZER),
        { PT(LBRACE), NI(INITIALIZER_LIST), ParserChar::beOptional(PT(COMMA)), PT(RBRACE) },
        [](auto c, auto ts) {
            assert(ts.size() == 4);
            get_ast(il, INITIALIZER_LIST, ASTNodeInitializerList, 1);
            return make_shared<NonTermINITIALIZER>(make_shared<ASTNodeInitializer>(c, ilast));
        });

    parser( NI(INITIALIZER_LIST),
        { ParserChar::beOptional(NI(INITIALIZER_LIST)), ParserChar::beOptional(NI(DESIGNATION)), NI(INITIALIZER) },
        [](auto c, auto ts) {
            assert(ts.size() == 3);
            get_ast_if_presents(il, INITIALIZER_LIST, ASTNodeInitializerList, 0);
            get_ast_if_presents(ds, DESIGNATION, ASTNodeDesignation, 1);
            get_ast(init, INITIALIZER, ASTNodeInitializer, 2);

            auto ax = make_shared<ASTNodeInitializerList>(c);
            if (ilast) ax = ilast;
            auto dx = make_shared<ASTNodeDesignation>(c);
            if (dsast) dx = dsast;
            dx->initializer() = initast;
            ax->push_back(dx);

            return make_shared<NonTermINITIALIZER_LIST>(ax);
        });

    parser( NI(DESIGNATION),
        { NI(DESIGNATOR_LIST), PT(ASSIGN) },
        [](auto c, auto ts) {
            assert(ts.size() == 2);
            get_ast(dl, DESIGNATOR_LIST, ASTNodeDesignation, 0);
            return make_shared<NonTermDESIGNATION>(dlast);
        });

    parser( NI(DESIGNATOR_LIST),
        { ParserChar::beOptional(NI(DESIGNATOR_LIST)), NI(DESIGNATOR) },
        [](auto c, auto ts) {
            assert(ts.size() == 2);
            get_ast_if_presents(dl, DESIGNATOR_LIST, ASTNodeDesignation, 0);
            get_ast(ds, DESIGNATOR, ASTNodeDesignation, 1);

            auto ax = make_shared<ASTNodeDesignation>(c);
            if (dlast) ax = dlast;

            assert(dsast->designators().size() == 1);
            for (auto d: dsast->designators())
                ax->add_designator(d);

            return make_shared<NonTermDESIGNATOR_LIST>(ax);
        });

    parser( NI(DESIGNATOR),
        { PT(LBRACKET), NI(CONSTANT_EXPRESSION), PT(RBRACKET) },
        [](auto c, auto ts) {
            assert(ts.size() == 3);
            get_ast(ce, CONSTANT_EXPRESSION, ASTNodeExpr, 1);

            auto ax = make_shared<ASTNodeDesignation>(c, ceast);
            return make_shared<NonTermDESIGNATOR>(ax);
        });

    parser( NI(DESIGNATOR),
        { PT(DOT), TI(ID) },
        [](auto c, auto ts) {
            assert(ts.size() == 2);
            auto id = dynamic_pointer_cast<TokenID>(ts[1]);
            assert(id);

            auto ax = make_shared<ASTNodeDesignation>(c, id);
            return make_shared<NonTermDESIGNATOR>(ax);
        });
}

void CParser::statement_rules()
{
    DCParser& parser = *this;


#define to_statement(xt) \
    parser( NI(STATEMENT), \
        { NI(xt) }, \
        [](auto c, auto ts) { \
            assert(ts.size() == 1); \
            get_ast(st, LABELED_STATEMENT, ASTNodeStat, 0); \
            return make_shared<NonTermSTATEMENT>(stast); \
        })

    to_statement(LABELED_STATEMENT);
    to_statement(COMPOUND_STATEMENT);
    to_statement(EXPRESSION_STATEMENT);
    to_statement(SELECTION_STATEMENT);
    to_statement(ITERATION_STATEMENT);
    to_statement(JUMP_STATEMENT);

    parser( NI(LABELED_STATEMENT),
        { TI(ID), PT(COLON), NI(STATEMENT) },
        [](auto c, auto ts) {
            assert(ts.size() == 3);
            auto id = dynamic_pointer_cast<TokenID>(ts[0]);
            assert(id);
            get_ast(st, STATEMENT, ASTNodeStat, 2);
            return make_shared<NonTermLABELED_STATEMENT>(make_shared<ASTNodeStatLabel>(c, id, stast));
        });

    parser( NI(LABELED_STATEMENT),
        { KW(case), NI(CONSTANT_EXPRESSION), PT(COLON), NI(STATEMENT) },
        [](auto c, auto ts) {
            assert(ts.size() == 4);
            get_ast(ce, CONSTANT_EXPRESSION, ASTNodeExpr, 1);
            get_ast(st, STATEMENT, ASTNodeStat, 3);
            return make_shared<NonTermLABELED_STATEMENT>(make_shared<ASTNodeStatCase>(c, ceast, stast));
        });

    parser( NI(LABELED_STATEMENT),
        { KW(default), PT(COLON), NI(STATEMENT) },
        [](auto c, auto ts) {
            assert(ts.size() == 3);
            get_ast(st, STATEMENT, ASTNodeStat, 2);
            return make_shared<NonTermLABELED_STATEMENT>(make_shared<ASTNodeStatDefault>(c, stast));
        });

    parser( NI(COMPOUND_STATEMENT),
        { PT(LBRACE), ParserChar::beOptional(NI(BLOCK_ITEM_LIST)), PT(RBRACE) },
        [](auto c, auto ts) {
            assert(ts.size() == 3);

            auto list = make_shared<ASTNodeBlockItemList>(c);
            get_ast_if_presents(bl, BLOCK_ITEM_LIST, ASTNodeBlockItemList, 1);
            if (blast) list = blast;

            auto ast = make_shared<ASTNodeStatCompound>(c, list);
            return make_shared<NonTermCOMPOUND_STATEMENT>(ast);
        });

    parser( NI(BLOCK_ITEM_LIST),
        { ParserChar::beOptional(NI(BLOCK_ITEM_LIST)), NI(BLOCK_ITEM) },
        [](auto c, auto ts) {
            assert(ts.size() == 2);

            auto list = make_shared<ASTNodeBlockItemList>(c);
            get_ast_if_presents(bl, BLOCK_ITEM_LIST, ASTNodeBlockItemList, 0);
            if (blast) list = blast;
            get_ast(bi, BLOCK_ITEM, ASTNodeBlockItem, 1);

            list->push_back(biast);
            return make_shared<NonTermBLOCK_ITEM_LIST>(list);
        });

    parser( NI(BLOCK_ITEM),
        { NI(DECLARATION) },
        [](auto c, auto ts) {
            assert(ts.size() == 2);
            get_ast(d, DECLARATION, ASTNodeBlockItem, 0);
            return make_shared<NonTermBLOCK_ITEM>(dast);
        });

    parser( NI(BLOCK_ITEM),
        { NI(STATEMENT) },
        [](auto c, auto ts) {
            assert(ts.size() == 2);
            get_ast(st, STATEMENT, ASTNodeBlockItem, 0);
            return make_shared<NonTermBLOCK_ITEM>(stast);
        });

    parser( NI(EXPRESSION_STATEMENT),
        { ParserChar::beOptional(NI(EXPRESSION)), PT(SEMICOLON) },
        [](auto c, auto ts) {
            assert(ts.size() == 2);
            get_ast_if_presents(ex, EXPRESSION, ASTNodeExpr, 0);

            shared_ptr<ASTNodeExpr> expr = nullptr;
            if (exast) expr = exast;

            auto ast = make_shared<ASTNodeStatExpr>(c, expr);
            return make_shared<NonTermEXPRESSION_STATEMENT>(ast);
        });

    parser( NI(SELECTION_STATEMENT),
        { KW(if), PT(LPAREN), NI(EXPRESSION), PT(RPAREN), NI(STATEMENT), KW(else), NI(STATEMENT) },
        [](auto c, auto ts) {
            assert(ts.size() == 7);
            get_ast(cond, EXPRESSION, ASTNodeExpr, 2);
            get_ast(truest, STATEMENT, ASTNodeStat, 4);
            get_ast(falsest, STATEMENT, ASTNodeStat, 6);

            auto ast = make_shared<ASTNodeStatIF>(c, condast, truestast, falsestast);
            return make_shared<NonTermSELECTION_STATEMENT>(ast);
        }, RuleAssocitiveRight);

    parser( NI(SELECTION_STATEMENT),
        { KW(if), PT(LPAREN), NI(EXPRESSION), PT(RPAREN), NI(STATEMENT) },
        [](auto c, auto ts) {
            assert(ts.size() == 5);
            get_ast(cond, EXPRESSION, ASTNodeExpr, 2);
            get_ast(truest, STATEMENT, ASTNodeStat, 4);

            auto ast = make_shared<ASTNodeStatIF>(c, condast, truestast, nullptr);
            return make_shared<NonTermSELECTION_STATEMENT>(ast);
        }, RuleAssocitiveRight);

    parser( NI(SELECTION_STATEMENT),
        { KW(switch), PT(LPAREN), NI(EXPRESSION), PT(RPAREN), NI(STATEMENT) },
        [](auto c, auto ts) {
            assert(ts.size() == 5);
            get_ast(cond, EXPRESSION, ASTNodeExpr, 2);
            get_ast(st, STATEMENT, ASTNodeStat, 4);

            auto ast = make_shared<ASTNodeStatSwitch>(c, condast, stast);
            return make_shared<NonTermSELECTION_STATEMENT>(ast);
        });

    parser( NI(ITERATION_STATEMENT),
        { KW(while), PT(LPAREN), NI(EXPRESSION), PT(RPAREN), NI(STATEMENT) },
        [](auto c, auto ts) {
            assert(ts.size() == 5);
            get_ast(cond, EXPRESSION, ASTNodeExpr, 2);
            get_ast(st, STATEMENT, ASTNodeStat, 4);

            auto ast = make_shared<ASTNodeStatFor>(c, nullptr, condast, nullptr, stast);
            return make_shared<NonTermITERATION_STATEMENT>(ast);
        });

    parser( NI(ITERATION_STATEMENT),
        { KW(do), NI(STATEMENT), KW(while), PT(LPAREN), NI(EXPRESSION), PT(RPAREN), PT(SEMICOLON) },
        [](auto c, auto ts) {
            assert(ts.size() == 7);
            get_ast(st, STATEMENT, ASTNodeStat, 1);
            get_ast(cond, EXPRESSION, ASTNodeExpr, 4);

            auto ast = make_shared<ASTNodeStatDoWhile>(c, condast, stast);
            return make_shared<NonTermITERATION_STATEMENT>(ast);
        });

    parser( NI(ITERATION_STATEMENT),
        { KW(for), PT(LPAREN), 
                   ParserChar::beOptional(NI(EXPRESSION)), PT(SEMICOLON), 
                   ParserChar::beOptional(NI(EXPRESSION)), PT(SEMICOLON), 
                   ParserChar::beOptional(NI(EXPRESSION)), PT(RPAREN), NI(STATEMENT) },
        [](auto c, auto ts) {
            assert(ts.size() == 9);
            get_ast_if_presents(init, EXPRESSION, ASTNodeExpr, 2);
            get_ast_if_presents(cond, EXPRESSION, ASTNodeExpr, 4);
            get_ast_if_presents(iter, EXPRESSION, ASTNodeExpr, 6);
            get_ast(st, STATEMENT, ASTNodeStat, 8);

            auto ast = make_shared<ASTNodeStatFor>(c, initast, condast, iterast, stast);
            return make_shared<NonTermITERATION_STATEMENT>(ast);
        });

    parser( NI(ITERATION_STATEMENT),
        { KW(for), PT(LPAREN),
                   NI(DECLARATION),
                   ParserChar::beOptional(NI(EXPRESSION)), PT(SEMICOLON), 
                   ParserChar::beOptional(NI(EXPRESSION)), PT(RPAREN), NI(STATEMENT) },
        [](auto c, auto ts) {
            assert(ts.size() == 9);
            get_ast(decl, DECLARATION, ASTNodeDeclarationList, 2);
            get_ast_if_presents(cond, EXPRESSION, ASTNodeExpr, 4);
            get_ast_if_presents(iter, EXPRESSION, ASTNodeExpr, 6);
            get_ast(st, STATEMENT, ASTNodeStat, 8);

            auto ast = make_shared<ASTNodeStatForDecl>(c, declast, condast, iterast, stast);
            return make_shared<NonTermITERATION_STATEMENT>(ast);
        });

    parser( NI(JUMP_STATEMENT),
        { KW(goto), TI(ID), PT(SEMICOLON) },
        [](auto c, auto ts) {
            assert(ts.size() == 3);
            auto id = dynamic_pointer_cast<TokenID>(ts[1]);
            assert(id);
            auto ast = make_shared<ASTNodeStatGoto>(c, id);
            return make_shared<NonTermJUMP_STATEMENT>(ast);
        });

    parser( NI(JUMP_STATEMENT),
        { KW(continue), PT(SEMICOLON) },
        [](auto c, auto ts) {
            assert(ts.size() == 2);
            auto ast = make_shared<ASTNodeStatContinue>(c);
            return make_shared<NonTermJUMP_STATEMENT>(ast);
        });

    parser( NI(JUMP_STATEMENT),
        { KW(break), PT(SEMICOLON) },
        [](auto c, auto ts) {
            assert(ts.size() == 2);
            auto ast = make_shared<ASTNodeStatBreak>(c);
            return make_shared<NonTermJUMP_STATEMENT>(ast);
        });

    parser( NI(JUMP_STATEMENT),
        { KW(return), ParserChar::beOptional(NI(EXPRESSION)), PT(SEMICOLON) },
        [](auto c, auto ts) {
            assert(ts.size() == 3);
            get_ast_if_presents(expr, EXPRESSION, ASTNodeExpr, 1);
            auto ast = make_shared<ASTNodeStatReturn>(c, exprast);
            return make_shared<NonTermJUMP_STATEMENT>(ast);
        });
}

void CParser::external_definitions()
{
    auto& parser = *this;


    parser( NI(TRANSLATION_UNIT),
        { ParserChar::beOptional(NI(TRANSLATION_UNIT)), NI(EXTERNAL_DECLARATION) },
        [](auto c, auto ts) {
            assert(ts.size() == 2);
            get_ast_if_presents(prev, TRANSLATION_UNIT, ASTNodeTranslationUnit, 0);
            get_ast(cur, EXTERNAL_DECLARATION, ASTNodeExternalDeclaration, 1);

            if (prevast == nullptr)
                prevast = make_shared<ASTNodeTranslationUnit>(c);

            prevast->push_back(curast);
            return make_shared<NonTermTRANSLATION_UNIT>(prevast);
        });

    parser( NI(EXTERNAL_DECLARATION),
        { NI(FUNCTION_DEFINITION) },
        [](auto c, auto ts) {
            assert(ts.size() == 1);
            get_ast(cur, FUNCTION_DEFINITION, ASTNodeFunctionDefinition, 0);
            return make_shared<NonTermEXTERNAL_DECLARATION>(curast);
        });

    parser( NI(EXTERNAL_DECLARATION),
        { NI(DECLARATION) },
        [](auto c, auto ts) {
            assert(ts.size() == 1);
            get_ast(cur, DECLARATION, ASTNodeDeclarationList, 0);
            return make_shared<NonTermEXTERNAL_DECLARATION>(curast);
        });

    parser( NI(FUNCTION_DEFINITION),
        { NI(DECLARATION_SPECIFIERS), NI(DECLARATOR), ParserChar::beOptional(NI(DECLARATION_LIST)), NI(COMPOUND_STATEMENT) },
        [](auto c, auto ts) {
            assert(ts.size() == 4);
            get_ast(spec, DECLARATION_SPECIFIERS, ASTNodeDeclarationSpecifier, 0);
            get_ast(decl, DECLARATOR, ASTNodeInitDeclarator, 1);
            declast->set_leaf_type(make_shared<ASTNodeVariableTypePlain>(c, specast));

            auto functype = dynamic_pointer_cast<ASTNodeVariableTypeFunction>(declast->type());
            assert(functype);
            get_ast(st, COMPOUND_STATEMENT, ASTNodeStatCompound, 3);

            get_ast_if_presents(decltx, DECLARATION_LIST, ASTNodeDeclarationList, 2);
            if (decltxast != nullptr) {
                // TODO old style C parameter declaration
            }

            auto ast = make_shared<ASTNodeFunctionDefinition>(c, functype, stast);
            return make_shared<NonTermFUNCTION_DEFINITION>(ast);
        });

    parser( NI(DECLARATION_LIST),
        { ParserChar::beOptional(NI(DECLARATION_LIST)), NI(DECLARATION) },
        [](auto c, auto ts) {
            assert(ts.size() == 2);
            get_ast_if_presents(dll, DECLARATION_LIST, ASTNodeDeclarationList, 0);
            get_ast(dl, DECLARATION, ASTNodeDeclarationList, 1);

            if (dllast == nullptr)
                dllast = make_shared<ASTNodeDeclarationList>(c);

            for (auto& d : *dlast)
                dllast->push_back(d);

            return make_shared<NonTermDECLARATION_LIST>(dllast);
        });
}


CParser::CParser()
{
    this->setContext(make_shared<CParserContext>(this));

    this->external_definitions();
    this->__________();
    this->statement_rules  ();
    this->__________();
    this->typedef_rule();
    this->__________();
    this->declaration_rules();
    this->__________();
    this->expression_rules();

    this->add_start_symbol(NI(TRANSLATION_UNIT).id);

    this->generate_table();
}

shared_ptr<ASTNodeTranslationUnit> CParser::get_translation_unit(shared_ptr<NonTerminal> node)
{
    auto unit = dynamic_pointer_cast<NonTermTRANSLATION_UNIT>(node);
    assert(unit);
    auto nunit = dynamic_pointer_cast<ASTNodeTranslationUnit>(unit->astnode);
    assert(nunit);
    return nunit;
}

}
