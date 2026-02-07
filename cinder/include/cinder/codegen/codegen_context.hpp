#ifndef CODEGEN_CONTEXT_H_
#define CODEGEN_CONTEXT_H_

#include <memory>

#include "llvm/Analysis/CGSCCPassManager.h"
#include "llvm/Analysis/LoopAnalysisManager.h"
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
#include "llvm/Passes/StandardInstrumentations.h"
#include "llvm/Target/TargetMachine.h"
#include "cinder/codegen/codegen_bindings.hpp"

class CodegenContext {
  std::unique_ptr<llvm::LLVMContext> llvm_ctx_;
  std::unique_ptr<llvm::Module> module_;
  std::unique_ptr<llvm::IRBuilder<>> builder_;
  std::unique_ptr<llvm::FunctionPassManager>
      TheFPM_; /**< Func pass optimizer */
  std::unique_ptr<llvm::LoopAnalysisManager> TheLAM_; /** Loop optimizer */
  std::unique_ptr<llvm::FunctionAnalysisManager> TheFAM_;
  std::unique_ptr<llvm::CGSCCAnalysisManager> TheCGAM_;
  std::unique_ptr<llvm::ModuleAnalysisManager> TheMAM_;
  std::unique_ptr<llvm::PassInstrumentationCallbacks> ThePIC_;
  std::unique_ptr<llvm::StandardInstrumentations> TheSI_;
  BindingMap bindings;

 public:
  explicit CodegenContext(const std::string& module_name);

  llvm::LLVMContext& GetContext();
  llvm::Module& GetModule();
  llvm::IRBuilder<>& GetBuilder();
};

#endif
