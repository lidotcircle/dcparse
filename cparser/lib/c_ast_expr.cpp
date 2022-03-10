#include "c_ast.h"
#include "c_ast_utils.h"
#include "c_parser.h"
#include "c_translation_unit_context.h"
using namespace cparser;
using namespace std;
using variable_basic_type = ASTNodeVariableType::variable_basic_type;


bool ASTNodeExprIdentifier::is_constexpr() const
{
    assert(this->type());
    return this->type()->basic_type() == variable_basic_type::ENUM;
}

bool ASTNodeExprString::is_constexpr() const
{
    return true;
}

bool ASTNodeExprInteger::is_constexpr() const
{
    return true;
}

bool ASTNodeExprFloat::is_constexpr() const
{
    return true;
}

bool ASTNodeExprIndexing::is_constexpr() const
{
    return false;
}

bool ASTNodeExprFunctionCall::is_constexpr() const
{
    return false;
}

bool ASTNodeExprMemberAccess::is_constexpr() const
{
    return this->m_obj->is_constexpr();
}

bool ASTNodeExprPointerMemberAccess::is_constexpr() const
{
    return false;
}

bool ASTNodeExprInitializer::is_constexpr() const
{
    return this->m_init->is_constexpr();
}

bool ASTNodeExprUnaryOp::is_constexpr() const
{
    return false;
}

bool ASTNodeExprSizeof::is_constexpr() const
{
    return true;
}

bool ASTNodeExprCast::is_constexpr() const
{
    return this->m_expr->is_constexpr();
}

bool ASTNodeExprBinaryOp::is_constexpr() const
{
    using BT = ASTNodeExprBinaryOp::BinaryOperatorType;
    switch (this->m_operator)
    {
        case BT::GREATER_THAN:
        case BT::GREATER_THAN_EQUAL:
        case BT::LESS_THAN:
        case BT::LESS_THAN_EQUAL:
        case BT::EQUAL:
        case BT::NOT_EQUAL:
        case BT::LOGICAL_AND:
        case BT::LOGICAL_OR:
        case BT::BITWISE_AND:
        case BT::BITWISE_OR:
        case BT::BITWISE_XOR:
        case BT::LEFT_SHIFT:
        case BT::RIGHT_SHIFT:
        case BT::PLUS:
        case BT::MINUS:
        case BT::MULTIPLY:
        case BT::DIVISION:
        case BT::REMAINDER:
            return this->m_left->is_constexpr() && this->m_right->is_constexpr();
        default: 
            return false;
    }
}

bool ASTNodeExprConditional::is_constexpr() const
{
    return this->m_cond->is_constexpr() &&
           this->m_true->is_constexpr() &&
           this->m_false->is_constexpr();
}

bool ASTNodeExprList::is_constexpr() const
{
    return false;
}


bool ASTNodeExprIdentifier::is_lvalue() const
{
    assert(this->type());
    return this->type()->basic_type() != variable_basic_type::ENUM;
}

bool ASTNodeExprString::is_lvalue() const
{
    return false;
}

bool ASTNodeExprInteger::is_lvalue() const
{
    return false;
}

bool ASTNodeExprFloat::is_lvalue() const
{
    return false;
}

bool ASTNodeExprIndexing::is_lvalue() const
{
    return true;
}

bool ASTNodeExprFunctionCall::is_lvalue() const
{
    return false;
}

bool ASTNodeExprMemberAccess::is_lvalue() const
{
    return this->m_obj->is_lvalue();
}

bool ASTNodeExprPointerMemberAccess::is_lvalue() const
{
    return true;
}

bool ASTNodeExprInitializer::is_lvalue() const
{
    return false;
}

bool ASTNodeExprUnaryOp::is_lvalue() const
{
    using OP = ASTNodeExprUnaryOp::UnaryOperatorType;
    switch (this->m_operator)
    {
        case OP::DEREFERENCE:
            return true;
        default:
            return false;
    }
}

bool ASTNodeExprSizeof::is_lvalue() const
{
    return false;
}

bool ASTNodeExprCast::is_lvalue() const
{
    return false;
}

bool ASTNodeExprBinaryOp::is_lvalue() const
{
    return false;
}

bool ASTNodeExprConditional::is_lvalue() const
{
    return false;
}

bool ASTNodeExprList::is_lvalue() const
{
    return false;
}
