#include "scalc/parser.h"
#include "scalc/token.h"
#include "scalc/context.h"
#include <iostream>
using namespace std;


#define NI(t) CharInfo<NonTerm##t>()
#define TI(t) CharInfo<Token##t>()

#define add_binary_rule(op, assoc) \
parser( NI(Expr), \
        { NI(Expr), TI(op), NI(Expr) }, \
        [](auto c, auto ts) { \
            assert(ts.size() == 3); \
            auto left  = dynamic_pointer_cast<NonTermExpr>(ts[0]); \
            auto right = dynamic_pointer_cast<NonTermExpr>(ts[2]); \
            assert(left && right); \
            auto astleft = dynamic_pointer_cast<ASTNodeExpr>(left->astnode); \
            auto astright = dynamic_pointer_cast<ASTNodeExpr>(right->astnode); \
            auto ast = make_shared<BinaryOperatorExpr>( \
                    c, \
                    BinaryOperatorExpr::BinaryOperatorType::op, \
                    astleft, \
                    astright); \
            return make_shared<NonTermExpr>(ast); \
        }, assoc );

#define add_unary_rule(t1, t2, isleft, type, assoc) \
parser( NI(Expr), \
        { t1, t2 }, \
        [](auto c, auto ts) { \
            assert(ts.size() == 2); \
            auto expr = dynamic_pointer_cast<NonTermExpr>(ts[isleft ? 0 : 1]); \
            assert(expr); \
            auto astexpr = dynamic_pointer_cast<ASTNodeExpr>(expr->astnode); \
            auto ast = make_shared<UnaryOperatorExpr>( \
                    c, \
                    UnaryOperatorExpr::UnaryOperatorType::type, \
                    astexpr); \
            return make_shared<NonTermExpr>(ast); \
        }, assoc );

#define pnonterm(type, nodetype, index, name) \
    auto name = dynamic_pointer_cast<NonTerm##type>(ts[index]); \
    assert(name); \
    auto name##ast = dynamic_pointer_cast<nodetype>(name->astnode); \
    assert(name##ast)

using ParserChar = DCParser::ParserChar;


SCalcParserContext::SCalcParserContext(DCParser& p, bool execute):
    DCParserContext(p),
    m_execution_context(make_shared<SCalcContext>()),
    m_execute(execute),
    m_out(nullptr)
{
}
std::shared_ptr<SCalcContext> SCalcParserContext::ExecutionContext()
{
    return this->m_execution_context;
}
const std::shared_ptr<SCalcContext> SCalcParserContext::ExecutionContext() const
{
    return this->m_execution_context;
}


void CalcParser::expression_rules()
{
    DCParser& parser = *this;

    parser( NI(Expr), { NI(Expr), TI(LPAREN), ParserChar::beOptional(NI(ExprList)), TI(RPAREN) },
            [](auto c, auto ts) {
                assert(ts.size() == 4);
                pnonterm(Expr,     ASTNodeExpr,     0, func);

                auto parasast = make_shared<ASTNodeExprList>(c);
                if (ts[2]) {
                    pnonterm(ExprList, ASTNodeExprList, 2, parasx);
                    parasast = parasxast;
                }

                auto ast = make_shared<FunctionCallExpr>(c, funcast, parasast);
                return make_shared<NonTermExpr>(ast);
            });

    parser.__________();

    add_unary_rule( NI(Expr), TI(PLUSPLUS),   true, POS_INC, RuleAssocitiveLeft );
    add_unary_rule( NI(Expr), TI(MINUSMINUS), true, POS_DEC, RuleAssocitiveLeft );

    parser.__________();

    add_unary_rule( TI(PLUSPLUS),   NI(Expr), false, PRE_INC, RuleAssocitiveRight );
    add_unary_rule( TI(MINUSMINUS), NI(Expr), false, PRE_DEC, RuleAssocitiveRight );

    parser.__________();

    add_binary_rule(CARET, RuleAssocitiveLeft);

    parser.__________();

    add_binary_rule(MULTIPLY, RuleAssocitiveLeft);
    add_binary_rule(DIVISION, RuleAssocitiveLeft);
    add_binary_rule(REMAINDER, RuleAssocitiveLeft);

    parser.__________();

    add_binary_rule(PLUS, RuleAssocitiveLeft);
    add_binary_rule(MINUS, RuleAssocitiveLeft);

    parser.__________();

    add_binary_rule(GREATERTHAN, RuleAssocitiveLeft);
    add_binary_rule(LESSTHAN, RuleAssocitiveLeft);
    add_binary_rule(GREATEREQUAL, RuleAssocitiveLeft);
    add_binary_rule(LESSEQUAL, RuleAssocitiveLeft);

    parser.__________();

    add_binary_rule(EQUAL, RuleAssocitiveLeft);
    add_binary_rule(NOTEQUAL, RuleAssocitiveLeft);

    parser.__________();

    add_binary_rule(ASSIGNMENT, RuleAssocitiveRight);

    parser.__________();

    parser( NI( Expr), { TI(ID) },
            [](auto c, auto ts) {
                assert(ts.size() == 1);
                auto p = dynamic_pointer_cast<TokenID>(ts[0]);
                auto ast = make_shared<IDExpr>(c, p->id);
                return make_shared<NonTermExpr>(ast);
            });

    parser( NI( Expr), { TI(NUMBER) },
            [](auto c, auto ts) {
                assert(ts.size() == 1);
                auto p = dynamic_pointer_cast<TokenNUMBER>(ts[0]);
                auto ast = make_shared<NumberExpr>(c, p->num);
                return make_shared<NonTermExpr>(ast);
            });

    parser.__________();


    parser( NI(ExprList), { NI(Expr) },
            [](auto c, auto ts) {
                assert(ts.size() == 1);
                pnonterm(Expr, ASTNodeExpr, 0, expr);
                auto ast = make_shared<ASTNodeExprList>(c); 
                ast->push_back(exprast);
                return make_shared<NonTermExprList>(ast);
            });

    parser( NI(ExprList), { NI(ExprList), TI(COMMA), NI(Expr) },
            [](auto c, auto ts) {
                assert(ts.size() == 3);
                pnonterm(ExprList, ASTNodeExprList, 0, exprlist);
                pnonterm(Expr, ASTNodeExpr, 2, expr);

                exprlistast->push_back(exprast);
                return exprlist;
            });
}

void CalcParser::statement_rules()
{
    DCParser& parser = *this;

    parser( NI(ExprStatement), { NI(ExprList), TI(SEMICOLON) },
            [] (auto c, auto ts) {
                assert(ts.size() == 2);
                pnonterm(ExprList, ASTNodeExprList, 0, exprlist);

                auto ast = make_shared<ASTNodeExprStat>(c, exprlistast);
                return make_shared<NonTermExprStatement>(ast);
            });

    parser( NI(ExprStatement), { TI(SEMICOLON) },
            [] (auto c, auto ts) {
                assert(ts.size() == 1);

                auto exprlistast = make_shared<ASTNodeExprList>(c);
                auto ast = make_shared<ASTNodeExprStat>(c, exprlistast);
                return make_shared<NonTermExprStatement>(ast);
            });

    parser( NI(StatementList), { NI(Statement) },
            [] (auto c, auto ts) {
                assert(ts.size() == 1);
                pnonterm(Statement, ASTNodeStat, 0, stat);

                auto ast = make_shared<ASTNodeStatList>(c);
                ast->push_back(statast);
                return make_shared<NonTermStatementList>(ast);
            });

    parser( NI(StatementList), { NI(StatementList), NI(Statement) },
            [] (auto c, auto ts) {
                assert(ts.size() == 2);
                pnonterm(StatementList, ASTNodeStatList, 0, statlist);
                pnonterm(Statement, ASTNodeStat, 1, stat);
                statlistast->push_back(statast);
                return statlist;
            });

    parser( NI(BlockStatement), { TI(LBRACE), ParserChar::beOptional(NI(StatementList)), TI(RBRACE) },
            [] (auto c, auto ts) {
                assert(ts.size() == 3);

                auto statlistast = make_shared<ASTNodeStatList>(c);
                if (ts[1]) {
                    pnonterm(StatementList, ASTNodeStatList, 1, statlistx);
                    statlistast = statlistxast;
                }

                auto ast = make_shared<ASTNodeBlockStat>(c, statlistast);
                return make_shared<NonTermBlockStatement>(ast);
            });

    parser( NI(IfStatement), { TI(IF), TI(LPAREN), NI(Expr), TI(RPAREN), NI(Statement), TI(ELSE), NI(Statement) },
            [] (auto c, auto ts) {
                assert(ts.size() == 7);
                pnonterm(Expr, ASTNodeExpr, 2, cond);
                pnonterm(Statement, ASTNodeStat, 4, truestat);
                pnonterm(Statement, ASTNodeStat, 6, falsestat);

                auto ast = make_shared<ASTNodeIFStat>(c, condast, truestatast, falsestatast);
                return make_shared<NonTermIfStatement>(ast);
            }, RuleAssocitiveRight);

    parser( NI(IfStatement), { TI(IF), TI(LPAREN), NI(Expr), TI(RPAREN), NI(Statement) },
            [] (auto c, auto ts) {
                assert(ts.size() == 5);
                pnonterm(Expr, ASTNodeExpr, 2, cond);
                pnonterm(Statement, ASTNodeStat, 4, truestat);

                auto ast = make_shared<ASTNodeIFStat>(c, condast, truestatast, nullptr);
                return make_shared<NonTermIfStatement>(ast);
            }, RuleAssocitiveRight);

    parser ( NI(ForStatement), 
           { TI(FOR), TI(LPAREN),
                      NI(ExprList), TI(SEMICOLON),
                      NI(Expr), TI(SEMICOLON),
                      NI(ExprList),
                      TI(RPAREN), NI(Statement) },
            [] (auto c, auto ts) {
                assert(ts.size() == 9);
                pnonterm(ExprList,  ASTNodeExprList, 2, pre);
                pnonterm(Expr,      ASTNodeExpr,     4, cond);
                pnonterm(ExprList,  ASTNodeExprList, 6, post);
                pnonterm(Statement, ASTNodeStat,     8, stat);

                auto ast = make_shared<ASTNodeFORStat>(c, preast, condast, postast, statast);
                return make_shared<NonTermForStatement>(ast);
            });

    parser ( NI(WhileStatement), { TI(WHILE), TI(LPAREN), NI(Expr), TI(RPAREN), NI(Statement) },
            [] (auto c, auto ts) {
                assert(ts.size() == 5);
                pnonterm(Expr, ASTNodeExpr, 2, cond);
                pnonterm(Statement, ASTNodeStat, 4, stat);
                auto preast  = make_shared<ASTNodeExprList>(c);
                auto postast = make_shared<ASTNodeExprList>(c);

                auto ast = make_shared<ASTNodeFORStat>(c, preast, condast, postast, statast);
                return make_shared<NonTermWhileStatement>(ast);
            });

    parser ( NI(ReturnStatement), { TI(RETURN), NI(Expr), TI(SEMICOLON) },
            [] (auto c, auto ts) {
                assert(ts.size() == 3);
                pnonterm(Expr, ASTNodeExpr, 1, expr);
                auto ast = make_shared<ASTNodeReturnStat>(c, exprast);

                return make_shared<NonTermReturnStatement>(ast);
            });

#define to_statement(type) \
    parser ( NI(Statement), { NI(type) }, \
            [] (auto c, auto ts) { \
                assert(ts.size() == 1); \
                pnonterm(type, ASTNodeStat, 0, stat); \
                return make_shared<NonTermStatement>(statast);\
            })

    to_statement(ExprStatement);
    to_statement(IfStatement);
    to_statement(ForStatement);
    to_statement(WhileStatement);
    to_statement(ReturnStatement);
    to_statement(BlockStatement);
}

void CalcParser::function_rules()
{
    DCParser& parser = *this;

    parser( NI(FunctionDef), { TI(FUNCTION), NI(Expr), TI(LPAREN), ParserChar::beOptional(NI(ArgList)), TI(RPAREN), NI(BlockStatement) },
            [] (auto c, auto ts) {
                assert(ts.size() == 6);
                pnonterm(Expr, ASTNodeExpr, 1, func);

                std::shared_ptr<ASTNodeArgList> argsast;
                if (ts[3]) {
                    pnonterm(ArgList, ASTNodeArgList, 3, argsx);
                    argsast = argsxast;
                } else {
                    argsast = make_shared<ASTNodeArgList>(c);
                }

                pnonterm(BlockStatement, ASTNodeBlockStat, 5, stat);

                auto ast = make_shared<ASTNodeFunctionDef>(c, funcast, argsast, statast);
                return make_shared<NonTermFunctionDef>(ast);
            });

    parser( NI(ArgList), { TI(ID) },
            [] (auto c, auto ts) {
                assert(ts.size() == 1);

                auto tk = dynamic_pointer_cast<TokenID>(ts[0]);
                assert(tk);
                auto argsast = make_shared<ASTNodeArgList>(c);
                argsast->push_back(tk->id);

                return make_shared<NonTermArgList>(argsast);
            });

    parser( NI(ArgList), { NI(ArgList), TI(COMMA), TI(ID) },
            [] (auto c, auto ts) {
                assert(ts.size() == 3);
                
                pnonterm(ArgList, ASTNodeArgList, 0, args);
                auto tk = dynamic_pointer_cast<TokenID>(ts[2]);
                assert(tk);
                argsast->push_back(tk->id);

                return make_shared<NonTermArgList>(argsast);
            });
}

void CalcParser::calcunit_rules()
{
    DCParser& parser = *this;

    parser( NI(CalcUnit), { NI(Statement) },
            [] (auto c, auto ts) {
                assert(ts.size() == 1);
                pnonterm(Statement, ASTNodeStat, 0, stat);

                auto ast = make_shared<ASTNodeCalcUnit>(c);
                ast->push_statement(statast);

                return make_shared<NonTermCalcUnit>(ast);
            });

    parser( NI(CalcUnit), { NI(FunctionDef) },
            [] (auto c, auto ts) {
                assert(ts.size() == 1);
                pnonterm(FunctionDef, ASTNodeFunctionDef, 0, func);

                auto ast = make_shared<ASTNodeCalcUnit>(c);
                ast->push_function(funcast);

                return make_shared<NonTermCalcUnit>(ast);
            });

    parser( NI(CalcUnit), { NI(CalcUnit), NI(Statement) },
            [] (auto c, auto ts) {
                assert(ts.size() == 2);
                pnonterm(CalcUnit,  ASTNodeCalcUnit, 0, unit);
                pnonterm(Statement, ASTNodeStat, 1, stat);

                unitast->push_statement(statast);
                return make_shared<NonTermCalcUnit>(unitast);
            });

    parser( NI(CalcUnit), { NI(CalcUnit), NI(FunctionDef) },
            [] (auto c, auto ts) {
                assert(ts.size() == 2);
                pnonterm(CalcUnit,  ASTNodeCalcUnit, 0, unit);
                pnonterm(FunctionDef, ASTNodeFunctionDef, 1, func);

                unitast->push_function(funcast);
                return make_shared<NonTermCalcUnit>(unitast);
            });
}

CalcParser::CalcParser(bool execute)
{
    DCParser& parser = *this;
    this->setContext(make_unique<SCalcParserContext>(parser, execute));

    this->statement_rules();
    this->function_rules();
    this->calcunit_rules();

    parser.__________();

    this->expression_rules();

    parser.add_start_symbol(NI(CalcUnit).id);
    parser.generate_table();
}
