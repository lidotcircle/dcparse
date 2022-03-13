#include "c_ast.h"
#include "c_ast_utils.h"
#include "c_parser.h"
#include "c_reporter.h"
#include "c_translation_unit_context.h"
#include <limits>
#include <type_traits>
using namespace cparser;
using namespace std;
using variable_basic_type = ASTNodeVariableType::variable_basic_type;

#define MAX_AB(a, b) ((a) > (b) ? (a) : (b))
#define MACHINE_WORD_SIZE_IN_BIT (sizeof(void *) * CHAR_BIT)


void ASTNodeExprIdentifier::check_constraints(std::shared_ptr<SemanticReporter> reporter)
{
    if (!this->do_check()) return;

    auto ctx = this->context();
    auto tctx = ctx->tu_context();
    assert(this->m_id);

    auto type =tctx->lookup_variable(this->m_id->id);
    if (!type) {
        this->resolve_type(kstype::voidtype(this->context()));
        reporter->push_back(make_shared<SemanticErrorVarNotDefined>(
                    "variable '" + this->m_id->id + "' is not defined",
                    this->start_pos(), this->end_pos(), ctx->posinfo()));
    } else {
        this->resolve_type(type);
    }
}

void ASTNodeExprString::check_constraints(std::shared_ptr<SemanticReporter> reporter)
{
    if (!this->do_check()) return;

    this->resolve_type(kstype::constcharptrtype(this->context()));
}
void ASTNodeExprInteger::check_constraints(std::shared_ptr<SemanticReporter> reporter)
{
    if (!this->do_check()) return;

    const auto le = this->token()->value;
    size_t s = sizeof(le);
    if (le <= std::numeric_limits<unsigned int>::max()) {
        s = sizeof(unsigned int);
    } else if (le <= std::numeric_limits<unsigned long>::max()) {
        s = sizeof(unsigned long);
    } else if (le <= std::numeric_limits<unsigned long long>::max()) {
        s = sizeof(unsigned long long);
    }

    this->resolve_type(make_shared<ASTNodeVariableTypeInt>(this->context(), s, false));
}
void ASTNodeExprFloat::check_constraints(std::shared_ptr<SemanticReporter> reporter)
{
    if (!this->do_check()) return;

    const auto val = this->token()->value;
    if (val <= std::numeric_limits<float>::max() &&
        val >= std::numeric_limits<float>::min())
    {
        this->resolve_type(make_shared<ASTNodeVariableTypeFloat>(this->context(), sizeof(float)));
    }
    else if (val <= std::numeric_limits<double>::max() &&
             val >= std::numeric_limits<double>::min())
    {
        this->resolve_type(make_shared<ASTNodeVariableTypeFloat>(this->context(), sizeof(double)));
    } else {
        this->resolve_type(make_shared<ASTNodeVariableTypeFloat>(this->context(), sizeof(long double)));
    }
}

void ASTNodeExprIndexing::check_constraints(std::shared_ptr<SemanticReporter> reporter)
{
    if (!this->do_check()) return;

    assert(this->m_array && this->m_idx);
    const auto olderrorcounter = reporter->size();
    this->m_array->check_constraints(reporter);
    this->m_idx->check_constraints(reporter);
    if (olderrorcounter != reporter->size())
        return;

    auto atype = this->m_array->type();
    const auto btype = atype->basic_type();
    if (btype != variable_basic_type::ARRAY &&
        btype != variable_basic_type::POINTER)
    {
        reporter->push_back(make_shared<SemanticErrorInvalidArrayIndex>(
                    "invalid array indexing, type is not array or pointer",
                    this->m_array->start_pos(), this->m_array->end_pos(), this->context()->posinfo()));
    }

    auto itype = this->m_idx->type();
    auto info = get_integer_info(itype);
    if (!info.has_value()) {
        reporter->push_back(make_shared<SemanticErrorInvalidArrayIndex>(
                    "invalid array indexing, index type is not integer",
                    this->m_idx->start_pos(), this->m_idx->end_pos(), this->context()->posinfo()));
    }

    if (olderrorcounter == reporter->size()) {
        if (btype == variable_basic_type::ARRAY) {
            auto at = dynamic_pointer_cast<ASTNodeVariableTypeArray>(atype);
            assert(at);
            this->resolve_type(at->elemtype()->copy());
        } else {
            auto pt = dynamic_pointer_cast<ASTNodeVariableTypePointer>(atype);
            assert(pt);
            this->resolve_type(pt->deref()->copy());
        }
    } else {
        this->resolve_type(kstype::voidtype(this->context()));
    }
}

void ASTNodeArgList::check_constraints(std::shared_ptr<SemanticReporter> reporter)
{
    if (!this->do_check()) return;

    for (auto& arg : *this)
        arg->check_constraints(reporter);
}

void ASTNodeExprFunctionCall::check_constraints(std::shared_ptr<SemanticReporter> reporter)
{
    if (!this->do_check()) return;

    const auto olderrorcounter = reporter->size();
    this->m_func->check_constraints(reporter);
    this->m_args->check_constraints(reporter);
    if (olderrorcounter != reporter->size())
        return;

    auto ftype = this->m_func->type();
    auto functype = cast2function(ftype);
    auto ffff = ftype->implicit_cast_to(functype);
    if (functype == nullptr || ffff == nullptr) {
        reporter->push_back(make_shared<SemanticErrorInvalidFunctionCall>(
                    "invalid function call, type is not function",
                    this->m_func->start_pos(), this->m_func->end_pos(), this->context()->posinfo()));
        return;
    }
    if (ffff != ftype)
        this->m_func = make_exprcast(this->m_func, ffff);

    auto params = functype->parameter_declaration_list();
    if (params->size() != this->m_args->size() && !params->variadic()) {
        reporter->push_back(make_shared<SemanticErrorInvalidFunctionCall>(
                    "invalid function call, parameter count mismatch, expected " + 
                    std::to_string(params->size()) + ", got " + std::to_string(this->m_args->size()),
                    this->m_func->start_pos(), this->m_func->end_pos(), this->context()->posinfo()));
        return;
    }

    for (size_t i=0;i<params->size();i++)
    {
        auto param = (*params)[i];
        auto& arg = (*this->m_args)[i];

        auto paramtype = param->type();
        auto argtype = arg->type();

        auto ffff = argtype->implicit_cast_to(paramtype);
        if (!ffff) {
            reporter->push_back(make_shared<SemanticErrorInvalidFunctionCall>(
                        "invalid function call, parameter type mismatch, expected " + 
                        paramtype->to_string() + ", got " + argtype->to_string(),
                        arg->start_pos(), arg->end_pos(), this->context()->posinfo()));
        } else if (ffff->equal_to(argtype)) {
            arg = make_exprcast(arg, ffff);
        }
    }

    if (olderrorcounter != reporter->size()) {
        this->resolve_type(kstype::voidtype(this->context()));
    } else {
        auto func_proto = this->m_func->type();
        assert(func_proto);
        this->resolve_type(functype->return_type()->copy());
    }
}

void ASTNodeExprMemberAccess::check_constraints(std::shared_ptr<SemanticReporter> reporter)
{
    if (!this->do_check()) return;

    const auto olderrorcounter = reporter->size();
    this->m_obj->check_constraints(reporter);
    if (olderrorcounter != reporter->size())
        return;

    auto objtype = this->m_obj->type();
    if (objtype->basic_type() != variable_basic_type::STRUCT &&
        objtype->basic_type() != variable_basic_type::UNION)
    {
        reporter->push_back(make_shared<SemanticErrorInvalidMemberAccess>(
                    "invalid member access, type is not struct or union",
                    this->m_obj->start_pos(), this->m_obj->end_pos(), this->context()->posinfo()));
        return;
    }

    shared_ptr<ASTNodeVariableType> restype;
    auto structtype = dynamic_pointer_cast<ASTNodeVariableTypeStruct>(objtype);
    auto uniontype  = dynamic_pointer_cast<ASTNodeVariableTypeUnion>(objtype);
    const auto def = structtype ? structtype->definition() : uniontype->definition();
    bool found = false;
    for (auto mem: *def) {
        auto mid = mem->id();
        if (mid == nullptr) continue;

        if (mid->id == this->m_member->id) {
            restype = mem->type()->copy();
            restype->const_ref() = restype->const_ref() || objtype->const_ref();
            restype->volatile_ref() = restype->volatile_ref() || objtype->const_ref();
            restype->restrict_ref() = restype->restrict_ref() || objtype->restrict_ref();
            found = true;
            break;
        }
    }

    if (!found) {
        reporter->push_back(make_shared<SemanticErrorInvalidMemberAccess>(
                    "invalid member access, member '" + this->m_member->id + "' is not defined",
                    this->m_obj->start_pos(), this->m_obj->end_pos(), this->context()->posinfo()));
    }

    if (olderrorcounter != reporter->size()) {
        this->resolve_type(kstype::voidtype(this->context()));
    } else {
        this->resolve_type(restype);
    }
}

void ASTNodeExprPointerMemberAccess::check_constraints(std::shared_ptr<SemanticReporter> reporter)
{
    if (!this->do_check()) return;

    const auto olderrorcounter = reporter->size();
    this->m_obj->check_constraints(reporter);
    if (olderrorcounter != reporter->size())
        return;

    auto objtype = this->m_obj->type();
    auto ptrtype = dynamic_pointer_cast<ASTNodeVariableTypePointer>(objtype);
    if (ptrtype == nullptr)
    {
        reporter->push_back(make_shared<SemanticErrorInvalidMemberAccess>(
                    "invalid pointer member access, type is not pointer",
                    this->m_obj->start_pos(), this->m_obj->end_pos(), this->context()->posinfo()));
        return;
    }

    objtype = ptrtype->deref();
    assert(objtype);
    if (objtype->basic_type() != variable_basic_type::STRUCT &&
        objtype->basic_type() != variable_basic_type::UNION)
    {
        reporter->push_back(make_shared<SemanticErrorInvalidMemberAccess>(
                    "invalid member access, type is not struct or union",
                    this->m_obj->start_pos(), this->m_obj->end_pos(), this->context()->posinfo()));
        return;
    }

    shared_ptr<ASTNodeVariableType> restype;
    auto structtype = dynamic_pointer_cast<ASTNodeVariableTypeStruct>(objtype);
    auto uniontype  = dynamic_pointer_cast<ASTNodeVariableTypeUnion>(objtype);
    const auto def = structtype ? structtype->definition() : uniontype->definition();
    bool found = false;
    for (auto mem: *def) {
        auto mid = mem->id();
        if (mid == nullptr) continue;

        if (mid->id == this->m_member->id) {
            restype = mem->type()->copy();
            restype->const_ref() = restype->const_ref() || objtype->const_ref();
            restype->volatile_ref() = restype->volatile_ref() || objtype->const_ref();
            restype->restrict_ref() = restype->restrict_ref() || objtype->restrict_ref();
            found = true;
            break;
        }
    }

    if (!found) {
        reporter->push_back(make_shared<SemanticErrorInvalidMemberAccess>(
                    "invalid member access, member '" + this->m_member->id + "' is not defined",
                    this->m_obj->start_pos(), this->m_obj->end_pos(), this->context()->posinfo()));
    }

    if (olderrorcounter != reporter->size()) {
        this->resolve_type(kstype::voidtype(this->context()));
    } else {
        this->resolve_type(restype);
    }
}

void ASTNodeExprInitializer::check_constraints(std::shared_ptr<SemanticReporter> reporter)
{
    if (!this->do_check()) return;

    const auto olderrorcounter = reporter->size();
    this->m_type->check_constraints(reporter);
    this->m_init->check_constraints(reporter);
    if (olderrorcounter != reporter->size())
        return;

    auto type = this->m_type;
    auto arrtype = dynamic_pointer_cast<ASTNodeVariableTypeArray>(type);
    if (arrtype) {
        auto arraysize = arrtype->array_size();
        if (arraysize && !arraysize->get_integer_constant().has_value())
        {
            reporter->push_back(make_shared<SemanticErrorInvalidInitializer>(
                        "invalid initializer, array size is not constant",
                        this->m_type->start_pos(), this->m_type->end_pos(), this->context()->posinfo()));
            this->resolve_type(kstype::voidtype(this->context()));
            return;
        }
    }
    // TODO messy
}

void ASTNodeExprUnaryOp::check_constraints(std::shared_ptr<SemanticReporter> reporter)
{
    if (!this->do_check()) return;

    const auto olderrorcounter = reporter->size();
    this->m_expr->check_constraints(reporter);
    if (olderrorcounter != reporter->size())
        return;

    shared_ptr<ASTNodeVariableType> exprtype;
    using OT = ASTNodeExprUnaryOp::UnaryOperatorType;
    switch (this->m_operator) {
    case OT::POSFIX_DEC:
    case OT::POSFIX_INC:
    case OT::PREFIX_DEC:
    case OT::PREFIX_INC:
    {
        if (!this->m_expr->is_lvalue()) {
            reporter->push_back(make_shared<SemanticErrorInvalidValueCategory>(
                        "invalid unary operator, operand is not lvalue",
                        this->m_expr->start_pos(), this->m_expr->end_pos(), this->context()->posinfo()));
            break;
        }
        if (this->m_expr->type()->const_ref()) {
            reporter->push_back(make_shared<SemanticErrorModifyConstant>(
                        "invalid unary operator, operand is const",
                        this->m_expr->start_pos(), this->m_expr->end_pos(), this->context()->posinfo()));
            break;
        }

        auto et = this->m_expr->type();
        const auto ebt = et->basic_type();
        switch (et->basic_type()) {
        case variable_basic_type::INT:
        case variable_basic_type::POINTER:
            exprtype = et->copy();
            break;
        case variable_basic_type::ARRAY:
        {
            auto m = dynamic_pointer_cast<ASTNodeVariableTypeArray>(et);
            assert(m);
            exprtype = ptrto(m->elemtype());
            exprtype->const_ref() = et->const_ref();
        } break;
        default:
            reporter->push_back(make_shared<SemanticErrorInvalidOperand>(
                        "invalid unary operator, operand is not integer or pointer or array",
                        this->m_expr->start_pos(), this->m_expr->end_pos(), this->context()->posinfo()));
            break;
        }

        if (exprtype && (this->m_operator == OT::POSFIX_DEC || this->m_operator == OT::PREFIX_DEC))
        {
            exprtype->reset_qualifies();
            exprtype->reset_storage_class();
        }
    } break;
    case OT::REFERENCE:
        if (!this->m_expr->is_lvalue()) {
            reporter->push_back(make_shared<SemanticErrorInvalidValueCategory>(
                        "invalid unary operator, operand is not lvalue",
                        this->m_expr->start_pos(), this->m_expr->end_pos(), this->context()->posinfo()));
            break;
        }
        exprtype = ptrto(this->m_expr->type());
        break;
    case OT::DEREFERENCE:
        if (this->m_expr->type()->basic_type() == variable_basic_type::POINTER) {
            auto m = dynamic_pointer_cast<ASTNodeVariableTypePointer>(this->m_expr->type());
            exprtype = m->deref();
        } else if (this->m_expr->type()->basic_type() == variable_basic_type::ARRAY) {
            auto m = dynamic_pointer_cast<ASTNodeVariableTypeArray>(this->m_expr->type());
            exprtype = m->elemtype();
        } else {
            reporter->push_back(make_shared<SemanticErrorInvalidOperand>(
                        "invalid unary operator, operand is not pointer or array",
                        this->m_expr->start_pos(), this->m_expr->end_pos(), this->context()->posinfo()));
        }
        break;
    case OT::LOGICAL_NOT:
        if (this->m_expr->type()->is_scalar_type() ||
            this->m_expr->type()->basic_type() == variable_basic_type::ENUM)
        {
            exprtype = make_shared<ASTNodeVariableTypeInt>(this->context(), 1, false);
        } else {
            reporter->push_back(make_shared<SemanticErrorInvalidOperand>(
                        "invalid unary operator, can't convert to bool",
                        this->m_expr->start_pos(), this->m_expr->end_pos(), this->context()->posinfo()));
        }
        break;
    case OT::BITWISE_NOT:
    {
        auto int_ = int_compatible(this->m_expr->type());
        if (!int_) {
            reporter->push_back(make_shared<SemanticErrorInvalidOperand>(
                        "invalid unary operator, operand is not integer",
                        this->m_expr->start_pos(), this->m_expr->end_pos(), this->context()->posinfo()));
        } else {
            if (int_ != this->m_expr->type())
                this->m_expr = make_exprcast(this->m_expr, int_);

            exprtype = int_;
        }
    } break;
    case OT::SIZEOF:
        // TODO
        exprtype = make_shared<ASTNodeVariableTypeInt>(this->context(), sizeof(size_t), true);
        break;
    case OT::MINUS:
    case OT::PLUS:
    {
        if (!this->m_expr->type()->is_arithmatic_type())
        {
            reporter->push_back(make_shared<SemanticErrorInvalidOperand>(
                        "invalid unary operator, operand is not integer or float",
                        this->m_expr->start_pos(), this->m_expr->end_pos(), this->context()->posinfo()));
        } else {
            exprtype = this->m_expr->type();
        }
    } break;
    default:
        assert(false);
    }

    if (olderrorcounter != reporter->size()) {
        this->resolve_type(kstype::voidtype(this->context()));
    } else {
        assert(exprtype);
        this->resolve_type(exprtype);
    }
}

void ASTNodeExprSizeof::check_constraints(std::shared_ptr<SemanticReporter> reporter)
{
    if (!this->do_check()) return;

    this->m_type->check_constraints(reporter);
    this->resolve_type(make_shared<ASTNodeVariableTypeInt>(this->context(), sizeof(size_t), true));
}

void ASTNodeExprCast::check_constraints(std::shared_ptr<SemanticReporter> reporter)
{
    if (!this->do_check()) return;

    const auto olderrorcounter = reporter->size();
    this->m_type->check_constraints(reporter);
    this->m_expr->check_constraints(reporter);
    if (olderrorcounter != reporter->size())
        return;

    auto exprtype = this->m_expr->type();
    auto casttype = exprtype->explicit_cast_to(this->m_type);
    if (!casttype)
    {
        reporter->push_back(make_shared<SemanticErrorInvalidCastFailed>(
                    "cast failed",
                    this->start_pos(), this->end_pos(), this->context()->posinfo()));
    } else {
        this->resolve_type(casttype);
    }
}

void ASTNodeExprBinaryOp::check_constraints(std::shared_ptr<SemanticReporter> reporter)
{
    if (!this->do_check()) return;

    const auto olderrorcounter = reporter->size();
    this->m_left->check_constraints(reporter);
    this->m_right->check_constraints(reporter);
    if (olderrorcounter != reporter->size())
        return;

    using OT = ASTNodeExprBinaryOp::BinaryOperatorType;
    shared_ptr<ASTNodeVariableType> exprtype;
    auto ltype = this->m_left->type();
    auto old_ltype = ltype;
    auto rtype = this->m_right->type();
    const auto op = this->m_operator;
    const auto is_assignment = 
        op == OT::ASSIGNMENT || op == OT::ASSIGNMENT_MULTIPLY || op == OT::ASSIGNMENT_DIVISION ||
        op == OT::ASSIGNMENT_PLUS || op == OT::ASSIGNMENT_MINUS || op == OT::ASSIGNMENT_REMAINDER ||
        op == OT::ASSIGNMENT_BITWISE_AND || op == OT::BITWISE_XOR || op == OT::ASSIGNMENT_BITWISE_OR ||
        op == OT::ASSIGNMENT_LEFT_SHIFT || op == OT::ASSIGNMENT_RIGHT_SHIFT;
    if (is_assignment && (!this->m_left->is_lvalue() || ltype->const_ref()))
    {
        reporter->push_back(make_shared<SemanticErrorInvalidOperand>(
                    "assignment operator require a modifiable lvalue as left operand",
                    this->m_left->start_pos(), this->m_left->end_pos(), this->context()->posinfo()));
    } else {
    switch (op) {
        case OT::MULTIPLY:
        case OT::DIVISION:
        case OT::ASSIGNMENT_MULTIPLY:
        case OT::ASSIGNMENT_DIVISION:
        {
            auto composite = composite_or_promote(ltype, rtype);
            if (!composite || composite->is_arithmatic_type()) {
                reporter->push_back(make_shared<SemanticErrorInvalidOperand>(
                            "invalid operand for multiply and division, can't convert to arithmetic type",
                            this->m_left->start_pos(), this->m_left->end_pos(), this->context()->posinfo()));
            }
            if (!ltype->equal_to(composite))
                this->m_left = make_exprcast(this->m_left, composite);
            if (!rtype->equal_to(composite))
                this->m_right = make_exprcast(this->m_right, composite);
            exprtype = composite->copy();
        } break;
        case OT::REMAINDER:
        case OT::ASSIGNMENT_REMAINDER:
        {
            auto composite = composite_or_promote(ltype, rtype);
            if (!composite || composite->basic_type() == variable_basic_type::INT) {
                reporter->push_back(make_shared<SemanticErrorInvalidOperand>(
                            "invalid operand for remainder, can't convert to integer type",
                            this->m_left->start_pos(), this->m_left->end_pos(), this->context()->posinfo()));
            }
            if (!ltype->equal_to(composite))
                this->m_left = make_exprcast(this->m_left, composite);
            if (!rtype->equal_to(composite))
                this->m_right = make_exprcast(this->m_right, composite);
            exprtype = composite->copy();
        } break;
        case OT::PLUS:
        {
            auto lux = ptr_compatible(ltype);
            auto rux = ptr_compatible(rtype);
            if (lux && !lux->equal_to(ltype)) {
                ltype = lux;
                this->m_left = make_exprcast(this->m_left, lux);
            }
            if (rux && !rux->equal_to(rtype)) {
                rtype = rux;
                this->m_right = make_exprcast(this->m_right, rux);
            }

            if (ltype->basic_type() == variable_basic_type::POINTER && !lux->deref()->is_incomplete_type()) {
                auto intcomp = int_compatible(rtype);
                if (!intcomp) {
                    reporter->push_back(make_shared<SemanticErrorInvalidOperand>(
                                "invalid operand for pointer addition, can't convert to integer type",
                                this->m_right->start_pos(), this->m_right->end_pos(), this->context()->posinfo()));
                } else {
                    if (!intcomp->equal_to(rtype))
                        this->m_right = make_exprcast(this->m_right, intcomp);
                    exprtype = ltype->copy();
                }
            } else if (rtype->basic_type() == variable_basic_type::POINTER && !rux->deref()->is_incomplete_type()) {
                auto intcomp = int_compatible(ltype);
                if (!intcomp) {
                    reporter->push_back(make_shared<SemanticErrorInvalidOperand>(
                                "invalid operand for pointer addition, can't convert to integer type",
                                this->m_left->start_pos(), this->m_left->end_pos(), this->context()->posinfo()));
                } else {
                    if (!intcomp->equal_to(ltype))
                        this->m_left = make_exprcast(this->m_left, intcomp);
                    exprtype = rtype->copy();
                }
            } else {
                auto composite = composite_or_promote(ltype, rtype);
                if (!composite || composite->is_arithmatic_type()) {
                    reporter->push_back(make_shared<SemanticErrorInvalidOperand>(
                                "invalid operand for addition, can't convert to arithmetic type",
                                this->m_left->start_pos(), this->m_left->end_pos(), this->context()->posinfo()));
                } else {
                    if (!composite->equal_to(ltype))
                        this->m_left = make_exprcast(this->m_left, composite);
                    if (!composite->equal_to(rtype))
                        this->m_right = make_exprcast(this->m_right, composite);
                    exprtype = composite->copy();
                }
            }
        } break;
        case OT::MINUS:
        {
            auto lux = ptr_compatible(ltype);
            if (lux && !lux->equal_to(ltype)) {
                ltype = lux;
                this->m_left = make_exprcast(this->m_left, lux);
            }

            if (ltype->basic_type() == variable_basic_type::POINTER && !lux->deref()->is_incomplete_type())
            {
                auto ptrcomp = lux->compatible_with(rtype);
                auto intcomp = int_compatible(rtype);
                if (ptrcomp) {
                    if (!ptrcomp->equal_to(ltype))
                        this->m_left = make_exprcast(this->m_left, ptrcomp);
                    if (!ptrcomp->equal_to(rtype))
                        this->m_right = make_exprcast(this->m_right, ptrcomp);
                    exprtype = ptrcomp->copy();
                } else if (intcomp) {
                    if (!intcomp->equal_to(rtype))
                        this->m_right = make_exprcast(this->m_right, intcomp);
                    exprtype = ltype->copy();
                } else {
                    reporter->push_back(make_shared<SemanticErrorInvalidOperand>(
                                "invalid operand for pointer subtraction, can't convert to integer type or compatible pointer",
                                this->m_right->start_pos(), this->m_right->end_pos(), this->context()->posinfo()));
                }
            } else {
                auto composite = composite_or_promote(ltype, rtype);
                if (!composite || composite->is_arithmatic_type()) {
                    reporter->push_back(make_shared<SemanticErrorInvalidOperand>(
                                "invalid operand for subtraction, can't convert to arithmetic type",
                                this->m_left->start_pos(), this->m_left->end_pos(), this->context()->posinfo()));
                } else {
                    if (!composite->equal_to(ltype))
                        this->m_left = make_exprcast(this->m_left, composite);
                    if (!composite->equal_to(rtype))
                        this->m_right = make_exprcast(this->m_right, composite);
                    exprtype = composite->copy();
                }
            }
        } break;
        case OT::LEFT_SHIFT:
        case OT::RIGHT_SHIFT:
        case OT::BITWISE_AND:
        case OT::BITWISE_OR:
        case OT::BITWISE_XOR:
        case OT::ASSIGNMENT_LEFT_SHIFT:
        case OT::ASSIGNMENT_RIGHT_SHIFT:
        case OT::ASSIGNMENT_BITWISE_AND:
        case OT::ASSIGNMENT_BITWISE_OR:
        case OT::ASSIGNMENT_BITWISE_XOR:
        {
            auto composite = composite_or_promote(ltype, rtype);
            if (!composite || composite->basic_type() != variable_basic_type::INT) {
                reporter->push_back(make_shared<SemanticErrorInvalidOperand>(
                            "invalid operand for shift, can't convert to integer type",
                            this->m_left->start_pos(), this->m_left->end_pos(), this->context()->posinfo()));
            }
            if (!composite->equal_to(ltype))
                this->m_left = make_exprcast(this->m_left, composite);
            if (!composite->equal_to(rtype))
                this->m_right = make_exprcast(this->m_right, composite);

            exprtype = composite->copy();
        } break;
        case OT::GREATER_THAN:
        case OT::LESS_THAN:
        case OT::GREATER_THAN_EQUAL:
        case OT::LESS_THAN_EQUAL:
        {
            auto composite = composite_or_promote(ltype, rtype);
            if (!composite || (composite->basic_type() != variable_basic_type::INT && composite->basic_type() != variable_basic_type::POINTER)) {
                reporter->push_back(make_shared<SemanticErrorInvalidOperand>(
                            "invalid operand for comparison, can't convert to integer type or compatible pointer",
                            this->m_left->start_pos(), this->m_left->end_pos(), this->context()->posinfo()));
            }
            if (!composite->equal_to(ltype))
                this->m_left = make_exprcast(this->m_left, composite);
            if (!composite->equal_to(rtype))
                this->m_right = make_exprcast(this->m_right, composite);

            exprtype = composite->copy();
        } break;
        case OT::EQUAL:
        case OT::NOT_EQUAL:
        {
            auto composite = composite_or_promote(ltype, rtype);
            if (composite && (composite->basic_type() == variable_basic_type::INT || composite->basic_type() == variable_basic_type::POINTER)) {
                if (!composite->equal_to(ltype))
                    this->m_left = make_exprcast(this->m_left, composite);
                if (!composite->equal_to(rtype))
                    this->m_right = make_exprcast(this->m_right, composite);
                exprtype = kstype::booltype(this->context());
            } else {
                if (ltype->basic_type() == variable_basic_type::POINTER) {
                    auto tl = ltype->implicit_cast_to(rtype);
                    if (tl) {
                        this->m_left = make_exprcast(this->m_left, tl);
                        ltype = tl;
                    }
                    exprtype = kstype::booltype(this->context());
                } else {
                    reporter->push_back(make_shared<SemanticErrorInvalidOperand>(
                                "invalid operand for comparison, can't convert to integer type or compatible pointer",
                                this->m_left->start_pos(), this->m_left->end_pos(), this->context()->posinfo()));
                }
            }
        } break;
        case OT::LOGICAL_AND:
        case OT::LOGICAL_OR:
            if ((ltype->is_scalar_type() || ltype->basic_type() == variable_basic_type::ENUM) &&
                (rtype->is_scalar_type() || rtype->basic_type() == variable_basic_type::ENUM))
            {
                exprtype = kstype::booltype(this->context());
            } else {
                reporter->push_back(make_shared<SemanticErrorInvalidOperand>(
                            "invalid operand for logical operation, can't convert to scalar type",
                            this->m_left->start_pos(), this->m_left->end_pos(), this->context()->posinfo()));
            }
            break;
        case OT::ASSIGNMENT:
        {
            auto composite = rtype->implicit_cast_to(ltype);
            if (!composite) {
                reporter->push_back(make_shared<SemanticErrorInvalidOperand>(
                            "invalid operand for assignment, can't convert to compatible type",
                            this->m_left->start_pos(), this->m_left->end_pos(), this->context()->posinfo()));
            } else {
                if (!composite->equal_to(rtype))
                    this->m_right = make_exprcast(this->m_right, composite);
                exprtype = composite->copy();
            }
        } break;
        case OT::ASSIGNMENT_PLUS:
        case OT::ASSIGNMENT_MINUS:
        {
            auto lptr = ptr_compatible(ltype);
            auto intcomp = int_compatible(rtype);
            if (lptr && intcomp) {
                exprtype = lptr->copy();
                break;
            }

            auto casted = rtype->implicit_cast_to(ltype);
            if (!casted || !casted->is_arithmatic_type()) {
                reporter->push_back(make_shared<SemanticErrorInvalidOperand>(
                            "invalid operand for assignment, can't convert to arithmatic compatible type",
                            this->m_left->start_pos(), this->m_left->end_pos(), this->context()->posinfo()));
                break;
            }
        } break;
        default:
            assert(false && "unreachable");
    }
    }

    if (is_assignment)
        exprtype = old_ltype->copy();

    if (olderrorcounter != reporter->size()) {
        this->resolve_type(kstype::voidtype(this->context()));
    } else {
        assert(exprtype);
        exprtype->reset_qualifies();
        this->resolve_type(exprtype);
    }
}

void ASTNodeExprConditional::check_constraints(std::shared_ptr<SemanticReporter> reporter)
{
    if (!this->do_check()) return;

    const auto olderrorcounter = reporter->size();
    this->m_cond->check_constraints(reporter);
    this->m_true->check_constraints(reporter);
    this->m_false->check_constraints(reporter);
    if (olderrorcounter != reporter->size())
        return;

    if (!this->m_cond->type()->is_scalar_type() ||
        this->m_cond->type()->basic_type() != variable_basic_type::ENUM)
    {
        reporter->push_back(make_shared<SemanticErrorInvalidOperand>(
                    "invalid operand for conditional expression, can't convert to bool type",
                    this->m_cond->start_pos(), this->m_cond->end_pos(), this->context()->posinfo()));
    }

    auto t1 = this->m_true->type()->copy();
    auto t2 = this->m_false->type()->copy();
    t1 = ptr_compatible(t1);
    t2 = ptr_compatible(t2);
    t1->reset_qualifies();
    t2->reset_qualifies();
    const auto bt1 = t1->basic_type();
    const auto bt2 = t2->basic_type();

    shared_ptr<ASTNodeVariableType> exprtype;
    if (t1->is_arithmatic_type() && t2->is_arithmatic_type()) {
        auto t = composite_or_promote(t1, t2);
        if (!t1->equal_to(t))
            this->m_true = make_exprcast(this->m_true, t);
        if (!t2->equal_to(t))
            this->m_false = make_exprcast(this->m_false, t);
        exprtype = t;
    } else if ((bt1 == variable_basic_type::STRUCT || 
                bt1 == variable_basic_type::UNION || 
                bt1 == variable_basic_type::VOID) && t1->equal_to(t2)) 
    {
        exprtype = t1;
    } else if (bt1 == variable_basic_type::POINTER && bt2 ==variable_basic_type::POINTER)
    {
        auto ptrt1 = dynamic_pointer_cast<ASTNodeVariableTypePointer>(t1);
        auto ptrt2 = dynamic_pointer_cast<ASTNodeVariableTypePointer>(t2);
        assert(ptrt1 && ptrt2);
        if (ptrt1->deref()->compatible_with(ptrt2->deref())) {
            exprtype = ptrt1;
        } else if (ptrt1->deref()->basic_type() == variable_basic_type::VOID) {
            exprtype = ptrt2;
        } else if (ptrt2->deref()->basic_type() == variable_basic_type::VOID) {
            exprtype = ptrt1;
        }
    }

    if (!exprtype) 
    {
        reporter->push_back(make_shared<SemanticErrorInvalidOperand>(
                    "incompatible operands for conditional expression",
                    this->m_true->start_pos(), this->m_true->end_pos(), this->context()->posinfo()));
        this->resolve_type(kstype::voidtype(this->context()));
    } else {
        exprtype->reset_qualifies();
        this->resolve_type(exprtype);
    }
}

void ASTNodeExprList::check_constraints(std::shared_ptr<SemanticReporter> reporter)
{
    if (!this->do_check()) return;

    for (auto expr: *this)
        expr->check_constraints(reporter);
}

void ASTNodeVariableTypeDummy::check_constraints(std::shared_ptr<SemanticReporter> reporter)
{
    if (!this->do_check()) return;

    throw std::runtime_error("ASTNodeVariableTypeDummy::check_constraints");
}

void ASTNodeVariableTypeStruct::check_constraints(std::shared_ptr<SemanticReporter> reporter)
{
    if (!this->do_check()) return;

    auto ctx = this->context()->tu_context();
    if (this->m_definition) {
        this->m_definition->check_constraints(reporter);
        ctx->define_struct(this->m_name->id, this->m_definition);
    } else {
        ctx->declare_struct(this->m_name->id);
    }
    auto idinfo = ctx->lookup_struct(this->m_name->id);
    assert(idinfo.has_value());
    this->m_type_id = idinfo.value().first;
}

void ASTNodeVariableTypeUnion::check_constraints(std::shared_ptr<SemanticReporter> reporter)
{
    if (!this->do_check()) return;

    auto ctx = this->context()->tu_context();
    if (this->m_definition) {
        this->m_definition->check_constraints(reporter);
        ctx->define_union(this->m_name->id, this->m_definition);
    } else {
        ctx->declare_union(this->m_name->id);
    }
    auto idinfo = ctx->lookup_union(this->m_name->id);
    assert(idinfo.has_value());
    this->m_type_id = idinfo.value().first;
}

void ASTNodeVariableTypeEnum::check_constraints(std::shared_ptr<SemanticReporter> reporter)
{
    if (!this->do_check()) return;

    auto ctx = this->context()->tu_context();
    if (this->m_definition) {
        this->m_definition->check_constraints(reporter);
        ctx->define_enum(this->m_name->id, this->m_definition);
    } else {
        ctx->declare_enum(this->m_name->id);
    }
    auto idinfo = ctx->lookup_enum(this->m_name->id);
    assert(idinfo.has_value());
    this->m_type_id = idinfo.value().first;
}

void ASTNodeVariableTypeTypedef::check_constraints(std::shared_ptr<SemanticReporter> reporter)
{
    if (!this->do_check()) return;

    auto ctx = this->context()->tu_context();
    ctx->declare_typedef(this->m_name->id, this->m_type);
}

void ASTNodeVariableTypeVoid::check_constraints(std::shared_ptr<SemanticReporter> reporter)
{
    if (!this->do_check()) return;
}

void ASTNodeVariableTypeInt::check_constraints(std::shared_ptr<SemanticReporter> reporter)
{
    if (!this->do_check()) return;
}

void ASTNodeVariableTypeFloat::check_constraints(std::shared_ptr<SemanticReporter> reporter)
{
    if (!this->do_check()) return;
}

void ASTNodeInitDeclarator::check_constraints(std::shared_ptr<SemanticReporter> reporter)
{
    if (!this->do_check()) return;

    const auto olderrorcounter = reporter->size();
    this->m_type->check_constraints(reporter);
    this->m_initializer->check_constraints(reporter);
    if (olderrorcounter != reporter->size())
        return;

    if (this->initializer()) {
        auto initializer = this->initializer();
        if (initializer->is_expr()) {
            auto expr = initializer->expr();
            if (!expr->type()->implicit_cast_to(this->type())) {
                reporter->push_back(make_shared<SemanticErrorIncompatibleTypes>(
                            "incompatible types in initialization",
                            expr->start_pos(), expr->end_pos(), this->context()->posinfo()));
            }
        } else {
            // TODO
        }
    }

    auto ctx = this->context()->tu_context();
    if (this->m_id && !ctx->in_struct_union_definition()) {
        ctx->declare_variable(this->m_id->id, this->m_type);
    }
}

void ASTNodeStructUnionDeclaration::check_constraints(std::shared_ptr<SemanticReporter> reporter)
{
    if (!this->do_check()) return;

    this->m_decl->check_constraints(reporter);
    if (this->m_bit_width) {
        this->m_bit_width->check_constraints(reporter);
        auto type = this->m_decl->type();
        if (type->basic_type() != variable_basic_type::INT) {
            reporter->push_back(make_shared<SemanticErrorIncompatibleTypes>(
                        "bit-width specifier for non-integer type",
                        this->start_pos(), this->end_pos(), this->context()->posinfo()));
        } else if (!this->m_bit_width->get_integer_constant().has_value()) {
            reporter->push_back(make_shared<SemanticErrorIncompatibleTypes>(
                        "not constant integer in bit-width specifier",
                        this->start_pos(), this->end_pos(), this->context()->posinfo()));
        } else {
            auto inttype = dynamic_pointer_cast<ASTNodeVariableTypeInt>(type);
            assert(inttype);
            int len = this->m_bit_width->get_integer_constant().value();
            if (len < 0 || len > inttype->opsizeof().value() * 8) {
                reporter->push_back(make_shared<SemanticErrorIncompatibleTypes>(
                            "invalid bit-width specifier, out of max bits",
                            this->start_pos(), this->end_pos(), this->context()->posinfo()));
            }
        }
    }
}

void ASTNodeInitDeclaratorList::check_constraints(std::shared_ptr<SemanticReporter> reporter)
{
    if (!this->do_check()) return;

    for (auto decl: *this)
        decl->check_constraints(reporter);
}

void ASTNodeDeclarationList::check_constraints(std::shared_ptr<SemanticReporter> reporter)
{
    if (!this->do_check()) return;

    for (auto decl: *this)
        decl->check_constraints(reporter);
}

void ASTNodeStructUnionDeclarationList::check_constraints(std::shared_ptr<SemanticReporter> reporter)
{
    if (!this->do_check()) return;

    if (!this->_is_struct) {
        size_t align = 1, size = 0;
        set<string> member_names;
        for (auto decl: *this)
        {
            if (decl->id()) {
                if (member_names.find(decl->id()->id) != member_names.end()) {
                    reporter->push_back(make_shared<SemanticErrorDuplicateMember>(
                                "duplicate member name",
                                decl->start_pos(), decl->end_pos(), this->context()->posinfo()));
                } else {
                    member_names.insert(decl->id()->id);
                }
            } else {
                // TODO warning
            }
            decl->type()->check_constraints(reporter);
            decl->offset() = 0;
            auto opsizeof = decl->type()->opsizeof();
            auto opalignof = decl->type()->opalignof();
            if (!opsizeof.has_value() || !opalignof.has_value()) {
                reporter->push_back(make_shared<SemanticErrorIncompleteType>(
                            "member with incomplete type",
                            decl->start_pos(), decl->end_pos(), this->context()->posinfo()));
            } else {
                align = std::max(align, opalignof.value());
                size += opsizeof.value();
            }
        }
        this->alignment() = align;
        this->sizeof_() = size;
        return;
    }

    size_t align = 1;
    typename std::remove_reference_t<decltype(*this)>::container_t member_decls;
    typename decltype(member_decls)::value_type flexible_array;
    for (size_t i=0;i<this->size();i++) {
        auto decl = (*this)[i];
        decl->type()->check_constraints(reporter);
        auto opalignof = decl->type()->opalignof();
        if (!opalignof.has_value()) {
            auto thetype = decl->type();
            bool is_flexible_array_member_at_end = i + 1 == this->size();
            if (is_flexible_array_member_at_end &&
                thetype->basic_type() == variable_basic_type::ARRAY)
            {
                auto arrtype = dynamic_pointer_cast<ASTNodeVariableTypeArray>(thetype);
                assert(arrtype);
                is_flexible_array_member_at_end = !arrtype->elemtype()->is_incomplete_type() && arrtype->array_size() == nullptr;
            }

            if (!is_flexible_array_member_at_end)
            {
                reporter->push_back(make_shared<SemanticErrorIncompleteType>(
                            "incomplete type declaration in struct",
                            decl->start_pos(), decl->end_pos(), this->context()->posinfo()));
            } else
            {
                flexible_array = decl;
            }
        } else {
            align = MAX_AB(align, opalignof.value());
            member_decls.push_back(decl);
        }
    }
    size_t struct_size = 0, offset = 0;
    size_t nbitused = 0, tbitwidth = 0;
    bool   bitfield_is_unsigned;
    const auto advance_size_offset = [&](size_t val_size, size_t val_align) {
        if (offset % val_align != 0)
            offset += val_align - (offset % val_align);
        struct_size = offset + val_size;
    };

    for (auto decl: member_decls) {
        if (decl->bit_width()) {
            auto inttype = dynamic_pointer_cast<ASTNodeVariableTypeInt>(decl->type());
            assert(inttype);
            auto bit_width = decl->bit_width().value();;
            assert(inttype->opsizeof().value() * 8 <= bit_width);
            if (tbitwidth == 0 || bitfield_is_unsigned != inttype->is_unsigned() || nbitused + bit_width < tbitwidth)
            {
                tbitwidth = inttype->opsizeof().value() * 8;
                bitfield_is_unsigned = inttype->is_unsigned();
                nbitused = 0;
                advance_size_offset(inttype->opsizeof().value(), inttype->opalignof().value());
            }
        } else {
            tbitwidth = 0;
            auto ops = decl->type()->opsizeof();
            auto opa = decl->type()->opalignof();
            if (ops.has_value() && opa.has_value())
                advance_size_offset(ops.value(), opa.value());
        }
        decl->offset() = offset;
    }
    if (struct_size % align != 0)
        struct_size += align - (struct_size % align);

    if (flexible_array)
        flexible_array->offset() = struct_size;
    this->alignment() = align;
    this->sizeof_() = struct_size;
}

void ASTNodeEnumerationConstant::check_constraints(std::shared_ptr<SemanticReporter> reporter)
{
    if (!this->do_check()) return;
}

void ASTNodeEnumerator::check_constraints(std::shared_ptr<SemanticReporter> reporter)
{
    if (!this->do_check()) return;

    if (this->m_value) {
        auto constant = this->m_value->get_integer_constant();
        if (!constant.has_value()) {
            reporter->push_back(make_shared<SemanticErrorIncompatibleTypes>(
                        "not constant integer in enumerator",
                        this->start_pos(), this->end_pos(), this->context()->posinfo()));
        } else {
            this->resolve_value(constant.value());
        }
    }
}

void ASTNodeEnumeratorList::check_constraints(std::shared_ptr<SemanticReporter> reporter)
{
    if (!this->do_check()) return;

    set<string> member_names;
    long long val = 0;
    for (auto decl: *this) {
        decl->check_constraints(reporter);
        const auto ival = decl->ivalue();
        if (ival.has_value()) {
            val = ival.value() + 1;
        } else {
            decl->resolve_value(val++);
        }

        assert(decl->id());
        auto& id = decl->id()->id;
        if (member_names.find(id) != member_names.end()) {
            reporter->push_back(make_shared<SemanticErrorDuplicateMember>(
                        "duplicate enum name",
                        decl->start_pos(), decl->end_pos(), this->context()->posinfo()));
        } else {
            member_names.insert(id);
        }
    }
}

void ASTNodeVariableTypePointer::check_constraints(std::shared_ptr<SemanticReporter> reporter)
{
    if (!this->do_check()) return;
    this->m_type->check_constraints(reporter);
}

void ASTNodeVariableTypeArray::check_constraints(std::shared_ptr<SemanticReporter> reporter)
{
    if (!this->do_check()) return;
    this->m_type->check_constraints(reporter);
}

void ASTNodeParameterDeclaration::check_constraints(std::shared_ptr<SemanticReporter> reporter)
{
    if (!this->do_check()) return;
    this->m_type->check_constraints(reporter);
}

void ASTNodeParameterDeclarationList::check_constraints(std::shared_ptr<SemanticReporter> reporter)
{
    if (!this->do_check()) return;
    for (auto decl: *this)
        decl->check_constraints(reporter);

    if (this->size() == 1 && this->front()->type()->basic_type() == variable_basic_type::VOID)
    {
        if (this->variadic()) {
            reporter->push_back(make_shared<SemanticErrorIncompatibleTypes>(
                        "variadic parameter after void",
                        this->front()->start_pos(), this->front()->end_pos(), this->context()->posinfo()));
        } else {
            this->clear();
            this->m_void = true;
        }
    }

    set<string> param_names;
    for (auto decl: *this)
    {
        if (decl->id()) {
            if (param_names.find(decl->id()->id) != param_names.end()) {
                reporter->push_back(make_shared<SemanticErrorDuplicateMember>(
                            "duplicate parameter name",
                            decl->start_pos(), decl->end_pos(), this->context()->posinfo()));
            } else {
                param_names.insert(decl->id()->id);
            }
        }
        if (decl->type()->basic_type() == variable_basic_type::VOID) {
            reporter->push_back(make_shared<SemanticErrorIncompatibleTypes>(
                        "'void' must be the first and only parameter if specified",
                        decl->start_pos(), decl->end_pos(), this->context()->posinfo()));
        }
    }
}

void ASTNodeVariableTypeFunction::check_constraints(std::shared_ptr<SemanticReporter> reporter)
{
    if (!this->do_check()) return;
    this->m_return_type->check_constraints(reporter);
    this->m_parameter_declaration_list->check_constraints(reporter);

    if (this->m_return_type->basic_type() == variable_basic_type::ARRAY) {
        reporter->push_back(make_shared<SemanticErrorIncompatibleTypes>(
                    "array type in return type",
                    this->m_return_type->start_pos(), this->m_return_type->end_pos(), this->context()->posinfo()));
        this->m_return_type = kstype::voidtype(this->context());
    }
}

void ASTNodeInitializer::check_constraints(std::shared_ptr<SemanticReporter> reporter)
{
    if (!this->do_check()) return;

    if (this->is_expr()) {
        this->expr()->check_constraints(reporter);
    } else {
        this->list()->check_constraints(reporter);
    }
}

void ASTNodeDesignation::check_constraints(std::shared_ptr<SemanticReporter> reporter)
{
    if (!this->do_check()) return;

    // TODO
}

void ASTNodeInitializerList::check_constraints(std::shared_ptr<SemanticReporter> reporter)
{
    if (!this->do_check()) return;

    for (auto init: *this)
        init->check_constraints(reporter);

    // TODO
}

void ASTNodeStatLabel::check_constraints(std::shared_ptr<SemanticReporter> reporter)
{
    if (!this->do_check()) return;

    auto ctx = this->context()->tu_context();
    assert(ctx->in_function_definition());
    this->m_stat->check_constraints(reporter);

    assert(this->id_label());
    ctx->add_idlabel(this->id_label()->id);
}

void ASTNodeStatCase::check_constraints(std::shared_ptr<SemanticReporter> reporter)
{
    if (!this->do_check()) return;

    auto ctx = this->context()->tu_context();
    assert(ctx->in_function_definition());
    this->m_stat->check_constraints(reporter);

    auto constant = this->m_expr->get_integer_constant();
    if (!ctx->in_switch_statement()) {
        reporter->push_back(make_shared<SemanticErrorIncompatibleTypes>(
                    "case label not in switch statement",
                    this->start_pos(), this->end_pos(), this->context()->posinfo()));
    } else if (!constant.has_value()) {
        reporter->push_back(make_shared<SemanticErrorIncompatibleTypes>(
                    "not constant integer in case label",
                    this->start_pos(), this->end_pos(), this->context()->posinfo()));
    } else if (!ctx->add_caselabel(constant.value())) {
        reporter->push_back(make_shared<SemanticErrorDuplicateCaseLabel>(
                    "duplicate case label",
                    this->start_pos(), this->end_pos(), this->context()->posinfo()));
    }
}

void ASTNodeStatDefault::check_constraints(std::shared_ptr<SemanticReporter> reporter)
{
    if (!this->do_check()) return;

    auto ctx = this->context()->tu_context();
    assert(ctx->in_function_definition());
    this->m_stat->check_constraints(reporter);

    if (!ctx->in_switch_statement()) {
        reporter->push_back(make_shared<SemanticErrorIncompatibleTypes>(
                    "default label not in switch statement",
                    this->start_pos(), this->end_pos(), this->context()->posinfo()));
    } else if (!ctx->add_defaultlabel()) {
        reporter->push_back(make_shared<SemanticErrorDuplicateCaseLabel>(
                    "duplicate default label",
                    this->start_pos(), this->end_pos(), this->context()->posinfo()));
    }
}

void ASTNodeBlockItemList::check_constraints(std::shared_ptr<SemanticReporter> reporter)
{
    if (!this->do_check()) return;

    for (auto item: *this)
        item->check_constraints(reporter);
}

void ASTNodeStatCompound::check_constraints(std::shared_ptr<SemanticReporter> reporter)
{
    if (!this->do_check()) return;

    auto ctx = this->context()->tu_context();
    assert(ctx->in_function_definition());

    ctx->enter_scope();
    this->_statlist->check_constraints(reporter);
    ctx->leave_scope();
}

void ASTNodeStatExpr::check_constraints(std::shared_ptr<SemanticReporter> reporter)
{
    if (!this->do_check()) return;

    this->_expr->check_constraints(reporter);
}

void ASTNodeStatIF::check_constraints(std::shared_ptr<SemanticReporter> reporter)
{
    if (!this->do_check()) return;

    const auto olderrorcounter = reporter->size();
    this->_cond->check_constraints(reporter);
    const auto check_cond_type = olderrorcounter != reporter->size();
    this->_falsestat->check_constraints(reporter);
    this->_truestat->check_constraints(reporter);

    if (check_cond_type) {
        if (!this->_cond->type()->implicit_cast_to(kstype::booltype(this->context()))) {
            reporter->push_back(make_shared<SemanticErrorIncompatibleTypes>(
                        "condition expression in if statement must be a type that can be implicitly cast to 'bool'",
                        this->_cond->start_pos(), this->_cond->end_pos(), this->context()->posinfo()));
        }
    }
}

void ASTNodeStatSwitch::check_constraints(std::shared_ptr<SemanticReporter> reporter)
{
    if (!this->do_check()) return;

    auto ctx = this->context()->tu_context();
    assert(ctx->in_function_definition());

    ctx->enter_switch();
    this->_stat->check_constraints(reporter);
    ctx->leave_switch();
}

void ASTNodeStatDoWhile::check_constraints(std::shared_ptr<SemanticReporter> reporter)
{
    if (!this->do_check()) return;

    const auto olderrorcounter = reporter->size();
    this->_expr->check_constraints(reporter);
    const auto check_cond_type = olderrorcounter != reporter->size();
    this->_stat->check_constraints(reporter);

    if (check_cond_type) {
        if (!this->_expr->type()->implicit_cast_to(kstype::booltype(this->context()))) {
            reporter->push_back(make_shared<SemanticErrorIncompatibleTypes>(
                        "condition expression in do-while statement must be a type that can be implicitly cast to 'bool'",
                        this->_expr->start_pos(), this->_expr->end_pos(), this->context()->posinfo()));
        }
    }
}

void ASTNodeStatFor::check_constraints(std::shared_ptr<SemanticReporter> reporter)
{
    if (!this->do_check()) return;

    this->_init->check_constraints(reporter);
    const auto olderrorcounter = reporter->size();
    this->_cond->check_constraints(reporter);
    const auto check_cond_type = olderrorcounter != reporter->size();
    this->_post->check_constraints(reporter);
    this->_stat->check_constraints(reporter);

    if (check_cond_type) {
        if (!this->_cond->type()->implicit_cast_to(kstype::booltype(this->context()))) {
            reporter->push_back(make_shared<SemanticErrorIncompatibleTypes>(
                        "condition expression in for statement must be a type that can be implicitly cast to 'bool'",
                        this->_cond->start_pos(), this->_cond->end_pos(), this->context()->posinfo()));
        }
    }
}

void ASTNodeStatForDecl::check_constraints(std::shared_ptr<SemanticReporter> reporter)
{
    if (!this->do_check()) return;

    auto ctx = this->context()->tu_context();
    assert(ctx->in_function_definition());

    ctx->enter_scope();
    this->_decls->check_constraints(reporter);
    const auto olderrorcounter = reporter->size();
    this->_cond->check_constraints(reporter);
    const auto check_cond_type = olderrorcounter != reporter->size();
    this->_post->check_constraints(reporter);
    this->_stat->check_constraints(reporter);
    ctx->leave_scope();

    if (check_cond_type) {
        if (!this->_cond->type()->implicit_cast_to(kstype::booltype(this->context()))) {
            reporter->push_back(make_shared<SemanticErrorIncompatibleTypes>(
                        "condition expression in for statement must be a type that can be implicitly cast to 'bool'",
                        this->_cond->start_pos(), this->_cond->end_pos(), this->context()->posinfo()));
        }
    }
}

void ASTNodeStatGoto::check_constraints(std::shared_ptr<SemanticReporter> reporter)
{
    if (!this->do_check()) return;

    auto ctx = this->context()->tu_context();
    assert(ctx->in_function_definition());

    if (!ctx->is_valid_idlabel(this->_label->id)) {
        reporter->push_back(make_shared<SemanticErrorUndefinedLabel>(
                    "undefined label",
                    this->start_pos(), this->end_pos(), this->context()->posinfo()));
    }
}

void ASTNodeStatContinue::check_constraints(std::shared_ptr<SemanticReporter> reporter)
{
    if (!this->do_check()) return;

    auto ctx = this->context()->tu_context();
    if (!ctx->continueable()) {
        reporter->push_back(make_shared<SemanticErrorContinueOutsideLoop>(
                    "continue statement must be inside a loop",
                    this->start_pos(), this->end_pos(), this->context()->posinfo()));
    }
}

void ASTNodeStatBreak::check_constraints(std::shared_ptr<SemanticReporter> reporter)
{
    if (!this->do_check()) return;

    auto ctx = this->context()->tu_context();
    if (!ctx->breakable()) {
        reporter->push_back(make_shared<SemanticErrorBreakOutsideLoop>(
                    "break statement must be inside a loop or switch",
                    this->start_pos(), this->end_pos(), this->context()->posinfo()));
    }
}

void ASTNodeStatReturn::check_constraints(std::shared_ptr<SemanticReporter> reporter)
{
    if (!this->do_check()) return;

    auto ctx = this->context()->tu_context();
    assert(ctx->in_function_definition());

    if (this->_expr) {
        this->_expr->check_constraints(reporter);
        if (!this->_expr->type()->implicit_cast_to(ctx->return_type())) {
            reporter->push_back(make_shared<SemanticErrorIncompatibleTypes>(
                        "return expression must be a type that can be implicitly cast to the return type of the function",
                        this->_expr->start_pos(), this->_expr->end_pos(), this->context()->posinfo()));
        }
    } else if (ctx->return_type() && ctx->return_type()->basic_type() != variable_basic_type::VOID) {
        reporter->push_back(make_shared<SemanticErrorMissingReturnValue>(
                    "missing return value",
                    this->start_pos(), this->end_pos(), this->context()->posinfo()));
    }
}

void ASTNodeFunctionDefinition::check_constraints(std::shared_ptr<SemanticReporter> reporter)
{
    if (!this->do_check()) return;

    auto ctx = this->context()->tu_context();
    assert(!ctx->in_function_definition());
    this->_decl->check_constraints(reporter);

    auto decl = this->_decl;
    ctx->function_begin(this->_id->id, decl->return_type(), decl->parameter_declaration_list());
    this->_body->check_constraints(reporter);
    ctx->function_end();
}

void ASTNodeTranslationUnit::check_constraints(std::shared_ptr<SemanticReporter> reporter)
{
    if (!this->do_check()) return;

    auto ctx = this->context();
    auto tctx = make_shared<CTranslationUnitContext>(reporter, ctx);
    ctx->set_tu_context(tctx);

    for (auto& decl : *this)
        decl->check_constraints(reporter);
}
