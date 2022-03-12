#include "c_translation_unit_context.h"
#include "c_parser.h"
#include <assert.h>
#include <set>
using namespace std;
using namespace cparser;
using variable_basic_type = ASTNodeVariableType::variable_basic_type;


CTranslationUnitContext::Scope::Scope(shared_ptr<SemanticReporter> reporter, shared_ptr<CParserContext> pctx):
    m_reporter(reporter), m_pctx(pctx)
{
    assert(reporter);
    assert(pctx);
}

shared_ptr<ASTNodeVariableType> CTranslationUnitContext::Scope::lookup_variable(const string& varname)
{
    if (this->m_decls.find(varname) != this->m_decls.end())
        return m_decls.at(varname);

    return nullptr;
}
shared_ptr<ASTNodeVariableType> CTranslationUnitContext::Scope::lookup_typedef (const string& typedef_name)
{
    if (this->m_typedefs.find(typedef_name) != this->m_typedefs.end())
        return m_typedefs.at(typedef_name);

    return nullptr;
}
optional<pair<size_t,shared_ptr<ASTNodeEnumeratorList>>> CTranslationUnitContext::Scope::lookup_enum(const string& enum_name)
{
    if (this->m_enums.find(enum_name) != this->m_enums.end())
        return m_enums.at(enum_name);

    return nullopt;
}
optional<pair<size_t,shared_ptr<ASTNodeStructUnionDeclarationList>>> CTranslationUnitContext::Scope::lookup_struct(const string& struct_name)
{
    if (this->m_structs.find(struct_name) != this->m_structs.end())
        return m_structs.at(struct_name);

    return nullopt;
}
optional<pair<size_t,shared_ptr<ASTNodeStructUnionDeclarationList>>> CTranslationUnitContext::Scope::lookup_union(const string& union_name)
{
    if (this->m_unions.find(union_name) != this->m_unions.end())
        return m_unions.at(union_name);

    return nullopt;
}

void CTranslationUnitContext::Scope::declare_variable(const string& varname, shared_ptr<ASTNodeVariableType> type)
{
    if (this->m_typedefs.find(varname) != this->m_typedefs.end()) {
        m_reporter->push_back(make_shared<SemanticErrorRedefinition>(
                    "redefinition of typedef '" + varname + "' as different kind of symbol",
                    type->start_pos(), type->end_pos(), this->m_pctx->posinfo()));
        return;
    }

    if (this->m_decls.find(varname) != this->m_decls.end()) {
        auto old_decl = this->m_decls.at(varname);
        assert(old_decl);

        if (!old_decl->compatible_with(type)) {
            this->m_reporter->push_back(make_shared<SemanticErrorRedefinition>(
                        "incompatible redefinition of variable '" + varname + "'",
                        type->start_pos(), type->end_pos(), this->m_pctx->posinfo()));
            return;
        }
    }

    this->m_decls[varname] = type;
}
void CTranslationUnitContext::Scope::declare_typedef(const string& typedef_name, shared_ptr<ASTNodeVariableType> type)
{
    if (this->m_decls.find(typedef_name) != this->m_decls.end()) {
        m_reporter->push_back(make_shared<SemanticErrorRedefinition>(
                    "redefinition of variable '" + typedef_name + "' as different kind of symbol",
                    type->start_pos(), type->end_pos(), this->m_pctx->posinfo()));
        return;
    }

    if (this->m_typedefs.find(typedef_name) != this->m_typedefs.end()) {
        auto old_decl = this->m_typedefs.at(typedef_name);
        assert(old_decl);

        if (!old_decl->compatible_with(type)) {
            this->m_reporter->push_back(make_shared<SemanticErrorRedefinition>(
                        "incompatible redefinition of typedef '" + typedef_name + "'",
                        type->start_pos(), type->end_pos(), this->m_pctx->posinfo()));
            return;
        }
    }

    this->m_typedefs[typedef_name] = type;
}

static size_t user_defined_type_id = 0x999;
void CTranslationUnitContext::Scope::declare_enum(const string& enum_name)
{
    if (this->m_enums.find(enum_name) != this->m_enums.end())
        return;

    this->m_enums[enum_name] = make_pair(user_defined_type_id++, nullptr);
}
void CTranslationUnitContext::Scope::declare_struct(const string& struct_name)
{
    if (this->m_structs.find(struct_name) != this->m_structs.end())
        return;

    this->m_structs[struct_name] = make_pair(user_defined_type_id++, nullptr);
}
void CTranslationUnitContext::Scope::declare_union(const string& struct_name)
{
    if (this->m_unions.find(struct_name) != this->m_unions.end())
        return;

    this->m_unions[struct_name] = make_pair(user_defined_type_id++, nullptr);
}

void CTranslationUnitContext::Scope::define_enum(const string& enum_name, shared_ptr<ASTNodeEnumeratorList> enum_node)
{
    if (this->m_enums.find(enum_name) != this->m_enums.end() &&
        this->m_enums.at(enum_name).second)
    {
        this->m_reporter->push_back(make_shared<SemanticErrorRedefinition>(
                    "redefinition of enum '" + enum_name + "'",
                    enum_node->start_pos(), enum_node->end_pos(), this->m_pctx->posinfo()));
        return;
    }

    size_t type_id;
    if (this->m_enums.find(enum_name) == this->m_enums.end()) {
        type_id = user_defined_type_id++;
    } else {
        type_id = this->m_enums.at(enum_name).first;
    }
    this->m_enums[enum_name] = make_pair(type_id, enum_node);

    auto enum_id = make_shared<TokenID>(enum_name, LexerToken::TokenInfo());
    auto enum_type = make_shared<ASTNodeVariableTypeEnum>(this->m_pctx, enum_id);
    enum_type->definition() = enum_node;

    set<string> ids;
    for (auto& enumerator: *enum_node)
    {
        const auto id = enumerator->id();
        const auto expr = enumerator->value();

        assert(id);
        if (ids.find(id->id) != ids.end()) {
            this->m_reporter->push_back(make_shared<SemanticErrorRedefinition>(
                        "redefinition of enumerator '" + id->id + "'",
                        id->position(), id->position() + id->length(), this->m_pctx->posinfo()));
            continue;
        }

        if (expr) {
            auto type = expr->type();
            assert(type);
            if (type->basic_type() != variable_basic_type::INT)
            {
                this->m_reporter->push_back(make_shared<SemanticErrorBadType>(
                            "enumerator value must be a int type",
                            expr->start_pos(), expr->end_pos(), this->m_pctx->posinfo()));
                continue;
            }

            if (!expr->is_constexpr()) {
                this->m_reporter->push_back(make_shared<SemanticErrorBadType>(
                            "enumerator value must be a constant expression",
                            expr->start_pos(), expr->end_pos(), this->m_pctx->posinfo()));
                continue;
            }
        }

        this->declare_variable(id->id, enum_type);
    }
}
void CTranslationUnitContext::Scope::define_struct(const string& struct_name, shared_ptr<ASTNodeStructUnionDeclarationList> struct_node)
{
    if (this->m_structs.find(struct_name) != this->m_structs.end() &&
        this->m_structs.at(struct_name).second)
    {
        this->m_reporter->push_back(make_shared<SemanticErrorRedefinition>(
                    "redefinition of struct '" + struct_name + "'",
                    struct_node->start_pos(), struct_node->end_pos(), this->m_pctx->posinfo()));
        return;
    }

    size_t type_id;
    if (this->m_structs.find(struct_name) == this->m_structs.end()) {
        type_id = user_defined_type_id++;
    } else {
        type_id = this->m_structs.at(struct_name).first;
    }
    this->m_structs[struct_name] = make_pair(type_id, struct_node);
}
void CTranslationUnitContext::Scope::define_union(const string& union_name,  shared_ptr<ASTNodeStructUnionDeclarationList> union_node)
{
    if (this->m_unions.find(union_name) != this->m_unions.end() &&
        this->m_unions.at(union_name).second)
    {
        this->m_reporter->push_back(make_shared<SemanticErrorRedefinition>(
                    "redefinition of union '" + union_name + "'",
                    union_node->start_pos(), union_node->end_pos(), this->m_pctx->posinfo()));
        return;
    }

    size_t type_id;
    if (this->m_unions.find(union_name) == this->m_unions.end()) {
        type_id = user_defined_type_id++;
    } else {
        type_id = this->m_unions.at(union_name).first;
    }
    this->m_unions[union_name] = make_pair(type_id, union_node);
}

optional<int> CTranslationUnitContext::Scope::resolve_enum_constant(const string& enumtag, const string& id)
{
    auto enumx = this->lookup_enum(enumtag);
    if (!enumx.has_value())
        return nullopt;

    auto& vals = enumx.value().second;
    for (auto& def: *vals) {
        assert(def && def->id());
        if (def->id()->id == id)
        {
            return def->value()->get_integer_constant();
        }
    }

    return nullopt;
}


CTranslationUnitContext::CTranslationUnitContext(shared_ptr<SemanticReporter> reporter, shared_ptr<CParserContext> pctx)
    : m_reporter(reporter), m_pctx(pctx)
{
    this->m_scopes.emplace_back(reporter, pctx);
    this->m_function_fake_scope = false;
    this->m_loop_level_counter = 0;
    this->m_switch_level_counter = 0;
}

shared_ptr<ASTNodeVariableType> CTranslationUnitContext::lookup_variable(const string& varname)
{
    for (auto rb = this->m_scopes.rbegin(); rb != this->m_scopes.rend(); ++rb)
    {
        auto& scope = *rb;
        if (auto var = scope.lookup_variable(varname))
            return var;
    }
    return nullptr;
}
shared_ptr<ASTNodeVariableType> CTranslationUnitContext::lookup_typedef (const string& typedef_name)
{
    for (auto rb = this->m_scopes.rbegin(); rb != this->m_scopes.rend(); ++rb)
    {
        auto& scope = *rb;
        if (auto td = scope.lookup_typedef(typedef_name))
            return td;
    }
    return nullptr;
}
optional<pair<size_t,shared_ptr<ASTNodeEnumeratorList>>> CTranslationUnitContext::lookup_enum(const string& enum_name)
{
    for (auto rb = this->m_scopes.rbegin(); rb != this->m_scopes.rend(); ++rb)
    {
        auto& scope = *rb;
        auto em = scope.lookup_enum(enum_name);
        if (em.has_value())
            return em;
    }
    return nullopt;
}
optional<pair<size_t,shared_ptr<ASTNodeStructUnionDeclarationList>>> CTranslationUnitContext::lookup_struct(const string& struct_name)
{
    for (auto rb = this->m_scopes.rbegin(); rb != this->m_scopes.rend(); ++rb)
    {
        auto& scope = *rb;
        auto st = scope.lookup_struct(struct_name);
        if (st.has_value())
            return st;
    }
    return nullopt;
}
optional<pair<size_t,shared_ptr<ASTNodeStructUnionDeclarationList>>> CTranslationUnitContext::lookup_union (const string& union_name)
{
    for (auto rb = this->m_scopes.rbegin(); rb != this->m_scopes.rend(); ++rb)
    {
        auto& scope = *rb;
        auto un = scope.lookup_union(union_name);
        if (un.has_value())
            return un;
    }
    return nullopt;
}

void CTranslationUnitContext::function_begin(
        const string& funcname, shared_ptr<ASTNodeVariableType> ret_type,
        shared_ptr<ASTNodeParameterDeclarationList> parameter_list)
{
    assert(this->m_function_fake_scope == false);
    assert(this->m_scopes.size() == 1);
    assert(!this->m_return_type.has_value());

    if (this->m_defined_functions.find(funcname) != this->m_defined_functions.end()) {
        this->m_reporter->push_back(make_shared<SemanticErrorRedefinition>(
                    "redefinition of function '" + funcname + "'",
                    parameter_list->start_pos(), parameter_list->end_pos(), this->m_pctx->posinfo()));
    }

    this->m_defined_functions.insert(funcname);
    this->m_return_type = ret_type;
    this->enter_scope();

    for (auto& param : *parameter_list)
    {
        auto id = param->id();
        assert(id && param->type());
        this->declare_variable(id->id, param->type());
    }

    this->m_function_fake_scope = true;
}
shared_ptr<ASTNodeVariableType> CTranslationUnitContext::return_type()
{
    assert(this->m_return_type.has_value());
    return this->m_return_type.value();
}
bool CTranslationUnitContext::in_function_definition()
{
    return this->m_return_type.has_value();
}
void CTranslationUnitContext::function_end()
{
    assert(this->in_function_definition());
    this->m_return_type = nullopt;
}

void CTranslationUnitContext::enter_loop()
{
    this->m_loop_level_counter++;
}
void CTranslationUnitContext::leave_loop()
{
    assert(this->m_loop_level_counter > 0);
    this->m_loop_level_counter--;
}
void CTranslationUnitContext::enter_switch()
{
    this->m_switch_level_counter++;
}
void CTranslationUnitContext::leave_switch()
{
    assert(this->m_switch_level_counter > 0);
    this->m_switch_level_counter--;
}
bool CTranslationUnitContext::breakable()
{
    return this->m_loop_level_counter > 0 || this->m_switch_level_counter > 0;
}
bool CTranslationUnitContext::continueable()
{
    return this->m_loop_level_counter > 0;
}

void CTranslationUnitContext::enter_scope()
{
    if (this->m_function_fake_scope) {
        this->m_function_fake_scope = false;
        return;
    }

    this->m_scopes.emplace_back(this->m_reporter, this->m_pctx);
}
void CTranslationUnitContext::leave_scope()
{
    assert(this->m_scopes.size() > 1);
    this->m_scopes.pop_back();
}

void CTranslationUnitContext::declare_variable(const string& varname, shared_ptr<ASTNodeVariableType> type)
{
    this->m_scopes.back().declare_variable(varname, type);
}
void CTranslationUnitContext::declare_typedef(const string& typedef_name, shared_ptr<ASTNodeVariableType> type)
{
    this->m_scopes.back().declare_typedef(typedef_name, type);
}
void CTranslationUnitContext::declare_enum(const string& enum_name)
{
    this->m_scopes.back().declare_enum(enum_name);
}
void CTranslationUnitContext::declare_struct(const string& struct_name)
{
    this->m_scopes.back().declare_struct(struct_name);
}
void CTranslationUnitContext::declare_union(const string& struct_name)
{
    this->m_scopes.back().declare_union(struct_name);
}

void CTranslationUnitContext::define_enum(const string& enum_name, shared_ptr<ASTNodeEnumeratorList> enum_node)
{
    this->m_scopes.back().define_enum(enum_name, enum_node);
}
void CTranslationUnitContext::define_struct(const string& struct_name, shared_ptr<ASTNodeStructUnionDeclarationList> struct_node)
{
    this->m_scopes.back().define_struct(struct_name, struct_node);
}
void CTranslationUnitContext::define_union(const string& union_name, shared_ptr<ASTNodeStructUnionDeclarationList> union_node)
{
    this->m_scopes.back().define_union(union_name, union_node);
}

optional<int> CTranslationUnitContext::resolve_enum_constant(const string& enumtag, const string& id)
{
    for (auto rb = this->m_scopes.rbegin(); rb != this->m_scopes.rend(); ++rb)
    {
        auto& scope = *rb;
        auto ec = scope.resolve_enum_constant(enumtag, id);
        if (ec.has_value())
            return ec;
    }

    return nullopt;
}
