#include "c_ast.h"
#include "c_ast_utils.h"
#include "c_parser.h"
#include "c_reporter.h"
#include "c_translation_unit_context.h"
#include <limits>
using namespace cparser;
using namespace std;
using variable_basic_type = ASTNodeVariableType::variable_basic_type;


void ASTNodeExprIdentifier::check_constraints(std::shared_ptr<SemanticReporter> reporter)
{
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
    this->resolve_type(kstype::constcharptrtype(this->context()));
}
void ASTNodeExprInteger::check_constraints(std::shared_ptr<SemanticReporter> reporter)
{
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
    for (auto& arg : *this)
        arg->check_constraints(reporter);
}

void ASTNodeExprFunctionCall::check_constraints(std::shared_ptr<SemanticReporter> reporter)
{
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
        } else if (ffff != argtype) {
            arg = make_exprcast(arg, ffff);
        }
    }

    if (olderrorcounter != reporter->size()) {
        this->resolve_type(kstype::voidtype(this->context()));
    } else {
        auto func_proto = this->m_func->type();
        assert(func_proto);
        auto rettype = functype->return_type()->copy();
        rettype->reset_qualifies();
        rettype->reset_storage_class();
        this->resolve_type(rettype);
    }
}

void ASTNodeExprMemberAccess::check_constraints(std::shared_ptr<SemanticReporter> reporter)
{
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
            restype->reset_qualifies();
            restype->reset_storage_class();
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
            restype->reset_qualifies();
            restype->reset_storage_class();
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
    const auto olderrorcounter = reporter->size();
    this->m_type->check_constraints(reporter);
    this->m_init->check_constraints(reporter);
    if (olderrorcounter != reporter->size())
        return;

    auto type = this->m_type;
    // TODO messy
}

void ASTNodeExprUnaryOp::check_constraints(std::shared_ptr<SemanticReporter> reporter)
{
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
        switch (this->m_expr->type()->basic_type()) {
            case variable_basic_type::INT:
            case variable_basic_type::FLOAT:
            case variable_basic_type::POINTER:
            case variable_basic_type::ARRAY:
            case variable_basic_type::ENUM:
            case variable_basic_type::FUNCTION:
                exprtype = make_shared<ASTNodeVariableTypeInt>(this->context(), 1, false);
                break;
            default:
                reporter->push_back(make_shared<SemanticErrorInvalidOperand>(
                            "invalid unary operator, can't convert to bool",
                            this->m_expr->start_pos(), this->m_expr->end_pos(), this->context()->posinfo()));
                break;
        }
        break;
    case OT::BITWISE_NOT:
        if (this->m_expr->type()->basic_type() != variable_basic_type::INT) {
            reporter->push_back(make_shared<SemanticErrorInvalidOperand>(
                        "invalid unary operator, operand is not integer",
                        this->m_expr->start_pos(), this->m_expr->end_pos(), this->context()->posinfo()));
        } else {
            exprtype = this->m_expr->type();
        }
        break;
    case OT::SIZEOF:
        exprtype = make_shared<ASTNodeVariableTypeInt>(this->context(), sizeof(size_t), true);
        break;
    case OT::MINUS:
    case OT::PLUS:
    {
        if (this->m_expr->type()->basic_type() != variable_basic_type::INT &&
            this->m_expr->type()->basic_type() != variable_basic_type::FLOAT)
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
    this->m_type->check_constraints(reporter);
    this->resolve_type(make_shared<ASTNodeVariableTypeInt>(this->context(), sizeof(size_t), true));
}

void ASTNodeExprCast::check_constraints(std::shared_ptr<SemanticReporter> reporter)
{
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
    }
}

void ASTNodeExprBinaryOp::check_constraints(std::shared_ptr<SemanticReporter> reporter)
{
}

void ASTNodeExprConditional::check_constraints(std::shared_ptr<SemanticReporter> reporter)
{
}

void ASTNodeExprList::check_constraints(std::shared_ptr<SemanticReporter> reporter)
{
}

void ASTNodeVariableTypeDummy::check_constraints(std::shared_ptr<SemanticReporter> reporter)
{
    throw std::runtime_error("ASTNodeVariableTypeDummy::check_constraints");
}

void ASTNodeVariableTypeStruct::check_constraints(std::shared_ptr<SemanticReporter> reporter)
{
}

void ASTNodeVariableTypeUnion::check_constraints(std::shared_ptr<SemanticReporter> reporter)
{
}

void ASTNodeVariableTypeEnum::check_constraints(std::shared_ptr<SemanticReporter> reporter)
{
}

void ASTNodeVariableTypeTypedef::check_constraints(std::shared_ptr<SemanticReporter> reporter)
{
}

void ASTNodeVariableTypeVoid::check_constraints(std::shared_ptr<SemanticReporter> reporter)

{
}

void ASTNodeVariableTypeInt::check_constraints(std::shared_ptr<SemanticReporter> reporter)
{
}

void ASTNodeVariableTypeFloat::check_constraints(std::shared_ptr<SemanticReporter> reporter)
{
}

void ASTNodeInitDeclarator::check_constraints(std::shared_ptr<SemanticReporter> reporter)
{
}

void ASTNodeStructUnionDeclaration::check_constraints(std::shared_ptr<SemanticReporter> reporter)
{
}

void ASTNodeInitDeclaratorList::check_constraints(std::shared_ptr<SemanticReporter> reporter)
{
}

void ASTNodeDeclarationList::check_constraints(std::shared_ptr<SemanticReporter> reporter)
{
}

void ASTNodeStructUnionDeclarationList::check_constraints(std::shared_ptr<SemanticReporter> reporter)
{
}

void ASTNodeEnumerationConstant::check_constraints(std::shared_ptr<SemanticReporter> reporter)
{
}

void ASTNodeEnumerator::check_constraints(std::shared_ptr<SemanticReporter> reporter)
{
}

void ASTNodeEnumeratorList::check_constraints(std::shared_ptr<SemanticReporter> reporter)
{
}

void ASTNodeVariableTypePointer::check_constraints(std::shared_ptr<SemanticReporter> reporter)
{
}

void ASTNodeVariableTypeArray::check_constraints(std::shared_ptr<SemanticReporter> reporter)
{
}

void ASTNodeParameterDeclaration::check_constraints(std::shared_ptr<SemanticReporter> reporter)
{
}

void ASTNodeParameterDeclarationList::check_constraints(std::shared_ptr<SemanticReporter> reporter)
{
}

void ASTNodeVariableTypeFunction::check_constraints(std::shared_ptr<SemanticReporter> reporter)
{
}

void ASTNodeInitializer::check_constraints(std::shared_ptr<SemanticReporter> reporter)
{
}

void ASTNodeDesignation::check_constraints(std::shared_ptr<SemanticReporter> reporter)
{
}

void ASTNodeInitializerList::check_constraints(std::shared_ptr<SemanticReporter> reporter)
{
}

void ASTNodeStatLabel::check_constraints(std::shared_ptr<SemanticReporter> reporter)
{
}

void ASTNodeStatCase::check_constraints(std::shared_ptr<SemanticReporter> reporter)
{
}

void ASTNodeStatDefault::check_constraints(std::shared_ptr<SemanticReporter> reporter)
{
}

void ASTNodeBlockItemList::check_constraints(std::shared_ptr<SemanticReporter> reporter)
{
}

void ASTNodeStatCompound::check_constraints(std::shared_ptr<SemanticReporter> reporter)
{
}

void ASTNodeStatExpr::check_constraints(std::shared_ptr<SemanticReporter> reporter)
{
}

void ASTNodeStatIF::check_constraints(std::shared_ptr<SemanticReporter> reporter)
{
}

void ASTNodeStatSwitch::check_constraints(std::shared_ptr<SemanticReporter> reporter)
{
}

void ASTNodeStatDoWhile::check_constraints(std::shared_ptr<SemanticReporter> reporter)
{
}

void ASTNodeStatFor::check_constraints(std::shared_ptr<SemanticReporter> reporter)
{
}

void ASTNodeStatForDecl::check_constraints(std::shared_ptr<SemanticReporter> reporter)
{
}

void ASTNodeStatGoto::check_constraints(std::shared_ptr<SemanticReporter> reporter)
{
}

void ASTNodeStatContinue::check_constraints(std::shared_ptr<SemanticReporter> reporter)
{
}

void ASTNodeStatBreak::check_constraints(std::shared_ptr<SemanticReporter> reporter)
{
}

void ASTNodeStatReturn::check_constraints(std::shared_ptr<SemanticReporter> reporter)
{
}

void ASTNodeFunctionDefinition::check_constraints(std::shared_ptr<SemanticReporter> reporter)
{
}

void ASTNodeTranslationUnit::check_constraints(std::shared_ptr<SemanticReporter> reporter)
{
    auto ctx = this->context();
    auto tctx = make_shared<CTranslationUnitContext>(reporter, ctx);
    ctx->set_tu_context(tctx);

    for (auto& decl : *this)
        decl->check_constraints(reporter);
}
