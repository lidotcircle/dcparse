#include "scalc/ast.h"
#include "scalc/parser.h"
#include "scalc/context.h"
#include "scalc/scalc_error.h"
#include "scalc/defer.hpp"
#include <math.h>
#include <iomanip>
using namespace std;

#define GetContext(var, node) \
    assert(node && !node->context().expired()); \
    auto ptr = node->context().lock(); \
    assert(ptr); \
    auto c##var = dynamic_pointer_cast<SCalcParserContext>(ptr); \
    assert(c##var && "get parser context failed"); \
    auto var = c##var->ExecutionContext()


class ASTNodeFunctionExec: public SCalcFunction
{
private:
    shared_ptr<ASTNodeFunctionDef> funcdef;

public:
    ASTNodeFunctionExec(shared_ptr<ASTNodeFunctionDef> def): funcdef(def) {}

    virtual double call(std::vector<double> parameters) const override
    {
        assert(this->funcdef);
        this->funcdef->call(parameters);

        GetContext(context, this->funcdef);
        auto ret = context->return_value();
        if (!ret.has_value())
            return 0;

        return ret.value();
    }
};


void ASTNodeCalcUnit::push_function(shared_ptr<ASTNodeFunctionDef> func)
{
    this->m_unititem.push_back(func);

    auto funcname = func->function();
    auto funcnamet = dynamic_pointer_cast<IDExpr>(funcname);
    if (!funcnamet)
        throw SCalcBadFunctionNameExpression("expect an identify as function name");

    GetContext(context, func);
    context->add_function(funcnamet->id(), make_shared<ASTNodeFunctionExec>(func));
}

void ASTNodeCalcUnit::push_function_decl(shared_ptr<ASTNodeFunctionDecl> func)
{
    this->m_unititem.push_back(func);
}

void ASTNodeCalcUnit::push_statement(shared_ptr<ASTNodeStat> stat)
{
    this->m_unititem.push_back(stat);

    auto context = this->context();
    auto ptr = context.lock();
    assert(ptr);
    auto cstat = dynamic_pointer_cast<SCalcParserContext>(ptr);

    if (cstat->executable())
        stat->execute();
}

double UnaryOperatorExpr::evaluate()
{
    GetContext(context, this->m_expr);

    auto id = dynamic_pointer_cast<IDExpr>(this->m_expr);
    if (id == nullptr)
        throw SCalcExpectAnLValue("expect an id expression for unary operator");

    auto val = this->m_expr->evaluate();
    switch (this->m_operator) {
    case UnaryOperatorType::POS_DEC:
        context->set_id(id->id(), val - 1);
        break;
    case UnaryOperatorType::POS_INC:
        context->set_id(id->id(), val + 1);
        break;
    case UnaryOperatorType::PRE_DEC:
        context->set_id(id->id(), val - 1);
        val--;
        break;
    case UnaryOperatorType::PRE_INC:
        context->set_id(id->id(), val + 1);
        val++;
        break;
    default:
        assert(false);
    }

    return val;
}

double BinaryOperatorExpr::evaluate()
{
    GetContext(context, this->m_left);

    if (this->m_operator == BinaryOperatorType::ASSIGNMENT) {
        auto id = dynamic_pointer_cast<IDExpr>(this->m_left);
        if (id == nullptr)
            throw SCalcExpectAnLValue("expect an id expression for assignment");

        auto val = this->m_right->evaluate();
        context->set_id(id->id(), val);
        return val;
    }

    auto l = this->m_left->evaluate();
    auto r  = this->m_right->evaluate();

    switch (this->m_operator) {
    case BinaryOperatorType::PLUS:
        return l + r;
    case BinaryOperatorType::MINUS:
        return l - r;
    case BinaryOperatorType::MULTIPLY:
        return l * r;
    case BinaryOperatorType::DIVISION:
        return l / r;
    case BinaryOperatorType::REMAINDER:
        return ::fmod(l, r);
    case BinaryOperatorType::CARET:
        return ::pow(l, r);
    case BinaryOperatorType::LESSTHAN:
        return (l < r) ? 1 : 0;
    case BinaryOperatorType::GREATERTHAN:
        return (l > r) ? 1 : 0;
    case BinaryOperatorType::LESSEQUAL:
        return (l <= r) ? 1 : 0;
    case BinaryOperatorType::GREATEREQUAL:
        return (l >= r) ? 1 : 0;
    case BinaryOperatorType::EQUAL:
        return (l == r) ? 1 : 0;
    case BinaryOperatorType::NOTEQUAL:
        return (l != r) ? 1 : 0;
    default:
        assert(false);
    }
    return 0;
}

double IDExpr::evaluate()
{
    GetContext(context, this);
    return context->get_id(this->id());
}

double  NumberExpr::evaluate()
{
    return this->val;
}

double FunctionCallExpr::evaluate()
{
    GetContext(context, this);

    auto func = this->function();
    auto funcname = dynamic_pointer_cast<IDExpr>(func);
    if (funcname == nullptr)
        throw SCalcBadFunctionNameExpression("expect an identify as function name");

    vector<double> parameters;
    for (auto param : *this->parameters())
        parameters.push_back(param->evaluate());

    return context->call_function(funcname->id(), parameters);
}


void ASTNodeBlockStat::execute()
{
    GetContext(context, this);

    context->push_scope();
    auto d1 = defer([&]() {
        context->pop_scope();
    });

    for (auto stat : *this->_statlist)
        stat->execute();
}

void ASTNodeExprStat::execute()
{
    GetContext(context, this);
    auto out = ccontext->output();
    const auto isoutest = context->in_outest_scope();

    bool printn = false;
    const auto exprlistlen = this->_exprlist->size();
    for (size_t i=0;i<exprlistlen;i++) {
        auto expr = (*this->_exprlist)[i];
        auto val = expr->evaluate();

        if (out && isoutest && (exprlistlen > 1 || !expr->used())) {
            printn = true;
            double intpart;
            if (std::modf(val, &intpart) == 0.0) {
                *out << std::fixed << std::setprecision(0) << intpart;
            } else {
                if (std::abs(val) < 1e10 && std::abs(val) > 1e-10) {
                    *out << std::fixed << val;
                } else {
                    *out << val;
                }
            }

            if (i != exprlistlen - 1)
                *out << ", ";
        }
    }

    if (printn)
        *out << endl;
}

void ASTNodeReturnStat::execute()
{
    GetContext(context, this);

    if (this->_expr) {
        auto val = this->_expr->evaluate();
        context->returnit(val);
    }
}

void ASTNodeIFStat::execute()
{
    auto cond = this->_cond->evaluate();
    if (cond) {
        this->trueStat()->execute();
    } else if (this->falseStat()) {
        this->falseStat()->execute();
    }
}

void ASTNodeFORStat::execute()
{
    for (auto expr: *this->pre())
        expr->evaluate();

    for (;this->condition()->evaluate()!=0;) {
        this->stat()->execute();

        for (auto expr: *this->post())
            expr->evaluate();
    }
}


void ASTNodeFunctionDef::call(vector<double> parameters)
{
    GetContext(context, this);

    if (this->arglist->size() != parameters.size())
        throw SCalcBadFunctionArgumentCount("expect " + to_string(this->arglist->size()) + " arguments");

    auto list = this->argList();
    for (size_t i=0;i<parameters.size();i++) {
        auto id = list->operator[](i);
        context->set_argument(id, parameters[i]);
    }

    this->blockStat()->execute();
}
