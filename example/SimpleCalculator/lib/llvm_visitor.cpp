#include "scalc/llvm_visitor.h"
#include "scalc/scalc_error.h"
#include "llvm/Support/raw_ostream.h"
#include <llvm/ADT/APFloat.h>
#include <llvm/ADT/STLExtras.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Verifier.h>

ASTNodeVisitorLLVMGen::ASTNodeVisitorLLVMGen(const std::string& filename)
    : m_context(std::make_unique<llvm::LLVMContext>()),
      m_module(std::make_unique<llvm::Module>(filename, *m_context)),
      m_builder(std::make_unique<llvm::IRBuilder<>>(*m_context)),
      m_namedValues(1),
      m_prevValue(nullptr),
      m_globalscope(true)
{
    { // snprintf
        std::vector<llvm::Type*> argstype;
        // TODO llvm::Type::getInt8PtrTy is deprecated
        argstype.push_back(llvm::Type::getInt8Ty(*m_context));
        argstype.push_back(llvm::Type::getInt64Ty(*m_context));
        argstype.push_back(llvm::Type::getInt8Ty(*m_context));
        llvm::FunctionType* ft =
            llvm::FunctionType::get(llvm::Type::getDoubleTy(*m_context), argstype, true);
        llvm::Function* f = llvm::Function::Create(
            ft, llvm::Function::LinkageTypes::ExternalLinkage, "snprintf", m_module.get());
        f->addFnAttr(llvm::Attribute::AttrKind::NoUnwind);
    }

    { // puts
        std::vector<llvm::Type*> argstype;
        argstype.push_back(llvm::Type::getInt8Ty(*m_context));
        llvm::FunctionType* ft =
            llvm::FunctionType::get(llvm::Type::getDoubleTy(*m_context), argstype, false);
        llvm::Function* f = llvm::Function::Create(
            ft, llvm::Function::LinkageTypes::ExternalLinkage, "puts", m_module.get());
    }

    { // double print(double)
        std::vector<llvm::Type*> argstype(1, llvm::Type::getDoubleTy(*m_context));
        ;
        llvm::FunctionType* ft =
            llvm::FunctionType::get(llvm::Type::getDoubleTy(*m_context), argstype, false);
        llvm::Function* f = llvm::Function::Create(
            ft, llvm::Function::LinkageTypes::ExternalLinkage, "print", m_module.get());
        llvm::BasicBlock* gblock = llvm::BasicBlock::Create(*m_context, "main_entry", f);
        m_builder->SetInsertPoint(gblock);
        llvm::Value* val = f->getArg(0);
        val->setName("value");
        auto buf = m_builder->CreateAlloca(
            llvm::ArrayType::get(m_builder->getInt8Ty(), 50), nullptr, "buf");
        llvm::Value* zeroIndex = m_builder->getInt64(0);
        llvm::Value* charArrayPtr = m_builder->CreateInBoundsGEP(
            buf->getAllocatedType(), buf, {zeroIndex, zeroIndex}, "buf_ptr");
        llvm::Value* format_str = m_builder->CreateGlobalString("%f", "format_str");
        auto snprintf = m_module->getFunction("snprintf");
        m_builder->CreateCall(snprintf, {charArrayPtr, m_builder->getInt64(50), format_str, val});
        auto puts = m_module->getFunction("puts");
        m_builder->CreateCall(puts, {charArrayPtr});
        m_builder->CreateRet(val);
    }
}

void ASTNodeVisitorLLVMGen::visitExpr(const ASTNodeExpr& expr)
{
    if (dynamic_cast<const IDExpr*>(&expr)) {
        auto id = dynamic_cast<const IDExpr*>(&expr)->id();
        auto v = this->lookup_var(id);
        if (!v) {
            throw SCalcError(id + " is not declared");
        }
        m_prevValue = m_builder->CreateLoad(llvm::Type::getDoubleTy(*m_context), v, id);
    } else if (dynamic_cast<const BinaryOperatorExpr*>(&expr)) {
        auto& e = dynamic_cast<const BinaryOperatorExpr&>(expr);
        e.right().accept(*this);
        auto right = m_prevValue;
        auto left = right;
        if (e.op() != BinaryOperatorExpr::ASSIGNMENT) {
            e.left().accept(*this);
            left = m_prevValue;
        }
        switch (e.op()) {
        case BinaryOperatorExpr::PLUS:
            m_prevValue = m_builder->CreateFAdd(left, right, "fplus_r");
            break;
        case BinaryOperatorExpr::MINUS:
            m_prevValue = m_builder->CreateFSub(left, right, "fminus_r");
            break;
        case BinaryOperatorExpr::MULTIPLY:
            m_prevValue = m_builder->CreateFMul(left, right, "fmul_r");
            break;
        case BinaryOperatorExpr::DIVISION:
            m_prevValue = m_builder->CreateFDiv(left, right, "fdiv_r");
            break;
        case BinaryOperatorExpr::REMAINDER:
            m_prevValue = m_builder->CreateFRem(left, right, "frem_r");
            break;
        case BinaryOperatorExpr::CARET: {
            auto func = m_module->getFunction("pow");
            assert(func);
            m_prevValue = m_builder->CreateCall(func, {left, right}, "fpow_r");
        } break;
        case BinaryOperatorExpr::ASSIGNMENT: {
            auto id = dynamic_cast<const IDExpr*>(&e.left());
            if (!id)
                throw SCalcError("expected an id");
            auto v = this->lookup_var(id->id());
            if (!v) {
                if (m_globalscope) {
                    auto gv = new llvm::GlobalVariable(llvm::Type::getDoubleTy(*m_context),
                                                       false,
                                                       llvm::GlobalVariable::PrivateLinkage,
                                                       nullptr,
                                                       id->id());
                    m_module->insertGlobalVariable(gv);
                    m_namedValues.back().insert({id->id(), gv});
                    v = gv;
                } else {
                    auto addr = m_builder->CreateAlloca(
                        llvm::Type::getDoubleTy(*m_context), nullptr, "local");
                    m_namedValues.back().insert({id->id(), addr});
                    v = addr;
                }
            }
            m_builder->CreateStore(right, v);
            m_prevValue = m_builder->CreateLoad(right->getType(), v, "assign_r");
        } break;
        case BinaryOperatorExpr::EQUAL:
            m_prevValue = m_builder->CreateFCmpOEQ(left, right, "cmp");
            m_prevValue = m_builder->CreateCast(llvm::Instruction::CastOps::UIToFP,
                                                m_prevValue,
                                                llvm::Type::getDoubleTy(*m_context),
                                                "cast2fp_r");
            break;
        case BinaryOperatorExpr::NOTEQUAL:
            m_prevValue = m_builder->CreateFCmpONE(left, right, "cmp");
            m_prevValue = m_builder->CreateCast(llvm::Instruction::CastOps::UIToFP,
                                                m_prevValue,
                                                llvm::Type::getDoubleTy(*m_context),
                                                "cast2fp_r");
            break;
        case BinaryOperatorExpr::GREATERTHAN:
            m_prevValue = m_builder->CreateFCmpOGT(left, right, "cmp");
            m_prevValue = m_builder->CreateCast(llvm::Instruction::CastOps::UIToFP,
                                                m_prevValue,
                                                llvm::Type::getDoubleTy(*m_context),
                                                "cast2fp_r");
            break;
        case BinaryOperatorExpr::LESSTHAN:
            m_prevValue = m_builder->CreateFCmpOLT(left, right, "cmp");
            m_prevValue = m_builder->CreateCast(llvm::Instruction::CastOps::UIToFP,
                                                m_prevValue,
                                                llvm::Type::getDoubleTy(*m_context),
                                                "cast2fp_r");
            break;
        case BinaryOperatorExpr::GREATEREQUAL:
            m_prevValue = m_builder->CreateFCmpOGE(left, right, "cmp");
            m_prevValue = m_builder->CreateCast(llvm::Instruction::CastOps::UIToFP,
                                                m_prevValue,
                                                llvm::Type::getDoubleTy(*m_context),
                                                "cast2fp_r");
            break;
        case BinaryOperatorExpr::LESSEQUAL:
            m_prevValue = m_builder->CreateFCmpOLE(left, right, "cmp");
            m_prevValue = m_builder->CreateCast(llvm::Instruction::CastOps::UIToFP,
                                                m_prevValue,
                                                llvm::Type::getDoubleTy(*m_context),
                                                "cast2fp_r");
            break;
        }
    } else if (dynamic_cast<const UnaryOperatorExpr*>(&expr)) {
        auto& e = dynamic_cast<const UnaryOperatorExpr&>(expr);
        auto cn = dynamic_cast<const IDExpr*>(&e.cc());
        if (!cn)
            throw SCalcError("require an lvalue");
        auto addr = this->lookup_var(cn->id());
        if (!addr)
            throw SCalcError("variable '" + cn->id() + "' is not defined");
        auto oldvalue = m_builder->CreateLoad(llvm::Type::getDoubleTy(*m_context), addr, cn->id());
        switch (e.op()) {
        case UnaryOperatorExpr::PRE_INC: {
            auto v = m_builder->CreateFAdd(
                oldvalue, llvm::ConstantFP::get(*m_context, llvm::APFloat(1.0)), "inc1");
            m_builder->CreateStore(v, addr);
            m_prevValue = v;
        } break;
        case UnaryOperatorExpr::PRE_DEC: {
            auto v = m_builder->CreateFAdd(
                oldvalue, llvm::ConstantFP::get(*m_context, llvm::APFloat(-1.0)), "dec1");
            m_builder->CreateStore(v, addr);
            m_prevValue = v;
        } break;
        case UnaryOperatorExpr::POS_INC: {
            auto v = m_builder->CreateFAdd(
                oldvalue, llvm::ConstantFP::get(*m_context, llvm::APFloat(1.0)), "inc1x");
            m_builder->CreateStore(v, addr);
            m_prevValue = oldvalue;
        } break;
        case UnaryOperatorExpr::POS_DEC: {
            auto v = m_builder->CreateFAdd(
                oldvalue, llvm::ConstantFP::get(*m_context, llvm::APFloat(-1.0)), "dec1x");
            m_builder->CreateStore(v, addr);
            m_prevValue = oldvalue;
        } break;
        }
    } else if (dynamic_cast<const FunctionCallExpr*>(&expr)) {
        auto& e = dynamic_cast<const FunctionCallExpr&>(expr);
        auto id = dynamic_cast<IDExpr*>(e.function().get());
        if (!id)
            throw SCalcError("invalid function call");
        const auto funcname = id->id();
        llvm::Function* f = m_module->getFunction(funcname);
        if (!f)
            throw SCalcError("undefined function '" + funcname + "'");
        if (f->arg_size() != e.parameters()->size())
            throw SCalcError("incorrect number of parameters");
        std::vector<llvm::Value*> args;
        for (auto& a : *e.parameters()) {
            a->accept(*this);
            args.push_back(m_prevValue);
        }
        m_prevValue = m_builder->CreateCall(f, args, funcname + "_result");
    } else if (dynamic_cast<const NumberExpr*>(&expr)) {
        auto& e = dynamic_cast<const NumberExpr&>(expr);
        m_prevValue = llvm::ConstantFP::get(*m_context, llvm::APFloat(e.value()));
    }
}

void ASTNodeVisitorLLVMGen::visitExprStat(const ASTNodeExprList& exprlist)
{
    m_prevValue = nullptr;
    for (auto& e : exprlist) {
        e->accept(*this);
        m_prevValue = nullptr;
    }
}

void ASTNodeVisitorLLVMGen::visitStatList(const std::vector<std::shared_ptr<ASTNodeStat>>& statlist)
{
    for (auto& stat : statlist)
        stat->accept(*this);
}

void ASTNodeVisitorLLVMGen::visitForStat(const ASTNodeExprList* pre,
                                         const ASTNodeExpr* cond,
                                         const ASTNodeExprList* post,
                                         const ASTNodeStat& stat)
{
    if (pre) {
        for (auto& e : *pre)
            e->accept(*this);
    }
    auto func = m_builder->GetInsertBlock()->getParent();
    llvm::BasicBlock* after_b = llvm::BasicBlock::Create(*m_context, "loop_after", func);
    llvm::BasicBlock* loop_b = llvm::BasicBlock::Create(*m_context, "loop", func, after_b);
    llvm::Value* cv = nullptr;
    if (cond) {
        cond->accept(*this);
        cv = m_builder->CreateFCmpONE(
            m_prevValue, llvm::ConstantFP::get(*m_context, llvm::APFloat(0.0)), "forcond");
    } else {
        cv = llvm::ConstantInt::getTrue(*m_context);
    }
    m_builder->CreateCondBr(cv, loop_b, after_b);
    m_builder->SetInsertPoint(loop_b);
    stat.accept(*this);
    if (post) {
        for (auto& e : *post)
            e->accept(*this);
    }
    if (cond) {
        cond->accept(*this);
        cv = m_builder->CreateFCmpONE(
            m_prevValue, llvm::ConstantFP::get(*m_context, llvm::APFloat(0.0)), "forcond");
    } else {
        cv = llvm::ConstantInt::getTrue(*m_context);
    }
    m_builder->CreateCondBr(cv, loop_b, after_b);
    m_builder->SetInsertPoint(after_b);
}

void ASTNodeVisitorLLVMGen::visitIfStat(const ASTNodeExpr& cond,
                                        const ASTNodeStat& then,
                                        const ASTNodeStat* else_)
{
    cond.accept(*this);
    auto cmp = m_builder->CreateFCmpONE(
        m_prevValue, llvm::ConstantFP::get(*m_context, llvm::APFloat(0.0)), "ifcond");
    auto func = m_builder->GetInsertBlock()->getParent();
    llvm::BasicBlock* then_b = llvm::BasicBlock::Create(*m_context, "then", func);
    llvm::BasicBlock* else_b = llvm::BasicBlock::Create(*m_context, "else", func);
    m_builder->CreateCondBr(cmp, then_b, else_b);
    m_builder->SetInsertPoint(then_b);
    then.accept(*this);
    m_builder->SetInsertPoint(else_b);
    if (else_) {
        else_->accept(*this);
    }
    if (!then_b->getTerminator() || !else_b->getTerminator()) {
        llvm::BasicBlock* ifcontinue_b = llvm::BasicBlock::Create(*m_context, "ifcontinue", func);
        if (!then_b->getTerminator()) {
            m_builder->SetInsertPoint(then_b);
            m_builder->CreateBr(ifcontinue_b);
        }
        if (!else_b->getTerminator()) {
            m_builder->SetInsertPoint(else_b);
            m_builder->CreateBr(ifcontinue_b);
        }
        m_builder->SetInsertPoint(ifcontinue_b);
        m_builder->CreateAnd(cmp, cmp, "damp");
    }
}

void ASTNodeVisitorLLVMGen::visitBlockStat(const ASTNodeStatList& stat)
{
    m_namedValues.emplace_back();
    for (auto& s : stat) {
        s->accept(*this);
    }
    m_namedValues.pop_back();
}

void ASTNodeVisitorLLVMGen::visitReturnStat(const ASTNodeExpr& expr)
{
    expr.accept(*this);
    m_builder->CreateRet(m_prevValue);
}

static llvm::AllocaInst* CreateEntryBlockAlloca(llvm::Function* TheFunction,
                                                llvm::StringRef VarName,
                                                llvm::LLVMContext& context)
{
    llvm::IRBuilder<> TmpB(&TheFunction->getEntryBlock(), TheFunction->getEntryBlock().begin());
    return TmpB.CreateAlloca(llvm::Type::getDoubleTy(context), nullptr, VarName);
}

void ASTNodeVisitorLLVMGen::visitFuncDef(const std::string& funcname,
                                         const std::vector<std::string>& args,
                                         const ASTNodeBlockStat& block)
{
    std::vector<llvm::Type*> argstype(args.size(), llvm::Type::getDoubleTy(*m_context));
    llvm::FunctionType* ft =
        llvm::FunctionType::get(llvm::Type::getDoubleTy(*m_context), argstype, false);
    llvm::Function* f = llvm::Function::Create(
        ft, llvm::Function::LinkageTypes::InternalLinkage, funcname, m_module.get());
    size_t i = 0;
    m_namedValues.emplace_back();
    llvm::BasicBlock* fblock = llvm::BasicBlock::Create(*m_context, "entry", f);
    m_builder->SetInsertPoint(fblock);
    for (auto& a : f->args()) {
        auto alloc = CreateEntryBlockAlloca(f, args.at(i), *m_context);
        m_builder->CreateStore(&a, alloc);
        m_namedValues.back()[args.at(i)] = alloc;
        i++;
    }
    block.accept(*this);
    llvm::verifyFunction(*f);
    m_namedValues.pop_back();
}

void ASTNodeVisitorLLVMGen::visitFuncDecl(const std::string& funcname,
                                          const std::vector<std::string>& args)
{
    std::vector<llvm::Type*> argstype(args.size(), llvm::Type::getDoubleTy(*m_context));
    llvm::FunctionType* ft =
        llvm::FunctionType::get(llvm::Type::getDoubleTy(*m_context), argstype, false);
    llvm::Function* f = llvm::Function::Create(
        ft, llvm::Function::LinkageTypes::ExternalLinkage, funcname, m_module.get());
}

void ASTNodeVisitorLLVMGen::visitCalcUnit(const std::vector<UnitItem>& items)
{
    std::vector<llvm::Type*> argstype;
    llvm::FunctionType* ft =
        llvm::FunctionType::get(llvm::Type::getInt32Ty(*m_context), argstype, false);
    llvm::Function* f = llvm::Function::Create(
        ft, llvm::Function::LinkageTypes::ExternalLinkage, "main", m_module.get());
    llvm::BasicBlock* gblock = llvm::BasicBlock::Create(*m_context, "main_entry", f);
    for (auto& item : items) {
        if (std::holds_alternative<std::shared_ptr<ASTNodeFunctionDef>>(item)) {
            auto& funcdef = std::get<std::shared_ptr<ASTNodeFunctionDef>>(item);
            m_globalscope = false;
            m_builder->SetInsertPoint(static_cast<llvm::BasicBlock*>(nullptr));
            funcdef->accept(*this);
        } else if (std::holds_alternative<std::shared_ptr<ASTNodeFunctionDecl>>(item)) {
            auto& funcdecl = std::get<std::shared_ptr<ASTNodeFunctionDecl>>(item);
            funcdecl->accept(*this);
        } else if (std::holds_alternative<std::shared_ptr<ASTNodeStat>>(item)) {
            auto& stat = std::get<std::shared_ptr<ASTNodeStat>>(item);
            m_builder->SetInsertPoint(gblock);
            m_globalscope = true;
            stat->accept(*this);
        } else {
            assert(false);
        }
    }
    m_builder->SetInsertPoint(gblock);
    m_builder->CreateRet(llvm::ConstantInt::get(llvm::Type::getInt32Ty(*m_context), 0));
    llvm::verifyFunction(*f);
}

std::string ASTNodeVisitorLLVMGen::codegen() const
{
    std::string buf;
    llvm::raw_string_ostream ss(buf);
    m_module->print(ss, nullptr);
    return buf;
}
