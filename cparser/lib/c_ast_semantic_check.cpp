#include "c_ast.h"
#include "c_ast_utils.h"
#include "c_parser.h"
#include "c_reporter.h"
#include "c_translation_unit_context.h"
using namespace cparser;
using namespace std;
using variable_basic_type = ASTNodeVariableType::variable_basic_type;


void ASTNodeExprIdentifier::check_semantic(std::shared_ptr<SemanticReporter> reporter)
{
    auto ctx = this->context();
    auto tctx = ctx->tu_context();
    assert(this->m_id);

    if (tctx->lookup_variable(this->m_id->id)) {
        reporter->push_back(make_shared<SemanticErrorVarNotDefined>(
                    "variable '" + this->m_id->id + "' is not defined",
                    this->start_pos(), this->end_pos(), ctx->posinfo()));
    }
}

void ASTNodeExprString::check_semantic(std::shared_ptr<SemanticReporter> reporter) {}
void ASTNodeExprInteger::check_semantic(std::shared_ptr<SemanticReporter> reporter) {}
void ASTNodeExprFloat::check_semantic(std::shared_ptr<SemanticReporter> reporter) {}

void ASTNodeExprIndexing::check_semantic(std::shared_ptr<SemanticReporter> reporter)
{
    assert(this->m_array && this->m_idx);
    const auto olderrorcounter = reporter->size();
    this->m_array->check_semantic(reporter);
    this->m_idx->check_semantic(reporter);
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
}

void ASTNodeArgList::check_semantic(std::shared_ptr<SemanticReporter> reporter)
{
    for (auto& arg : *this)
        arg->check_semantic(reporter);
}

void ASTNodeExprFunctionCall::check_semantic(std::shared_ptr<SemanticReporter> reporter)
{
    const auto olderrorcounter = reporter->size();
    this->m_func->check_semantic(reporter);
    this->m_args->check_semantic(reporter);
    if (olderrorcounter != reporter->size())
        return;

    auto ftype = this->m_func->type();
    auto functype = cast2function(ftype);
    auto ffff = ftype->implicitly_convert_to(functype);
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

        auto ffff = argtype->implicitly_convert_to(paramtype);
        if (!ffff) {
            reporter->push_back(make_shared<SemanticErrorInvalidFunctionCall>(
                        "invalid function call, parameter type mismatch, expected " + 
                        paramtype->to_string() + ", got " + argtype->to_string(),
                        arg->start_pos(), arg->end_pos(), this->context()->posinfo()));
        } else if (ffff != argtype) {
            arg = make_exprcast(arg, ffff);
        }
    }
}

void ASTNodeExprMemberAccess::check_semantic(std::shared_ptr<SemanticReporter> reporter)
{
    const auto olderrorcounter = reporter->size();
    this->m_obj->check_semantic(reporter);
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

    auto structtype = dynamic_pointer_cast<ASTNodeVariableTypeStruct>(objtype);
    auto uniontype  = dynamic_pointer_cast<ASTNodeVariableTypeUnion>(objtype);
    const auto def = structtype ? structtype->definition() : uniontype->definition();
    bool found = false;
    for (auto mem: *def) {
        auto mid = mem->id();
        if (mid == nullptr) continue;

        if (mid->id == this->m_member->id) {
            found = true;
            break;
        }
    }

    if (!found) {
        reporter->push_back(make_shared<SemanticErrorInvalidMemberAccess>(
                    "invalid member access, member '" + this->m_member->id + "' is not defined",
                    this->m_obj->start_pos(), this->m_obj->end_pos(), this->context()->posinfo()));
    }
}

void ASTNodeExprPointerMemberAccess::check_semantic(std::shared_ptr<SemanticReporter> reporter)
{
    const auto olderrorcounter = reporter->size();
    this->m_obj->check_semantic(reporter);
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

    auto structtype = dynamic_pointer_cast<ASTNodeVariableTypeStruct>(objtype);
    auto uniontype  = dynamic_pointer_cast<ASTNodeVariableTypeUnion>(objtype);
    const auto def = structtype ? structtype->definition() : uniontype->definition();
    bool found = false;
    for (auto mem: *def) {
        auto mid = mem->id();
        if (mid == nullptr) continue;

        if (mid->id == this->m_member->id) {
            found = true;
            break;
        }
    }

    if (!found) {
        reporter->push_back(make_shared<SemanticErrorInvalidMemberAccess>(
                    "invalid member access, member '" + this->m_member->id + "' is not defined",
                    this->m_obj->start_pos(), this->m_obj->end_pos(), this->context()->posinfo()));
    }
}

void ASTNodeExprInitializer::check_semantic(std::shared_ptr<SemanticReporter> reporter)
{
    const auto olderrorcounter = reporter->size();
    this->m_type->check_semantic(reporter);
    this->m_init->check_semantic(reporter);
    if (olderrorcounter != reporter->size())
        return;

    auto type = this->m_type;
    // TODO
}

void ASTNodeExprUnaryOp::check_semantic(std::shared_ptr<SemanticReporter> reporter)
{
}

void ASTNodeExprSizeof::check_semantic(std::shared_ptr<SemanticReporter> reporter)
{
    this->m_type->check_semantic(reporter);
}

void ASTNodeExprCast::check_semantic(std::shared_ptr<SemanticReporter> reporter)
{
}

void ASTNodeExprBinaryOp::check_semantic(std::shared_ptr<SemanticReporter> reporter)
{
}

void ASTNodeExprConditional::check_semantic(std::shared_ptr<SemanticReporter> reporter)
{
}

void ASTNodeExprList::check_semantic(std::shared_ptr<SemanticReporter> reporter)
{
}

void ASTNodeVariableTypeDummy::check_semantic(std::shared_ptr<SemanticReporter> reporter)
{
    throw std::runtime_error("ASTNodeVariableTypeDummy::check_semantic");
}

void ASTNodeVariableTypeStruct::check_semantic(std::shared_ptr<SemanticReporter> reporter)
{
}

void ASTNodeVariableTypeUnion::check_semantic(std::shared_ptr<SemanticReporter> reporter)
{
}

void ASTNodeVariableTypeEnum::check_semantic(std::shared_ptr<SemanticReporter> reporter)
{
}

void ASTNodeVariableTypeTypedef::check_semantic(std::shared_ptr<SemanticReporter> reporter)
{
}

void ASTNodeVariableTypeVoid::check_semantic(std::shared_ptr<SemanticReporter> reporter)

{
}

void ASTNodeVariableTypeInt::check_semantic(std::shared_ptr<SemanticReporter> reporter)
{
}

void ASTNodeVariableTypeFloat::check_semantic(std::shared_ptr<SemanticReporter> reporter)
{
}

void ASTNodeInitDeclarator::check_semantic(std::shared_ptr<SemanticReporter> reporter)
{
}

void ASTNodeStructUnionDeclaration::check_semantic(std::shared_ptr<SemanticReporter> reporter)
{
}

void ASTNodeInitDeclaratorList::check_semantic(std::shared_ptr<SemanticReporter> reporter)
{
}

void ASTNodeDeclarationList::check_semantic(std::shared_ptr<SemanticReporter> reporter)
{
}

void ASTNodeStructUnionDeclarationList::check_semantic(std::shared_ptr<SemanticReporter> reporter)
{
}

void ASTNodeEnumerationConstant::check_semantic(std::shared_ptr<SemanticReporter> reporter)
{
}

void ASTNodeEnumerator::check_semantic(std::shared_ptr<SemanticReporter> reporter)
{
}

void ASTNodeEnumeratorList::check_semantic(std::shared_ptr<SemanticReporter> reporter)
{
}

void ASTNodeVariableTypePointer::check_semantic(std::shared_ptr<SemanticReporter> reporter)
{
}

void ASTNodeVariableTypeArray::check_semantic(std::shared_ptr<SemanticReporter> reporter)
{
}

void ASTNodeParameterDeclaration::check_semantic(std::shared_ptr<SemanticReporter> reporter)
{
}

void ASTNodeParameterDeclarationList::check_semantic(std::shared_ptr<SemanticReporter> reporter)
{
}

void ASTNodeVariableTypeFunction::check_semantic(std::shared_ptr<SemanticReporter> reporter)
{
}

void ASTNodeInitializer::check_semantic(std::shared_ptr<SemanticReporter> reporter)
{
}

void ASTNodeDesignation::check_semantic(std::shared_ptr<SemanticReporter> reporter)
{
}

void ASTNodeInitializerList::check_semantic(std::shared_ptr<SemanticReporter> reporter)
{
}

void ASTNodeStatLabel::check_semantic(std::shared_ptr<SemanticReporter> reporter)
{
}

void ASTNodeStatCase::check_semantic(std::shared_ptr<SemanticReporter> reporter)
{
}

void ASTNodeStatDefault::check_semantic(std::shared_ptr<SemanticReporter> reporter)
{
}

void ASTNodeBlockItemList::check_semantic(std::shared_ptr<SemanticReporter> reporter)
{
}

void ASTNodeStatCompound::check_semantic(std::shared_ptr<SemanticReporter> reporter)
{
}

void ASTNodeStatExpr::check_semantic(std::shared_ptr<SemanticReporter> reporter)
{
}

void ASTNodeStatIF::check_semantic(std::shared_ptr<SemanticReporter> reporter)
{
}

void ASTNodeStatSwitch::check_semantic(std::shared_ptr<SemanticReporter> reporter)
{
}

void ASTNodeStatDoWhile::check_semantic(std::shared_ptr<SemanticReporter> reporter)
{
}

void ASTNodeStatFor::check_semantic(std::shared_ptr<SemanticReporter> reporter)
{
}

void ASTNodeStatForDecl::check_semantic(std::shared_ptr<SemanticReporter> reporter)
{
}

void ASTNodeStatGoto::check_semantic(std::shared_ptr<SemanticReporter> reporter)
{
}

void ASTNodeStatContinue::check_semantic(std::shared_ptr<SemanticReporter> reporter)
{
}

void ASTNodeStatBreak::check_semantic(std::shared_ptr<SemanticReporter> reporter)
{
}

void ASTNodeStatReturn::check_semantic(std::shared_ptr<SemanticReporter> reporter)
{
}

void ASTNodeFunctionDefinition::check_semantic(std::shared_ptr<SemanticReporter> reporter)
{
}

void ASTNodeTranslationUnit::check_semantic(std::shared_ptr<SemanticReporter> reporter)
{
    auto ctx = this->context();
    auto tctx = make_shared<CTranslationUnitContext>(reporter, ctx);
    ctx->set_tu_context(tctx);

    for (auto& decl : *this)
        decl->check_semantic(reporter);
}
