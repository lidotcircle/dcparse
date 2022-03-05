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
    TENTRY(STRUCT_OR_UNION) \
    TENTRY(STRUCT_DECLARATION_LIST) \
    TENTRY(STRUCT_DECLARATION) \
    TENTRY(SPECIFIER_QUALIFIER_LIST) \
    TENTRY(STRUCT_DECLARATOR_LIST) \
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


#define expr_reduce(to, from) \
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
    add_prefix_unary_rule(UNARY_EXPRESSION, UNARY_EXPRESSION, PLUSPLUS, REFERENCE, RuleAssocitiveRight);
    add_prefix_unary_rule(UNARY_EXPRESSION, UNARY_EXPRESSION, PLUSPLUS, REFERENCE, RuleAssocitiveRight);

    add_prefix_unary_rule(UNARY_EXPRESSION, CAST_EXPRESSION,  REF, REFERENCE, RuleAssocitiveRight);
    add_prefix_unary_rule(UNARY_EXPRESSION, CAST_EXPRESSION,  MULTIPLY, REFERENCE, RuleAssocitiveRight);
    add_prefix_unary_rule(UNARY_EXPRESSION, CAST_EXPRESSION,  PLUS, REFERENCE, RuleAssocitiveRight);
    add_prefix_unary_rule(UNARY_EXPRESSION, CAST_EXPRESSION,  MINUS, REFERENCE, RuleAssocitiveRight);
    add_prefix_unary_rule(UNARY_EXPRESSION, CAST_EXPRESSION,  BIT_NOT, REFERENCE, RuleAssocitiveRight);
    add_prefix_unary_rule(UNARY_EXPRESSION, CAST_EXPRESSION,  LOGIC_NOT, REFERENCE, RuleAssocitiveRight);

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
            get_ast(decl_spec, DECLARATION_SPECIFIERS, ASTNodeDeclarationSpecifier, 0);

            auto init_decl_list_ast = make_shared<ASTNodeInitDeclaratorList>(c);
            if (ts[1]) {
                get_ast(init_decl_listx, INIT_DECLARATOR_LIST, ASTNodeInitDeclaratorList, 1);
                init_decl_list_ast = init_decl_listxast;
            }

            auto ast = make_shared<ASTNodeDeclarationList>(c);

            for (auto decl: *init_decl_list_ast) {
                decl->set_declaration_specifier(decl_specast);
                ast->push_back(decl);
            }

            return make_shared<NonTermDECLARATION>(ast);
        });

    parser( NI(DECLARATION_SPECIFIERS),
        { NI(STORAGE_CLASS_SPECIFIER), ParserChar::beOptional(NI(DECLARATION_SPECIFIERS)) },
        [](auto c, auto ts) {
            assert(ts.size() == 2);
            get_ast(sspec, STORAGE_CLASS_SPECIFIER, ASTNodeDeclarationSpecifier, 0);
            if (ts[1]) {
                get_ast(ds, DECLARATION_SPECIFIERS, ASTNodeDeclarationSpecifier, 1);
                if (dsast->storage_class() != ASTNodeDeclarationSpecifier::StorageClass::SC_Default) {
                    // TODO warnning or error
                }
                dsast->storage_class() = sspecast->storage_class();
                std::swap(sspecast, dsast);
            }
            return make_shared<NonTermDECLARATION_SPECIFIERS>(sspecast);
        }, RuleAssocitiveLeft);

    parser( NI(DECLARATION_SPECIFIERS),
        { NI(TYPE_SPECIFIER), ParserChar::beOptional(NI(DECLARATION_SPECIFIERS)) },
        [](auto c, auto ts) {
            assert(ts.size() == 2);
            get_ast(tspec, TYPE_SPECIFIER, ASTNodeDeclarationSpecifier, 0);
            if (ts[1]) {
                get_ast(ds, DECLARATION_SPECIFIERS, ASTNodeDeclarationSpecifier, 1);
                if (dsast->type_specifier() != nullptr) {
                    // TODO mixed type, long signed ..., and warnning or error
                }
                dsast->type_specifier() = tspecast->type_specifier();
                std::swap(tspecast, dsast);
            }
            return make_shared<NonTermDECLARATION_SPECIFIERS>(tspecast);
        }, RuleAssocitiveLeft);

    parser( NI(DECLARATION_SPECIFIERS),
        { NI(TYPE_QUALIFIER), ParserChar::beOptional(NI(DECLARATION_SPECIFIERS)) },
        [](auto c, auto ts) {
            assert(ts.size() == 2);
            get_ast(tqual, TYPE_QUALIFIER, ASTNodeDeclarationSpecifier, 0);
            if (ts[1]) {
                get_ast(ds, DECLARATION_SPECIFIERS, ASTNodeDeclarationSpecifier, 1);
                dsast->const_ref() = dsast->const_ref() || tqualast->const_ref();;
                dsast->restrict_ref() = dsast->restrict_ref() || tqualast->restrict_ref();;
                dsast->volatile_ref() = dsast->volatile_ref() || tqualast->volatile_ref();;
                std::swap(tqualast, dsast);
            }
            return make_shared<NonTermDECLARATION_SPECIFIERS>(tqualast);
        }, RuleAssocitiveLeft);

    parser( NI(DECLARATION_SPECIFIERS),
        { NI(FUNCTION_SPECIFIER), ParserChar::beOptional(NI(DECLARATION_SPECIFIERS)) },
        [](auto c, auto ts) {
            assert(ts.size() == 2);
            get_ast(tqual, FUNCTION_SPECIFIER, ASTNodeDeclarationSpecifier, 0);
            if (ts[1]) {
                get_ast(ds, DECLARATION_SPECIFIERS, ASTNodeDeclarationSpecifier, 1);
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
        });

    parser( NI(INIT_DECLARATOR),
        { NI(DECLARATOR), PT(ASSIGN), NI(INITIALIZER) },
        [](auto c, auto ts) {
            assert(ts.size() == 3);
            get_ast(decl, DECLARATOR, ASTNodeInitDeclarator, 0);
            get_ast(init, INITIALIZER, ASTNodeInitializer, 2);
            declast->initializer() = initast;
            return make_shared<NonTermINIT_DECLARATOR>(declast);
        });

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
            auto kast = make_shared<ASTNodeTypeSpecifier##en>(c, ##__VA_ARGS__); \
            auto ast = make_shared<ASTNodeDeclarationSpecifier>(c); \
            ast->type_specifier() = kast; \
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

static int anonymous_struct_counter = 0;
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
                        "anonymous_struct_" + std::to_string(++anonymous_struct_counter),
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

            for (auto sd: *sdlast)
                sd->set_declaration_specifier(sqlast);

            return make_shared<NonTermSTRUCT_DECLARATION>(sdlast);
        });

    parser( NI(SPECIFIER_QUALIFIER_LIST),
        { NI(TYPE_SPECIFIER), ParserChar::beOptional(NI(SPECIFIER_QUALIFIER_LIST)) },
        [](auto c, auto ts) {
            assert(ts.size() == 2);
            get_ast(tspec, TYPE_SPECIFIER, ASTNodeDeclarationSpecifier, 0);
            if (ts[1]) {
                get_ast(ds, SPECIFIER_QUALIFIER_LIST, ASTNodeDeclarationSpecifier, 1);
                if (dsast->type_specifier() != nullptr) {
                    // TODO mixed type, long signed ..., and warnning or error
                }
                dsast->type_specifier() = tspecast->type_specifier();
                std::swap(tspecast, dsast);
            }
            return make_shared<NonTermSPECIFIER_QUALIFIER_LIST>(tspecast);
        }, RuleAssocitiveLeft);

    parser( NI(SPECIFIER_QUALIFIER_LIST),
        { NI(TYPE_QUALIFIER), ParserChar::beOptional(NI(SPECIFIER_QUALIFIER_LIST)) },
        [](auto c, auto ts) {
            assert(ts.size() == 2);
            get_ast(tqual, TYPE_QUALIFIER, ASTNodeDeclarationSpecifier, 0);
            if (ts[1]) {
                get_ast(ds, SPECIFIER_QUALIFIER_LIST, ASTNodeDeclarationSpecifier, 1);
                dsast->const_ref() = dsast->const_ref() || tqualast->const_ref();;
                dsast->restrict_ref() = dsast->restrict_ref() || tqualast->restrict_ref();;
                dsast->volatile_ref() = dsast->volatile_ref() || tqualast->volatile_ref();;
                std::swap(tqualast, dsast);
            }
            return make_shared<NonTermSPECIFIER_QUALIFIER_LIST>(tqualast);
        }, RuleAssocitiveLeft);
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