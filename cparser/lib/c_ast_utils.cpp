#include "c_ast.h"
#include "c_ast_utils.h"
using namespace std;
using namespace cparser;
using variable_basic_type = ASTNodeVariableType::variable_basic_type;


namespace cparser {

optional<IntegerInfo> get_integer_info(shared_ptr<ASTNodeVariableType> type)
{
    auto itype = dynamic_pointer_cast<ASTNodeVariableTypeInt>(type);
    if (!itype) return nullopt;

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
    if (exprtype->equal_ignore_storage_class(type)) return expr;

    return make_shared<ASTNodeExprCast>(expr->lcontext(), type, expr);
}

}
