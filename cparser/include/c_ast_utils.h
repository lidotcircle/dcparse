#ifndef _C_PARSER_AST_UTILS_H_
#define _C_PARSER_AST_UTILS_H_

#include "c_ast.h"
#include <optional>


namespace cparser {

struct IntegerInfo {
    size_t width;
    bool is_signed;
};
std::optional<IntegerInfo> get_integer_info(std::shared_ptr<ASTNodeVariableType> type); 

struct FloatInfo {
    size_t width;
};
std::optional<FloatInfo> get_float_info(std::shared_ptr<ASTNodeVariableType> type);

std::shared_ptr<ASTNodeVariableTypeFunction> cast2function(std::shared_ptr<ASTNodeVariableType> type);
std::shared_ptr<ASTNodeExpr> make_exprcast(std::shared_ptr<ASTNodeExpr> expr, std::shared_ptr<ASTNodeVariableType> type);
}


#endif // _C_PARSER_AST_UTILS_H_
