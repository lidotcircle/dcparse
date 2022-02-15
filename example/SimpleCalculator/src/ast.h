#ifndef _SIMPLE_CALCULATOR_AST_H_
#define _SIMPLE_CALCULATOR_AST_H_

#include "parser/parser.h"


class ASTNode {
private:
    DCParser::DCParserContext* m_parser_context;

public:
    inline ASTNode(DCParser::DCParserContext* p): m_parser_context(p) {}
    inline DCParser::DCParserContext* context() { return this->m_parser_context; }
    virtual ~ASTNode() = default;
};

class ASTNodeExpr : public ASTNode {
public:
    inline ASTNodeExpr(DCParser::DCParserContext* p): ASTNode(p) {}
};

class UnaryOperatorExpr: public ASTNodeExpr {
public:
    enum UnaryOperatorType {
        PRE_INC, PRE_DEC,
        POS_INC, POS_DEC,
    };

private:
    UnaryOperatorType m_operator;
    std::shared_ptr<ASTNodeExpr> m_expr;

public:
    inline UnaryOperatorExpr(
            DCParser::DCParserContext* c,
            UnaryOperatorType optype,
            std::shared_ptr<ASTNodeExpr> expr
            ):
        ASTNodeExpr(c), m_operator(optype),
        m_expr(expr) {}
};

class BinaryOperatorExpr: public ASTNodeExpr {
public:
    enum BinaryOperatorType {
        PLUS       = '+', MINUS    = '-',
        MULTIPLY   = '*', DIVISION = '/',
        REMAINDER  = '%', CARET    = '^',
        ASSIGNMENT = '=',
        EQUAL,        NOTEQUAL,
        GREATERTHAN,  LESSTHAN,
        GREATEREQUAL, LESSEQUAL,
    };

private:
    BinaryOperatorType m_operator;
    std::shared_ptr<ASTNodeExpr> m_left, m_right;

public:
    inline BinaryOperatorExpr(
            DCParser::DCParserContext* c,
            BinaryOperatorType optype,
            std::shared_ptr<ASTNodeExpr> left,
            std::shared_ptr<ASTNodeExpr> right
            ):
        ASTNodeExpr(c), m_operator(optype),
        m_left(std::move(left)), m_right(std::move(right)) {}
};

#endif // _SIMPLE_CALCULATOR_AST_H_
