#include "ast.h"
#include <llvm/ADT/APFloat.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <map>
#include <memory>
#include <string>
#include <variant>


class ASTNodeVisitorLLVMGen : public ASTNodeVisitor
{
  private:
    std::unique_ptr<llvm::LLVMContext> m_context;
    std::unique_ptr<llvm::Module> m_module;
    std::unique_ptr<llvm::IRBuilder<>> m_builder;
    std::vector<std::map<std::string, std::variant<llvm::AllocaInst*, llvm::GlobalVariable*>>>
        m_namedValues;
    llvm::Value* m_prevValue;
    bool m_globalscope;

    llvm::Value* lookup_var(const std::string& name)
    {
        for (size_t i = m_namedValues.size(); i > 0; i--) {
            auto& k = m_namedValues.at(i - 1);
            if (k.count(name)) {
                if (std::holds_alternative<llvm::AllocaInst*>(k.at(name))) {
                    return std::get<0>(k.at(name));
                } else {
                    return std::get<1>(k.at(name));
                }
            }
        }
        return nullptr;
    }

  public:
    ASTNodeVisitorLLVMGen(const std::string& filename);

    void visitExpr(const ASTNodeExpr&) override;
    void visitExprStat(const ASTNodeExprList&) override;
    void visitBlockStat(const ASTNodeStatList&) override;
    void visitStatList(const std::vector<std::shared_ptr<ASTNodeStat>>&) override;
    void visitForStat(const ASTNodeExprList*,
                      const ASTNodeExpr*,
                      const ASTNodeExprList*,
                      const ASTNodeStat&) override;
    void visitIfStat(const ASTNodeExpr&, const ASTNodeStat&, const ASTNodeStat*) override;
    void visitReturnStat(const ASTNodeExpr&) override;
    void visitFuncDef(const std::string&,
                      const std::vector<std::string>&,
                      const ASTNodeBlockStat&) override;
    void visitFuncDecl(const std::string&, const std::vector<std::string>&) override;
    void visitCalcUnit(const std::vector<UnitItem>&) override;

    std::string codegen() const;
};
