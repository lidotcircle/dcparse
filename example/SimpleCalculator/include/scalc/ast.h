#ifndef _SIMPLE_CALCULATOR_AST_H_
#define _SIMPLE_CALCULATOR_AST_H_

#include "parser/parser.h"
#include "./token.h"
#include <string>
#include <memory>


using ASTNodeParserContext = std::weak_ptr<DCParser::DCParserContext>;

class ASTNode {
private:
    ASTNodeParserContext m_parser_context;

public:
    inline ASTNode(ASTNodeParserContext p): m_parser_context(p) {}
    inline ASTNodeParserContext context() { return this->m_parser_context; }
    virtual ~ASTNode() = default;
};


class ASTNodeExpr : public ASTNode {
public:
    inline ASTNodeExpr(ASTNodeParserContext p): ASTNode(p) {}

    virtual double evaluate() = 0;
    virtual bool used() { return false; }
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
            ASTNodeParserContext c,
            UnaryOperatorType optype,
            std::shared_ptr<ASTNodeExpr> expr
            ):
        ASTNodeExpr(c), m_operator(optype),
        m_expr(expr) {}

    virtual double evaluate() override;
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
            ASTNodeParserContext c,
            BinaryOperatorType optype,
            std::shared_ptr<ASTNodeExpr> left,
            std::shared_ptr<ASTNodeExpr> right
            ):
        ASTNodeExpr(c), m_operator(optype),
        m_left(std::move(left)), m_right(std::move(right)) {}

    virtual double evaluate() override;
    bool used() override { return m_operator == ASSIGNMENT; }
};

class IDExpr: public ASTNodeExpr
{
private:
    std::string _id;

public:
    inline IDExpr(
            ASTNodeParserContext c,
            std::string id): ASTNodeExpr(c), _id(id) {}
    inline const std::string& id() const { return this->_id; }

    virtual double evaluate() override;
};

class NumberExpr: public ASTNodeExpr
{
private:
    double val;

public:
    inline NumberExpr(
            ASTNodeParserContext c,
            double val): ASTNodeExpr(c), val(val) {}
    inline double value() const { return this->val; }

    virtual double evaluate() override;
};

class ASTNodeExprList: public ASTNode, private std::vector<std::shared_ptr<ASTNodeExpr>>
{
private:
    using container_t = std::vector<std::shared_ptr<ASTNodeExpr>>;

public:
    using reference = container_t::reference;
    using const_reference = container_t::const_reference;
    using iterator = container_t::iterator;
    using const_iterator = container_t::const_iterator;

    inline ASTNodeExprList(
            ASTNodeParserContext c): ASTNode(c) {}

    using container_t::begin;
    using container_t::end;
    using container_t::empty;
    using container_t::size;
    using container_t::operator[];
    using container_t::push_back;
};

class FunctionCallExpr: public ASTNodeExpr {
private:
    std::shared_ptr<ASTNodeExpr> _func;
    std::shared_ptr<ASTNodeExprList> _parameters;

public:
    inline FunctionCallExpr(
            ASTNodeParserContext c,
            std::shared_ptr<ASTNodeExpr> func,
            std::shared_ptr<ASTNodeExprList> parameters):
        ASTNodeExpr(c), _func(func), _parameters(parameters) {}

    inline std::shared_ptr<ASTNodeExpr>     function()   { return this->_func; }
    inline std::shared_ptr<ASTNodeExprList> parameters() { return this->_parameters; }
    inline const std::shared_ptr<ASTNodeExpr>     function()   const { return this->_func; }
    inline const std::shared_ptr<ASTNodeExprList> parameters() const { return this->_parameters; }

    virtual double evaluate() override;
};


class ASTNodeStat: public ASTNode
{
public:
    inline ASTNodeStat(ASTNodeParserContext c): ASTNode(c) {}

    virtual void execute() = 0;
};

class ASTNodeStatList: public ASTNode, private std::vector<std::shared_ptr<ASTNodeStat>>
{
private:
    using container_t = std::vector<std::shared_ptr<ASTNodeStat>>;

public:
    using reference = container_t::reference;
    using const_reference = container_t::const_reference;
    using iterator = container_t::iterator;
    using const_iterator = container_t::const_iterator;

    inline ASTNodeStatList(
            ASTNodeParserContext c): ASTNode(c) {}

    using container_t::begin;
    using container_t::end;
    using container_t::empty;
    using container_t::size;
    using container_t::operator[];
    using container_t::push_back;
};

class ASTNodeBlockStat: public ASTNodeStat
{
private:
    std::shared_ptr<ASTNodeStatList> _statlist;

public:
    inline ASTNodeBlockStat(
            ASTNodeParserContext c,
            std::shared_ptr<ASTNodeStatList> statlist):
        ASTNodeStat(c), _statlist(statlist) {}

    inline std::shared_ptr<ASTNodeStatList>       StatementList()       { return this->_statlist; }
    inline const std::shared_ptr<ASTNodeStatList> StatementList() const { return this->_statlist; }

    virtual void execute() override;
};

class ASTNodeExprStat: public ASTNodeStat
{ 
private:
    std::shared_ptr<ASTNodeExprList> _exprlist;

public:
    inline ASTNodeExprStat(
            ASTNodeParserContext c,
            std::shared_ptr<ASTNodeExprList> el): ASTNodeStat(c), _exprlist(el) {}

    inline       std::shared_ptr<ASTNodeExprList> exprList()       { return this->_exprlist; }
    inline const std::shared_ptr<ASTNodeExprList> exprList() const { return this->_exprlist; }

    virtual void execute() override;
};

class ASTNodeReturnStat: public ASTNodeStat
{ 
private:
    std::shared_ptr<ASTNodeExpr> _expr;

public:
    inline ASTNodeReturnStat(
            ASTNodeParserContext c,
            std::shared_ptr<ASTNodeExpr> expr): ASTNodeStat(c), _expr(expr) {}

    inline       std::shared_ptr<ASTNodeExpr> expr()       { return this->_expr; }
    inline const std::shared_ptr<ASTNodeExpr> expr() const { return this->_expr; }

    virtual void execute() override;
};

class ASTNodeIFStat: public ASTNodeStat
{
private:
    std::shared_ptr<ASTNodeStat> _truestat, _falsestat;
    std::shared_ptr<ASTNodeExpr> _cond;

public:
    inline ASTNodeIFStat(
            ASTNodeParserContext c,
            std::shared_ptr<ASTNodeExpr> condition,
            std::shared_ptr<ASTNodeStat> truestat,
            std::shared_ptr<ASTNodeStat> falsestat):
        ASTNodeStat(c), _cond(condition),
        _truestat(truestat), _falsestat(falsestat) {}

    inline std::shared_ptr<ASTNodeStat> trueStat()  { return this->_truestat; }
    inline std::shared_ptr<ASTNodeStat> falseStat() { return this->_falsestat; }
    inline std::shared_ptr<ASTNodeExpr> condition() { return this->_cond; }

    virtual void execute() override;
};

class ASTNodeFORStat: public ASTNodeStat
{
private:
    std::shared_ptr<ASTNodeStat> _stat;
    std::shared_ptr<ASTNodeExprList> _pre, _post;
    std::shared_ptr<ASTNodeExpr> _cond;

public:
    inline ASTNodeFORStat(
            ASTNodeParserContext c,
            std::shared_ptr<ASTNodeExprList> pre,
            std::shared_ptr<ASTNodeExpr> condition,
            std::shared_ptr<ASTNodeExprList> post,
            std::shared_ptr<ASTNodeStat> stat):
        ASTNodeStat(c), _cond(condition),
        _stat(stat), _pre(pre), _post(post) {}

    inline std::shared_ptr<ASTNodeStat> stat()  { return this->_stat; }
    inline std::shared_ptr<ASTNodeExpr> condition() { return this->_cond; }
    inline std::shared_ptr<ASTNodeExprList> pre () { return this->_pre; }
    inline std::shared_ptr<ASTNodeExprList> post() { return this->_post; }

    virtual void execute() override;
};

/*
class ASTNodeWHILEStat: public ASTNodeStat
{
private:
    std::shared_ptr<ASTNodeStat> _stat;
    std::shared_ptr<ASTNodeExpr> _cond;

public:
    inline ASTNodeWHILEStat(
            ASTNodeParserContext c,
            std::shared_ptr<ASTNodeExprStat> stat,
            std::shared_ptr<ASTNodeExpr> condition):
        ASTNodeStat(c), _cond(condition),
        _stat(stat) {}

    inline std::shared_ptr<ASTNodeStat> stat()  { return this->_stat; }
    inline std::shared_ptr<ASTNodeExpr> condition() { return this->_cond; }
};
*/

class ASTNodeArgList: public ASTNode, private std::vector<std::string>
{
private:
    using container_t = std::vector<std::string>;

public:
    inline ASTNodeArgList(ASTNodeParserContext c): ASTNode(c) {}

    using container_t::begin;
    using container_t::end;
    using container_t::empty;
    using container_t::size;
    using container_t::operator[];
    using container_t::push_back;
};

class ASTNodeFunctionDef: public ASTNode
{
private:
    std::shared_ptr<ASTNodeExpr> func;
    std::shared_ptr<ASTNodeArgList> arglist;
    std::shared_ptr<ASTNodeBlockStat> blockstat;

public:
    inline ASTNodeFunctionDef(
            ASTNodeParserContext c,
            std::shared_ptr<ASTNodeExpr> func,
            std::shared_ptr<ASTNodeArgList> args,
            std::shared_ptr<ASTNodeBlockStat> stat):
        ASTNode(c), func(func), arglist(args), blockstat(stat) {}

    inline std::shared_ptr<ASTNodeExpr>      function()  { return this->func; }
    inline std::shared_ptr<ASTNodeArgList>   argList()   { return this->arglist; }
    inline std::shared_ptr<ASTNodeBlockStat> blockStat() { return this->blockstat; }

    void call(std::vector<double> parameters);
};

class ASTNodeCalcUnit: public ASTNode
{
private:
    std::vector<std::shared_ptr<ASTNodeFunctionDef>> functions;
    std::vector<std::shared_ptr<ASTNodeStat>> statements;

public:
    inline ASTNodeCalcUnit(ASTNodeParserContext c): ASTNode(c) {}

    void push_function(std::shared_ptr<ASTNodeFunctionDef> func) ;
    void push_statement(std::shared_ptr<ASTNodeStat> stat);
};

#endif // _SIMPLE_CALCULATOR_AST_H_
