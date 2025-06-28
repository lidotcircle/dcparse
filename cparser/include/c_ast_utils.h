#ifndef _C_PARSER_AST_UTILS_H_
#define _C_PARSER_AST_UTILS_H_

#include "c_ast.h"
#include <optional>


namespace cparser {

struct IntegerInfo
{
    size_t width;
    bool is_signed;
};
std::optional<IntegerInfo> get_integer_info(std::shared_ptr<ASTNodeVariableType> type);

struct FloatInfo
{
    size_t width;
};
std::optional<FloatInfo> get_float_info(std::shared_ptr<ASTNodeVariableType> type);

std::shared_ptr<ASTNodeVariableTypeFunction>
cast2function(std::shared_ptr<ASTNodeVariableType> type);
std::shared_ptr<ASTNodeExpr> make_exprcast(std::shared_ptr<ASTNodeExpr> expr,
                                           std::shared_ptr<ASTNodeVariableType> type);

namespace kstype {

std::shared_ptr<ASTNodeVariableTypeVoid> voidtype(std::shared_ptr<CParserContext> ctx);
std::shared_ptr<ASTNodeVariableTypeInt> booltype(std::shared_ptr<CParserContext> ctx);
std::shared_ptr<ASTNodeVariableTypePointer> constcharptrtype(std::shared_ptr<CParserContext> ctx);
std::shared_ptr<ASTNodeVariableTypeInt> chartype(std::shared_ptr<CParserContext> ctx);

bool is_char(std::shared_ptr<ASTNodeVariableType> type);

} // namespace kstype

namespace ksexpr {

bool is_string_literal(std::shared_ptr<ASTNodeExpr> expr);

}

std::shared_ptr<ASTNodeVariableTypePointer> ptrto(std::shared_ptr<ASTNodeVariableType> type);
std::shared_ptr<ASTNodeVariableTypeArray> arrayto(std::shared_ptr<ASTNodeVariableType> type);

std::shared_ptr<ASTNodeVariableTypeInt> int_compatible(std::shared_ptr<ASTNodeVariableType> type);
std::shared_ptr<ASTNodeVariableTypePointer>
ptr_compatible(std::shared_ptr<ASTNodeVariableType> type);

std::shared_ptr<ASTNodeVariableType> composite_or_promote(std::shared_ptr<ASTNodeVariableType> t1,
                                                          std::shared_ptr<ASTNodeVariableType> t2);

} // namespace cparser


#endif // _C_PARSER_AST_UTILS_H_
