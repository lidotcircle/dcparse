#include "c_ast.h"
#include <stdexcept>
using namespace std;
using variable_basic_type = cparser::ASTNodeVariableType::variable_basic_type;

namespace cparser {

variable_basic_type ASTNodeVariableTypePlain::basic_type()
{
    return variable_basic_type::PLAIN;
}

void ASTNodeVariableTypePlain::set_declaration_specifier(shared_ptr<ASTNodeDeclarationSpecifier> ds)
{
    throw runtime_error("unexpected calling ASTNodeVariableTypePlain::set_declaration_specifier()");
}


variable_basic_type ASTNodeVariableTypeArray::basic_type()
{
    return variable_basic_type::ARRAY;
}

void ASTNodeVariableTypeArray::set_declaration_specifier(shared_ptr<ASTNodeDeclarationSpecifier> ds)
{
    if (this->m_type == nullptr) {
        this->m_type = make_shared<ASTNodeVariableTypePlain>(this->context(), ds);
        return;
    }

    const auto bt = this->m_type->basic_type();
    assert(bt != variable_basic_type::PLAIN && "ARRAY: declaration specifier should only be set once");
    this->m_type->set_declaration_specifier(ds);
}


variable_basic_type ASTNodeVariableTypePointer::basic_type()
{
    return variable_basic_type::POINTER;
}

void ASTNodeVariableTypePointer::set_declaration_specifier(shared_ptr<ASTNodeDeclarationSpecifier> ds)
{
    if (this->m_type == nullptr) {
        this->m_type = make_shared<ASTNodeVariableTypePlain>(this->context(), ds);
        return;
    }

    const auto bt = this->m_type->basic_type();
    assert(bt != variable_basic_type::PLAIN && "PTR: declaration specifier should only be set once");
    this->m_type->set_declaration_specifier(ds);
}


variable_basic_type ASTNodeVariableTypeFunction::basic_type()
{
    return variable_basic_type::FUNCTION;
}

void ASTNodeVariableTypeFunction::set_declaration_specifier(shared_ptr<ASTNodeDeclarationSpecifier> ds)
{
    if (this->m_return_type == nullptr) {
        this->m_return_type = make_shared<ASTNodeVariableTypePlain>(this->context(), ds);
        return;
    }

    const auto bt = this->m_return_type->basic_type();
    assert(bt != variable_basic_type::PLAIN && "FUNC: declaration specifier should only be set once");
    this->m_return_type->set_declaration_specifier(ds);
}


void ASTNodeInitDeclarator::set_declaration_specifier(std::shared_ptr<ASTNodeDeclarationSpecifier> specifier)
{
    if (this->m_type == nullptr) {
        this->m_type = make_shared<ASTNodeVariableTypePlain>(this->context(), specifier);
        return;
    }

    const auto bt = this->m_type->basic_type();
    assert(bt != variable_basic_type::PLAIN && "INIT: declaration specifier should only be set once");
    this->m_type->set_declaration_specifier(specifier);
}

};
