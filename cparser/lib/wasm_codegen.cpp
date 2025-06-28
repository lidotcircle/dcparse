#include "wasm_codegen.h"
#include "c_ast_utils.h"
#include <algorithm>
#include <iostream>
#include <stdexcept>

namespace cparser {

// WasmInstruction implementation
std::string WasmInstruction::toString() const
{
    std::string result = opcode;
    for (const auto& operand : operands) {
        result += " " + operand;
    }
    return result;
}

// WasmFunction implementation
std::string WasmFunction::toString() const
{
    std::ostringstream oss;
    oss << "  (func $" << name;

    // Parameters
    for (size_t i = 0; i < params.size(); ++i) {
        std::string paramName =
            (i < paramNames.size()) ? paramNames[i] : ("param" + std::to_string(i));
        oss << " (param $" << paramName << " ";
        switch (params[i]) {
        case WasmType::I32:
            oss << "i32";
            break;
        case WasmType::I64:
            oss << "i64";
            break;
        case WasmType::F32:
            oss << "f32";
            break;
        case WasmType::F64:
            oss << "f64";
            break;
        default:
            break;
        }
        oss << ")";
    }

    // Return type
    if (returnType != WasmType::VOID) {
        oss << " (result ";
        switch (returnType) {
        case WasmType::I32:
            oss << "i32";
            break;
        case WasmType::I64:
            oss << "i64";
            break;
        case WasmType::F32:
            oss << "f32";
            break;
        case WasmType::F64:
            oss << "f64";
            break;
        default:
            break;
        }
        oss << ")";
    }

    oss << "\n";

    // Local variables
    for (const auto& local : locals) {
        oss << "    (local $" << local.name << " ";
        switch (local.type) {
        case WasmType::I32:
            oss << "i32";
            break;
        case WasmType::I64:
            oss << "i64";
            break;
        case WasmType::F32:
            oss << "f32";
            break;
        case WasmType::F64:
            oss << "f64";
            break;
        default:
            break;
        }
        oss << ")\n";
    }

    // Instructions
    for (const auto& instruction : instructions) {
        oss << "    " << instruction.toString() << "\n";
    }

    oss << "  )";
    return oss.str();
}

// WasmCodeGenerator implementation
WasmCodeGenerator::WasmCodeGenerator()
    : currentFunction(nullptr), labelCounter(0), memorySize(65536)
{
    reset();
}

void WasmCodeGenerator::reset()
{
    functions.clear();
    globals.clear();
    dataSegments.clear();
    memoryLayout = MemoryLayout();
    currentFunction = nullptr;
    currentLocals.clear();
    labelCounter = 0;
}

WasmType WasmCodeGenerator::mapCTypeToWasm(std::shared_ptr<ASTNodeVariableType> type)
{
    if (!type)
        return WasmType::VOID;

    auto basicType = type->basic_type();
    switch (basicType) {
    case ASTNodeVariableType::variable_basic_type::VOID:
        return WasmType::VOID;

    case ASTNodeVariableType::variable_basic_type::INT: {
        auto intType = std::dynamic_pointer_cast<ASTNodeVariableTypeInt>(type);
        if (intType) {
            if (intType->byte_length() <= 4) {
                return WasmType::I32;
            } else {
                return WasmType::I64;
            }
        }
        return WasmType::I32;
    }

    case ASTNodeVariableType::variable_basic_type::FLOAT: {
        auto floatType = std::dynamic_pointer_cast<ASTNodeVariableTypeFloat>(type);
        if (floatType) {
            if (floatType->byte_length() <= 4) {
                return WasmType::F32;
            } else {
                return WasmType::F64;
            }
        }
        return WasmType::F32;
    }

    case ASTNodeVariableType::variable_basic_type::POINTER:
    case ASTNodeVariableType::variable_basic_type::ARRAY:
        return WasmType::I32; // Pointers are 32-bit addresses in WASM

    case ASTNodeVariableType::variable_basic_type::ENUM:
        return WasmType::I32;

    case ASTNodeVariableType::variable_basic_type::STRUCT:
    case ASTNodeVariableType::variable_basic_type::UNION:
        return WasmType::I32; // Return pointer to struct/union

    default:
        return WasmType::VOID;
    }
}

size_t WasmCodeGenerator::getTypeSize(std::shared_ptr<ASTNodeVariableType> type)
{
    if (!type)
        return 0;

    auto sizeOpt = type->opsizeof();
    if (sizeOpt.has_value()) {
        return sizeOpt.value();
    }

    // Default sizes
    auto basicType = type->basic_type();
    switch (basicType) {
    case ASTNodeVariableType::variable_basic_type::INT: {
        auto intType = std::dynamic_pointer_cast<ASTNodeVariableTypeInt>(type);
        return intType ? intType->byte_length() : 4;
    }
    case ASTNodeVariableType::variable_basic_type::FLOAT: {
        auto floatType = std::dynamic_pointer_cast<ASTNodeVariableTypeFloat>(type);
        return floatType ? floatType->byte_length() : 4;
    }
    case ASTNodeVariableType::variable_basic_type::POINTER:
        return 4; // 32-bit pointers
    default:
        return 4;
    }
}

bool WasmCodeGenerator::isIntegerType(std::shared_ptr<ASTNodeVariableType> type)
{
    if (!type)
        return false;
    auto basicType = type->basic_type();
    return basicType == ASTNodeVariableType::variable_basic_type::INT ||
           basicType == ASTNodeVariableType::variable_basic_type::ENUM ||
           basicType == ASTNodeVariableType::variable_basic_type::POINTER;
}

bool WasmCodeGenerator::isFloatType(std::shared_ptr<ASTNodeVariableType> type)
{
    if (!type)
        return false;
    return type->basic_type() == ASTNodeVariableType::variable_basic_type::FLOAT;
}

bool WasmCodeGenerator::isPointerType(std::shared_ptr<ASTNodeVariableType> type)
{
    if (!type)
        return false;
    auto basicType = type->basic_type();
    return basicType == ASTNodeVariableType::variable_basic_type::POINTER ||
           basicType == ASTNodeVariableType::variable_basic_type::ARRAY;
}

void WasmCodeGenerator::emitInstruction(const std::string& opcode)
{
    if (currentFunction) {
        currentFunction->instructions.emplace_back(opcode);
    }
}

void WasmCodeGenerator::emitInstruction(const std::string& opcode, const std::string& operand)
{
    if (currentFunction) {
        currentFunction->instructions.emplace_back(opcode, std::vector<std::string>{operand});
    }
}

void WasmCodeGenerator::emitInstruction(const std::string& opcode,
                                        const std::vector<std::string>& operands)
{
    if (currentFunction) {
        currentFunction->instructions.emplace_back(opcode, operands);
    }
}

std::string WasmCodeGenerator::generateLabel()
{
    return "label" + std::to_string(labelCounter++);
}

size_t WasmCodeGenerator::allocateGlobalMemory(const std::string& name, size_t size)
{
    size_t offset = memoryLayout.currentOffset;
    memoryLayout.globalOffsets[name] = offset;
    memoryLayout.currentOffset += size;
    return offset;
}

size_t WasmCodeGenerator::allocateStringLiteral(const std::string& str)
{
    auto it = memoryLayout.stringOffsets.find(str);
    if (it != memoryLayout.stringOffsets.end()) {
        return it->second;
    }

    size_t offset = memoryLayout.currentOffset;
    memoryLayout.stringOffsets[str] = offset;
    memoryLayout.currentOffset += str.length() + 1; // +1 for null terminator

    // Add to data segments
    std::ostringstream dataSegment;
    dataSegment << "  (data (i32.const " << offset << ") \"";
    for (char c : str) {
        if (c == '"' || c == '\\') {
            dataSegment << '\\' << c;
        } else if (c >= 32 && c <= 126) {
            dataSegment << c;
        } else {
            dataSegment << "\\x" << std::hex << static_cast<unsigned char>(c);
        }
    }
    dataSegment << "\\00\")";
    dataSegments.push_back(dataSegment.str());

    return offset;
}

size_t WasmCodeGenerator::allocateLocal(const std::string& name, WasmType type, size_t size)
{
    if (!currentFunction)
        return 0;

    size_t index = currentFunction->localCount++;
    LocalVariable local(name, type, index, size);
    currentFunction->locals.push_back(local);
    currentLocals[name] = local;
    return index;
}

LocalVariable* WasmCodeGenerator::findLocal(const std::string& name)
{
    auto it = currentLocals.find(name);
    return (it != currentLocals.end()) ? &it->second : nullptr;
}

void WasmCodeGenerator::generateExpression(std::shared_ptr<ASTNodeExpr> expr)
{
    if (!expr)
        return;

    if (auto identExpr = std::dynamic_pointer_cast<ASTNodeExprIdentifier>(expr)) {
        generateIdentifierExpr(identExpr);
    } else if (auto intExpr = std::dynamic_pointer_cast<ASTNodeExprInteger>(expr)) {
        generateIntegerExpr(intExpr);
    } else if (auto floatExpr = std::dynamic_pointer_cast<ASTNodeExprFloat>(expr)) {
        generateFloatExpr(floatExpr);
    } else if (auto stringExpr = std::dynamic_pointer_cast<ASTNodeExprString>(expr)) {
        generateStringExpr(stringExpr);
    } else if (auto binaryOpExpr = std::dynamic_pointer_cast<ASTNodeExprBinaryOp>(expr)) {
        generateBinaryOpExpr(binaryOpExpr);
    } else if (auto unaryOpExpr = std::dynamic_pointer_cast<ASTNodeExprUnaryOp>(expr)) {
        generateUnaryOpExpr(unaryOpExpr);
    } else if (auto funcCallExpr = std::dynamic_pointer_cast<ASTNodeExprFunctionCall>(expr)) {
        generateFunctionCallExpr(funcCallExpr);
    } else if (auto indexExpr = std::dynamic_pointer_cast<ASTNodeExprIndexing>(expr)) {
        generateIndexingExpr(indexExpr);
    } else if (auto memberExpr = std::dynamic_pointer_cast<ASTNodeExprMemberAccess>(expr)) {
        generateMemberAccessExpr(memberExpr);
    } else if (auto ptrMemberExpr =
                   std::dynamic_pointer_cast<ASTNodeExprPointerMemberAccess>(expr)) {
        generatePointerMemberAccessExpr(ptrMemberExpr);
    } else if (auto castExpr = std::dynamic_pointer_cast<ASTNodeExprCast>(expr)) {
        generateCastExpr(castExpr);
    } else if (auto sizeofExpr = std::dynamic_pointer_cast<ASTNodeExprSizeof>(expr)) {
        generateSizeofExpr(sizeofExpr);
    } else if (auto condExpr = std::dynamic_pointer_cast<ASTNodeExprConditional>(expr)) {
        generateConditionalExpr(condExpr);
    }
}

void WasmCodeGenerator::generateIdentifierExpr(std::shared_ptr<ASTNodeExprIdentifier> expr)
{
    std::string name = expr->id()->id;

    // Check if it's a local variable
    auto local = findLocal(name);
    if (local) {
        emitInstruction("local.get", "$" + local->name);
        return;
    }

    // Check if it's a global variable
    auto globalIt = memoryLayout.globalOffsets.find(name);
    if (globalIt != memoryLayout.globalOffsets.end()) {
        emitInstruction("i32.const", std::to_string(globalIt->second));
        auto wasmType = mapCTypeToWasm(expr->type());
        generateLoad(wasmType);
        return;
    }

    // Handle function names or enum constants
    if (expr->type() &&
        expr->type()->basic_type() == ASTNodeVariableType::variable_basic_type::ENUM) {
        auto constVal = expr->get_integer_constant();
        if (constVal.has_value()) {
            emitInstruction("i32.const", std::to_string(constVal.value()));
        }
    }
}

void WasmCodeGenerator::generateIntegerExpr(std::shared_ptr<ASTNodeExprInteger> expr)
{
    auto wasmType = mapCTypeToWasm(expr->type());
    long long value = expr->token()->value;

    if (wasmType == WasmType::I32) {
        emitInstruction("i32.const", std::to_string(static_cast<int32_t>(value)));
    } else if (wasmType == WasmType::I64) {
        emitInstruction("i64.const", std::to_string(value));
    }
}

void WasmCodeGenerator::generateFloatExpr(std::shared_ptr<ASTNodeExprFloat> expr)
{
    auto wasmType = mapCTypeToWasm(expr->type());
    long double value = expr->token()->value;

    if (wasmType == WasmType::F32) {
        emitInstruction("f32.const", std::to_string(static_cast<float>(value)));
    } else if (wasmType == WasmType::F64) {
        emitInstruction("f64.const", std::to_string(static_cast<double>(value)));
    }
}

void WasmCodeGenerator::generateStringExpr(std::shared_ptr<ASTNodeExprString> expr)
{
    std::string str = expr->token()->value;
    size_t offset = allocateStringLiteral(str);
    emitInstruction("i32.const", std::to_string(offset));
}

void WasmCodeGenerator::generateBinaryOpExpr(std::shared_ptr<ASTNodeExprBinaryOp> expr)
{
    auto op = expr->get_operator();

    // Handle assignment operators
    if (op >= ASTNodeExprBinaryOp::BinaryOperatorType::ASSIGNMENT) {
        // For assignments, we need the address of the left operand
        generateMemoryAddress(expr->left());
        generateExpression(expr->right());

        auto wasmType = mapCTypeToWasm(expr->type());
        generateStore(wasmType);
        return;
    }

    // Handle logical operators with short-circuit evaluation
    if (op == ASTNodeExprBinaryOp::BinaryOperatorType::LOGICAL_AND) {
        std::string falseLabel = generateLabel();
        std::string endLabel = generateLabel();

        generateExpression(expr->left());
        emitInstruction("i32.eqz");
        emitInstruction("br_if", "$" + falseLabel);

        generateExpression(expr->right());
        emitInstruction("br", "$" + endLabel);

        emitInstruction("block", "$" + falseLabel);
        emitInstruction("i32.const", "0");
        emitInstruction("end");

        emitInstruction("block", "$" + endLabel);
        emitInstruction("end");
        return;
    }

    if (op == ASTNodeExprBinaryOp::BinaryOperatorType::LOGICAL_OR) {
        std::string trueLabel = generateLabel();
        std::string endLabel = generateLabel();

        generateExpression(expr->left());
        emitInstruction("br_if", "$" + trueLabel);

        generateExpression(expr->right());
        emitInstruction("br", "$" + endLabel);

        emitInstruction("block", "$" + trueLabel);
        emitInstruction("i32.const", "1");
        emitInstruction("end");

        emitInstruction("block", "$" + endLabel);
        emitInstruction("end");
        return;
    }

    // Regular binary operations
    generateExpression(expr->left());
    generateExpression(expr->right());

    auto wasmType = mapCTypeToWasm(expr->type());

    switch (op) {
    case ASTNodeExprBinaryOp::BinaryOperatorType::PLUS:
        generateArithmeticOp("add", wasmType);
        break;
    case ASTNodeExprBinaryOp::BinaryOperatorType::MINUS:
        generateArithmeticOp("sub", wasmType);
        break;
    case ASTNodeExprBinaryOp::BinaryOperatorType::MULTIPLY:
        generateArithmeticOp("mul", wasmType);
        break;
    case ASTNodeExprBinaryOp::BinaryOperatorType::DIVISION:
        if (isFloatType(expr->type())) {
            generateArithmeticOp("div", wasmType);
        } else {
            // Check if type is signed or unsigned
            auto intType = std::dynamic_pointer_cast<ASTNodeVariableTypeInt>(expr->type());
            if (intType && intType->is_unsigned()) {
                generateArithmeticOp("div_u", wasmType);
            } else {
                generateArithmeticOp("div_s", wasmType);
            }
        }
        break;
    case ASTNodeExprBinaryOp::BinaryOperatorType::REMAINDER:
        generateArithmeticOp("rem_s", wasmType);
        break;
    case ASTNodeExprBinaryOp::BinaryOperatorType::LEFT_SHIFT:
        generateBitwiseOp("shl", wasmType);
        break;
    case ASTNodeExprBinaryOp::BinaryOperatorType::RIGHT_SHIFT:
        generateBitwiseOp("shr_s", wasmType);
        break;
    case ASTNodeExprBinaryOp::BinaryOperatorType::BITWISE_AND:
        generateBitwiseOp("and", wasmType);
        break;
    case ASTNodeExprBinaryOp::BinaryOperatorType::BITWISE_OR:
        generateBitwiseOp("or", wasmType);
        break;
    case ASTNodeExprBinaryOp::BinaryOperatorType::BITWISE_XOR:
        generateBitwiseOp("xor", wasmType);
        break;
    case ASTNodeExprBinaryOp::BinaryOperatorType::LESS_THAN:
        generateComparisonOp("lt", wasmType);
        break;
    case ASTNodeExprBinaryOp::BinaryOperatorType::GREATER_THAN:
        generateComparisonOp("gt", wasmType);
        break;
    case ASTNodeExprBinaryOp::BinaryOperatorType::LESS_THAN_EQUAL:
        generateComparisonOp("le", wasmType);
        break;
    case ASTNodeExprBinaryOp::BinaryOperatorType::GREATER_THAN_EQUAL:
        generateComparisonOp("ge", wasmType);
        break;
    case ASTNodeExprBinaryOp::BinaryOperatorType::EQUAL:
        generateComparisonOp("eq", wasmType);
        break;
    case ASTNodeExprBinaryOp::BinaryOperatorType::NOT_EQUAL:
        generateComparisonOp("ne", wasmType);
        break;
    default:
        reportError("Unsupported binary operator");
        break;
    }
}

void WasmCodeGenerator::generateUnaryOpExpr(std::shared_ptr<ASTNodeExprUnaryOp> expr)
{
    auto op = expr->get_operator();

    switch (op) {
    case ASTNodeExprUnaryOp::UnaryOperatorType::PLUS:
        generateExpression(expr->get_expr());
        break;

    case ASTNodeExprUnaryOp::UnaryOperatorType::MINUS: {
        if (isFloatType(expr->type())) {
            generateExpression(expr->get_expr());
            auto wasmType = mapCTypeToWasm(expr->type());
            if (wasmType == WasmType::F32) {
                emitInstruction("f32.neg");
            } else {
                emitInstruction("f64.neg");
            }
        } else {
            emitInstruction("i32.const", "0");
            generateExpression(expr->get_expr());
            auto wasmType = mapCTypeToWasm(expr->type());
            generateArithmeticOp("sub", wasmType);
        }
        break;
    }

    case ASTNodeExprUnaryOp::UnaryOperatorType::LOGICAL_NOT:
        generateExpression(expr->get_expr());
        emitInstruction("i32.eqz");
        break;

    case ASTNodeExprUnaryOp::UnaryOperatorType::BITWISE_NOT:
        generateExpression(expr->get_expr());
        emitInstruction("i32.const", "-1");
        emitInstruction("i32.xor");
        break;

    case ASTNodeExprUnaryOp::UnaryOperatorType::DEREFERENCE: {
        generateExpression(expr->get_expr());
        auto wasmType = mapCTypeToWasm(expr->type());
        generateLoad(wasmType);
        break;
    }

    case ASTNodeExprUnaryOp::UnaryOperatorType::REFERENCE:
        generateMemoryAddress(expr->get_expr());
        break;

    case ASTNodeExprUnaryOp::UnaryOperatorType::PREFIX_INC:
    case ASTNodeExprUnaryOp::UnaryOperatorType::PREFIX_DEC: {
        generateMemoryAddress(expr->get_expr());
        generateMemoryAddress(expr->get_expr());
        auto wasmType = mapCTypeToWasm(expr->type());
        generateLoad(wasmType);

        if (wasmType == WasmType::I32) {
            emitInstruction("i32.const", "1");
        } else if (wasmType == WasmType::I64) {
            emitInstruction("i64.const", "1");
        } else if (wasmType == WasmType::F32) {
            emitInstruction("f32.const", "1.0");
        } else if (wasmType == WasmType::F64) {
            emitInstruction("f64.const", "1.0");
        }

        if (op == ASTNodeExprUnaryOp::UnaryOperatorType::PREFIX_INC) {
            generateArithmeticOp("add", wasmType);
        } else {
            generateArithmeticOp("sub", wasmType);
        }

        emitInstruction("tee_local", "$temp");
        generateStore(wasmType);
        emitInstruction("local.get", "$temp");
        break;
    }

    case ASTNodeExprUnaryOp::UnaryOperatorType::SIZEOF: {
        size_t size = getTypeSize(expr->get_expr()->type());
        emitInstruction("i32.const", std::to_string(size));
        break;
    }

    default:
        reportError("Unsupported unary operator");
        break;
    }
}

void WasmCodeGenerator::generateFunctionCallExpr(std::shared_ptr<ASTNodeExprFunctionCall> expr)
{
    // Generate arguments first (left to right)
    auto args = expr->args();
    for (size_t i = 0; i < args->size(); ++i) {
        generateExpression((*args)[i]);
    }

    // Get function name
    auto funcExpr = std::dynamic_pointer_cast<ASTNodeExprIdentifier>(expr->func());
    if (funcExpr) {
        std::string funcName = funcExpr->id()->id;
        emitInstruction("call", "$" + funcName);
    } else {
        // Indirect function call - not implemented for now
        reportError("Indirect function calls not yet supported");
    }
}

void WasmCodeGenerator::generateIndexingExpr(std::shared_ptr<ASTNodeExprIndexing> expr)
{
    // Calculate address: base + index * element_size
    generateExpression(expr->array());
    generateExpression(expr->idx());

    auto elemType = expr->type();
    size_t elemSize = getTypeSize(elemType);

    if (elemSize > 1) {
        emitInstruction("i32.const", std::to_string(elemSize));
        emitInstruction("i32.mul");
    }

    emitInstruction("i32.add");

    // Load the value
    auto wasmType = mapCTypeToWasm(elemType);
    generateLoad(wasmType);
}

void WasmCodeGenerator::generateMemberAccessExpr(std::shared_ptr<ASTNodeExprMemberAccess> expr)
{
    // Get the object address
    generateMemoryAddress(expr->obj());

    // Add member offset (simplified - would need struct layout info)
    std::string memberName = expr->member()->id;
    // For now, assume offset 0 - would need to calculate actual offset
    emitInstruction("i32.const", "0");
    emitInstruction("i32.add");

    auto wasmType = mapCTypeToWasm(expr->type());
    generateLoad(wasmType);
}

void WasmCodeGenerator::generatePointerMemberAccessExpr(
    std::shared_ptr<ASTNodeExprPointerMemberAccess> expr)
{
    // Get the pointer value
    generateExpression(expr->obj());

    // Add member offset
    std::string memberName = expr->member()->id;
    // For now, assume offset 0
    emitInstruction("i32.const", "0");
    emitInstruction("i32.add");

    auto wasmType = mapCTypeToWasm(expr->type());
    generateLoad(wasmType);
}

void WasmCodeGenerator::generateCastExpr(std::shared_ptr<ASTNodeExprCast> expr)
{
    generateExpression(expr->get_expr());

    auto fromType = mapCTypeToWasm(expr->get_expr()->type());
    auto toType = mapCTypeToWasm(expr->get_type());

    generateTypeConversion(fromType, toType);
}

void WasmCodeGenerator::generateSizeofExpr(std::shared_ptr<ASTNodeExprSizeof> expr)
{
    size_t size = getTypeSize(expr->get_type());
    emitInstruction("i32.const", std::to_string(size));
}

void WasmCodeGenerator::generateConditionalExpr(std::shared_ptr<ASTNodeExprConditional> expr)
{
    std::string elseLabel = generateLabel();
    std::string endLabel = generateLabel();

    generateExpression(expr->get_cond());
    emitInstruction("i32.eqz");
    emitInstruction("br_if", "$" + elseLabel);

    generateExpression(expr->get_true());
    emitInstruction("br", "$" + endLabel);

    emitInstruction("block", "$" + elseLabel);
    generateExpression(expr->get_false());
    emitInstruction("end");

    emitInstruction("block", "$" + endLabel);
    emitInstruction("end");
}

void WasmCodeGenerator::generateStatement(std::shared_ptr<ASTNodeStat> stmt)
{
    if (!stmt)
        return;

    if (auto compoundStmt = std::dynamic_pointer_cast<ASTNodeStatCompound>(stmt)) {
        generateCompoundStatement(compoundStmt);
    } else if (auto exprStmt = std::dynamic_pointer_cast<ASTNodeStatExpr>(stmt)) {
        generateExpressionStatement(exprStmt);
    } else if (auto ifStmt = std::dynamic_pointer_cast<ASTNodeStatIF>(stmt)) {
        generateIfStatement(ifStmt);
    } else if (auto forStmt = std::dynamic_pointer_cast<ASTNodeStatFor>(stmt)) {
        generateForStatement(forStmt);
    } else if (auto forDeclStmt = std::dynamic_pointer_cast<ASTNodeStatForDecl>(stmt)) {
        generateForDeclStatement(forDeclStmt);
    } else if (auto doWhileStmt = std::dynamic_pointer_cast<ASTNodeStatDoWhile>(stmt)) {
        generateDoWhileStatement(doWhileStmt);
    } else if (auto returnStmt = std::dynamic_pointer_cast<ASTNodeStatReturn>(stmt)) {
        generateReturnStatement(returnStmt);
    } else if (auto breakStmt = std::dynamic_pointer_cast<ASTNodeStatBreak>(stmt)) {
        generateBreakStatement(breakStmt);
    } else if (auto continueStmt = std::dynamic_pointer_cast<ASTNodeStatContinue>(stmt)) {
        generateContinueStatement(continueStmt);
    }
    // Add other statement types as needed
}

void WasmCodeGenerator::generateCompoundStatement(std::shared_ptr<ASTNodeStatCompound> stmt)
{
    auto statList = stmt->StatementList();
    if (!statList)
        return;

    for (size_t i = 0; i < statList->size(); ++i) {
        auto blockItem = (*statList)[i];

        if (auto statement = std::dynamic_pointer_cast<ASTNodeStat>(blockItem)) {
            generateStatement(statement);
        } else if (auto declaration = std::dynamic_pointer_cast<ASTNodeDeclaration>(blockItem)) {
            generateDeclaration(declaration);
        }
    }
}

void WasmCodeGenerator::generateExpressionStatement(std::shared_ptr<ASTNodeStatExpr> stmt)
{
    if (stmt->expr()) {
        generateExpression(stmt->expr());
        // Drop the result if it's not void
        auto exprType = mapCTypeToWasm(stmt->expr()->type());
        if (exprType != WasmType::VOID) {
            emitInstruction("drop");
        }
    }
}

void WasmCodeGenerator::generateIfStatement(std::shared_ptr<ASTNodeStatIF> stmt)
{
    std::string elseLabel = generateLabel();
    std::string endLabel = generateLabel();

    generateExpression(stmt->condition());
    emitInstruction("i32.eqz");
    emitInstruction("br_if", "$" + elseLabel);

    generateStatement(stmt->trueStat());

    if (stmt->falseStat()) {
        emitInstruction("br", "$" + endLabel);
        emitInstruction("block", "$" + elseLabel);
        generateStatement(stmt->falseStat());
        emitInstruction("end");

        emitInstruction("block", "$" + endLabel);
        emitInstruction("end");
    } else {
        emitInstruction("block", "$" + elseLabel);
        emitInstruction("end");
    }
}

void WasmCodeGenerator::generateForStatement(std::shared_ptr<ASTNodeStatFor> stmt)
{
    std::string loopLabel = generateLabel();
    std::string breakLabel = generateLabel();
    std::string condLabel = generateLabel();

    // Generate initialization
    if (stmt->init()) {
        generateExpression(stmt->init());
        emitInstruction("drop"); // Discard result
    }

    // Jump to condition check
    emitInstruction("br", "$" + condLabel);

    // Loop body
    emitInstruction("block", "$" + breakLabel);
    emitInstruction("loop", "$" + loopLabel);
    emitInstruction("block", "$" + condLabel);

    // Generate condition
    if (stmt->cond()) {
        generateExpression(stmt->cond());
        emitInstruction("i32.eqz");
        emitInstruction("br_if", "$" + breakLabel);
    }

    // Generate loop body
    if (stmt->stat()) {
        generateStatement(stmt->stat());
    }

    // Generate post-increment
    if (stmt->post()) {
        generateExpression(stmt->post());
        emitInstruction("drop");
    }

    emitInstruction("br", "$" + loopLabel);
    emitInstruction("end"); // end condition block
    emitInstruction("end"); // end loop
    emitInstruction("end"); // end break block
}

void WasmCodeGenerator::generateForDeclStatement(std::shared_ptr<ASTNodeStatForDecl> stmt)
{
    std::string loopLabel = generateLabel();
    std::string breakLabel = generateLabel();
    std::string condLabel = generateLabel();

    // Generate declarations
    if (stmt->decls()) {
        for (size_t i = 0; i < stmt->decls()->size(); ++i) {
            generateDeclaration((*stmt->decls())[i]);
        }
    }

    // Jump to condition check
    emitInstruction("br", "$" + condLabel);

    // Loop body
    emitInstruction("block", "$" + breakLabel);
    emitInstruction("loop", "$" + loopLabel);
    emitInstruction("block", "$" + condLabel);

    // Generate condition
    if (stmt->cond()) {
        generateExpression(stmt->cond());
        emitInstruction("i32.eqz");
        emitInstruction("br_if", "$" + breakLabel);
    }

    // Generate loop body
    if (stmt->stat()) {
        generateStatement(stmt->stat());
    }

    // Generate post-increment
    if (stmt->post()) {
        generateExpression(stmt->post());
        emitInstruction("drop");
    }

    emitInstruction("br", "$" + loopLabel);
    emitInstruction("end"); // end condition block
    emitInstruction("end"); // end loop
    emitInstruction("end"); // end break block
}

void WasmCodeGenerator::generateDoWhileStatement(std::shared_ptr<ASTNodeStatDoWhile> stmt)
{
    std::string loopLabel = generateLabel();
    std::string breakLabel = generateLabel();

    emitInstruction("block", "$" + breakLabel);
    emitInstruction("loop", "$" + loopLabel);

    // Generate loop body first (do-while executes body at least once)
    if (stmt->stat()) {
        generateStatement(stmt->stat());
    }

    // Generate condition
    if (stmt->expr()) {
        generateExpression(stmt->expr());
        emitInstruction("br_if", "$" + loopLabel); // Continue if condition is true
    }

    emitInstruction("end"); // end loop
    emitInstruction("end"); // end break block
}

void WasmCodeGenerator::generateReturnStatement(std::shared_ptr<ASTNodeStatReturn> stmt)
{
    if (stmt->expr()) {
        generateExpression(stmt->expr());
    }
    emitInstruction("return");
}

void WasmCodeGenerator::generateBreakStatement(std::shared_ptr<ASTNodeStatBreak> stmt)
{
    // Would need to track loop labels
    emitInstruction("br", "$break_label");
}

void WasmCodeGenerator::generateContinueStatement(std::shared_ptr<ASTNodeStatContinue> stmt)
{
    // Would need to track loop labels
    emitInstruction("br", "$continue_label");
}

void WasmCodeGenerator::generateDeclaration(std::shared_ptr<ASTNodeDeclaration> decl)
{
    if (auto initDecl = std::dynamic_pointer_cast<ASTNodeInitDeclarator>(decl)) {
        generateInitDeclarator(initDecl);
    }
}

void WasmCodeGenerator::generateInitDeclarator(std::shared_ptr<ASTNodeInitDeclarator> decl)
{
    if (!decl->get_id())
        return;

    std::string varName = decl->get_id()->id;
    auto varType = decl->get_type();

    if (!varType)
        return;

    auto wasmType = mapCTypeToWasm(varType);
    size_t varSize = getTypeSize(varType);

    // Allocate local variable
    allocateLocal(varName, wasmType, varSize);

    // Handle initialization
    auto initializer = decl->get_initializer();
    if (initializer && initializer->is_expr()) {
        generateExpression(initializer->expr());
        emitInstruction("local.set", "$" + varName);
    }
}

void WasmCodeGenerator::generateTypeConversion(WasmType fromType, WasmType toType)
{
    if (fromType == toType)
        return;

    // Integer conversions
    if (fromType == WasmType::I32 && toType == WasmType::I64) {
        emitInstruction("i64.extend_i32_s");
    } else if (fromType == WasmType::I64 && toType == WasmType::I32) {
        emitInstruction("i32.wrap_i64");
    }
    // Float conversions
    else if (fromType == WasmType::F32 && toType == WasmType::F64) {
        emitInstruction("f64.promote_f32");
    } else if (fromType == WasmType::F64 && toType == WasmType::F32) {
        emitInstruction("f32.demote_f64");
    }
    // Integer to float
    else if (fromType == WasmType::I32 && toType == WasmType::F32) {
        emitInstruction("f32.convert_i32_s");
    } else if (fromType == WasmType::I32 && toType == WasmType::F64) {
        emitInstruction("f64.convert_i32_s");
    }
    // Float to integer
    else if (fromType == WasmType::F32 && toType == WasmType::I32) {
        emitInstruction("i32.trunc_f32_s");
    } else if (fromType == WasmType::F64 && toType == WasmType::I32) {
        emitInstruction("i32.trunc_f64_s");
    }
}

void WasmCodeGenerator::generateArithmeticOp(const std::string& op, WasmType type)
{
    switch (type) {
    case WasmType::I32:
        emitInstruction("i32." + op);
        break;
    case WasmType::I64:
        emitInstruction("i64." + op);
        break;
    case WasmType::F32:
        emitInstruction("f32." + op);
        break;
    case WasmType::F64:
        emitInstruction("f64." + op);
        break;
    default:
        break;
    }
}

void WasmCodeGenerator::generateComparisonOp(const std::string& op, WasmType type)
{
    switch (type) {
    case WasmType::I32:
        emitInstruction("i32." + op + "_s"); // signed comparison by default
        break;
    case WasmType::I64:
        emitInstruction("i64." + op + "_s");
        break;
    case WasmType::F32:
        emitInstruction("f32." + op);
        break;
    case WasmType::F64:
        emitInstruction("f64." + op);
        break;
    default:
        break;
    }
}

void WasmCodeGenerator::generateBitwiseOp(const std::string& op, WasmType type)
{
    switch (type) {
    case WasmType::I32:
        emitInstruction("i32." + op);
        break;
    case WasmType::I64:
        emitInstruction("i64." + op);
        break;
    default:
        break;
    }
}

void WasmCodeGenerator::generateLoad(WasmType type, size_t offset)
{
    std::string offsetStr = offset > 0 ? " offset=" + std::to_string(offset) : "";

    switch (type) {
    case WasmType::I32:
        emitInstruction("i32.load" + offsetStr);
        break;
    case WasmType::I64:
        emitInstruction("i64.load" + offsetStr);
        break;
    case WasmType::F32:
        emitInstruction("f32.load" + offsetStr);
        break;
    case WasmType::F64:
        emitInstruction("f64.load" + offsetStr);
        break;
    default:
        break;
    }
}

void WasmCodeGenerator::generateStore(WasmType type, size_t offset)
{
    std::string offsetStr = offset > 0 ? " offset=" + std::to_string(offset) : "";

    switch (type) {
    case WasmType::I32:
        emitInstruction("i32.store" + offsetStr);
        break;
    case WasmType::I64:
        emitInstruction("i64.store" + offsetStr);
        break;
    case WasmType::F32:
        emitInstruction("f32.store" + offsetStr);
        break;
    case WasmType::F64:
        emitInstruction("f64.store" + offsetStr);
        break;
    default:
        break;
    }
}

void WasmCodeGenerator::generateMemoryAddress(std::shared_ptr<ASTNodeExpr> expr)
{
    if (auto identExpr = std::dynamic_pointer_cast<ASTNodeExprIdentifier>(expr)) {
        std::string name = identExpr->id()->id;

        // Check if it's a local variable - can't get address of locals in WASM
        auto local = findLocal(name);
        if (local) {
            reportError("Cannot take address of local variable in WebAssembly");
            return;
        }

        // Global variable
        auto globalIt = memoryLayout.globalOffsets.find(name);
        if (globalIt != memoryLayout.globalOffsets.end()) {
            emitInstruction("i32.const", std::to_string(globalIt->second));
        }
    } else if (auto indexExpr = std::dynamic_pointer_cast<ASTNodeExprIndexing>(expr)) {
        generateExpression(indexExpr->array());
        generateExpression(indexExpr->idx());

        auto elemType = indexExpr->type();
        size_t elemSize = getTypeSize(elemType);

        if (elemSize > 1) {
            emitInstruction("i32.const", std::to_string(elemSize));
            emitInstruction("i32.mul");
        }

        emitInstruction("i32.add");
    } else {
        // For other expressions, generate the expression itself
        generateExpression(expr);
    }
}

std::string
WasmCodeGenerator::generateModule(std::shared_ptr<ASTNodeTranslationUnit> translationUnit)
{
    if (!translationUnit)
        return "";

    reset();

    // Process all external declarations
    for (size_t i = 0; i < translationUnit->size(); ++i) {
        auto extDecl = (*translationUnit)[i];

        if (auto funcDef = std::dynamic_pointer_cast<ASTNodeFunctionDefinition>(extDecl)) {
            generateFunctionDefinition(funcDef);
        } else if (auto declList = std::dynamic_pointer_cast<ASTNodeDeclarationList>(extDecl)) {
            for (size_t j = 0; j < declList->size(); ++j) {
                generateDeclaration((*declList)[j]);
            }
        }
    }

    return getWasmModule();
}

void WasmCodeGenerator::generateFunctionDefinition(
    std::shared_ptr<ASTNodeFunctionDefinition> funcDef)
{
    if (!funcDef->get_id())
        return;

    std::string funcName = funcDef->get_id()->id;
    auto funcType = funcDef->decl();

    if (!funcType)
        return;

    // Create new function
    WasmFunction wasmFunc(funcName);

    // Map parameters
    auto params = funcType->parameter_declaration_list();
    if (params) {
        for (size_t i = 0; i < params->size(); ++i) {
            auto param = (*params)[i];
            if (param->get_type()) {
                auto wasmType = mapCTypeToWasm(param->get_type());
                wasmFunc.params.push_back(wasmType);

                // Store parameter name
                if (param->get_id()) {
                    wasmFunc.paramNames.push_back(param->get_id()->id);
                } else {
                    wasmFunc.paramNames.push_back("param" + std::to_string(i));
                }
            }
        }
    }

    // Map return type
    if (funcType->return_type()) {
        wasmFunc.returnType = mapCTypeToWasm(funcType->return_type());
    }

    // Set current function and clear locals
    functions.push_back(wasmFunc);
    currentFunction = &functions.back();
    currentLocals.clear();

    // Parameters are automatically available as locals in WASM
    // Add parameter names to local tracking for identifier resolution
    if (params) {
        for (size_t i = 0; i < params->size(); ++i) {
            auto param = (*params)[i];
            if (param->get_id() && param->get_type()) {
                std::string paramName = param->get_id()->id;
                auto wasmType = mapCTypeToWasm(param->get_type());
                // Parameters get indices 0, 1, 2... so use index i directly
                LocalVariable paramLocal(paramName, wasmType, i, 0);
                currentLocals[paramName] = paramLocal;
            }
        }
        // Set localCount to start after parameters
        currentFunction->localCount = params->size();
    }

    // Generate function body
    if (funcDef->body()) {
        generateCompoundStatement(funcDef->body());
    }

    // Add implicit return for void functions
    if (wasmFunc.returnType == WasmType::VOID) {
        emitInstruction("return");
    }

    currentFunction = nullptr;
}

std::string WasmCodeGenerator::getWasmModule() const
{
    std::ostringstream oss;

    oss << "(module\n";

    // Memory declaration
    oss << "  (memory " << (memorySize / 65536) << ")\n";
    oss << "  (export \"memory\" (memory 0))\n\n";

    // Data segments
    for (const auto& segment : dataSegments) {
        oss << segment << "\n";
    }
    if (!dataSegments.empty()) {
        oss << "\n";
    }

    // Functions
    for (const auto& func : functions) {
        oss << func.toString() << "\n\n";

        // Export main function if it exists
        if (func.name == "main") {
            oss << "  (export \"main\" (func $main))\n\n";
        }
    }

    oss << ")";

    return oss.str();
}

void WasmCodeGenerator::reportError(const std::string& message)
{
    throw std::runtime_error("WASM Code Generation Error: " + message);
}

} // namespace cparser
