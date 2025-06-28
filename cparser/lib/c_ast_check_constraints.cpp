#include "c_ast.h"
#include "c_ast_utils.h"
#include "c_initializer.h"
#include "c_parser.h"
#include "c_reporter.h"
#include "c_translation_unit_context.h"
#include <limits>
#include <type_traits>
using namespace cparser;
using namespace std;
using variable_basic_type = ASTNodeVariableType::variable_basic_type;
using storage_class_t = ASTNodeVariableType::storage_class_t;

#define MAX_AB(a, b) ((a) > (b) ? (a) : (b))
#define MACHINE_WORD_SIZE_IN_BIT (sizeof(void*) * CHAR_BIT)
#define OREPORT(textinfo, error, range, msg)                                                       \
    reporter->push_back(make_shared<SemanticError##error>(msg, range, textinfo))
#define REPORT(error, range, msg) OREPORT(this->context()->textinfo(), error, range, msg)


void ASTNodeExprIdentifier::check_constraints(std::shared_ptr<SemanticReporter> reporter)
{
    if (!this->do_check())
        return;

    auto ctx = this->context();
    auto tctx = ctx->tu_context();
    assert(this->m_id);

    auto type = tctx->lookup_variable(this->m_id->id);
    if (!type) {
        this->resolve_type(kstype::voidtype(this->context()));
        REPORT(VarNotDefined, *this, "variable '" + this->m_id->id + "' is not defined");
    } else {
        this->resolve_type(type);
    }
}

void ASTNodeExprString::check_constraints(std::shared_ptr<SemanticReporter> reporter)
{
    if (!this->do_check())
        return;

    this->resolve_type(kstype::constcharptrtype(this->context()));
}
void ASTNodeExprInteger::check_constraints(std::shared_ptr<SemanticReporter> reporter)
{
    if (!this->do_check())
        return;

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
    if (!this->do_check())
        return;

    const auto val = this->token()->value;
    if (val <= std::numeric_limits<float>::max() && val >= std::numeric_limits<float>::min()) {
        this->resolve_type(make_shared<ASTNodeVariableTypeFloat>(this->context(), sizeof(float)));
    } else if (val <= std::numeric_limits<double>::max() &&
               val >= std::numeric_limits<double>::min()) {
        this->resolve_type(make_shared<ASTNodeVariableTypeFloat>(this->context(), sizeof(double)));
    } else {
        this->resolve_type(
            make_shared<ASTNodeVariableTypeFloat>(this->context(), sizeof(long double)));
    }
}

void ASTNodeExprIndexing::check_constraints(std::shared_ptr<SemanticReporter> reporter)
{
    if (!this->do_check())
        return;

    assert(this->m_array && this->m_idx);
    const auto olderrorcounter = reporter->size();
    this->m_array->check_constraints(reporter);
    this->m_idx->check_constraints(reporter);
    if (olderrorcounter != reporter->size())
        return;

    auto atype = this->m_array->type();
    const auto btype = atype->basic_type();
    if (btype != variable_basic_type::ARRAY && btype != variable_basic_type::POINTER) {
        REPORT(InvalidArrayIndex, *this, "invalid array indexing, type is not array or pointer");
    }

    auto itype = this->m_idx->type();
    auto info = get_integer_info(itype);
    if (!info.has_value()) {
        REPORT(InvalidArrayIndex, *this, "invalid array indexing, index type is not integer");
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
    if (!this->do_check())
        return;

    for (auto& arg : *this)
        arg->check_constraints(reporter);
}

void ASTNodeExprFunctionCall::check_constraints(std::shared_ptr<SemanticReporter> reporter)
{
    if (!this->do_check())
        return;

    const auto olderrorcounter = reporter->size();
    this->m_func->check_constraints(reporter);
    this->m_args->check_constraints(reporter);
    if (olderrorcounter != reporter->size())
        return;

    auto ftype = this->m_func->type();
    auto functype = cast2function(ftype);
    auto ffff = ftype->implicit_cast_to(functype);
    if (functype == nullptr || ffff == nullptr) {
        REPORT(InvalidFunctionCall, *this, "invalid function call, type is not function");
        return;
    }
    if (ffff != ftype)
        this->m_func = make_exprcast(this->m_func, ffff);

    auto params = functype->parameter_declaration_list();
    if (params->size() != this->m_args->size() && !params->variadic()) {
        REPORT(InvalidFunctionCall,
               *this->m_func,
               "invalid function call, parameter count mismatch, expected " +
                   std::to_string(params->size()) + ", got " +
                   std::to_string(this->m_args->size()));
        return;
    }

    for (size_t i = 0; i < params->size(); i++) {
        auto param = (*params)[i];
        auto& arg = (*this->m_args)[i];

        auto paramtype = param->get_type();
        auto argtype = arg->type();

        auto ffff = argtype->implicit_cast_to(paramtype);
        if (!ffff) {
            REPORT(InvalidFunctionCall,
                   *arg,
                   "invalid function call, parameter type mismatch, expected " +
                       paramtype->to_string() + ", got " + argtype->to_string());
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
    if (!this->do_check())
        return;

    const auto olderrorcounter = reporter->size();
    this->m_obj->check_constraints(reporter);
    if (olderrorcounter != reporter->size())
        return;

    auto objtype = this->m_obj->type();
    if (objtype->basic_type() != variable_basic_type::STRUCT &&
        objtype->basic_type() != variable_basic_type::UNION) {
        REPORT(InvalidMemberAccess, *this, "invalid member access, type is not struct or union");
        return;
    }

    shared_ptr<ASTNodeVariableType> restype;
    auto structtype = dynamic_pointer_cast<ASTNodeVariableTypeStruct>(objtype);
    auto uniontype = dynamic_pointer_cast<ASTNodeVariableTypeUnion>(objtype);
    const auto def = structtype ? structtype->get_definition() : uniontype->get_definition();
    bool found = false;
    for (auto mem : *def) {
        auto mid = mem->get_id();
        if (mid == nullptr)
            continue;

        if (mid->id == this->m_member->id) {
            restype = mem->get_type()->copy();
            restype->const_ref() = restype->const_ref() || objtype->const_ref();
            restype->volatile_ref() = restype->volatile_ref() || objtype->const_ref();
            restype->restrict_ref() = restype->restrict_ref() || objtype->restrict_ref();
            found = true;
            break;
        }
    }

    if (!found) {
        REPORT(InvalidMemberAccess,
               *this,
               "invalid member access, member '" + this->m_member->id + "' is not defined");
    }

    if (olderrorcounter != reporter->size()) {
        this->resolve_type(kstype::voidtype(this->context()));
    } else {
        this->resolve_type(restype);
    }
}

void ASTNodeExprPointerMemberAccess::check_constraints(std::shared_ptr<SemanticReporter> reporter)
{
    if (!this->do_check())
        return;

    const auto olderrorcounter = reporter->size();
    this->m_obj->check_constraints(reporter);
    if (olderrorcounter != reporter->size())
        return;

    auto objtype = this->m_obj->type();
    auto ptrtype = dynamic_pointer_cast<ASTNodeVariableTypePointer>(objtype);
    if (ptrtype == nullptr) {
        REPORT(InvalidMemberAccess, *this, "invalid pointer member access, type is not pointer");
        return;
    }

    objtype = ptrtype->deref();
    assert(objtype);
    if (objtype->basic_type() != variable_basic_type::STRUCT &&
        objtype->basic_type() != variable_basic_type::UNION) {
        REPORT(InvalidMemberAccess, *this, "invalid member access, type is not struct or union");
        return;
    }

    shared_ptr<ASTNodeVariableType> restype;
    auto structtype = dynamic_pointer_cast<ASTNodeVariableTypeStruct>(objtype);
    auto uniontype = dynamic_pointer_cast<ASTNodeVariableTypeUnion>(objtype);
    const auto def = structtype ? structtype->get_definition() : uniontype->get_definition();
    bool found = false;
    for (auto mem : *def) {
        auto mid = mem->get_id();
        if (mid == nullptr)
            continue;

        if (mid->id == this->m_member->id) {
            restype = mem->get_type()->copy();
            restype->const_ref() = restype->const_ref() || objtype->const_ref();
            restype->volatile_ref() = restype->volatile_ref() || objtype->const_ref();
            restype->restrict_ref() = restype->restrict_ref() || objtype->restrict_ref();
            found = true;
            break;
        }
    }

    if (!found) {
        REPORT(InvalidMemberAccess,
               *this,
               "invalid member access, member '" + this->m_member->id + "' is not defined");
    }

    if (olderrorcounter != reporter->size()) {
        this->resolve_type(kstype::voidtype(this->context()));
    } else {
        this->resolve_type(restype);
    }
}

static DesignatorList make_designator_list(std::shared_ptr<ASTNodeDesignation> designation)
{
    DesignatorList ret;
    for (auto& d : designation->designators()) {
        if (std::holds_alternative<ASTNodeDesignation::member_designator_t>(d)) {
            auto vx = std::get<ASTNodeDesignation::member_designator_t>(d);
            ret.emplace_back(vx->id);
        } else {
            auto vx = std::get<ASTNodeDesignation::array_designator_t>(d);
            const auto cv = vx->get_integer_constant();
            if (!cv.has_value())
                continue;
            ret.emplace_back(cv.value());
        }
    }
    return ret;
}

static optional<shared_ptr<ASTNodeExpr>>
check_initialization_of_object_with_intializer(std::shared_ptr<ASTNodeVariableType> type,
                                               std::shared_ptr<ASTNodeInitializer> init,
                                               std::shared_ptr<SemanticReporter> reporter);

static bool initialization_implicit_cast(shared_ptr<ASTNodeExpr> fromexpr,
                                         shared_ptr<ASTNodeVariableType> to)
{
    if (ksexpr::is_string_literal(fromexpr) && to->basic_type() == variable_basic_type::ARRAY) {
        auto to_arr = dynamic_pointer_cast<ASTNodeVariableTypeArray>(to);
        if (kstype::is_char(to_arr->elemtype()))
            return true;
    }

    auto from = fromexpr->type();
    return from->implicit_cast_to(to) != nullptr;
}

static void
check_initialization_of_object_with_intializer_list(std::shared_ptr<ASTNodeVariableType> type,
                                                    std::shared_ptr<ASTNodeInitializerList> list,
                                                    std::shared_ptr<SemanticReporter> reporter)
{
    auto textinfo = type->context()->textinfo();
    switch (type->basic_type()) {
    case variable_basic_type::ARRAY: {
        auto arrtype = dynamic_pointer_cast<ASTNodeVariableTypeArray>(type);
        if (kstype::is_char(arrtype->elemtype()) && list->size() == 1) {
            auto init = list->front();
            if (init->designators().size() == 0 && init->get_initializer()->is_expr() &&
                ksexpr::is_string_literal(init->get_initializer()->expr())) {
                auto strexpr =
                    dynamic_pointer_cast<ASTNodeExprString>(init->get_initializer()->expr());
                assert(strexpr);
                const auto& str = strexpr->token()->value;
                if (arrtype->array_size()) {
                    auto size = arrtype->array_size()->get_integer_constant();
                    assert(size.has_value());
                    if (size.value() <= str.size()) {
                        OREPORT(textinfo,
                                InitializerStringTooLong,
                                *init,
                                "initializer-string for char array is too long");
                    }
                } else {
                    auto valtoken = make_shared<TokenConstantInteger>(str.size() + 1);
                    auto valast = make_shared<ASTNodeExprInteger>(type->context(), valtoken);
                    arrtype->set_array_size(valast);
                }
                break;
            }
        }
    }
    case variable_basic_type::STRUCT:
    case variable_basic_type::UNION: {
        auto init_tree = create_initializer_tree(type);
        auto iter = init_tree->first();
        assert(init_tree);
        for (auto designation : *list) {
            auto dlist = make_designator_list(designation);
            if (!dlist.empty()) {
                auto new_iter = init_tree->access(dlist);
                if (!new_iter.has_value()) {
                    OREPORT(textinfo,
                            InvalidDesignation,
                            *designation,
                            "can't find member with this designation");
                    break;
                } else {
                    iter = new_iter.value();
                }
            }

            auto init_type = iter.type();
            if (init_type == nullptr) {
                OREPORT(
                    textinfo, InvalidDesignation, *designation, "no member with this designation");
                break;
            }

            auto init_value = designation->get_initializer();
            if (init_value->is_expr()) {
                auto init_expr = init_value->expr();
                bool nofalse = true;
                while (nofalse && !initialization_implicit_cast(init_expr, init_type)) {
                    if (!iter.down()) {
                        OREPORT(textinfo,
                                InvalidDesignation,
                                *designation,
                                "can't cast initializer to member type");
                        nofalse = false;
                        break;
                    }
                    init_type = iter.type();
                }
                if (!nofalse)
                    break;
            }

            auto new_expr =
                check_initialization_of_object_with_intializer(init_type, init_value, reporter);
            if (new_expr.has_value()) {
                // TODO substitue initialation expression with new value if it presents
            }
            iter.next();
        }

        if (type->basic_type() == variable_basic_type::ARRAY) {
            auto arrtype = dynamic_pointer_cast<ASTNodeVariableTypeArray>(type);
            auto arrinittree = dynamic_pointer_cast<AggregateUnionObjectMemberTreeArray>(init_tree);
            assert(arrtype && arrinittree);
            const auto unknown_size = arrinittree->get_unknown_array_size();
            if (unknown_size.has_value()) {
                auto tokenval = make_shared<TokenConstantInteger>(unknown_size.value());
                auto ast = make_shared<ASTNodeExprInteger>(arrtype->context(), tokenval);
                ast->check_constraints(reporter);
                arrtype->set_array_size(ast);
            }
        }
    } break;
    // the initializer for a scalar shall be a single expression, optionally enclosed in braces
    case variable_basic_type::ENUM:
    case variable_basic_type::FLOAT:
    case variable_basic_type::INT:
    case variable_basic_type::POINTER: {
        if (list->empty() || list->size() > 1)
            OREPORT(
                textinfo, InvalidInitializer, *list, "invalid initializer, expected single value");

        if (list->empty())
            break;

        auto init = list->front();
        auto init__ = init->get_initializer();

        if (init->designators().size() > 0) {
            OREPORT(
                textinfo, InvalidInitializer, *init, "invalid initializer, unexpected designator");
            break;
        }

        if (init__->is_list()) {
            OREPORT(textinfo,
                    InvalidInitializer,
                    *init,
                    "invalid initializer, scalar value initialization should enclosed at most one "
                    "level of braces");
            break;
        }

        auto val = check_initialization_of_object_with_intializer(type, init__, reporter);
        if (val.has_value()) {
            auto newinit = make_shared<ASTNodeInitializer>(init->context(), val.value());
            newinit->check_constraints(reporter);
            init->set_initializer(newinit);
        }
        return;
    } break;
    case variable_basic_type::VOID:
        assert(false && "try to initialize a value with void type");
        break;
    case variable_basic_type::DUMMY:
    case variable_basic_type::FUNCTION:
        assert(false && "unreachable");
        break;
    }
}

static optional<shared_ptr<ASTNodeExpr>>
check_initialization_of_object_with_intializer(std::shared_ptr<ASTNodeVariableType> type,
                                               std::shared_ptr<ASTNodeInitializer> init,
                                               std::shared_ptr<SemanticReporter> reporter)
{
    auto textinfo = type->context()->textinfo();
    auto ctx = type->context();
    if (init->is_expr()) {
        if (type->basic_type() == variable_basic_type::ARRAY) {
            auto arrtype = dynamic_pointer_cast<ASTNodeVariableTypeArray>(type);
            assert(arrtype);
            auto initexpr = init->expr();
            assert(!arrtype->elemtype()->is_incomplete_type());
            auto strexpr = dynamic_pointer_cast<ASTNodeExprString>(initexpr);
            if (!kstype::is_char(arrtype->elemtype())) {
                OREPORT(textinfo,
                        InvalidInitializer,
                        *init,
                        "these value can't be initialized with an expression");
            } else if (!strexpr) {
                OREPORT(textinfo, InvalidInitializer, *init, "bad initializer for char array");
            } else if (arrtype->is_incomplete_type()) {
                const auto& str = strexpr->token()->value;
                auto strsize = make_shared<TokenConstantInteger>(str.size() + 1);
                auto strsize_ast = make_shared<ASTNodeExprInteger>(ctx, strsize);
                strsize_ast->check_constraints(reporter);
                arrtype->set_array_size(strsize_ast);
            } else {
                const auto& str = strexpr->token()->value;
                auto size = arrtype->array_size()->get_integer_constant();
                assert(size.has_value());
                if (size.value() <= str.size()) {
                    OREPORT(textinfo,
                            InitializerStringTooLong,
                            *init,
                            "initializer-string for char array is too long");
                }
            }
        } else {
            using OT = typename ASTNodeExprBinaryOp::BinaryOperatorType;
            static int fakeid_counter = 0;
            auto fakeid = make_shared<TokenID>("#fake_" + to_string(fakeid_counter++));
            auto fakeidast = make_shared<ASTNodeExprIdentifier>(ctx, fakeid);
            auto tctx = ctx->tu_context();
            auto copytype = type->copy();
            copytype->reset_qualifies();
            tctx->declare_variable(fakeid->id, copytype);
            auto assign_expr =
                make_shared<ASTNodeExprBinaryOp>(ctx, OT::ASSIGNMENT, fakeidast, init->expr());
            assign_expr->check_constraints(reporter);
            if (assign_expr->right() != init->expr())
                return assign_expr->right();
        }
    } else {
        if (type->storage_class() != storage_class_t::SC_Default) {
            type = type->copy();
            type->reset_storage_class();
        }
        check_initialization_of_object_with_intializer_list(type, init->list(), reporter);
    }

    return nullopt;
}

static optional<shared_ptr<ASTNodeExpr>>
check_initialization_of_object(std::shared_ptr<ASTNodeVariableType> type,
                               std::shared_ptr<ASTNodeInitializer> init,
                               std::shared_ptr<SemanticReporter> reporter)
{
    auto textinfo = type->context()->textinfo();
    if (type->storage_class() == storage_class_t::SC_Static && init->is_constexpr()) {
        OREPORT(textinfo,
                InvalidInitializer,
                *init,
                "invalid initializer, static storage value should be initialzed by a const value");
    }

    if (type->is_function_type()) {
        OREPORT(textinfo,
                InvalidInitializer,
                *init,
                "invalid initializer, function type cannot be initialized");
        return nullopt;
    }

    bool bad = false;
    if (type->is_incomplete_type()) {
        bad = true;
        if (type->basic_type() == variable_basic_type::ARRAY) {
            auto arrtype = dynamic_pointer_cast<ASTNodeVariableTypeArray>(type);
            assert(arrtype);
            bad = arrtype->elemtype()->is_incomplete_type();
        }
    } else {
        auto arrtype = dynamic_pointer_cast<ASTNodeVariableTypeArray>(type);
        if (!arrtype->array_size()->get_integer_constant().has_value())
            bad = true;
    }

    if (bad) {
        OREPORT(textinfo,
                InvalidInitializer,
                *init,
                "invalid initializer, incomplete type object or variable size object cannot be "
                "initialized");

        return nullopt;
    }

    return check_initialization_of_object_with_intializer(type, init, reporter);
}

void ASTNodeExprCompoundLiteral::check_constraints(std::shared_ptr<SemanticReporter> reporter)
{
    if (!this->do_check())
        return;

    const auto olderrorcounter = reporter->size();
    this->m_type->check_constraints(reporter);
    this->m_init->check_constraints(reporter);
    if (olderrorcounter != reporter->size())
        return;

    auto init = make_shared<ASTNodeInitializer>(this->context(), this->m_init);
    check_initialization_of_object_with_intializer(this->m_type, init, reporter);
}

void ASTNodeExprUnaryOp::check_constraints(std::shared_ptr<SemanticReporter> reporter)
{
    if (!this->do_check())
        return;

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
    case OT::PREFIX_INC: {
        if (!this->m_expr->is_lvalue()) {
            REPORT(InvalidValueCategory, *this, "invalid unary operator, operand is not lvalue");
            break;
        }
        if (this->m_expr->type()->const_ref()) {
            REPORT(ModifyConstant, *this, "invalid unary operator, operand is const");
            break;
        }

        auto et = this->m_expr->type();
        const auto ebt = et->basic_type();
        switch (et->basic_type()) {
        case variable_basic_type::INT:
        case variable_basic_type::POINTER:
            exprtype = et->copy();
            break;
        case variable_basic_type::ARRAY: {
            auto m = dynamic_pointer_cast<ASTNodeVariableTypeArray>(et);
            assert(m);
            exprtype = ptrto(m->elemtype());
            exprtype->const_ref() = et->const_ref();
        } break;
        default:
            REPORT(InvalidOperand,
                   *this,
                   "invalid unary operator, operand is not integer or pointer or array");
            break;
        }

        if (exprtype &&
            (this->m_operator == OT::POSFIX_DEC || this->m_operator == OT::PREFIX_DEC)) {
            exprtype->reset_qualifies();
            exprtype->reset_storage_class();
        }
    } break;
    case OT::REFERENCE:
        if (!this->m_expr->is_lvalue()) {
            REPORT(InvalidValueCategory, *this, "invalid unary operator, operand is not lvalue");
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
            REPORT(
                InvalidOperand, *this, "invalid unary operator, operand is not pointer or array");
        }
        break;
    case OT::LOGICAL_NOT:
        if (this->m_expr->type()->is_scalar_type() ||
            this->m_expr->type()->basic_type() == variable_basic_type::ENUM) {
            exprtype = make_shared<ASTNodeVariableTypeInt>(this->context(), 1, false);
        } else {
            REPORT(InvalidOperand, *this, "invalid unary operator, can't convert to bool");
        }
        break;
    case OT::BITWISE_NOT: {
        auto int_ = int_compatible(this->m_expr->type());
        if (!int_) {
            REPORT(InvalidOperand, *this, "invalid unary operator, operand is not integer");
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
    case OT::PLUS: {
        if (!this->m_expr->type()->is_arithmatic_type()) {
            REPORT(
                InvalidOperand, *this, "invalid unary operator, operand is not integer or float");
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
    if (!this->do_check())
        return;

    this->m_type->check_constraints(reporter);
    this->resolve_type(make_shared<ASTNodeVariableTypeInt>(this->context(), sizeof(size_t), true));
}

void ASTNodeExprCast::check_constraints(std::shared_ptr<SemanticReporter> reporter)
{
    if (!this->do_check())
        return;

    const auto olderrorcounter = reporter->size();
    this->m_type->check_constraints(reporter);
    this->m_expr->check_constraints(reporter);
    if (olderrorcounter != reporter->size())
        return;

    auto exprtype = this->m_expr->type();
    auto casttype = exprtype->explicit_cast_to(this->m_type);
    if (!casttype) {
        REPORT(InvalidCastFailed, *this, "cast failed");
    } else {
        this->resolve_type(casttype);
    }
}

void ASTNodeExprBinaryOp::check_constraints(std::shared_ptr<SemanticReporter> reporter)
{
    if (!this->do_check())
        return;

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
    const auto is_assignment = op == OT::ASSIGNMENT || op == OT::ASSIGNMENT_MULTIPLY ||
                               op == OT::ASSIGNMENT_DIVISION || op == OT::ASSIGNMENT_PLUS ||
                               op == OT::ASSIGNMENT_MINUS || op == OT::ASSIGNMENT_REMAINDER ||
                               op == OT::ASSIGNMENT_BITWISE_AND || op == OT::BITWISE_XOR ||
                               op == OT::ASSIGNMENT_BITWISE_OR || op == OT::ASSIGNMENT_LEFT_SHIFT ||
                               op == OT::ASSIGNMENT_RIGHT_SHIFT;
    if (is_assignment && (!this->m_left->is_lvalue() || ltype->const_ref())) {
        REPORT(InvalidOperand,
               *this,
               "assignment operator require a modifiable lvalue as left operand");
    } else {
        switch (op) {
        case OT::MULTIPLY:
        case OT::DIVISION:
        case OT::ASSIGNMENT_MULTIPLY:
        case OT::ASSIGNMENT_DIVISION: {
            auto composite = composite_or_promote(ltype, rtype);
            if (!composite || !composite->is_arithmatic_type()) {
                REPORT(
                    InvalidOperand,
                    *this,
                    "invalid operand for multiply and division, can't convert to arithmetic type");
            }
            if (!ltype->equal_to(composite))
                this->m_left = make_exprcast(this->m_left, composite);
            if (!rtype->equal_to(composite))
                this->m_right = make_exprcast(this->m_right, composite);
            exprtype = composite->copy();
        } break;
        case OT::REMAINDER:
        case OT::ASSIGNMENT_REMAINDER: {
            auto composite = composite_or_promote(ltype, rtype);
            if (!composite || composite->basic_type() == variable_basic_type::INT) {
                REPORT(InvalidOperand,
                       *this,
                       "invalid operand for remainder, can't convert to integer type");
            }
            if (!ltype->equal_to(composite))
                this->m_left = make_exprcast(this->m_left, composite);
            if (!rtype->equal_to(composite))
                this->m_right = make_exprcast(this->m_right, composite);
            exprtype = composite->copy();
        } break;
        case OT::PLUS: {
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

            if (ltype->basic_type() == variable_basic_type::POINTER &&
                !lux->deref()->is_incomplete_type()) {
                auto intcomp = int_compatible(rtype);
                if (!intcomp) {
                    REPORT(InvalidOperand,
                           *this,
                           "invalid operand for pointer addition, can't convert to integer type");
                } else {
                    if (!intcomp->equal_to(rtype))
                        this->m_right = make_exprcast(this->m_right, intcomp);
                    exprtype = ltype->copy();
                }
            } else if (rtype->basic_type() == variable_basic_type::POINTER &&
                       !rux->deref()->is_incomplete_type()) {
                auto intcomp = int_compatible(ltype);
                if (!intcomp) {
                    REPORT(InvalidOperand,
                           *this,
                           "invalid operand for pointer addition, can't convert to integer type");
                } else {
                    if (!intcomp->equal_to(ltype))
                        this->m_left = make_exprcast(this->m_left, intcomp);
                    exprtype = rtype->copy();
                }
            } else {
                auto composite = composite_or_promote(ltype, rtype);
                if (!composite || composite->is_arithmatic_type()) {
                    REPORT(InvalidOperand,
                           *this,
                           "invalid operand for addition, can't convert to arithmetic type");
                } else {
                    if (!composite->equal_to(ltype))
                        this->m_left = make_exprcast(this->m_left, composite);
                    if (!composite->equal_to(rtype))
                        this->m_right = make_exprcast(this->m_right, composite);
                    exprtype = composite->copy();
                }
            }
        } break;
        case OT::MINUS: {
            auto lux = ptr_compatible(ltype);
            if (lux && !lux->equal_to(ltype)) {
                ltype = lux;
                this->m_left = make_exprcast(this->m_left, lux);
            }

            if (ltype->basic_type() == variable_basic_type::POINTER &&
                !lux->deref()->is_incomplete_type()) {
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
                    REPORT(InvalidOperand,
                           *this,
                           "invalid operand for pointer subtraction, can't convert to integer type "
                           "or compatible pointer");
                }
            } else {
                auto composite = composite_or_promote(ltype, rtype);
                if (!composite || composite->is_arithmatic_type()) {
                    REPORT(InvalidOperand,
                           *this,
                           "invalid operand for subtraction, can't convert to arithmetic type");
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
        case OT::ASSIGNMENT_BITWISE_XOR: {
            auto composite = composite_or_promote(ltype, rtype);
            if (!composite || composite->basic_type() != variable_basic_type::INT) {
                REPORT(InvalidOperand,
                       *this,
                       "invalid operand for shift, can't convert to integer type");
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
        case OT::LESS_THAN_EQUAL: {
            auto composite = composite_or_promote(ltype, rtype);
            if (!composite || (composite->basic_type() != variable_basic_type::INT &&
                               composite->basic_type() != variable_basic_type::POINTER)) {
                REPORT(InvalidOperand,
                       *this,
                       "invalid operand for comparison, can't convert to integer type or "
                       "compatible pointer");
            }
            if (!composite->equal_to(ltype))
                this->m_left = make_exprcast(this->m_left, composite);
            if (!composite->equal_to(rtype))
                this->m_right = make_exprcast(this->m_right, composite);

            exprtype = composite->copy();
        } break;
        case OT::EQUAL:
        case OT::NOT_EQUAL: {
            auto composite = composite_or_promote(ltype, rtype);
            if (composite && (composite->basic_type() == variable_basic_type::INT ||
                              composite->basic_type() == variable_basic_type::POINTER)) {
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
                    REPORT(InvalidOperand,
                           *this,
                           "invalid operand for comparison, can't convert to integer type or "
                           "compatible pointer");
                }
            }
        } break;
        case OT::LOGICAL_AND:
        case OT::LOGICAL_OR:
            if ((ltype->is_scalar_type() || ltype->basic_type() == variable_basic_type::ENUM) &&
                (rtype->is_scalar_type() || rtype->basic_type() == variable_basic_type::ENUM)) {
                exprtype = kstype::booltype(this->context());
            } else {
                REPORT(InvalidOperand,
                       *this,
                       "invalid operand for logical operation, can't convert to scalar type");
            }
            break;
        case OT::ASSIGNMENT: {
            auto composite = rtype->implicit_cast_to(ltype);
            if (!composite) {
                REPORT(InvalidOperand,
                       *this,
                       "invalid operand for assignment, can't convert to compatible type");
            } else {
                if (!composite->equal_to(rtype))
                    this->m_right = make_exprcast(this->m_right, composite);
                exprtype = composite->copy();
            }
        } break;
        case OT::ASSIGNMENT_PLUS:
        case OT::ASSIGNMENT_MINUS: {
            auto lptr = ptr_compatible(ltype);
            auto intcomp = int_compatible(rtype);
            if (lptr && intcomp) {
                exprtype = lptr->copy();
                break;
            }

            auto casted = rtype->implicit_cast_to(ltype);
            if (!casted || !casted->is_arithmatic_type()) {
                REPORT(
                    InvalidOperand,
                    *this,
                    "invalid operand for assignment, can't convert to arithmatic compatible type");
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
    if (!this->do_check())
        return;

    const auto olderrorcounter = reporter->size();
    this->m_cond->check_constraints(reporter);
    this->m_true->check_constraints(reporter);
    this->m_false->check_constraints(reporter);
    if (olderrorcounter != reporter->size())
        return;

    if (!this->m_cond->type()->is_scalar_type() ||
        this->m_cond->type()->basic_type() != variable_basic_type::ENUM) {
        REPORT(InvalidOperand,
               *this,
               "invalid operand for conditional expression, can't convert to bool type");
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
    } else if ((bt1 == variable_basic_type::STRUCT || bt1 == variable_basic_type::UNION ||
                bt1 == variable_basic_type::VOID) &&
               t1->equal_to(t2)) {
        exprtype = t1;
    } else if (bt1 == variable_basic_type::POINTER && bt2 == variable_basic_type::POINTER) {
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

    if (!exprtype) {
        REPORT(InvalidOperand, *this, "incompatible operands for conditional expression");
        this->resolve_type(kstype::voidtype(this->context()));
    } else {
        exprtype->reset_qualifies();
        this->resolve_type(exprtype);
    }
}

void ASTNodeExprList::check_constraints(std::shared_ptr<SemanticReporter> reporter)
{
    if (!this->do_check())
        return;

    assert(this->size() > 0);
    for (auto expr : *this)
        expr->check_constraints(reporter);

    this->resolve_type(this->back()->type());
}

void ASTNodeVariableTypeDummy::check_constraints(std::shared_ptr<SemanticReporter> reporter)
{
    if (!this->do_check())
        return;

    throw std::runtime_error("ASTNodeVariableTypeDummy::check_constraints");
}

void ASTNodeVariableTypeStruct::check_constraints(std::shared_ptr<SemanticReporter> reporter)
{
    if (!this->do_check())
        return;

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
    this->m_definition = idinfo.value().second;
}

void ASTNodeVariableTypeUnion::check_constraints(std::shared_ptr<SemanticReporter> reporter)
{
    if (!this->do_check())
        return;

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
    this->m_definition = idinfo.value().second;
}

void ASTNodeVariableTypeEnum::check_constraints(std::shared_ptr<SemanticReporter> reporter)
{
    if (!this->do_check())
        return;

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
    this->m_definition = idinfo.value().second;
}

void ASTNodeVariableTypeTypedef::check_constraints(std::shared_ptr<SemanticReporter> reporter)
{
    if (!this->do_check())
        return;

    auto ctx = this->context()->tu_context();
    ctx->declare_typedef(this->m_name->id, this->m_type);
}

void ASTNodeVariableTypeVoid::check_constraints(std::shared_ptr<SemanticReporter> reporter)
{
    if (!this->do_check())
        return;
}

void ASTNodeVariableTypeInt::check_constraints(std::shared_ptr<SemanticReporter> reporter)
{
    if (!this->do_check())
        return;
}

void ASTNodeVariableTypeFloat::check_constraints(std::shared_ptr<SemanticReporter> reporter)
{
    if (!this->do_check())
        return;
}

void ASTNodeStaticAssertDeclaration::check_constraints(std::shared_ptr<SemanticReporter> reporter)
{
    if (!this->do_check())
        return;

    const auto olderrorcounter = reporter->size();
    this->m_expr->check_constraints(reporter);
    if (reporter->size() > olderrorcounter)
        return;

    if (!this->m_expr->type()->is_scalar_type()) {
        REPORT(InvalidOperand, *this, "invalid operand for _Static_assert");
    } else if (!this->m_expr->get_integer_constant().has_value()) {
        REPORT(InvalidOperand, *this, "invalid operand for _Static_assert");
    } else if (!this->m_expr->get_integer_constant().value()) {
        REPORT(StaticAssertFailed, *this, "_Static_assert failed");
        throw CErrorStaticAssert(this->m_message);
    }
}

void ASTNodeInitDeclarator::check_constraints(std::shared_ptr<SemanticReporter> reporter)
{
    if (!this->do_check())
        return;

    const auto olderrorcounter = reporter->size();
    this->m_type->check_constraints(reporter);
    if (this->m_initializer)
        this->m_initializer->check_constraints(reporter);
    if (olderrorcounter != reporter->size())
        return;

    if (this->m_initializer) {
        auto initializer = this->get_initializer();
        auto newval =
            check_initialization_of_object_with_intializer(this->m_type, initializer, reporter);
        if (newval.has_value()) {
            assert(newval.value());
            this->m_initializer = make_shared<ASTNodeInitializer>(this->context(), newval.value());
        }
    }

    auto ctx = this->context()->tu_context();
    if (this->m_id && !ctx->in_struct_union_definition()) {
        ctx->declare_variable(this->m_id->id, this->m_type);
    }
}

void ASTNodeStructUnionDeclaration::check_constraints(std::shared_ptr<SemanticReporter> reporter)
{
    if (!this->do_check())
        return;

    this->m_decl->check_constraints(reporter);
    if (this->m_bit_width) {
        this->m_bit_width->check_constraints(reporter);
        auto type = this->m_decl->get_type();
        if (type->basic_type() != variable_basic_type::INT) {
            REPORT(IncompatibleTypes, *this, "bit-width specifier for non-integer type");
        } else if (!this->m_bit_width->get_integer_constant().has_value()) {
            REPORT(IncompatibleTypes, *this, "not constant integer in bit-width specifier");
        } else {
            auto inttype = dynamic_pointer_cast<ASTNodeVariableTypeInt>(type);
            assert(inttype);
            int len = this->m_bit_width->get_integer_constant().value();
            if (len < 0 || len > inttype->opsizeof().value() * 8) {
                REPORT(IncompatibleTypes, *this, "invalid bit-width specifier, out of max bits");
            }
        }
    }
}

void ASTNodeInitDeclaratorList::check_constraints(std::shared_ptr<SemanticReporter> reporter)
{
    if (!this->do_check())
        return;

    for (auto decl : *this)
        decl->check_constraints(reporter);
}

void ASTNodeDeclarationList::check_constraints(std::shared_ptr<SemanticReporter> reporter)
{
    if (!this->do_check())
        return;

    for (auto decl : *this)
        decl->check_constraints(reporter);
}

void ASTNodeStructUnionDeclarationList::check_constraints(
    std::shared_ptr<SemanticReporter> reporter)
{
    if (!this->do_check())
        return;

    if (!this->_is_struct) {
        size_t align = 1, size = 0;
        set<string> member_names;
        for (auto decl : *this) {
            if (decl->get_id()) {
                if (member_names.find(decl->get_id()->id) != member_names.end()) {
                    REPORT(DuplicateMember, *decl, "duplicate member name");
                } else {
                    member_names.insert(decl->get_id()->id);
                }
            } else {
                // TODO warning
            }
            decl->get_type()->check_constraints(reporter);
            decl->set_offset(0);
            auto opsizeof = decl->get_type()->opsizeof();
            auto opalignof = decl->get_type()->opalignof();
            if (!opsizeof.has_value() || !opalignof.has_value()) {
                REPORT(IncompleteType, *decl, "member with incomplete type");
            } else {
                align = std::max(align, opalignof.value());
                size += opsizeof.value();
            }
        }
        this->set_alignment(align);
        this->set_sizeof_(size);
        if (size == 0) {
            REPORT(EmptyStructUnionDefinition, *this, "empty struct or union isn't permitted");
        }
        return;
    }

    set<string> member_names;
    size_t align = 1;
    typename std::remove_reference_t<decltype(*this)>::container_t member_decls;
    typename decltype(member_decls)::value_type flexible_array;
    for (size_t i = 0; i < this->size(); i++) {
        auto decl = (*this)[i];
        decl->get_type()->check_constraints(reporter);

        if (decl->get_id()) {
            if (member_names.find(decl->get_id()->id) != member_names.end()) {
                REPORT(DuplicateMember, *decl, "duplicate member name");
            } else {
                member_names.insert(decl->get_id()->id);
            }
        }

        auto opalignof = decl->get_type()->opalignof();
        if (!opalignof.has_value()) {
            auto thetype = decl->get_type();
            bool is_flexible_array_member_at_end = i + 1 == this->size();
            if (is_flexible_array_member_at_end &&
                thetype->basic_type() == variable_basic_type::ARRAY) {
                auto arrtype = dynamic_pointer_cast<ASTNodeVariableTypeArray>(thetype);
                assert(arrtype);
                is_flexible_array_member_at_end =
                    !arrtype->elemtype()->is_incomplete_type() && arrtype->array_size() == nullptr;
            }

            if (!is_flexible_array_member_at_end) {
                REPORT(IncompleteType, *decl, "incomplete type declaration in struct");
            } else {
                flexible_array = decl;
            }
        } else {
            align = MAX_AB(align, opalignof.value());
            member_decls.push_back(decl);
        }
    }
    size_t struct_size = 0, offset = 0;
    size_t nbitused = 0, tbitwidth = 0, tbytewidth = 0, tbytealign = 1;
    bool bitfield_is_unsigned;
    const auto advance_size_offset = [&](size_t val_size, size_t val_align) {
        if (offset % val_align != 0)
            offset += val_align - (offset % val_align);
        struct_size = offset + val_size;
    };

    for (auto decl : member_decls) {
        if (decl->bit_width()) {
            auto inttype = dynamic_pointer_cast<ASTNodeVariableTypeInt>(decl->get_type());
            if (!inttype) {
                REPORT(RequireIntegerType, *decl, "bit-width specifier requires integer type");
                continue;
            } else if (!decl->bit_width().has_value()) {
                REPORT(NonConstant, *decl, "non constant integer in bit-width specifier");
                continue;
            } else if (inttype->opsizeof().value() * 8 < decl->bit_width().value()) {
                REPORT(BitFieldIsTooLarge, *decl, "invalid bit-width specifier, out of max bits");
                continue;
            } else if (inttype->opsizeof().value() <= 0) {
                REPORT(NegativeBitField, *decl, "non-positive bit-field specifier");
                continue;
            }

            auto bit_width = decl->bit_width().value();
            ;
            if (nbitused + bit_width > tbitwidth || inttype->byte_length() != tbytewidth ||
                bitfield_is_unsigned != inttype->is_unsigned()) {
                if (tbitwidth > 0) {
                    advance_size_offset(tbytewidth, tbytealign);
                    offset += tbytewidth;
                }

                offset += tbitwidth / 8;
                tbytewidth = inttype->opsizeof().value();
                tbytealign = inttype->opalignof().value();
                tbitwidth = tbytewidth * 8;
                bitfield_is_unsigned = inttype->is_unsigned();
                nbitused = 0;
            } else {
                nbitused += bit_width;
            }
        } else {
            if (tbitwidth > 0) {
                advance_size_offset(tbytewidth, tbytealign);
                offset += tbytewidth;
            }

            tbitwidth = 0;
            auto ops = decl->get_type()->opsizeof();
            auto opa = decl->get_type()->opalignof();
            if (ops.has_value() && opa.has_value()) {
                advance_size_offset(ops.value(), opa.value());
                offset += ops.value();
            }
        }
        decl->set_offset(offset);
    }
    if (tbitwidth > 0) {
        advance_size_offset(tbytewidth, tbytealign);
        offset += tbytewidth;
    }

    if (struct_size % align != 0)
        struct_size += align - (struct_size % align);

    if (flexible_array)
        flexible_array->set_offset(struct_size);
    this->set_alignment(align);
    this->set_sizeof_(struct_size);

    if (struct_size == 0) {
        REPORT(EmptyStructUnionDefinition, *this, "empty struct or union isn't permitted");
    }
}

void ASTNodeEnumerationConstant::check_constraints(std::shared_ptr<SemanticReporter> reporter)
{
    if (!this->do_check())
        return;
}

void ASTNodeEnumerator::check_constraints(std::shared_ptr<SemanticReporter> reporter)
{
    if (!this->do_check())
        return;

    if (this->m_value) {
        auto constant = this->m_value->get_integer_constant();
        if (!constant.has_value()) {
            REPORT(IncompatibleTypes, *this, "not constant integer in enumerator");
        } else {
            this->resolve_value(constant.value());
        }
    }
}

void ASTNodeEnumeratorList::check_constraints(std::shared_ptr<SemanticReporter> reporter)
{
    if (!this->do_check())
        return;

    set<string> member_names;
    long long val = 0;
    for (auto decl : *this) {
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
            REPORT(DuplicateMember, *decl, "duplicate enum name");
        } else {
            member_names.insert(id);
        }
    }
}

void ASTNodeVariableTypePointer::check_constraints(std::shared_ptr<SemanticReporter> reporter)
{
    if (!this->do_check())
        return;
    this->m_type->check_constraints(reporter);
}

void ASTNodeVariableTypeArray::check_constraints(std::shared_ptr<SemanticReporter> reporter)
{
    if (!this->do_check())
        return;
    this->m_type->check_constraints(reporter);
}

void ASTNodeParameterDeclaration::check_constraints(std::shared_ptr<SemanticReporter> reporter)
{
    if (!this->do_check())
        return;
    this->m_type->check_constraints(reporter);
}

void ASTNodeParameterDeclarationList::check_constraints(std::shared_ptr<SemanticReporter> reporter)
{
    if (!this->do_check())
        return;
    for (auto decl : *this)
        decl->check_constraints(reporter);

    if (this->size() == 1 && this->front()->get_type()->basic_type() == variable_basic_type::VOID) {
        if (this->variadic()) {
            REPORT(IncompatibleTypes, *this, "variadic parameter after void");
        } else {
            this->clear();
            this->m_void = true;
        }
    }

    set<string> param_names;
    for (auto decl : *this) {
        if (decl->get_id()) {
            if (param_names.find(decl->get_id()->id) != param_names.end()) {
                REPORT(DuplicateMember, *decl, "duplicate parameter name");
            } else {
                param_names.insert(decl->get_id()->id);
            }
        }
        if (decl->get_type()->basic_type() == variable_basic_type::VOID) {
            REPORT(InvalidFunctionParameter,
                   *decl,
                   "'void' must be the first and only parameter if specified");
        }
    }
}

void ASTNodeVariableTypeFunction::check_constraints(std::shared_ptr<SemanticReporter> reporter)
{
    if (!this->do_check())
        return;
    this->m_return_type->check_constraints(reporter);
    this->m_parameter_declaration_list->check_constraints(reporter);

    if (this->m_return_type->basic_type() == variable_basic_type::ARRAY) {
        REPORT(IncompatibleTypes, *this, "array type in return type");
        this->m_return_type = kstype::voidtype(this->context());
    }
}

void ASTNodeInitializer::check_constraints(std::shared_ptr<SemanticReporter> reporter)
{
    if (!this->do_check())
        return;

    if (this->is_expr()) {
        this->expr()->check_constraints(reporter);
    } else {
        this->list()->check_constraints(reporter);
    }
}

void ASTNodeDesignation::check_constraints(std::shared_ptr<SemanticReporter> reporter)
{
    if (!this->do_check())
        return;

    for (auto d : this->designators()) {
        if (std::holds_alternative<array_designator_t>(d)) {
            auto& ad = std::get<array_designator_t>(d);
            auto val = ad->get_integer_constant();
            if (!val.has_value()) {
                REPORT(InvalidArrayDesignator, *ad, "not constant integer in array designator");
            }
        }
    }

    this->m_initializer->check_constraints(reporter);
}

void ASTNodeInitializerList::check_constraints(std::shared_ptr<SemanticReporter> reporter)
{
    if (!this->do_check())
        return;

    for (auto init : *this)
        init->check_constraints(reporter);
}

void ASTNodeStatLabel::check_constraints(std::shared_ptr<SemanticReporter> reporter)
{
    if (!this->do_check())
        return;

    auto ctx = this->context()->tu_context();
    assert(ctx->in_function_definition());
    this->m_stat->check_constraints(reporter);

    assert(this->id_label());
    ctx->add_idlabel(this->id_label()->id);
}

void ASTNodeStatCase::check_constraints(std::shared_ptr<SemanticReporter> reporter)
{
    if (!this->do_check())
        return;

    auto ctx = this->context()->tu_context();
    assert(ctx->in_function_definition());
    this->m_stat->check_constraints(reporter);

    auto constant = this->m_expr->get_integer_constant();
    if (!ctx->in_switch_statement()) {
        REPORT(IncompatibleTypes, *this, "case label not in switch statement");
    } else if (!constant.has_value()) {
        REPORT(IncompatibleTypes, *this, "not constant integer in case label");
    } else if (!ctx->add_caselabel(constant.value())) {
        REPORT(DuplicateCaseLabel, *this, "duplicate case label");
    }
}

void ASTNodeStatDefault::check_constraints(std::shared_ptr<SemanticReporter> reporter)
{
    if (!this->do_check())
        return;

    auto ctx = this->context()->tu_context();
    assert(ctx->in_function_definition());
    this->m_stat->check_constraints(reporter);

    if (!ctx->in_switch_statement()) {
        REPORT(IncompatibleTypes, *this, "default label not in switch statement");
    } else if (!ctx->add_defaultlabel()) {
        REPORT(DuplicateCaseLabel, *this, "duplicate default label");
    }
}

void ASTNodeBlockItemList::check_constraints(std::shared_ptr<SemanticReporter> reporter)
{
    if (!this->do_check())
        return;

    for (auto item : *this)
        item->check_constraints(reporter);
}

void ASTNodeStatCompound::check_constraints(std::shared_ptr<SemanticReporter> reporter)
{
    if (!this->do_check())
        return;

    auto ctx = this->context()->tu_context();
    assert(ctx->in_function_definition());

    ctx->enter_scope();
    this->_statlist->check_constraints(reporter);
    ctx->leave_scope();
}

void ASTNodeStatExpr::check_constraints(std::shared_ptr<SemanticReporter> reporter)
{
    if (!this->do_check())
        return;

    this->_expr->check_constraints(reporter);
}

void ASTNodeStatIF::check_constraints(std::shared_ptr<SemanticReporter> reporter)
{
    if (!this->do_check())
        return;

    const auto olderrorcounter = reporter->size();
    this->_cond->check_constraints(reporter);
    const auto check_cond_type = olderrorcounter != reporter->size();
    this->_falsestat->check_constraints(reporter);
    this->_truestat->check_constraints(reporter);

    if (check_cond_type) {
        if (!this->_cond->type()->implicit_cast_to(kstype::booltype(this->context()))) {
            REPORT(IncompatibleTypes,
                   *this,
                   "condition expression in if statement must be a type that can be implicitly "
                   "cast to 'bool'");
        }
    }
}

void ASTNodeStatSwitch::check_constraints(std::shared_ptr<SemanticReporter> reporter)
{
    if (!this->do_check())
        return;

    auto ctx = this->context()->tu_context();
    assert(ctx->in_function_definition());

    ctx->enter_switch();
    this->_stat->check_constraints(reporter);
    ctx->leave_switch();
}

void ASTNodeStatDoWhile::check_constraints(std::shared_ptr<SemanticReporter> reporter)
{
    if (!this->do_check())
        return;

    const auto olderrorcounter = reporter->size();
    this->_expr->check_constraints(reporter);
    const auto check_cond_type = olderrorcounter != reporter->size();
    this->_stat->check_constraints(reporter);

    if (check_cond_type) {
        if (!this->_expr->type()->implicit_cast_to(kstype::booltype(this->context()))) {
            REPORT(IncompatibleTypes,
                   *this,
                   "condition expression in do-while statement must be a type that can be "
                   "implicitly cast to 'bool'");
        }
    }
}

void ASTNodeStatFor::check_constraints(std::shared_ptr<SemanticReporter> reporter)
{
    if (!this->do_check())
        return;

    this->_init->check_constraints(reporter);
    const auto olderrorcounter = reporter->size();
    this->_cond->check_constraints(reporter);
    const auto check_cond_type = olderrorcounter != reporter->size();
    this->_post->check_constraints(reporter);
    this->_stat->check_constraints(reporter);

    if (check_cond_type) {
        if (!this->_cond->type()->implicit_cast_to(kstype::booltype(this->context()))) {
            REPORT(IncompatibleTypes,
                   *this,
                   "condition expression in for statement must be a type that can be implicitly "
                   "cast to 'bool'");
        }
    }
}

void ASTNodeStatForDecl::check_constraints(std::shared_ptr<SemanticReporter> reporter)
{
    if (!this->do_check())
        return;

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
            REPORT(IncompatibleTypes,
                   *this,
                   "condition expression in for statement must be a type that can be implicitly "
                   "cast to 'bool'");
        }
    }
}

void ASTNodeStatGoto::check_constraints(std::shared_ptr<SemanticReporter> reporter)
{
    if (!this->do_check())
        return;

    auto ctx = this->context()->tu_context();
    assert(ctx->in_function_definition());

    if (!ctx->is_valid_idlabel(this->_label->id)) {
        REPORT(UndefinedLabel, *this, "undefined label");
    }
}

void ASTNodeStatContinue::check_constraints(std::shared_ptr<SemanticReporter> reporter)
{
    if (!this->do_check())
        return;

    auto ctx = this->context()->tu_context();
    if (!ctx->continueable()) {
        REPORT(ContinueOutsideLoop, *this, "continue statement must be inside a loop");
    }
}

void ASTNodeStatBreak::check_constraints(std::shared_ptr<SemanticReporter> reporter)
{
    if (!this->do_check())
        return;

    auto ctx = this->context()->tu_context();
    if (!ctx->breakable()) {
        REPORT(BreakOutsideLoop, *this, "break statement must be inside a loop or switch");
    }
}

void ASTNodeStatReturn::check_constraints(std::shared_ptr<SemanticReporter> reporter)
{
    if (!this->do_check())
        return;

    auto ctx = this->context()->tu_context();
    assert(ctx->in_function_definition());

    if (this->_expr) {
        this->_expr->check_constraints(reporter);
        if (!this->_expr->type()->implicit_cast_to(ctx->return_type())) {
            REPORT(IncompatibleTypes,
                   *this,
                   "return expression must be a type that can be implicitly cast to the return "
                   "type of the function");
        }
    } else if (ctx->return_type() &&
               ctx->return_type()->basic_type() != variable_basic_type::VOID) {
        REPORT(MissingReturnValue, *this, "missing return value");
    }
}

void ASTNodeFunctionDefinition::check_constraints(std::shared_ptr<SemanticReporter> reporter)
{
    if (!this->do_check())
        return;

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
    if (!this->do_check())
        return;

    auto ctx = this->context();
    auto tctx = make_shared<CTranslationUnitContext>(reporter, ctx);
    ctx->set_tu_context(tctx);

    for (auto& decl : *this)
        decl->check_constraints(reporter);
}
