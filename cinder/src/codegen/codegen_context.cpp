#include "cinder/codegen/codegen_context.hpp"

#include <cstddef>
#include <functional>
#include <unordered_map>

#include "cinder/ast/types.hpp"
#include "cinder/frontend/tokens.hpp"
#include "cinder/support/utils.hpp"
#include "llvm/ADT/Twine.h"
#include "llvm/CodeGen/MachineMemOperand.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/PassManager.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Value.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Linker/Linker.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/StandardInstrumentations.h"
#include "llvm/Support/CodeGen.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/TargetParser/Triple.h"
#include "llvm/Transforms/InstCombine/InstCombine.h"
#include "llvm/Transforms/Scalar/GVN.h"
#include "llvm/Transforms/Scalar/Reassociate.h"
#include "llvm/Transforms/Scalar/SimplifyCFG.h"

using namespace llvm;
using namespace cinder;

CodegenContext::CodegenContext(const std::string& module_name)
    : llvm_ctx_(std::make_unique<LLVMContext>()),
      module_(std::make_unique<Module>(module_name, *llvm_ctx_)),
      builder_(std::make_unique<IRBuilder<>>(*llvm_ctx_)),
      const_int(ConstantInt::get(*llvm_ctx_, APInt(32, 1))),
      const_flt(ConstantFP::get(*llvm_ctx_, APFloat(1.0f))),
      debug_info_(*llvm_ctx_, *builder_) {
  TheFPM_ = std::make_unique<FunctionPassManager>();
  TheLAM_ = std::make_unique<LoopAnalysisManager>();
  TheFAM_ = std::make_unique<FunctionAnalysisManager>();
  TheCGAM_ = std::make_unique<CGSCCAnalysisManager>();
  TheMAM_ = std::make_unique<ModuleAnalysisManager>();
  ThePIC_ = std::make_unique<PassInstrumentationCallbacks>();

  TheSI_ = std::make_unique<StandardInstrumentations>(*llvm_ctx_, true);
  TheSI_->registerCallbacks(*ThePIC_, TheMAM_.get());

  TheFPM_->addPass(InstCombinePass());
  TheFPM_->addPass(ReassociatePass());
  TheFPM_->addPass(GVNPass());
  TheFPM_->addPass(SimplifyCFGPass());

  PassBuilder PB;
  PB.registerModuleAnalyses(*TheMAM_);
  PB.registerFunctionAnalyses(*TheFAM_);
  PB.crossRegisterProxies(*TheLAM_, *TheFAM_, *TheCGAM_, *TheMAM_);
}

DebugInfoContext& CodegenContext::DebugInfo() {
  return debug_info_;
}

LLVMContext& CodegenContext::GetContext() {
  return *llvm_ctx_;
}

Module& CodegenContext::GetModule() {
  return *module_;
}

IRBuilder<>& CodegenContext::GetBuilder() {
  return *builder_;
}

Type* CodegenContext::CreateTypeFromToken(Token& tok) {
  switch (tok.kind) {
    case Token::Type::INT32_SPECIFIER:
      return Type::getInt32Ty(*llvm_ctx_);
    case Token::Type::FLT32_SPECIFIER:
      return Type::getFloatTy(*llvm_ctx_);
    case Token::Type::FLT64_SPECIFIER:
      return Type::getDoubleTy(*llvm_ctx_);
    case Token::Type::BOOL_SPECIFIER:
      return Type::getInt1Ty(*llvm_ctx_);
    case Token::Type::STR_SPECIFIER:
      return PointerType::getInt8Ty(*llvm_ctx_);
    case Token::Type::VOID_SPECIFIER:
      return Type::getVoidTy(*llvm_ctx_);
    default:
      return nullptr;
  }
}

AllocaInst* CodegenContext::CreateAlloca(Type* ty, Value* array_size,
                                         const Twine& name) {
  return builder_->CreateAlloca(ty, array_size, name);
}

StoreInst* CodegenContext::CreateStore(Value* value, Value* ptr,
                                       bool is_volatile) {
  return builder_->CreateStore(value, ptr, is_volatile);
}

Value* CodegenContext::CreateIntCmp(Token::Type ty, Value* left, Value* right) {
  switch (ty) {
    case Token::Type::BANGEQ:
      return builder_->CreateCmp(CmpInst::ICMP_NE, left, right, "cmptmp");
    case Token::Type::EQEQ:
      return builder_->CreateCmp(CmpInst::ICMP_EQ, left, right, "cmptmp");
    case Token::Type::LESSER:
      return builder_->CreateCmp(CmpInst::ICMP_SLT, left, right, "cmptmp");
    case Token::Type::LESSER_EQ:
      return builder_->CreateCmp(CmpInst::ICMP_SLE, left, right, "cmptmp");
    case Token::Type::GREATER:
      return builder_->CreateCmp(CmpInst::ICMP_SGT, left, right, "cmptmp");
    case Token::Type::GREATER_EQ:
      return builder_->CreateCmp(CmpInst::ICMP_SGE, left, right, "cmptmp");
    default:
      UNREACHABLE(CodegenContext, CreateIntCmp);
      return nullptr;
  }
}

Value* CodegenContext::CreateFltCmp(Token::Type ty, Value* left, Value* right) {
  switch (ty) {
    case Token::Type::BANGEQ:
      return builder_->CreateCmp(CmpInst::FCMP_ONE, left, right, "cmptmp");
    case Token::Type::EQEQ:
      return builder_->CreateCmp(CmpInst::FCMP_OEQ, left, right, "cmptmp");
    case Token::Type::LESSER:
      return builder_->CreateCmp(CmpInst::FCMP_OLT, left, right, "cmptmp");
    case Token::Type::LESSER_EQ:
      return builder_->CreateCmp(CmpInst::FCMP_OLE, left, right, "cmptmp");
    case Token::Type::GREATER:
      return builder_->CreateCmp(CmpInst::FCMP_OGT, left, right, "cmptmp");
    case Token::Type::GREATER_EQ:
      return builder_->CreateCmp(CmpInst::FCMP_OGE, left, right, "cmptmp");
    default:
      UNREACHABLE(CodegenContext, CreateFltCmp);
      return nullptr;
  }
}

Value* CodegenContext::CreateIntBinop(Token::Type op, Value* l, Value* r) {
  switch (op) {
    case Token::Type::Plus:
      return builder_->CreateAdd(l, r, "addtmp");
    case Token::Type::Minus:
      return builder_->CreateSub(l, r, "subtmp");
    case Token::Type::STAR:
      return builder_->CreateMul(l, r, "multmp");
    case Token::Type::SLASH:
      return builder_->CreateSDiv(l, r, "divtmp");
    default:
      return nullptr;
  }
}

Value* CodegenContext::CreateFltBinop(Token::Type op, Value* l, Value* r) {
  switch (op) {
    case Token::Type::Plus:
      return builder_->CreateFAdd(l, r, "addtmp");
    case Token::Type::Minus:
      return builder_->CreateFSub(l, r, "subtmp");
    case Token::Type::STAR:
      return builder_->CreateFMul(l, r, "multmp");
    case Token::Type::SLASH:
      return builder_->CreateFDiv(l, r, "divtmp");
    default:
      return nullptr;
  }
}

Value* CodegenContext::CreateLoad(Type* ty, Value* val, const Twine& name) {
  return builder_->CreateLoad(ty, val, name);
}

Value* CodegenContext::CreatePreOp(types::Type* ty, Token::Type op, Value* val,
                                   AllocaInst* alloca) {
  const bool isFloat = (ty->kind == types::TypeKind::Float);
  const bool isInt = (ty->kind == types::TypeKind::Int);
  if (!isFloat && !isInt) UNREACHABLE(CodegenContext, CreatePreOp);

  Value* one = isFloat ? const_flt : const_int;

  auto add = [&](Value* a, Value* b) {
    return isFloat ? builder_->CreateFAdd(a, b, "inc")
                   : builder_->CreateAdd(a, b, "inc");
  };
  auto sub = [&](Value* a, Value* b) {
    return isFloat ? builder_->CreateFSub(a, b, "dec")
                   : builder_->CreateSub(a, b, "dec");
  };

  Value* var = nullptr;
  switch (op) {
    case Token::Type::PlusPlus:
      var = add(val, one);
      break;
    case Token::Type::MinusMinus:
      var = sub(val, one);
      break;
    default:
      UNREACHABLE(CodegenContext, CreatePreOp);
  }

  builder_->CreateStore(var, alloca);
  return var;
}

BasicBlock* CodegenContext::GetInsertBlock() {
  return builder_->GetInsertBlock();
}

Function* CodegenContext::GetInsertBlockParent() {
  return builder_->GetInsertBlock()->getParent();
}

Instruction* CodegenContext::GetInsertBlockTerminator() {
  return builder_->GetInsertBlock()->getTerminator();
}

BasicBlock* CodegenContext::CreateBasicBlock(const Twine name, Function* parent,
                                             BasicBlock* before) {
  return BasicBlock::Create(*llvm_ctx_, name, parent, before);
}

BranchInst* CodegenContext::CreateBr(BasicBlock* destination) {
  return builder_->CreateBr(destination);
}

BranchInst* CodegenContext::CreateBasicCondBr(Value* val,
                                              BasicBlock* true_block,
                                              BasicBlock* false_block) {
  return builder_->CreateCondBr(val, true_block, false_block);
}

FunctionType* CodegenContext::GetFuncType(Type* res, ArrayRef<Type*> params,
                                          bool is_variadic) {
  return FunctionType::get(res, params, is_variadic);
}

Function* CodegenContext::CreatePublicFunc(FunctionType* type,
                                           const Twine& name) {
  return Function::Create(type, Function::ExternalLinkage, name, *module_);
}

void CodegenContext::SetInsertPoint(BasicBlock* block) {
  builder_->SetInsertPoint(block);
}

ReturnInst* CodegenContext::CreateVoidReturn() {
  return builder_->CreateRetVoid();
}

llvm::ReturnInst* CodegenContext::CreateReturn(llvm::Value* val) {
  return builder_->CreateRet(val);
}

llvm::CallInst* CodegenContext::CreateVoidCall(
    llvm::FunctionCallee callee, llvm::ArrayRef<llvm::Value*> args) {
  return builder_->CreateCall(callee, args);
}

llvm::CallInst* CodegenContext::CreateCall(llvm::FunctionCallee callee,
                                           llvm::ArrayRef<llvm::Value*> args,
                                           const llvm::Twine name) {
  return builder_->CreateCall(callee, args, name);
}

void CodegenContext::SetTargetTriple(llvm::Triple trip) {
  module_->setTargetTriple(trip);
}

const llvm::Target* CodegenContext::LookupTarget() {
  std::string err;
  return TargetRegistry::lookupTarget(module_->getTargetTriple(), err);
}

TargetMachine* CodegenContext::CreateTargetMachine(const Target* target,
                                                   const std::string& trip) {
  return target->createTargetMachine(Triple(trip), "generic", "", {},
                                     Reloc::PIC_);
}

void CodegenContext::SetModDataLayout(TargetMachine* tm) {
  module_->setDataLayout(tm->createDataLayout());
}
