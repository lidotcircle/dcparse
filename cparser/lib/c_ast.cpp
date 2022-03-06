#include "c_ast.h"
#include <stdexcept>
using namespace std;
using data_type = cparser::ASTNodeTypeSpecifier::data_type;
using variable_basic_type = cparser::ASTNodeVariableType::variable_basic_type;

namespace cparser {

data_type ASTNodeTypeSpecifierStruct::dtype()  const { return data_type::STRUCT; }
data_type ASTNodeTypeSpecifierUnion::dtype()   const { return data_type::STRUCT; }
data_type ASTNodeTypeSpecifierEnum::dtype()    const { return data_type::ENUM; }
data_type ASTNodeTypeSpecifierTypedef::dtype() const { return data_type::TYPEDEF; }
data_type ASTNodeTypeSpecifierInt::dtype()     const { return data_type::INT; }
data_type ASTNodeTypeSpecifierFloat::dtype()   const { return data_type::INT; }
data_type ASTNodeTypeSpecifierVoid::dtype()    const { return data_type::INT; }


variable_basic_type ASTNodeVariableTypePlain::basic_type()
{
    return variable_basic_type::PLAIN;
}

void ASTNodeVariableTypePlain::set_leaf_type(shared_ptr<ASTNodeVariableType> type)
{
    throw runtime_error("unexpected calling ASTNodeVariableTypePlain::set_declaration_specifier()");
}


variable_basic_type ASTNodeVariableTypeArray::basic_type()
{
    return variable_basic_type::ARRAY;
}

void ASTNodeVariableTypeArray::set_leaf_type(shared_ptr<ASTNodeVariableType> type)
{
    if (this->m_type == nullptr) {
        this->m_type = type;
        return;
    }

    const auto bt = this->m_type->basic_type();
    assert(bt != variable_basic_type::PLAIN && "ARRAY: declaration specifier should only be set once");
    this->m_type->set_leaf_type(type);
}


variable_basic_type ASTNodeVariableTypePointer::basic_type()
{
    return variable_basic_type::POINTER;
}

void ASTNodeVariableTypePointer::set_leaf_type(shared_ptr<ASTNodeVariableType> type)
{
    if (this->m_type == nullptr) {
        this->m_type = type;
        return;
    }

    const auto bt = this->m_type->basic_type();
    assert(bt != variable_basic_type::PLAIN && "PTR: declaration specifier should only be set once");
    this->m_type->set_leaf_type(type);
}


variable_basic_type ASTNodeVariableTypeFunction::basic_type()
{
    return variable_basic_type::FUNCTION;
}

void ASTNodeVariableTypeFunction::set_leaf_type(shared_ptr<ASTNodeVariableType> type)
{
    if (this->m_return_type == nullptr) {
        this->m_return_type = type;
        return;
    }

    const auto bt = this->m_return_type->basic_type();
    assert(bt != variable_basic_type::PLAIN && "FUNC: declaration specifier should only be set once");
    this->m_return_type->set_leaf_type(type);
}


void ASTNodeInitDeclarator::set_leaf_type(std::shared_ptr<ASTNodeVariableType> type)
{
    if (this->m_type == nullptr) {
        this->m_type = type;
        return;
    }

    const auto bt = this->m_type->basic_type();
    assert(bt != variable_basic_type::PLAIN && "INIT: declaration specifier should only be set once");
    this->m_type->set_leaf_type(type);
}

void ASTNodeStructUnionDeclaration::set_leaf_type(std::shared_ptr<ASTNodeVariableType> type)
{
    if (this->m_decl == nullptr)
        this->m_decl = make_shared<ASTNodeInitDeclarator>(this->context(), nullptr, type, nullptr);

    this->m_decl->set_leaf_type(type);
}

};
