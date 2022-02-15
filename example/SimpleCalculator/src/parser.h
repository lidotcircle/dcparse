#ifndef _SIMPLE_CALCULATOR_PARSER_H_
#define _SIMPLE_CALCULATOR_PARSER_H_

#include "parser/parser.h"
#include "ast.h"


#define NONTERMS \
    TENTRY(CalcUnit) \
    TENTRY(ArgList) \
    TENTRY(FunctionDef) \
    \
    TENTRY(IfStatement) \
    TENTRY(ForStatement) \
    TENTRY(WhileStatement) \
    TENTRY(ReturnStatement) \
    TENTRY(StatementList) \
    TENTRY(BlockStatement) \
    TENTRY(Statement) \
    \
    TENTRY(ExprList) \
    TENTRY(Expr)

#define TENTRY(n) \
    struct NonTerm##n: public NonTerminal { \
        std::shared_ptr<ASTNode> astnode; \
        inline NonTerm##n(std::shared_ptr<ASTNode> node): astnode(node) {} \
    };
NONTERMS
#undef TENTRY


class CalcParser: private DCParser {
public:
    CalcParser();

    using DCParser::feed;
    using DCParser::end;
    using DCParser::parse;
    using DCParser::reset;
};

#endif // _SIMPLE_CALCULATOR_PARSER_H_
