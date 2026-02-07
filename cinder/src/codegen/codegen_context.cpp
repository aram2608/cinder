#include "cinder/codegen/codegen_context.hpp"

#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/DIBuilder.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
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

CodegenContext::CodegenContext(const std::string& module_name)
    : llvm_ctx_(std::make_unique<llvm::LLVMContext>()),
      module_(std::make_unique<llvm::Module>(module_name, *llvm_ctx_)),
      builder_(std::make_unique<llvm::IRBuilder<>>(*llvm_ctx_)) {
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

llvm::LLVMContext& CodegenContext::GetContext() {
  return *llvm_ctx_;
}

llvm::Module& CodegenContext::GetModule() {
  return *module_;
}

llvm::IRBuilder<>& CodegenContext::GetBuilder() {
  return *builder_;
}
