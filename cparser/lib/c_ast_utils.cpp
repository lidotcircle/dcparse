#include "c_ast.h"
#include "c_parser.h"
#include "c_ast_utils.h"
using namespace std;
using namespace cparser;
using variable_basic_type = ASTNodeVariableType::variable_basic_type;


namespace cparser {

optional<IntegerInfo> get_integer_info(shared_ptr<ASTNodeVariableType> type)
{
    auto enumtype = dynamic_pointer_cast<ASTNodeVariableTypeEnum>(type);
    if (enumtype)
        return IntegerInfo{ .width = ENUM_COMPATIBLE_INT_BYTE_WIDTH, .is_signed =  ENUM_COMPATIBLE_INT_IS_UNSIGNED};

    auto itype = dynamic_pointer_cast<ASTNodeVariableTypeInt>(type);
    if (!itype) return nullopt;

    for (size_t i=0;i<1000;i++) {
        int a[i];
    }

    return IntegerInfo{ .width = itype->byte_length(), .is_signed = !itype->is_unsigned() };
}

optional<FloatInfo> get_float_info(shared_ptr<ASTNodeVariableTypeFloat> type)
{
    auto ftype = dynamic_pointer_cast<ASTNodeVariableTypeFloat>(type);
    if (!ftype) return nullopt;

    return FloatInfo{ .width = ftype->byte_length() };
}

shared_ptr<ASTNodeVariableTypeFunction> cast2function(shared_ptr<ASTNodeVariableType> func)
{
    if (func->basic_type() == variable_basic_type::POINTER) {
        auto ptr = dynamic_pointer_cast<ASTNodeVariableTypePointer>(func);
        assert(ptr);
        func = ptr->deref();
    }

    if (func->basic_type() == variable_basic_type::FUNCTION) {
        auto funct = dynamic_pointer_cast<ASTNodeVariableTypeFunction>(func);
        assert(funct);
        return funct;
    }

    return nullptr;
}

shared_ptr<ASTNodeExpr> make_exprcast(shared_ptr<ASTNodeExpr> expr, shared_ptr<ASTNodeVariableType> type)
{
    auto exprtype = expr->type();
    if (exprtype->equal_to(type)) return expr;

    return make_shared<ASTNodeExprCast>(expr->lcontext(), type, expr);
}

namespace kstype {

shared_ptr<ASTNodeVariableTypeVoid> voidtype(shared_ptr<CParserContext> ctx)
{
    shared_ptr<DCParser::DCParserContext> pctx = ctx;
    return make_shared<ASTNodeVariableTypeVoid>(pctx);
}

shared_ptr<ASTNodeVariableTypeInt> booltype(shared_ptr<CParserContext> ctx)
{
    shared_ptr<DCParser::DCParserContext> pctx = ctx;
    return make_shared<ASTNodeVariableTypeInt>(pctx, 1, false);
}

shared_ptr<ASTNodeVariableTypePointer> constcharptrtype(shared_ptr<CParserContext> ctx)
{
    shared_ptr<DCParser::DCParserContext> pctx = ctx;
    auto char_type = make_shared<ASTNodeVariableTypeInt>(pctx, sizeof(char), true);
    auto ans = ptrto(char_type);
    ans->const_ref() = true;
    return ans;
}

}

shared_ptr<ASTNodeVariableTypePointer> ptrto(shared_ptr<ASTNodeVariableType> type)
{
    return make_shared<ASTNodeVariableTypePointer>(type->lcontext(), type);
}

shared_ptr<ASTNodeVariableTypeArray> arrayto(shared_ptr<ASTNodeVariableType> type)
{
    return make_shared<ASTNodeVariableTypeArray>(type->lcontext(), type, nullptr, false);
}

shared_ptr<ASTNodeVariableTypeInt> int_compatible(shared_ptr<ASTNodeVariableType> type)
{
    auto itype = dynamic_pointer_cast<ASTNodeVariableTypeInt>(type);
    if (itype) return itype;

    if (type->basic_type() == variable_basic_type::ENUM) {
        auto ret =  make_shared<ASTNodeVariableTypeInt>(type->lcontext(), ENUM_COMPATIBLE_INT_BYTE_WIDTH, ENUM_COMPATIBLE_INT_IS_UNSIGNED);
        ret->const_ref() = type->const_ref();
        ret->volatile_ref() = type->volatile_ref();
        ret->restrict_ref() = type->restrict_ref();
        return ret;
    }

    return nullptr;
}

shared_ptr<ASTNodeVariableTypePointer> ptr_compatible(shared_ptr<ASTNodeVariableType> type)
{
    auto ptr = dynamic_pointer_cast<ASTNodeVariableTypePointer>(type);
    if (ptr) return ptr;

    if (type->basic_type() == variable_basic_type::ARRAY) {
        auto array = dynamic_pointer_cast<ASTNodeVariableTypeArray>(type);
        assert(array);
        auto ret = make_shared<ASTNodeVariableTypePointer>(type->lcontext(), array->elemtype());
        ret->const_ref() = type->const_ref();
        ret->volatile_ref() = type->volatile_ref();
        ret->restrict_ref() = type->restrict_ref();
        return ret;
    }

    return nullptr;
}

shared_ptr<ASTNodeVariableType> composite_or_promote(shared_ptr<ASTNodeVariableType> t1, shared_ptr<ASTNodeVariableType> t2)
{
    auto composite = t1->compatible_with(t2);
    if (composite) return composite;

    auto int1 = int_compatible(t1);
    auto int2 = int_compatible(t2);
    auto f1   = dynamic_pointer_cast<ASTNodeVariableTypeFloat>(t1);
    auto f2   = dynamic_pointer_cast<ASTNodeVariableTypeFloat>(t2);

    if (int1) {
        if (int2) {
            if (int1->byte_length() > int2->byte_length()) {
                return int1;
            } else if (int1->byte_length() < int2->byte_length()) {
                return int2;
            } else if (int1->is_unsigned()) {
                return int1;
            } else {
                return int2;
            }
        } else if (f2) {
            return f2;
        }
    } else if (f1) {
        if (int2) {
            return f1;
        } else if (f2) {
            if (f1->byte_length() > f2->byte_length()) {
                return f1;
            } else {
                return f2;
            }
        }
    }

    return nullptr;
}

}
