#include "c_ast.h"
#include <map>
using namespace std;
using namespace cparser;
using TestFlags = ASTNodeVariableType::TestFlags;
using storage_class_t = ASTNodeVariableType::storage_class_t;
using variable_basic_type = ASTNodeVariableType::variable_basic_type;


bool& ASTNodeVariableType::const_ref() { return this->m_const; }
bool& ASTNodeVariableType::volatile_ref() { return this->m_volatile; }
bool& ASTNodeVariableType::restrict_ref() { return this->m_restrict; }

bool ASTNodeVariableType::equal_wrt_declaration(shared_ptr<ASTNodeVariableType> other) const
{
    TestFlags flags;

    flags.ignore_storage_class_static = true;
    flags.ignore_storage_class_extern = true;
    flags.ignore_storage_class_register = true;
    flags.ignore_storage_class_auto = true;
    flags.ignore_storage_class_default = true;

    flags.ignore_qualifiers_const = false;
    flags.ignore_qualifiers_volatile = false;
    flags.ignore_qualifiers_restrict = false;

    flags.pointer_to_voidptr = false;
    flags.voidptr_to_pointer = false;
    flags.array_to_pointer = true;
    flags.pointer_to_array = true;
    flags.ignore_array_size = true;
    flags.function_to_pointer = false;
    flags.pointer_to_function = false;
    flags.pointer_to_integer = false;
    flags.integer_to_pointer = false;
    flags.enum_to_integer = false;
    flags.integer_to_enum = false;

    flags.ignore_integer_sign = false;
    flags.integer_downcast = false;
    flags.integer_upcast = false;
    flags.float_to_integer = false;
    flags.integer_to_float = false;
    flags.float_downcast = false;
    flags.float_upcast = false;

    flags.any_to_void = false;

    if (!this->equal_to(other, flags))
        return false;;

    if (this->storage_class() == other->storage_class())
        return true;

    static map<storage_class_t,map<storage_class_t,bool>> storage_class_map = 
    {
        { storage_class_t::SC_Default, {
            { storage_class_t::SC_Static, false },
            { storage_class_t::SC_Extern, true },
            { storage_class_t::SC_Register, false },
            { storage_class_t::SC_Auto, true },
            { storage_class_t::SC_Default, true },
        } },
        { storage_class_t::SC_Static, {
            { storage_class_t::SC_Static, true },
            { storage_class_t::SC_Extern, false },
            { storage_class_t::SC_Register, false },
            { storage_class_t::SC_Auto, false },
            { storage_class_t::SC_Default, false },
        } },
        { storage_class_t::SC_Extern, {
            { storage_class_t::SC_Static, false },
            { storage_class_t::SC_Extern, true },
            { storage_class_t::SC_Register, false },
            { storage_class_t::SC_Auto, true },
            { storage_class_t::SC_Default, true },
        } },
        { storage_class_t::SC_Register, {
            { storage_class_t::SC_Static, false },
            { storage_class_t::SC_Extern, false },
            { storage_class_t::SC_Register, true },
            { storage_class_t::SC_Auto, false },
            { storage_class_t::SC_Default, false },
        } },
        { storage_class_t::SC_Auto, {
            { storage_class_t::SC_Static, false },
            { storage_class_t::SC_Extern, true },
            { storage_class_t::SC_Register, false },
            { storage_class_t::SC_Auto, true },
            { storage_class_t::SC_Default, true },
        } },
    };
    return storage_class_map[this->storage_class()][other->storage_class()];
}

bool ASTNodeVariableType::equal_ignore_storage_class(shared_ptr<ASTNodeVariableType> other) const
{
    TestFlags flags;

    flags.ignore_storage_class_static = true;
    flags.ignore_storage_class_extern = true;
    flags.ignore_storage_class_register = true;
    flags.ignore_storage_class_auto = true;
    flags.ignore_storage_class_default = true;

    flags.ignore_qualifiers_const = false;
    flags.ignore_qualifiers_volatile = false;
    flags.ignore_qualifiers_restrict = false;

    flags.pointer_to_voidptr = false;
    flags.voidptr_to_pointer = false;
    flags.array_to_pointer = false;
    flags.pointer_to_array = false;
    flags.ignore_array_size = false;
    flags.function_to_pointer = false;
    flags.pointer_to_function = false;
    flags.pointer_to_integer = false;
    flags.integer_to_pointer = false;
    flags.enum_to_integer = false;
    flags.integer_to_enum = false;

    flags.ignore_integer_sign = false;
    flags.integer_downcast = false;
    flags.integer_upcast = false;
    flags.float_to_integer = false;
    flags.integer_to_float = false;
    flags.float_downcast = false;
    flags.float_upcast = false;

    flags.any_to_void = false;

    return this->equal_to(other, flags);
}

bool ASTNodeVariableType::equal_implicitly(std::shared_ptr<ASTNodeVariableType> type) const
{
    TestFlags flags;

    flags.ignore_storage_class_static = true;
    flags.ignore_storage_class_extern = true;
    flags.ignore_storage_class_register = true;
    flags.ignore_storage_class_auto = true;
    flags.ignore_storage_class_default = true;

    flags.ignore_qualifiers_const = true;
    flags.ignore_qualifiers_volatile = true;
    flags.ignore_qualifiers_restrict = true;

    flags.pointer_to_voidptr = true;
    flags.voidptr_to_pointer = true;
    flags.array_to_pointer = true;
    flags.pointer_to_array = false;
    flags.ignore_array_size = false;
    flags.function_to_pointer = false;
    flags.pointer_to_function = false;
    flags.pointer_to_integer = false;
    flags.integer_to_pointer = false;
    flags.enum_to_integer = false;
    flags.integer_to_enum = false;

    flags.ignore_integer_sign = false;
    flags.integer_downcast = false;
    flags.integer_upcast = false;
    flags.float_to_integer = false;
    flags.integer_to_float = false;
    flags.float_downcast = false;
    flags.float_upcast = false;

    flags.any_to_void = false;

    return this->equal_to(type, flags);
}

shared_ptr<ASTNodeVariableType> ASTNodeVariableType::implicitly_convert_to(std::shared_ptr<ASTNodeVariableType> type) const
{
    TestFlags flags;

    flags.ignore_storage_class_static = true;
    flags.ignore_storage_class_extern = true;
    flags.ignore_storage_class_register = true;
    flags.ignore_storage_class_auto = true;
    flags.ignore_storage_class_default = true;

    flags.ignore_qualifiers_const = true;
    flags.ignore_qualifiers_volatile = true;
    flags.ignore_qualifiers_restrict = true;

    flags.pointer_to_voidptr = true;
    flags.voidptr_to_pointer = true;
    flags.array_to_pointer = true;
    flags.pointer_to_array = false;
    flags.ignore_array_size = false;
    flags.function_to_pointer = false;
    flags.pointer_to_function = false;
    flags.pointer_to_integer = false;
    flags.integer_to_pointer = false;
    flags.enum_to_integer = false;
    flags.integer_to_enum = false;

    flags.ignore_integer_sign = false;
    flags.integer_downcast = false;
    flags.integer_upcast = false;
    flags.float_to_integer = false;
    flags.integer_to_float = false;
    flags.float_downcast = false;
    flags.float_upcast = false;

    flags.any_to_void = false;

    if (this->equal_to(type, flags))
    {
        auto t = type->copy();
        t->storage_class() = storage_class_t::SC_Default;
        t->inlined() = false;
        return t;
    }

    return nullptr;
}

bool ASTNodeVariableType::equal_to(shared_ptr<ASTNodeVariableType> type, TestFlags& flags) const
{
    if (this->storage_class() != type->storage_class())
    {
        bool this_ignore = false, type_ignore = false;
        switch (this->storage_class())
        {
            case storage_class_t::SC_Extern: this_ignore = flags.ignore_storage_class_extern; break;
            case storage_class_t::SC_Static: this_ignore = flags.ignore_storage_class_static; break;
            case storage_class_t::SC_Auto: this_ignore = flags.ignore_storage_class_auto; break;
            case storage_class_t::SC_Register: this_ignore = flags.ignore_storage_class_register; break;
            default: break;
        }
        switch (type->storage_class())
        {
            case storage_class_t::SC_Extern: type_ignore = flags.ignore_storage_class_extern; break;
            case storage_class_t::SC_Static: type_ignore = flags.ignore_storage_class_static; break;
            case storage_class_t::SC_Auto: type_ignore = flags.ignore_storage_class_auto; break;
            case storage_class_t::SC_Register: type_ignore = flags.ignore_storage_class_register; break;
            default: break;
        }

        if (!this_ignore || !type_ignore) return false;
    }

    auto _this = const_cast<ASTNodeVariableType*>(this);
    if (_this->const_ref() != type->const_ref() && !flags.ignore_qualifiers_const)
        return false;

    if (_this->volatile_ref() != type->volatile_ref() && !flags.ignore_qualifiers_volatile)
        return false;

    if (_this->restrict_ref() != type->restrict_ref() && !flags.ignore_qualifiers_restrict)
        return false;

    return true;
}


storage_class_t& ASTNodeVariableTypePlain::storage_class()
{
    return this->m_storage_class;
}
bool& ASTNodeVariableTypePlain::inlined()
{
    return this->m_inlined;
}
void ASTNodeVariableTypePlain::set_leaf_type(std::shared_ptr<ASTNodeVariableType> type)
{
    throw std::runtime_error("ASTNodeVariableTypePlain::set_leaf_type() is not permitted, maybe incorrectly call it multiple times?");
}


variable_basic_type ASTNodeVariableTypeDummy::basic_type() const
{
    return variable_basic_type::DUMMY;
}

bool ASTNodeVariableTypeDummy::equal_to(std::shared_ptr<ASTNodeVariableType> type, TestFlags& flags) const
{
    throw std::runtime_error("ASTNodeVariableTypeDummy::copy() unexpected");
}

size_t ASTNodeVariableTypeDummy::opsizeof() const
{
    throw std::runtime_error("ASTNodeVariableTypeDummy::copy() unexpected");
}

size_t ASTNodeVariableTypeDummy::opalignof() const
{
    throw std::runtime_error("ASTNodeVariableTypeDummy::copy() unexpected");
}

std::shared_ptr<ASTNodeVariableType> ASTNodeVariableTypeDummy::copy() const
{
    throw std::runtime_error("ASTNodeVariableTypeDummy::copy() unexpected");
}


variable_basic_type ASTNodeVariableTypeStruct::basic_type() const
{
    return variable_basic_type::STRUCT;
}

bool ASTNodeVariableTypeStruct::equal_to(std::shared_ptr<ASTNodeVariableType> type, TestFlags& flags) const
{
    if (!ASTNodeVariableType::equal_to(type, flags))
        return false;

    if (type->basic_type() == variable_basic_type::VOID && flags.any_to_void)
        return true;

    if (type->basic_type() != variable_basic_type::STRUCT)
        return false;

    auto _type = dynamic_pointer_cast<ASTNodeVariableTypeStruct>(type);
    assert(_type);

    assert(this->m_definition && _type->m_definition);
    return this->m_definition == _type->m_definition;
}

size_t ASTNodeVariableTypeStruct::opsizeof() const
{
    assert(this->m_definition);
    return this->m_definition->sizeof_().value();
}

size_t ASTNodeVariableTypeStruct::opalignof() const
{
    assert(this->m_definition);
    return this->m_definition->alignment().value();
}

std::shared_ptr<ASTNodeVariableType> ASTNodeVariableTypeStruct::copy() const
{
    assert(this->m_definition);
    auto ast = std::make_shared<ASTNodeVariableTypeStruct>(this->lcontext(), this->m_name);
    ast->m_definition = this->m_definition;
    ast->const_ref() = this->const_ref();
    ast->volatile_ref() = this->volatile_ref();
    ast->restrict_ref() = this->restrict_ref();
    const ASTNodeVariableType* _this = this;
    ast->storage_class() = _this->storage_class();
    return ast;
}

variable_basic_type ASTNodeVariableTypeUnion::basic_type() const
{
    return variable_basic_type::UNION;
}

bool ASTNodeVariableTypeUnion::equal_to(std::shared_ptr<ASTNodeVariableType> type, TestFlags& flags) const
{
    if (!ASTNodeVariableType::equal_to(type, flags))
        return false;

    if (type->basic_type() == variable_basic_type::VOID && flags.any_to_void)
        return true;

    if (type->basic_type() != variable_basic_type::UNION)
        return false;

    auto _type = dynamic_pointer_cast<ASTNodeVariableTypeUnion>(type);
    assert(_type);

    assert(this->m_definition && _type->m_definition);
    return this->m_definition == _type->m_definition;
}

size_t ASTNodeVariableTypeUnion::opsizeof() const
{
    assert(this->m_definition);
    return this->m_definition->sizeof_().value();
}

size_t ASTNodeVariableTypeUnion::opalignof() const
{
    assert(this->m_definition);
    return this->m_definition->alignment().value();
}

std::shared_ptr<ASTNodeVariableType> ASTNodeVariableTypeUnion::copy() const
{
    assert(this->m_definition);
    auto ast = std::make_shared<ASTNodeVariableTypeUnion>(this->lcontext(), this->m_name);
    ast->m_definition = this->m_definition;
    ast->const_ref() = this->const_ref();
    ast->volatile_ref() = this->volatile_ref();
    ast->restrict_ref() = this->restrict_ref();
    const ASTNodeVariableType* _this = this;
    ast->storage_class() = _this->storage_class();
    return ast;
}


variable_basic_type ASTNodeVariableTypeEnum::basic_type() const
{
    return variable_basic_type::ENUM;
}

bool ASTNodeVariableTypeEnum::equal_to(std::shared_ptr<ASTNodeVariableType> type, TestFlags& flags) const
{
    if (!ASTNodeVariableType::equal_to(type, flags))
        return false;

    switch (this->basic_type()) 
    {
    case variable_basic_type::ENUM:
    {
        auto _type = dynamic_pointer_cast<ASTNodeVariableTypeEnum>(type);
        assert(_type);
        if (_type->m_definition != this->m_definition)
            return true;
    } break;
    case variable_basic_type::INT:
        if (flags.enum_to_integer)
            return true;
        break;
    case variable_basic_type::VOID:
        if (flags.any_to_void)
            return true;
    default:
        break;
    }

    return false;
}

size_t ASTNodeVariableTypeEnum::opsizeof() const
{
    return sizeof(int);
}

size_t ASTNodeVariableTypeEnum::opalignof() const
{
    return alignof(int);
}

std::shared_ptr<ASTNodeVariableType> ASTNodeVariableTypeEnum::copy() const
{
    auto ast = std::make_shared<ASTNodeVariableTypeEnum>(this->lcontext(), this->m_name);
    ast->m_definition = this->m_definition;
    ast->const_ref() = this->const_ref();
    ast->volatile_ref() = this->volatile_ref();
    ast->restrict_ref() = this->restrict_ref();
    const ASTNodeVariableType* _this = this;
    ast->storage_class() = _this->storage_class();
    return ast;
}


variable_basic_type ASTNodeVariableTypeTypedef::basic_type() const
{
    if (!this->type())
        throw std::runtime_error("unresolved typedef");

    return this->type()->basic_type();
}

bool ASTNodeVariableTypeTypedef::equal_to(std::shared_ptr<ASTNodeVariableType> type, TestFlags& flags) const
{
    if (!this->type())
        throw std::runtime_error("unresolved typedef");

    return this->type()->equal_to(type, flags);
}

size_t ASTNodeVariableTypeTypedef::opsizeof() const
{
    if (!this->type())
        throw std::runtime_error("unresolved typedef");

    return this->type()->opsizeof();
}

size_t ASTNodeVariableTypeTypedef::opalignof() const
{
    if (!this->type())
        throw std::runtime_error("unresolved typedef");

    return this->type()->opalignof();
}

std::shared_ptr<ASTNodeVariableType> ASTNodeVariableTypeTypedef::copy() const
{
    if (!this->type())
        throw std::runtime_error("unresolved typedef");

    return this->type()->copy();
}


variable_basic_type ASTNodeVariableTypeVoid::basic_type() const
{
    return variable_basic_type::VOID;
}

bool ASTNodeVariableTypeVoid::equal_to(std::shared_ptr<ASTNodeVariableType> type, TestFlags& flags) const
{
    if (!ASTNodeVariableType::equal_to(type, flags))
        return false;

    return type->basic_type() == variable_basic_type::VOID;
}

size_t ASTNodeVariableTypeVoid::opsizeof() const
{
    return 0;
}

size_t ASTNodeVariableTypeVoid::opalignof() const
{
    return 0;
}

std::shared_ptr<ASTNodeVariableType> ASTNodeVariableTypeVoid::copy() const
{
    auto ast = std::make_shared<ASTNodeVariableTypeVoid>(this->lcontext());
    ast->const_ref() = this->const_ref();
    ast->volatile_ref() = this->volatile_ref();
    ast->restrict_ref() = this->restrict_ref();
    const ASTNodeVariableType* _this = this;
    ast->storage_class() = _this->storage_class();
    return ast;
}


variable_basic_type ASTNodeVariableTypeInt::basic_type() const
{
    return variable_basic_type::INT;
}

bool ASTNodeVariableTypeInt::equal_to(std::shared_ptr<ASTNodeVariableType> type, TestFlags& flags) const
{
    if (!ASTNodeVariableType::equal_to(type, flags))
        return false;

    switch (type->basic_type())
    {
    case variable_basic_type::VOID:
    {
        if (flags.any_to_void)
            return true;
    } break;
    case variable_basic_type::INT:
    {
        auto _type = dynamic_pointer_cast<ASTNodeVariableTypeInt>(type);
        assert(_type);

        if (!flags.ignore_integer_sign && _type->m_is_unsigned != this->m_is_unsigned)
            break;

        if (this->m_byte_length == _type->m_byte_length)
            return true;

        if (flags.integer_downcast && this->m_byte_length > _type->m_byte_length)
            return true;

        if (flags.integer_upcast && this->m_byte_length < _type->m_byte_length)
            return true;
    } break;
    case variable_basic_type::FLOAT:
    {
        if (flags.integer_to_float)
            return true;
    } break;
    case variable_basic_type::POINTER:
    {
        if (this->m_byte_length < type->opsizeof()) {
            // TODO warning
            return false;
        }

        if (flags.integer_to_pointer)
            return true;
    }
    case variable_basic_type::ARRAY:
    {
        // TODO to pointer
        if (flags.integer_to_pointer && flags.pointer_to_array)
            return true;
    }
    default:
        break;
    }

    return false;
}

size_t ASTNodeVariableTypeInt::opsizeof() const
{
    return this->m_byte_length;
}

size_t ASTNodeVariableTypeInt::opalignof() const
{
    return this->m_byte_length;
}

std::shared_ptr<ASTNodeVariableType> ASTNodeVariableTypeInt::copy() const
{
    auto ast = std::make_shared<ASTNodeVariableTypeInt>(this->lcontext(), this->m_byte_length, this->m_is_unsigned);
    ast->const_ref() = this->const_ref();
    ast->volatile_ref() = this->volatile_ref();
    ast->restrict_ref() = this->restrict_ref();
    const ASTNodeVariableType* _this = this;
    ast->storage_class() = _this->storage_class();
    return ast;
}


variable_basic_type ASTNodeVariableTypeFloat::basic_type() const
{
    return variable_basic_type::FLOAT;
}

bool ASTNodeVariableTypeFloat::equal_to(std::shared_ptr<ASTNodeVariableType> type, TestFlags& flags) const
{
    if (!ASTNodeVariableType::equal_to(type, flags))
        return false;

    switch (type->basic_type())
    {
    case variable_basic_type::VOID:
    {
        if (flags.any_to_void)
            return true;
    } break;
    case variable_basic_type::INT:
    {
        if (flags.float_to_integer)
            return true;
    } break;
    case variable_basic_type::FLOAT:
    {
        auto _type = dynamic_pointer_cast<ASTNodeVariableTypeFloat>(type);
        assert(_type);

        if (this->m_byte_length == _type->m_byte_length)
            return true;

        if (flags.float_downcast && this->m_byte_length > _type->m_byte_length)
            return true;

        if (flags.float_upcast && this->m_byte_length < _type->m_byte_length)
            return true;
    } break;
    default:
        break;
    }

    return false;
}

size_t ASTNodeVariableTypeFloat::opsizeof() const
{
    return this->m_byte_length;
}

size_t ASTNodeVariableTypeFloat::opalignof() const
{
    return this->m_byte_length;
}

std::shared_ptr<ASTNodeVariableType> ASTNodeVariableTypeFloat::copy() const
{
    auto ast = std::make_shared<ASTNodeVariableTypeFloat>(this->lcontext(), this->m_byte_length);
    ast->const_ref() = this->const_ref();
    ast->volatile_ref() = this->volatile_ref();
    ast->restrict_ref() = this->restrict_ref();
    const ASTNodeVariableType* _this = this;
    ast->storage_class() = _this->storage_class();
    return ast;
}


variable_basic_type ASTNodeVariableTypePointer::basic_type() const
{
    return variable_basic_type::POINTER;
}

bool ASTNodeVariableTypePointer::equal_to(std::shared_ptr<ASTNodeVariableType> type, TestFlags& flags) const
{
    if (!ASTNodeVariableType::equal_to(type, flags))
        return false;

    switch (type->basic_type())
    {
    case variable_basic_type::VOID:
    {
        if (flags.any_to_void)
            return true;
    } break;
    case variable_basic_type::POINTER:
    {
        auto _type = dynamic_pointer_cast<ASTNodeVariableTypePointer>(type);

        if (_type->deref()->basic_type() == variable_basic_type::VOID && flags.pointer_to_voidptr)
            return true;

        if (this->deref()->basic_type() == variable_basic_type::VOID && flags.voidptr_to_pointer)
            return true;

        flags.any_to_void = false;
        return this->deref()->equal_to(_type->deref(), flags);
    } break;
    case variable_basic_type::ARRAY:
    {
        if (!flags.pointer_to_array)
            break;

        auto _type = dynamic_pointer_cast<ASTNodeVariableTypeArray>(type);
        assert(_type);
        flags.any_to_void = false;
        return this->deref()->equal_to(_type->elemtype(), flags);
    } break;
    case variable_basic_type::FUNCTION:
    {
        if (!flags.pointer_to_function)
            break;
        auto funct = this->deref();
        if (funct->basic_type() != variable_basic_type::FUNCTION)
            break;

        flags.any_to_void = false;
        return funct->equal_to(type, flags);
    } break;
    default:
        break;
    }

    return false;
}

size_t ASTNodeVariableTypePointer::opsizeof() const
{
    // TODO
    return sizeof(void*);
}

size_t ASTNodeVariableTypePointer::opalignof() const
{
    // TODO
    return sizeof(void*);
}

std::shared_ptr<ASTNodeVariableType> ASTNodeVariableTypePointer::copy() const
{
    auto ast = std::make_shared<ASTNodeVariableTypePointer>(this->lcontext(), this->deref());
    ast->const_ref() = this->const_ref();
    ast->volatile_ref() = this->volatile_ref();
    ast->restrict_ref() = this->restrict_ref();
    const ASTNodeVariableType* _this = this;
    ast->storage_class() = _this->storage_class();
    return ast;
}


variable_basic_type ASTNodeVariableTypeArray::basic_type() const
{
    return variable_basic_type::ARRAY;
}

bool ASTNodeVariableTypeArray::equal_to(std::shared_ptr<ASTNodeVariableType> type, TestFlags& flags) const
{
    if (!ASTNodeVariableType::equal_to(type, flags))
        return false;

    switch (type->basic_type())
    {
    case variable_basic_type::VOID:
    {
        if (flags.any_to_void)
            return true;
    } break;
    case variable_basic_type::ARRAY:
    {
        auto _type = dynamic_pointer_cast<ASTNodeVariableTypeArray>(type);
        assert(_type);
        flags.any_to_void = false;

        if (!this->elemtype()->equal_to(_type->elemtype(), flags))
            break;;

        if (_type->m_static && this->m_size < _type->m_size && flags.ignore_array_size)
            break;

        return true;
    } break;
    case variable_basic_type::POINTER:
    {
        if (!flags.array_to_pointer)
            break;

        auto _type = dynamic_pointer_cast<ASTNodeVariableTypePointer>(type);
        assert(_type);
        flags.any_to_void = false;
        return this->elemtype()->equal_to(_type->deref(), flags);
    } break;
    case variable_basic_type::INT:
    {
        if (!flags.array_to_pointer || flags.pointer_to_integer)
            break;

        // TODO pointer size
        if (type->opsizeof() < sizeof(void*))
            break;

        return true;
    } break;
    default:
        break;
    }

    return false;
}

size_t ASTNodeVariableTypeArray::opsizeof() const
{
    return sizeof(void*);

    // TODO align
    // return this->m_size * this->elemtype()->opsizeof();
}

size_t ASTNodeVariableTypeArray::opalignof() const
{
    return sizeof(void*);

    // TODO align
}

std::shared_ptr<ASTNodeVariableType> ASTNodeVariableTypeArray::copy() const
{
    auto ast = std::make_shared<ASTNodeVariableTypeArray>(
            this->lcontext(), this->elemtype(), this->array_size(), this->m_static);
    ast->const_ref() = this->const_ref();
    ast->volatile_ref() = this->volatile_ref();
    ast->restrict_ref() = this->restrict_ref();
    const ASTNodeVariableType* _this = this;
    ast->storage_class() = _this->storage_class();
    return ast;
}


variable_basic_type ASTNodeVariableTypeFunction::basic_type() const
{
    return variable_basic_type::FUNCTION;
}

bool ASTNodeVariableTypeFunction::equal_to(std::shared_ptr<ASTNodeVariableType> type, TestFlags& flags) const
{
    if (!ASTNodeVariableType::equal_to(type, flags))
        return false;

    switch (type->basic_type())
    {
    case variable_basic_type::VOID:
    {
        if (flags.any_to_void)
            return true;
    } break;
    case variable_basic_type::FUNCTION:
    {
        auto _type = dynamic_pointer_cast<ASTNodeVariableTypeFunction>(type);
        assert(_type);
        flags.any_to_void = false;

        if (!this->return_type()->equal_to(_type->return_type(), flags))
            break;

        auto p1 = this->parameter_declaration_list();
        auto p2 = _type->parameter_declaration_list();
        if (p1->size() != p2->size())
            break;
        if (p1->variadic() != p2->variadic())
            break;

        for (size_t i = 0; i < p1->size(); ++i)
        {
            if (!(*p1)[i]->type()->equal_to((*p2)[i]->type(), flags))
                break;
        }

        return true;
    } break;
    case variable_basic_type::POINTER:
    {
        if (!flags.function_to_pointer)
            break;

        auto _type = dynamic_pointer_cast<ASTNodeVariableTypePointer>(type);
        assert(_type);
        flags.any_to_void = flags.pointer_to_voidptr;
        return this->equal_to(_type->deref(), flags);
    } break;
    default:
        break;
    }

    return false;
}

size_t ASTNodeVariableTypeFunction::opsizeof() const
{
    // TODO report error
    return 0;
}

size_t ASTNodeVariableTypeFunction::opalignof() const
{
    // TODO report error
    return 0;
}

std::shared_ptr<ASTNodeVariableType> ASTNodeVariableTypeFunction::copy() const
{
    auto params = make_shared<ASTNodeParameterDeclarationList>(this->lcontext());
    params->variadic() = this->parameter_declaration_list()->variadic();
    for (auto p : *this->parameter_declaration_list())
    {
        auto tc = p->type()->copy();
        params->push_back(make_shared<ASTNodeParameterDeclaration>(this->lcontext(), p->id(), tc));
    }
    auto ret = make_shared<ASTNodeVariableTypeFunction>(this->lcontext(), params, this->return_type()->copy());
    const ASTNodeVariableType* _this = this;
    ret->storage_class() = _this->storage_class();
    return ret;
}


storage_class_t& ASTNodeVariableTypeArray::storage_class()
{
    return this->m_type->storage_class();
}

void ASTNodeVariableTypeArray::set_leaf_type(shared_ptr<ASTNodeVariableType> type)
{
    if (this->m_type == nullptr) {
        this->m_type = type;
        return;
    }

    const auto bt = this->m_type->basic_type();
    this->m_type->set_leaf_type(type);
}

bool& ASTNodeVariableTypeArray::inlined() { return this->m_type->inlined(); }


storage_class_t& ASTNodeVariableTypePointer::storage_class()
{
    return this->m_type->storage_class();
}

void ASTNodeVariableTypePointer::set_leaf_type(shared_ptr<ASTNodeVariableType> type)
{
    if (this->m_type == nullptr) {
        this->m_type = type;
        return;
    }

    const auto bt = this->m_type->basic_type();
    this->m_type->set_leaf_type(type);
}

bool& ASTNodeVariableTypePointer::inlined() { return this->m_type->inlined(); }


storage_class_t& ASTNodeVariableTypeFunction::storage_class()
{
    return this->return_type()->storage_class();
}

void ASTNodeVariableTypeFunction::set_leaf_type(shared_ptr<ASTNodeVariableType> type)
{
    if (this->m_return_type == nullptr) {
        this->m_return_type = type;
        return;
    }

    const auto bt = this->m_return_type->basic_type();
    this->m_return_type->set_leaf_type(type);
}

bool& ASTNodeVariableTypeFunction::inlined() { return this->m_return_type->inlined(); }
