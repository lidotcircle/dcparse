#include "c_ast.h"
#include "c_ast_utils.h"
#include "c_parser.h"
#include "c_translation_unit_context.h"
using namespace cparser;
using namespace std;
using variable_basic_type = ASTNodeVariableType::variable_basic_type;


bool ASTNodeExpr::is_constexpr() const
{
    return this->get_integer_constant().has_value() ||
           this->get_float_constant().has_value();
}

optional<long long> ASTNodeExprIdentifier::get_integer_constant() const
{
    if (this->type()->basic_type() == variable_basic_type::ENUM) {
        auto enumtype = dynamic_pointer_cast<ASTNodeVariableTypeEnum>(this->type());
        assert(this->type()->is_object_type());
        auto ctx = this->context()->tu_context();
        auto cons = ctx->resolve_enum_constant(enumtype->name()->id, this->m_id->id);
        assert(cons);
        return cons;
    } else {
        return nullopt;
    }
}

optional<long double> ASTNodeExprIdentifier::get_float_constant() const
{
    return this->get_integer_constant();
}

optional<long long> ASTNodeExprString::get_integer_constant() const
{
    return nullopt;
}

optional<long double> ASTNodeExprString::get_float_constant() const
{
    return nullopt;
}

optional<long long> ASTNodeExprInteger::get_integer_constant() const
{
    return this->m_token->value;
}

optional<long double> ASTNodeExprInteger::get_float_constant() const
{
    return this->get_integer_constant();
}

optional<long long> ASTNodeExprFloat::get_integer_constant() const
{
    return nullopt;
}

optional<long double> ASTNodeExprFloat::get_float_constant() const
{
    return this->m_token->value;
}

optional<long long> ASTNodeExprIndexing::get_integer_constant() const
{
    return nullopt;
}

optional<long double> ASTNodeExprIndexing::get_float_constant() const
{
    return nullopt;
}

optional<long long> ASTNodeExprFunctionCall::get_integer_constant() const
{
    return nullopt;
}

optional<long double> ASTNodeExprFunctionCall::get_float_constant() const
{
    return nullopt;
}

optional<long long> ASTNodeExprMemberAccess::get_integer_constant() const
{
    return nullopt;
}

optional<long double> ASTNodeExprMemberAccess::get_float_constant() const
{
    return nullopt;
}

optional<long long> ASTNodeExprPointerMemberAccess::get_integer_constant() const
{
    return nullopt;
}

optional<long double> ASTNodeExprPointerMemberAccess::get_float_constant() const
{
    return nullopt;
}

optional<long long> ASTNodeExprInitializer::get_integer_constant() const
{
    return nullopt;
}

optional<long double> ASTNodeExprInitializer::get_float_constant() const
{
    return nullopt;
}

optional<long long> ASTNodeExprUnaryOp::get_integer_constant() const
{
    using OT = ASTNodeExprUnaryOp::UnaryOperatorType;
    switch (this->m_operator) {
        case OT::MINUS:
        {
            auto val = this->m_expr->get_integer_constant();
            if (val.has_value())
                return -val.value();
        } break;
        case OT::PLUS:
            return this->m_expr->get_integer_constant();
        case OT::BITWISE_NOT:
        {
            auto val = this->m_expr->get_integer_constant();
            if (val.has_value())
                return ~val.value();
        } break;
        case OT::LOGICAL_NOT:
        {
            auto val = this->m_expr->get_integer_constant();
            if (val.has_value())
                return !val.value();
        } break;
        case OT::SIZEOF:
        {
            auto type = this->m_expr->type();
            return type->opsizeof();
        } break;
        default:
            break;
    }
    return nullopt;
}

optional<long double> ASTNodeExprUnaryOp::get_float_constant() const
{
    using OT = ASTNodeExprUnaryOp::UnaryOperatorType;
    switch (this->m_operator) {
        case OT::MINUS:
        {
            auto val = this->m_expr->get_float_constant();
            if (val.has_value())
                return -val.value();
        } break;
        case OT::PLUS:
            return this->m_expr->get_float_constant();
        case OT::LOGICAL_NOT:
        {
            auto val = this->m_expr->get_float_constant();
            if (val.has_value())
                return !val.value();
        } break;
        default:
            return this->get_integer_constant();
    }
    return nullopt;
}

optional<long long> ASTNodeExprSizeof::get_integer_constant() const
{
    return this->m_type->opsizeof();
}

optional<long double> ASTNodeExprSizeof::get_float_constant() const
{
    return this->get_integer_constant();
}

optional<long long> ASTNodeExprCast::get_integer_constant() const
{
    if (this->m_type->basic_type() != variable_basic_type::INT)
        return nullopt;

    auto val = this->m_expr->get_integer_constant();
    auto val2 = this->m_expr->get_float_constant();
    if (!val.has_value() && !val2.has_value())
        return nullopt;

    // TODO
    auto inttype = dynamic_pointer_cast<ASTNodeVariableTypeInt>(this->m_type);
    assert(inttype);
    long long _val = val.has_value() ? val.value() : val2.value();
    if (inttype->is_unsigned())
        _val = (long long unsigned)_val;

    switch (inttype->byte_length()) {
        case 1: 
            return _val & 0xFF;
        case 2:
            return _val & 0xFFFF;
        case 4:
            return _val & 0xFFFFFFFF;
        case 8:
            return _val;
    }

    return nullopt;
}

optional<long double> ASTNodeExprCast::get_float_constant() const
{
    if (this->m_type->basic_type() == variable_basic_type::FLOAT)
        return this->m_expr->get_float_constant();

    return this->get_integer_constant();
}

optional<long long> ASTNodeExprBinaryOp::get_integer_constant() const
{
    auto left = this->m_left->get_integer_constant();
    auto right = this->m_right->get_integer_constant();
    if (left.has_value() && right.has_value()) {
        auto _l = left.value();
        auto _r = right.value();
        using OT = ASTNodeExprBinaryOp::BinaryOperatorType;
        switch (this->m_operator) {
            case OT::PLUS:
                return _l + _r;
            case OT::MINUS:
                return _l - _r;
            case OT::MULTIPLY:
                return _l * _r;
            case OT::DIVISION:
                return _l / _r;
            case OT::REMAINDER:
                return _l % _r;
            case OT::BITWISE_AND:
                return _l & _r;
            case OT::BITWISE_OR:
                return _l | _r;
            case OT::BITWISE_XOR:
                return _l ^ _r;
            case OT::LEFT_SHIFT:
                return _l << _r;
            case OT::RIGHT_SHIFT:
                return _l >> _r;
            case OT::LESS_THAN:
                return _l < _r;
            case OT::LESS_THAN_EQUAL:
                return _l <= _r;
            case OT::GREATER_THAN:
                return _l > _r;
            case OT::GREATER_THAN_EQUAL:
                return _l >= _r;
            case OT::EQUAL:
                return _l == _r;
            case OT::NOT_EQUAL:
                return _l != _r;
            case OT::LOGICAL_AND:
                return _l && _r;
            case OT::LOGICAL_OR:
                return _l || _r;
            default:
                return nullopt;
        }
    }

    auto left2 = this->m_left->get_float_constant();
    auto right2 = this->m_right->get_float_constant();
    if (left2.has_value() && right2.has_value()) {
        auto _l = left2.value();
        auto _r = right2.value();
        using OT = ASTNodeExprBinaryOp::BinaryOperatorType;
        switch (this->m_operator) {
            case OT::LESS_THAN:
                return _l < _r;
            case OT::LESS_THAN_EQUAL:
                return _l <= _r;
            case OT::GREATER_THAN:
                return _l > _r;
            case OT::GREATER_THAN_EQUAL:
                return _l >= _r;
            case OT::EQUAL:
                return _l == _r;
            case OT::NOT_EQUAL:
                return _l != _r;
            case OT::LOGICAL_AND:
            default:
                return nullopt;
        }
    }

    return nullopt;
}

optional<long double> ASTNodeExprBinaryOp::get_float_constant() const
{
    auto left = this->m_left->get_float_constant();
    auto right = this->m_right->get_integer_constant();
    if (!left.has_value() || !right.has_value())
        return nullopt;

    auto _l = left.value();
    auto _r = right.value();
    using OT = ASTNodeExprBinaryOp::BinaryOperatorType;
    switch (this->m_operator) {
        case OT::PLUS:
            return _l + _r;
        case OT::MINUS:
            return _l - _r;
        case OT::MULTIPLY:
            return _l * _r;
        case OT::DIVISION:
            return _l / _r;
        case OT::LESS_THAN:
            return _l < _r;
        case OT::LESS_THAN_EQUAL:
            return _l <= _r;
        case OT::GREATER_THAN:
            return _l > _r;
        case OT::GREATER_THAN_EQUAL:
            return _l >= _r;
        case OT::EQUAL:
            return _l == _r;
        case OT::NOT_EQUAL:
            return _l != _r;
        case OT::LOGICAL_AND:
            return _l && _r;
        case OT::LOGICAL_OR:
            return _l || _r;
        default:
            return this->get_integer_constant();
    }

    return nullopt;
}

optional<long long> ASTNodeExprConditional::get_integer_constant() const
{
    auto cond = this->m_cond->get_integer_constant();
    if (!cond.has_value())
        return nullopt;

    if (cond.value())
        return this->m_true->get_integer_constant();
    else
        return this->m_false->get_integer_constant();
}

optional<long double> ASTNodeExprConditional::get_float_constant() const
{
    auto cond = this->m_cond->get_integer_constant();
    if (!cond.has_value())
        return nullopt;

    if (cond.value())
        return this->m_true->get_float_constant();
    else
        return this->m_false->get_float_constant();
}

optional<long long> ASTNodeExprList::get_integer_constant() const
{
    return nullopt;
}

optional<long double> ASTNodeExprList::get_float_constant() const
{
    return nullopt;
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
        case OP::PREFIX_INC:
        case OP::PREFIX_DEC:
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
