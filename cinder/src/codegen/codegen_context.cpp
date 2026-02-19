#include "cinder/codegen/codegen_context.hpp"

#include <cstddef>
#include <functional>
#include <unordered_map>

#include "cinder/ast/types.hpp"
#include "cinder/frontend/tokens.hpp"
#include "cinder/support/utils.hpp"
#include "llvm/ADT/Twine.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/DIBuilder.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/PassManager.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Value.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Linker/Linker.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/StandardInstrumentations.h"
#include "llvm/Target/TargetMachine.h"
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
      const_flt(ConstantFP::get(*llvm_ctx_, APFloat(1.0f))) {
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

LLVMContext& CodegenContext::GetContext() {
  return *llvm_ctx_;
}

Module& CodegenContext::GetModule() {
  return *module_;
}

IRBuilder<>& CodegenContext::GetBuilder() {
  return *builder_;
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
  CmpInst::Predicate p;
  switch (ty) {
    case Token::Type::BANGEQ:
      p = CmpInst::ICMP_NE;
      break;
    case Token::Type::EQEQ:
      p = CmpInst::ICMP_EQ;
      break;
    case Token::Type::LESSER:
      p = CmpInst::ICMP_SLT;
      break;
    case Token::Type::LESSER_EQ:
      p = CmpInst::ICMP_SLE;
      break;
    case Token::Type::GREATER:
      p = CmpInst::ICMP_SGT;
      break;
    case Token::Type::GREATER_EQ:
      p = CmpInst::ICMP_SGE;
      break;
    default:
      UNREACHABLE(CodegenContext, CreateIntCmp);
  }
  return builder_->CreateCmp(p, left, right, "cmptmp");
}

Value* CodegenContext::CreateFltCmp(Token::Type ty, Value* left, Value* right) {
  CmpInst::Predicate p;
  switch (ty) {
    case Token::Type::BANGEQ:
      p = CmpInst::FCMP_ONE;
      break;
    case Token::Type::EQEQ:
      p = CmpInst::FCMP_OEQ;
      break;
    case Token::Type::LESSER:
      p = CmpInst::FCMP_OLT;
      break;
    case Token::Type::LESSER_EQ:
      p = CmpInst::FCMP_OLE;
      break;
    case Token::Type::GREATER:
      p = CmpInst::FCMP_OGT;
      break;
    case Token::Type::GREATER_EQ:
      p = CmpInst::FCMP_OGE;
      break;
    default:
      UNREACHABLE(CodegenContext, CreateFltCmp);
  }
  return builder_->CreateCmp(p, left, right, "cmptmp");
}

Value* CodegenContext::CreateIntBinop(Token::Type ty, Value* left,
                                      Value* right) {
  using Builder = IRBuilder<>;
  using OpFn = std::function<Value*(Builder&, Value*, Value*)>;

  static const std::unordered_map<Token::Type, OpFn, TokenTypeHash> table = {
      {Token::Type::Plus, [](Builder& b, Value* l,
                             Value* r) { return b.CreateAdd(l, r, "addtmp"); }},
      {Token::Type::Minus,
       [](Builder& b, Value* l, Value* r) {
         return b.CreateSub(l, r, "subtmp");
       }},
      {Token::Type::STAR, [](Builder& b, Value* l,
                             Value* r) { return b.CreateMul(l, r, "multmp"); }},
      {Token::Type::SLASH,
       [](Builder& b, Value* l, Value* r) {
         return b.CreateSDiv(l, r, "divtmp");
       }},
  };

  auto fn = table.find(ty);
  if (fn == table.end()) {
    return nullptr;
  }
  return fn->second(*builder_, left, right);
}

Value* CodegenContext::CreateFltBinop(Token::Type ty, Value* left,
                                      Value* right) {
  using Builder = IRBuilder<>;
  using OpFn = std::function<Value*(Builder&, Value*, Value*)>;

  static const std::unordered_map<Token::Type, OpFn, TokenTypeHash> table = {
      {Token::Type::Plus,
       [](Builder& b, Value* l, Value* r) {
         return b.CreateFAdd(l, r, "addtmp");
       }},
      {Token::Type::Minus,
       [](Builder& b, Value* l, Value* r) {
         return b.CreateFSub(l, r, "subtmp");
       }},
      {Token::Type::STAR,
       [](Builder& b, Value* l, Value* r) {
         return b.CreateFMul(l, r, "multmp");
       }},
      {Token::Type::SLASH,
       [](Builder& b, Value* l, Value* r) {
         return b.CreateFDiv(l, r, "divtmp");
       }},
  };

  auto fn = table.find(ty);
  if (fn == table.end()) {
    return nullptr;
  }
  return fn->second(*builder_, left, right);
}

Value* CodegenContext::CreateLoad(Type* ty, Value* val, const Twine& name) {
  return builder_->CreateLoad(ty, val, name);
}

Value* CodegenContext::CreatePreOp(types::Type* ty, Token::Type op, Value* val,
                                   AllocaInst* alloca) {
  using Builder = IRBuilder<>;
  using OpFn = std::function<Value*(Builder&, Value*, Value*)>;

  static const std::unordered_map<Token::Type, OpFn, TokenTypeHash> flt = {
      {Token::Type::PlusPlus,
       [](Builder& b, Value* l, Value* r) {
         return b.CreateFAdd(l, r, "addtmp");
       }},
      {Token::Type::MinusMinus,
       [](Builder& b, Value* l, Value* r) {
         return b.CreateFAdd(l, r, "addtmp");
       }},
  };

  static const std::unordered_map<Token::Type, OpFn, TokenTypeHash> integer = {
      {Token::Type::PlusPlus,
       [](Builder& b, Value* l, Value* r) {
         return b.CreateAdd(l, r, "addtmp");
       }},
      {Token::Type::MinusMinus,
       [](Builder& b, Value* l, Value* r) {
         return b.CreateAdd(l, r, "addtmp");
       }},
  };

  Value* var;

  switch (ty->kind) {
    case types::TypeKind::Int:
      var = integer.find(op)->second(*builder_, val, const_int);
      break;
    case types::TypeKind::Float:
      var = flt.find(op)->second(*builder_, val, const_flt);
      break;
    default:
      UNREACHABLE(CodegenConxt, CreatePreOp);
  }

  builder_->CreateStore(var, alloca);
  return var;
}