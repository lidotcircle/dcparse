#include "c_ast.h"
#include <map>
using namespace std;
using namespace cparser;
using storage_class_t = ASTNodeVariableType::storage_class_t;
using variable_basic_type = ASTNodeVariableType::variable_basic_type;


bool& ASTNodeVariableType::const_ref() { return this->m_const; }
bool& ASTNodeVariableType::volatile_ref() { return this->m_volatile; }
bool& ASTNodeVariableType::restrict_ref() { return this->m_restrict; }

bool ASTNodeVariableType::is_object_type() const
{
    const auto bt = this->basic_type();
    if (bt == variable_basic_type::ARRAY ||
        bt == variable_basic_type::STRUCT ||
        bt == variable_basic_type::ENUM ||
        bt == variable_basic_type::UNION)
    {
        throw runtime_error("unexpected call to this function, it should be overrided");
    }

    return bt == variable_basic_type::INT ||
           bt == variable_basic_type::FLOAT ||
           bt == variable_basic_type::POINTER;
}
bool ASTNodeVariableType::is_function_type() const
{
    return this->basic_type() == variable_basic_type::FUNCTION;
}
bool ASTNodeVariableType::is_incomplete_type() const
{
    const auto bt = this->basic_type();
    if (bt == variable_basic_type::VOID)
        return true;

    return !this->is_function_type() && !this->is_object_type();
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


shared_ptr<ASTNodeVariableType>  ASTNodeVariableTypeDummy::implicit_cast_to(shared_ptr<ASTNodeVariableType> type) const
{
    assert(false);
}

shared_ptr<ASTNodeVariableType>  ASTNodeVariableTypeDummy::explicit_cast_to(shared_ptr<ASTNodeVariableType> type) const
{
    assert(false);
}

bool  ASTNodeVariableTypeDummy::equal_to(shared_ptr<ASTNodeVariableType> type) const
{
    assert(false);
}

shared_ptr<ASTNodeVariableType> ASTNodeVariableTypeDummy::compatible_with(shared_ptr<ASTNodeVariableType> type) const
{
    assert(false);
}

variable_basic_type ASTNodeVariableTypeDummy::basic_type() const
{
    return variable_basic_type::DUMMY;
}

optional<size_t> ASTNodeVariableTypeDummy::opsizeof() const
{
    throw std::runtime_error("ASTNodeVariableTypeDummy::copy() unexpected");
}

optional<size_t> ASTNodeVariableTypeDummy::opalignof() const
{
    throw std::runtime_error("ASTNodeVariableTypeDummy::copy() unexpected");
}

std::shared_ptr<ASTNodeVariableType> ASTNodeVariableTypeDummy::copy() const
{
    throw std::runtime_error("ASTNodeVariableTypeDummy::copy() unexpected");
}


shared_ptr<ASTNodeVariableType>  ASTNodeVariableTypeStruct::implicit_cast_to(shared_ptr<ASTNodeVariableType> type) const
{
    return this->compatible_with(type);
}

shared_ptr<ASTNodeVariableType>  ASTNodeVariableTypeStruct::explicit_cast_to(shared_ptr<ASTNodeVariableType> type) const
{
    if (type->basic_type() == variable_basic_type::VOID)
        return type;

    if (type->basic_type() != variable_basic_type::STRUCT)
        return nullptr;

    auto scttype = dynamic_pointer_cast<ASTNodeVariableTypeStruct>(type);
    assert(scttype);

    assert(this->m_type_id.has_value() && scttype->m_type_id.has_value());
    if (this->m_type_id != scttype->m_type_id)
        return nullptr;

    return type;
}

bool  ASTNodeVariableTypeStruct::equal_to(shared_ptr<ASTNodeVariableType> type) const
{
    if (type->basic_type() != variable_basic_type::STRUCT)
        return false;

    if (this->const_ref() != type->const_ref() ||
        this->volatile_ref() != type->volatile_ref() ||
        this->restrict_ref() != type->restrict_ref())
    {
        return false;
    }

    auto scttype = dynamic_pointer_cast<ASTNodeVariableTypeStruct>(type);
    assert(scttype);

    assert(this->m_type_id.has_value() && scttype->m_type_id.has_value());
    return this->m_type_id == scttype->m_type_id;
}

shared_ptr<ASTNodeVariableType> ASTNodeVariableTypeStruct::compatible_with(shared_ptr<ASTNodeVariableType> type) const
{
    if (type->basic_type() != variable_basic_type::STRUCT)
        return nullptr;

    auto t = std::dynamic_pointer_cast<ASTNodeVariableTypeStruct>(type);
    assert(t);

    assert(this->m_type_id.has_value() && t->m_type_id.has_value());
    if (this->m_type_id.value() == this->m_type_id.value() &&
        this->const_ref() == t->const_ref() &&
        this->volatile_ref() == t->volatile_ref() &&
        this->restrict_ref() == t->restrict_ref())
    {
        return type;
    } else {
        return nullptr;
    }
}

variable_basic_type ASTNodeVariableTypeStruct::basic_type() const
{
    return variable_basic_type::STRUCT;
}

bool ASTNodeVariableTypeStruct::is_object_type() const
{
    return this->m_definition != nullptr;
}

optional<size_t> ASTNodeVariableTypeStruct::opsizeof() const
{
    assert(this->m_definition);
    return this->m_definition->sizeof_().value();
}

optional<size_t> ASTNodeVariableTypeStruct::opalignof() const
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

shared_ptr<ASTNodeVariableType>  ASTNodeVariableTypeUnion::implicit_cast_to(shared_ptr<ASTNodeVariableType> type) const
{
    return this->compatible_with(type);
}

shared_ptr<ASTNodeVariableType>  ASTNodeVariableTypeUnion::explicit_cast_to(shared_ptr<ASTNodeVariableType> type) const
{
    if (type->basic_type() == variable_basic_type::VOID)
        return type;

    if (type->basic_type() != variable_basic_type::UNION)
        return nullptr;

    auto scttype = dynamic_pointer_cast<ASTNodeVariableTypeUnion>(type);
    assert(scttype);

    assert(this->m_type_id.has_value() && scttype->m_type_id.has_value());
    if (this->m_type_id != scttype->m_type_id)
        return nullptr;

    return type;
}

bool  ASTNodeVariableTypeUnion::equal_to(shared_ptr<ASTNodeVariableType> type) const
{
    if (type->basic_type() != variable_basic_type::UNION)
        return false;

    if (this->const_ref() != type->const_ref() ||
        this->volatile_ref() != type->volatile_ref() ||
        this->restrict_ref() != type->restrict_ref())
    {
        return false;
    }

    auto scttype = dynamic_pointer_cast<ASTNodeVariableTypeUnion>(type);
    assert(scttype);

    assert(this->m_type_id.has_value() && scttype->m_type_id.has_value());
    return this->m_type_id == scttype->m_type_id;
}

shared_ptr<ASTNodeVariableType> ASTNodeVariableTypeUnion::compatible_with(shared_ptr<ASTNodeVariableType> type) const
{
    if (type->basic_type() != variable_basic_type::UNION)
        return nullptr;

    auto t = std::dynamic_pointer_cast<ASTNodeVariableTypeUnion>(type);
    assert(t);

    assert(this->m_type_id.has_value() && t->m_type_id.has_value());
    if (this->m_type_id.value() == this->m_type_id.value() &&
        this->const_ref() == t->const_ref() &&
        this->volatile_ref() == t->volatile_ref() &&
        this->restrict_ref() == t->restrict_ref())
    {
        return type;
    } else {
        return nullptr;
    }
}

variable_basic_type ASTNodeVariableTypeUnion::basic_type() const
{
    return variable_basic_type::UNION;
}

bool ASTNodeVariableTypeUnion::is_object_type() const
{
    return this->m_definition != nullptr;
}

optional<size_t> ASTNodeVariableTypeUnion::opsizeof() const
{
    assert(this->m_definition);
    return this->m_definition->sizeof_().value();
}

optional<size_t> ASTNodeVariableTypeUnion::opalignof() const
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


shared_ptr<ASTNodeVariableType>  ASTNodeVariableTypeEnum::implicit_cast_to(shared_ptr<ASTNodeVariableType> type) const
{
    auto enumt = dynamic_pointer_cast<ASTNodeVariableTypeEnum>(type);
    if (enumt->m_type_id == this->m_type_id)
        return type;

    if (type->is_arithmatic_type())
        return type;

    return nullptr;
}

shared_ptr<ASTNodeVariableType>  ASTNodeVariableTypeEnum::explicit_cast_to(shared_ptr<ASTNodeVariableType> type) const
{
    if (type->basic_type() == variable_basic_type::VOID)
        return type;

    if (type->is_scalar_type() || type->basic_type() == variable_basic_type::ENUM)
        return type;

    return nullptr;
}

bool  ASTNodeVariableTypeEnum::equal_to(shared_ptr<ASTNodeVariableType> type) const
{
    if (this->const_ref() != type->const_ref() ||
        this->volatile_ref() != type->volatile_ref() ||
        this->restrict_ref() != type->restrict_ref())
    {
        return false;
    }

    auto enumt = dynamic_pointer_cast<ASTNodeVariableTypeEnum>(type);
    if (enumt)
        return this->m_type_id == enumt->m_type_id;

    return false;
}

shared_ptr<ASTNodeVariableType> ASTNodeVariableTypeEnum::compatible_with(shared_ptr<ASTNodeVariableType> type) const
{
    if (this->const_ref() != type->const_ref() ||
        this->volatile_ref() != type->volatile_ref() ||
        this->restrict_ref() != type->restrict_ref())
    {
        return nullptr;
    }

    auto tenum = std::dynamic_pointer_cast<ASTNodeVariableTypeEnum>(type);

    if (tenum) {
        assert(this->m_type_id.has_value() && tenum->m_type_id.has_value());
        if (this->m_type_id.value() == tenum->m_type_id.value())
        {
            return type;
        } else {
            return nullptr;
        }
    }

    auto tint = std::dynamic_pointer_cast<ASTNodeVariableTypeInt>(type);
    if (tint) {
        if (tint->byte_length() == ENUM_COMPATIBLE_INT_BYTE_WIDTH &&
            tint->is_unsigned() == ENUM_COMPATIBLE_INT_IS_UNSIGNED)
        {
            return tint;
        } else {
            return nullptr;
        }
    }

    return nullptr;
}

variable_basic_type ASTNodeVariableTypeEnum::basic_type() const
{
    return variable_basic_type::ENUM;
}

bool ASTNodeVariableTypeEnum::is_object_type() const
{
    return this->m_definition != nullptr;
}

optional<size_t> ASTNodeVariableTypeEnum::opsizeof() const
{
    return ENUM_COMPATIBLE_INT_BYTE_WIDTH;
}

optional<size_t> ASTNodeVariableTypeEnum::opalignof() const
{
    // TODO
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


shared_ptr<ASTNodeVariableType>  ASTNodeVariableTypeTypedef::implicit_cast_to(shared_ptr<ASTNodeVariableType> type) const
{
    return this->m_type->implicit_cast_to(type);
}

shared_ptr<ASTNodeVariableType>  ASTNodeVariableTypeTypedef::explicit_cast_to(shared_ptr<ASTNodeVariableType> type) const
{
    return this->m_type->explicit_cast_to(type);
}

bool  ASTNodeVariableTypeTypedef::equal_to(shared_ptr<ASTNodeVariableType> type) const
{
    return this->m_type->equal_to(type);
}

shared_ptr<ASTNodeVariableType> ASTNodeVariableTypeTypedef::compatible_with(shared_ptr<ASTNodeVariableType> type) const
{
    return this->m_type->compatible_with(type);
}

variable_basic_type ASTNodeVariableTypeTypedef::basic_type() const
{
    assert(this->type());
    return this->type()->basic_type();
}

bool ASTNodeVariableTypeTypedef::is_object_type() const
{
    assert(this->type());
    return this->type()->is_object_type();
}

optional<size_t> ASTNodeVariableTypeTypedef::opsizeof() const
{
    assert(this->type());
    return this->type()->opsizeof();
}

optional<size_t> ASTNodeVariableTypeTypedef::opalignof() const
{
    assert(this->type());
    return this->type()->opalignof();
}

std::shared_ptr<ASTNodeVariableType> ASTNodeVariableTypeTypedef::copy() const
{
    assert(this->type());
    return this->type()->copy();
}


shared_ptr<ASTNodeVariableType>  ASTNodeVariableTypeVoid::implicit_cast_to(shared_ptr<ASTNodeVariableType> type) const
{
    if (type->basic_type() == variable_basic_type::VOID)
        return type;

    return nullptr;
}

shared_ptr<ASTNodeVariableType>  ASTNodeVariableTypeVoid::explicit_cast_to(shared_ptr<ASTNodeVariableType> type) const
{
    if (type->basic_type() == variable_basic_type::VOID)
        return type;

    return nullptr;
}

bool  ASTNodeVariableTypeVoid::equal_to(shared_ptr<ASTNodeVariableType> type) const
{
    return this->implicit_cast_to(type) != nullptr;
}

shared_ptr<ASTNodeVariableType> ASTNodeVariableTypeVoid::compatible_with(shared_ptr<ASTNodeVariableType> type) const
{
    if (type->basic_type() != variable_basic_type::VOID)
        return nullptr;
    return type;
}

variable_basic_type ASTNodeVariableTypeVoid::basic_type() const
{
    return variable_basic_type::VOID;
}

optional<size_t> ASTNodeVariableTypeVoid::opsizeof() const
{
    return 0;
}

optional<size_t> ASTNodeVariableTypeVoid::opalignof() const
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


shared_ptr<ASTNodeVariableType>  ASTNodeVariableTypeInt::implicit_cast_to(shared_ptr<ASTNodeVariableType> type) const
{
    if (this->is_arithmatic_type())
        return type;

    if (type->basic_type() == variable_basic_type::ENUM &&
        this->m_byte_length == ENUM_COMPATIBLE_INT_BYTE_WIDTH &&
        this->m_is_unsigned == ENUM_COMPATIBLE_INT_IS_UNSIGNED)
    {
        return type;
    }

    return nullptr;
}

shared_ptr<ASTNodeVariableType>  ASTNodeVariableTypeInt::explicit_cast_to(shared_ptr<ASTNodeVariableType> type) const
{
    if (type->is_scalar_type() || type->basic_type() == variable_basic_type::ENUM)
        return type;

    return nullptr;
}

bool  ASTNodeVariableTypeInt::equal_to(shared_ptr<ASTNodeVariableType> type) const
{
    if (type->basic_type() != this->basic_type() ||
        type->const_ref() != this->const_ref() ||
        type->volatile_ref() != this->volatile_ref() ||
        type->restrict_ref() != this->restrict_ref())
    {
        return false;
    }

    auto intt = dynamic_pointer_cast<ASTNodeVariableTypeInt>(type);
    return intt->byte_length() == this->m_byte_length &&
           intt->is_unsigned() == this->m_is_unsigned;
}

shared_ptr<ASTNodeVariableType> ASTNodeVariableTypeInt::compatible_with(shared_ptr<ASTNodeVariableType> type) const
{
    if (this->const_ref() != type->const_ref() ||
        this->volatile_ref() != type->volatile_ref() ||
        this->restrict_ref() != type->restrict_ref())
    {
        return nullptr;
    }

    auto tint = std::dynamic_pointer_cast<ASTNodeVariableTypeInt>(type);
    if (tint) {
        if (this->m_is_unsigned == tint->m_is_unsigned &&
            this->m_byte_length == tint->m_byte_length)
        {
            return type;
        } else {
            return nullptr;
        }
    }

    auto tenum = std::dynamic_pointer_cast<ASTNodeVariableTypeEnum>(type);
    if (tenum) {
        if (this->m_is_unsigned == ENUM_COMPATIBLE_INT_IS_UNSIGNED &&
            this->m_byte_length == ENUM_COMPATIBLE_INT_BYTE_WIDTH)
        {
            return make_shared<ASTNodeVariableTypeInt>(this->lcontext(), this->m_byte_length, this->m_is_unsigned);
        } else {
            return nullptr;
        }
    }

    return nullptr;
}

variable_basic_type ASTNodeVariableTypeInt::basic_type() const
{
    return variable_basic_type::INT;
}

optional<size_t> ASTNodeVariableTypeInt::opsizeof() const
{
    return this->m_byte_length;
}

optional<size_t> ASTNodeVariableTypeInt::opalignof() const
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


shared_ptr<ASTNodeVariableType>  ASTNodeVariableTypeFloat::implicit_cast_to(shared_ptr<ASTNodeVariableType> type) const
{
    if (this->is_arithmatic_type())
        return type;
    else
        return nullptr;
}

shared_ptr<ASTNodeVariableType>  ASTNodeVariableTypeFloat::explicit_cast_to(shared_ptr<ASTNodeVariableType> type) const
{
    if (type->is_arithmatic_type() || type->basic_type() == variable_basic_type::ENUM)
        return type;

    return nullptr;
}

bool  ASTNodeVariableTypeFloat::equal_to(shared_ptr<ASTNodeVariableType> type) const
{
    if (type->basic_type() != this->basic_type() ||
        type->const_ref() != this->const_ref() ||
        type->volatile_ref() != this->volatile_ref() ||
        type->restrict_ref() != this->restrict_ref())
    {
        return false;
    }

    auto m = dynamic_pointer_cast<ASTNodeVariableTypeFloat>(type);
    return m->m_byte_length == this->m_byte_length;
}

shared_ptr<ASTNodeVariableType> ASTNodeVariableTypeFloat::compatible_with(shared_ptr<ASTNodeVariableType> type) const
{
    if (type->basic_type() != variable_basic_type::FLOAT)
        return nullptr;

    auto t = std::dynamic_pointer_cast<ASTNodeVariableTypeFloat>(type);
    assert(t);
    if (this->m_byte_length == t->m_byte_length &&
        this->const_ref() == t->const_ref() &&
        this->volatile_ref() == t->volatile_ref() &&
        this->restrict_ref() == t->restrict_ref())
    {
        return type;
    } else {
        return nullptr;
    }
}

variable_basic_type ASTNodeVariableTypeFloat::basic_type() const
{
    return variable_basic_type::FLOAT;
}

optional<size_t> ASTNodeVariableTypeFloat::opsizeof() const
{
    return this->m_byte_length;
}

optional<size_t> ASTNodeVariableTypeFloat::opalignof() const
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


shared_ptr<ASTNodeVariableType>  ASTNodeVariableTypePointer::implicit_cast_to(shared_ptr<ASTNodeVariableType> type) const
{
    assert(!type->is_function_type());
    auto ptrtype = dynamic_pointer_cast<ASTNodeVariableTypePointer>(type);
    if (ptrtype) {
        if ((this->m_type->const_ref() && !ptrtype->m_type->const_ref()) ||
            (this->m_type->volatile_ref() && !ptrtype->m_type->volatile_ref()))
        {
            return nullptr;
        }

        if (this->m_type->basic_type() == variable_basic_type::VOID ||
            ptrtype->m_type->basic_type() == variable_basic_type::VOID)
        {
            return type;
        }

        if (this->m_type->compatible_with(ptrtype->m_type)) {
            return type;
        } else {
            return nullptr;
        }
    }

    auto arrtype = dynamic_pointer_cast<ASTNodeVariableTypeArray>(type);
    if (arrtype) {
        if ((this->m_type->const_ref() && !arrtype->elemtype()->const_ref()) ||
            (this->m_type->volatile_ref() && !arrtype->elemtype()->volatile_ref()))
        {
            return nullptr;
        }

        if (this->m_type->basic_type() == variable_basic_type::VOID)
        {
            return type;
        }

        if (this->m_type->compatible_with(arrtype->elemtype())) {
            return type;
        } else {
            return nullptr;
        }
    }

    auto functype = dynamic_pointer_cast<ASTNodeVariableTypeFunction>(type);
    if (functype) {
        if (functype->compatible_with(this->m_type))
            return functype;
    }

    return nullptr;
}

shared_ptr<ASTNodeVariableType>  ASTNodeVariableTypePointer::explicit_cast_to(shared_ptr<ASTNodeVariableType> type) const
{
    const auto bt = type->basic_type();
    if (bt == variable_basic_type::POINTER ||
        bt == variable_basic_type::ENUM ||
        bt == variable_basic_type::INT ||
        bt == variable_basic_type::ARRAY ||
        bt == variable_basic_type::FUNCTION)
    {
        return type;
    }

    return nullptr;
}

bool  ASTNodeVariableTypePointer::equal_to(shared_ptr<ASTNodeVariableType> type) const
{
    if (type->const_ref() != this->const_ref() ||
        type->volatile_ref() != this->volatile_ref() ||
        type->restrict_ref() != this->restrict_ref())
    {
        return false;
    }

    if (type->basic_type() != variable_basic_type::POINTER)
        return false;

    auto ptrtype = dynamic_pointer_cast<ASTNodeVariableTypePointer>(type);
    return this->m_type->equal_to(ptrtype->m_type);
}

shared_ptr<ASTNodeVariableType> ASTNodeVariableTypePointer::compatible_with(shared_ptr<ASTNodeVariableType> type) const
{
    auto tptr = std::dynamic_pointer_cast<ASTNodeVariableTypePointer>(type);
    if (tptr) {
        if (this->const_ref() != tptr->const_ref() ||
            this->volatile_ref() != tptr->volatile_ref() ||
            this->restrict_ref() != tptr->restrict_ref())
        {
            return nullptr;
        }

        auto composite_type = this->m_type->compatible_with(tptr->m_type);
        if (!composite_type)
            return nullptr;

        auto ret = std::make_shared<ASTNodeVariableTypePointer>(this->lcontext(), composite_type);
        ret->const_ref() = this->const_ref();
        ret->volatile_ref() = this->volatile_ref();
        ret->restrict_ref() = this->restrict_ref();
        return ret;
    }

    auto tarr = std::dynamic_pointer_cast<ASTNodeVariableTypeArray>(type);
    if (tarr) {
        if (this->const_ref() != tarr->const_ref() ||
            this->volatile_ref() != tarr->volatile_ref() ||
            this->restrict_ref() != tarr->restrict_ref())
        {
            return nullptr;
        }

        auto composite_type = this->m_type->compatible_with(tarr->elemtype());
        if (composite_type)
            return nullptr;

        auto ret = std::make_shared<ASTNodeVariableTypePointer>(this->lcontext(), composite_type);
        ret->const_ref() = this->const_ref();
        ret->volatile_ref() = this->volatile_ref();
        ret->restrict_ref() = this->restrict_ref();
        return ret;
    }

    return nullptr;
}

variable_basic_type ASTNodeVariableTypePointer::basic_type() const
{
    return variable_basic_type::POINTER;
}

optional<size_t> ASTNodeVariableTypePointer::opsizeof() const
{
    // TODO
    return sizeof(void*);
}

optional<size_t> ASTNodeVariableTypePointer::opalignof() const
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


shared_ptr<ASTNodeVariableType>  ASTNodeVariableTypeArray::implicit_cast_to(shared_ptr<ASTNodeVariableType> type) const
{
    auto ptrtype = dynamic_pointer_cast<ASTNodeVariableTypePointer>(type);
    if (ptrtype) {
        if (ptrtype->deref()->basic_type() == variable_basic_type::VOID &&
            (ptrtype->deref()->const_ref() || !this->elemtype()->const_ref()) &&
            (ptrtype->deref()->volatile_ref() || !this->elemtype()->volatile_ref()))
        {
            return type;
        }

        if (this->elemtype()->compatible_with(ptrtype->deref()))
            return type;
    }

    auto arrtype = dynamic_pointer_cast<ASTNodeVariableTypeArray>(type);
    if (arrtype) {
        if (!this->elemtype()->compatible_with(arrtype->elemtype()))
            return nullptr;

        if (arrtype->m_static && 
            arrtype->m_size && 
            this->m_size)
        {
            auto s1 = arrtype->m_size->get_integer_constant();
            auto s2 = this->m_size->get_integer_constant();

            if (s1.has_value() && s2.has_value() &&
                s1.value() > s2.value())
            {
                return nullptr;
            }
        }

        return type;
    }

    return nullptr;
}

shared_ptr<ASTNodeVariableType>  ASTNodeVariableTypeArray::explicit_cast_to(shared_ptr<ASTNodeVariableType> type) const
{
    const auto bt = type->basic_type();
    if (bt == variable_basic_type::POINTER ||
        bt == variable_basic_type::ARRAY ||
        bt == variable_basic_type::INT ||
        bt == variable_basic_type::ENUM)
    {
        return type;
    }

    return nullptr;
}

bool  ASTNodeVariableTypeArray::equal_to(shared_ptr<ASTNodeVariableType> type) const
{
    if (type->basic_type() != this->basic_type())
        return false;

    if (type->const_ref() != this->const_ref() ||
        type->volatile_ref() != this->volatile_ref() ||
        type->restrict_ref() != this->restrict_ref())
    {
        return false;
    }

    auto arrtype= dynamic_pointer_cast<ASTNodeVariableTypeArray>(type);
    assert(arrtype);
    if (!this->m_type->equal_to(arrtype->m_type) ||
        this->m_static != arrtype->m_static)
    {
        return false;
    }

    if (!this->m_size && !arrtype->m_size)
        return true;

    if (this->m_size && arrtype->m_size) {
        auto s1 = this->m_size->get_integer_constant();
        auto s2 = arrtype->m_size->get_integer_constant();

        if (s1.has_value())
        {
            if (s2.has_value()) {
                return s1.value() == s2.value();
            } else {
                return false;
            }
        } else
        {
            if (s2.has_value()) {
                return false;
            } else {
                // TODO
                return false;
            }
        }
    } else {
        return false;
    }

    return true;
}

shared_ptr<ASTNodeVariableType> ASTNodeVariableTypeArray::compatible_with(shared_ptr<ASTNodeVariableType> type) const
{
    auto tptr = std::dynamic_pointer_cast<ASTNodeVariableTypePointer>(type);
    if (tptr) {
        if (this->const_ref() != tptr->const_ref() ||
            this->volatile_ref() != tptr->volatile_ref() ||
            this->restrict_ref() != tptr->restrict_ref())
        {
            return nullptr;
        }

        auto composite_type = this->elemtype()->compatible_with(tptr->deref());
        if (!composite_type)
            return nullptr;

        auto ret = make_shared<ASTNodeVariableTypePointer>(this->lcontext(), composite_type);
        ret->const_ref() = this->const_ref();
        ret->volatile_ref() = this->volatile_ref();
        ret->restrict_ref() = this->restrict_ref();
        return ret;
    }

    auto tarr = std::dynamic_pointer_cast<ASTNodeVariableTypeArray>(type);
    if (tarr) {
        if (this->const_ref() != tarr->const_ref() ||
            this->volatile_ref() != tarr->volatile_ref() ||
            this->restrict_ref() != tarr->restrict_ref())
        {
            return nullptr;
        }

        auto composite_type = this->elemtype()->compatible_with(tarr->elemtype());
        if (!composite_type) 
            return nullptr;

        shared_ptr<ASTNodeExpr> size_expr;
        if (this->m_size && tarr->m_size)
        {
            auto this_size = this->m_size->get_integer_constant();
            auto type_size = tarr->m_size->get_integer_constant();
            if (this_size.has_value() && type_size.has_value() &&
                this_size.value() != type_size.value())
            {
                return nullptr;
            } else {
                size_expr = this->m_size;
            }
        }

        if (!size_expr && this->m_size) {
            auto this_size = this->m_size->get_integer_constant();
            if (this_size.has_value())
                size_expr = this->m_size;
        }

        if (!size_expr && tarr->m_size) {
            auto type_size = tarr->m_size->get_integer_constant();
            if (type_size.has_value())
                size_expr = tarr->m_size;
        }

        auto tx = make_shared<ASTNodeVariableTypeArray>(this->lcontext(), composite_type, size_expr, this->m_static && tarr->m_static);
        tx->const_ref() = this->const_ref();
        tx->volatile_ref() = this->volatile_ref();
        tx->restrict_ref() = this->restrict_ref();
        return tx;
    }

    return nullptr;
}

variable_basic_type ASTNodeVariableTypeArray::basic_type() const
{
    return variable_basic_type::ARRAY;
}

bool ASTNodeVariableTypeArray::is_object_type() const
{
    return this->m_size != nullptr;
}

optional<size_t> ASTNodeVariableTypeArray::opsizeof() const
{
    if (!this->m_size || 
        !this->m_size->get_integer_constant().has_value() ||
        !this->m_type->opsizeof().has_value())
    {
        return nullopt;
    }

    return this->m_type->opsizeof().value() * this->m_size->get_integer_constant().value();
}

optional<size_t> ASTNodeVariableTypeArray::opalignof() const
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


shared_ptr<ASTNodeVariableType>  ASTNodeVariableTypeFunction::implicit_cast_to(shared_ptr<ASTNodeVariableType> type) const
{
    if (type->basic_type() != variable_basic_type::POINTER)
        return nullptr;

    auto ptrtype = dynamic_pointer_cast<ASTNodeVariableTypePointer>(type);
    assert(ptrtype);

    if (this->compatible_with(ptrtype->deref()))
        return type;
    else
        return nullptr;
}

shared_ptr<ASTNodeVariableType>  ASTNodeVariableTypeFunction::explicit_cast_to(shared_ptr<ASTNodeVariableType> type) const
{
    const auto bt = type->basic_type();
    if (bt == variable_basic_type::POINTER ||
        bt == variable_basic_type::ENUM ||
        bt == variable_basic_type::INT ||
        bt == variable_basic_type::ARRAY)
    {
        return type;
    }

    return nullptr;
}

bool  ASTNodeVariableTypeFunction::equal_to(shared_ptr<ASTNodeVariableType> type) const
{
    if (type->basic_type() != variable_basic_type::FUNCTION)
        return false;

    auto functype = dynamic_pointer_cast<ASTNodeVariableTypeFunction>(type);
    assert(functype);

    if (!this->return_type()->equal_to(functype->return_type()))
        return false;

    auto l1 = this->parameter_declaration_list();
    auto l2 = functype->parameter_declaration_list();
    if (l1->variadic() != l2->variadic() ||
        l1->size() != l2->size())
    {
        return false;
    }

    for (size_t i=0;i<l1->size();i++) {
        auto t1 = (*l1)[i]->type();
        auto t2 = (*l2)[i]->type();
        if (!t1->equal_to(t2))
            return false;
    }

    return true;
}

shared_ptr<ASTNodeVariableType> ASTNodeVariableTypeFunction::compatible_with(shared_ptr<ASTNodeVariableType> type) const
{
    if (type->basic_type() != variable_basic_type::FUNCTION)
        return nullptr;

    auto tfunc = std::dynamic_pointer_cast<ASTNodeVariableTypeFunction>(type);
    if (!this->m_return_type->compatible_with(tfunc->m_return_type))
        return nullptr;

    if (this->m_parameter_declaration_list->empty() &&
        !this->m_parameter_declaration_list->variadic())
    {
        return tfunc;
    }

    if (tfunc->m_parameter_declaration_list->empty() &&
        !tfunc->m_parameter_declaration_list->variadic())
    {
        return make_shared<ASTNodeVariableTypeFunction>(
                this->lcontext(), this->m_parameter_declaration_list, this->m_return_type);
    }

    if (this->m_parameter_declaration_list->variadic() != tfunc->m_parameter_declaration_list->variadic())
        return nullptr;

    if (this->m_parameter_declaration_list->size() != tfunc->m_parameter_declaration_list->size())
        return nullptr;

    auto ret_type = this->m_return_type->compatible_with(tfunc->m_return_type);
    if (!ret_type) return nullptr;

    auto params = make_shared<ASTNodeParameterDeclarationList>(this->lcontext());
    for (size_t i=0;i<this->m_parameter_declaration_list->size();i++)
    {
        auto param = (*this->m_parameter_declaration_list)[i];
        auto tparam = (*tfunc->m_parameter_declaration_list)[i];
        auto tparam_type = param->type()->compatible_with(tparam->type());
        if (!tparam_type) return nullptr;

        params->push_back(make_shared<ASTNodeParameterDeclaration>(
                this->lcontext(), param->id(), tparam_type));
    }

    return make_shared<ASTNodeVariableTypeFunction>(
            this->lcontext(), params, ret_type);
}

variable_basic_type ASTNodeVariableTypeFunction::basic_type() const
{
    return variable_basic_type::FUNCTION;
}

optional<size_t> ASTNodeVariableTypeFunction::opsizeof() const
{
    // TODO report error
    return nullopt;
}

optional<size_t> ASTNodeVariableTypeFunction::opalignof() const
{
    // TODO report error
    return nullopt;
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
