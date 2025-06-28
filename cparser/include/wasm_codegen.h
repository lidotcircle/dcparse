#ifndef _WASM_CODEGEN_H_
#define _WASM_CODEGEN_H_

#include "c_ast.h"
#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

namespace cparser {

// WebAssembly type representation
enum class WasmType
{
    I32,
    I64,
    F32,
    F64,
    VOID
};

// WebAssembly instruction representation
struct WasmInstruction
{
    std::string opcode;
    std::vector<std::string> operands;

    WasmInstruction(const std::string& op) : opcode(op)
    {}
    WasmInstruction(const std::string& op, const std::vector<std::string>& ops)
        : opcode(op), operands(ops)
    {}

    std::string toString() const;
};

// Local variable information
struct LocalVariable
{
    std::string name;
    WasmType type;
    size_t index;
    size_t size;

    LocalVariable() : name(""), type(WasmType::I32), index(0), size(0)
    {}
    LocalVariable(const std::string& n, WasmType t, size_t idx, size_t sz = 0)
        : name(n), type(t), index(idx), size(sz)
    {}
};

// Function information
struct WasmFunction
{
    std::string name;
    std::vector<WasmType> params;
    std::vector<std::string> paramNames;
    WasmType returnType;
    std::vector<LocalVariable> locals;
    std::vector<WasmInstruction> instructions;
    size_t localCount;

    WasmFunction(const std::string& n) : name(n), returnType(WasmType::VOID), localCount(0)
    {}

    std::string toString() const;
};

// Memory layout information
struct MemoryLayout
{
    std::unordered_map<std::string, size_t> globalOffsets;
    std::unordered_map<std::string, size_t> stringOffsets;
    size_t currentOffset;

    MemoryLayout() : currentOffset(0)
    {}
};

class WasmCodeGenerator
{
  private:
    std::vector<WasmFunction> functions;
    std::vector<std::string> globals;
    std::vector<std::string> dataSegments;
    MemoryLayout memoryLayout;
    WasmFunction* currentFunction;
    std::unordered_map<std::string, LocalVariable> currentLocals;
    size_t labelCounter;
    size_t memorySize;

    // Type mapping utilities
    WasmType mapCTypeToWasm(std::shared_ptr<ASTNodeVariableType> type);
    size_t getTypeSize(std::shared_ptr<ASTNodeVariableType> type);
    bool isIntegerType(std::shared_ptr<ASTNodeVariableType> type);
    bool isFloatType(std::shared_ptr<ASTNodeVariableType> type);
    bool isPointerType(std::shared_ptr<ASTNodeVariableType> type);

    // Code generation utilities
    void emitInstruction(const std::string& opcode);
    void emitInstruction(const std::string& opcode, const std::string& operand);
    void emitInstruction(const std::string& opcode, const std::vector<std::string>& operands);
    std::string generateLabel();

    // Memory management
    size_t allocateGlobalMemory(const std::string& name, size_t size);
    size_t allocateStringLiteral(const std::string& str);

    // Local variable management
    size_t allocateLocal(const std::string& name, WasmType type, size_t size = 0);
    LocalVariable* findLocal(const std::string& name);

    // Expression generation
    void generateExpression(std::shared_ptr<ASTNodeExpr> expr);
    void generateIdentifierExpr(std::shared_ptr<ASTNodeExprIdentifier> expr);
    void generateIntegerExpr(std::shared_ptr<ASTNodeExprInteger> expr);
    void generateFloatExpr(std::shared_ptr<ASTNodeExprFloat> expr);
    void generateStringExpr(std::shared_ptr<ASTNodeExprString> expr);
    void generateBinaryOpExpr(std::shared_ptr<ASTNodeExprBinaryOp> expr);
    void generateUnaryOpExpr(std::shared_ptr<ASTNodeExprUnaryOp> expr);
    void generateFunctionCallExpr(std::shared_ptr<ASTNodeExprFunctionCall> expr);
    void generateIndexingExpr(std::shared_ptr<ASTNodeExprIndexing> expr);
    void generateMemberAccessExpr(std::shared_ptr<ASTNodeExprMemberAccess> expr);
    void generatePointerMemberAccessExpr(std::shared_ptr<ASTNodeExprPointerMemberAccess> expr);
    void generateCastExpr(std::shared_ptr<ASTNodeExprCast> expr);
    void generateSizeofExpr(std::shared_ptr<ASTNodeExprSizeof> expr);
    void generateConditionalExpr(std::shared_ptr<ASTNodeExprConditional> expr);

    // Statement generation
    void generateStatement(std::shared_ptr<ASTNodeStat> stmt);
    void generateCompoundStatement(std::shared_ptr<ASTNodeStatCompound> stmt);
    void generateExpressionStatement(std::shared_ptr<ASTNodeStatExpr> stmt);
    void generateIfStatement(std::shared_ptr<ASTNodeStatIF> stmt);
    void generateForStatement(std::shared_ptr<ASTNodeStatFor> stmt);
    void generateForDeclStatement(std::shared_ptr<ASTNodeStatForDecl> stmt);
    void generateDoWhileStatement(std::shared_ptr<ASTNodeStatDoWhile> stmt);
    void generateSwitchStatement(std::shared_ptr<ASTNodeStatSwitch> stmt);
    void generateReturnStatement(std::shared_ptr<ASTNodeStatReturn> stmt);
    void generateBreakStatement(std::shared_ptr<ASTNodeStatBreak> stmt);
    void generateContinueStatement(std::shared_ptr<ASTNodeStatContinue> stmt);
    void generateGotoStatement(std::shared_ptr<ASTNodeStatGoto> stmt);
    void generateLabelStatement(std::shared_ptr<ASTNodeStatLabel> stmt);
    void generateCaseStatement(std::shared_ptr<ASTNodeStatCase> stmt);
    void generateDefaultStatement(std::shared_ptr<ASTNodeStatDefault> stmt);

    // Declaration generation
    void generateDeclaration(std::shared_ptr<ASTNodeDeclaration> decl);
    void generateInitDeclarator(std::shared_ptr<ASTNodeInitDeclarator> decl);
    void generateFunctionDefinition(std::shared_ptr<ASTNodeFunctionDefinition> funcDef);

    // Type casting utilities
    void generateTypeConversion(WasmType fromType, WasmType toType);
    void generateImplicitCast(std::shared_ptr<ASTNodeVariableType> fromType,
                              std::shared_ptr<ASTNodeVariableType> toType);

    // Arithmetic and logical operations
    void generateArithmeticOp(const std::string& op, WasmType type);
    void generateComparisonOp(const std::string& op, WasmType type);
    void generateLogicalOp(const std::string& op);
    void generateBitwiseOp(const std::string& op, WasmType type);

    // Memory operations
    void generateLoad(WasmType type, size_t offset = 0);
    void generateStore(WasmType type, size_t offset = 0);
    void generateMemoryAddress(std::shared_ptr<ASTNodeExpr> expr);

    // Control flow utilities
    void generateConditionalJump(const std::string& trueLabel, const std::string& falseLabel);
    void generateLoop(const std::string& loopLabel, const std::string& breakLabel);

  public:
    WasmCodeGenerator();
    ~WasmCodeGenerator() = default;

    // Main generation methods
    std::string generateModule(std::shared_ptr<ASTNodeTranslationUnit> translationUnit);
    std::string generateFunction(std::shared_ptr<ASTNodeFunctionDefinition> funcDef);

    // Utility methods
    void reset();
    std::string getWasmModule() const;

    // Error handling
    void reportError(const std::string& message);
};

} // namespace cparser

#endif // _WASM_CODEGEN_H_
