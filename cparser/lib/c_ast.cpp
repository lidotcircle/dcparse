#include "c_ast.h"
#include "c_parser.h"
#include <stdexcept>
using namespace std;
using variable_basic_type = cparser::ASTNodeVariableType::variable_basic_type;
#define MIN_AB(a, b) ((a) < (b) ? (a) : (b))
#define MAX_AB(a, b) ((a) > (b) ? (a) : (b))

namespace cparser {


shared_ptr<CParserContext> ASTNode::context() const
{
    auto c1 = this->m_parser_context.lock();;
    assert(c1);
    auto ret = dynamic_pointer_cast<CParserContext>(c1);
    assert(ret);
    return ret;
}

string ASTNode::to_string() const
{
    auto ctx = this->context();
    auto pi = ctx->textinfo();
    size_t beg = 0, end = 0;
    if (this->range()) {
        beg = this->beg().value();
        end = this->end().value();
    }
    return pi->query_string(beg, end);
}

void ASTNodeInitDeclarator::set_leaf_type(std::shared_ptr<ASTNodeVariableType> type)
{
    if (this->m_type == nullptr) {
        this->m_type = type;
        return;
    }

    const auto bt = this->m_type->basic_type();
    this->m_type->set_leaf_type(type);
}

void ASTNodeStructUnionDeclaration::set_leaf_type(std::shared_ptr<ASTNodeVariableType> type)
{
    if (this->m_decl == nullptr)
        this->m_decl = make_shared<ASTNodeInitDeclarator>(this->context(), nullptr, nullptr, nullptr);

    this->m_decl->set_leaf_type(type);
}

bool ASTNodeInitializer::is_constexpr() const
{
    if (std::holds_alternative<std::shared_ptr<ASTNodeExpr>>(this->m_value))
    {
        return std::get<std::shared_ptr<ASTNodeExpr>>(this->m_value)->is_constexpr();
    }
    else
    {
        return std::get<std::shared_ptr<ASTNodeInitializerList>>(this->m_value)->is_constexpr();
    }
}

};
