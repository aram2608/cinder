#ifndef CODEGEN_CONTEXT_H_
#define CODEGEN_CONTEXT_H_

#include <memory>

#include "cinder/ast/types.hpp"
#include "cinder/codegen/codegen_bindings.hpp"
#include "cinder/frontend/tokens.hpp"
#include "llvm/ADT/Twine.h"
#include "llvm/Analysis/CGSCCPassManager.h"
#include "llvm/Analysis/LoopAnalysisManager.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/DIBuilder.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
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

/**
 * @brief Owns LLVM IR construction state for a single compilation unit.
 *
 * This context centralizes the LLVM context, module, builder, and pass-manager
 * objects needed by code generation.
 */
class CodegenContext {
  std::unique_ptr<llvm::LLVMContext> llvm_ctx_; /**< Core LLVM context. */
  std::unique_ptr<llvm::Module> module_;        /**< Active LLVM module. */
  std::unique_ptr<llvm::IRBuilder<>> builder_;  /**< Active IR builder. */
  std::unique_ptr<llvm::FunctionPassManager>
      TheFPM_; /**< Function-level optimization passes. */
  std::unique_ptr<llvm::LoopAnalysisManager> TheLAM_; /**< Loop analyses. */
  std::unique_ptr<llvm::FunctionAnalysisManager> TheFAM_;
  std::unique_ptr<llvm::CGSCCAnalysisManager> TheCGAM_;
  std::unique_ptr<llvm::ModuleAnalysisManager> TheMAM_;
  std::unique_ptr<llvm::PassInstrumentationCallbacks> ThePIC_;
  std::unique_ptr<llvm::StandardInstrumentations> TheSI_;
  BindingMap bindings; /**< Reserved binding storage for context-local use. */
  llvm::Value* const_int;
  llvm::Value* const_flt;

 public:
  /**
   * @brief Creates and initializes LLVM state for `module_name`.
   * @param module_name Name of the generated LLVM module.
   */
  explicit CodegenContext(const std::string& module_name);

  /** @brief Returns the mutable LLVM context. */
  llvm::LLVMContext& GetContext();
  /** @brief Returns the mutable LLVM module. */
  llvm::Module& GetModule();
  /** @brief Returns the mutable IR builder. */
  llvm::IRBuilder<>& GetBuilder();

  llvm::AllocaInst* CreateAlloca(llvm::Type* ty, llvm::Value* array_size,
                                 const llvm::Twine& name = "");
  llvm::StoreInst* CreateStore(llvm::Value* value, llvm::Value* ptr,
                               bool is_volatile = false);

  llvm::Value* CreateIntCmp(cinder::Token::Type ty, llvm::Value* left,
                            llvm::Value* right);

  llvm::Value* CreateFltCmp(cinder::Token::Type ty, llvm::Value* left,
                            llvm::Value* right);

  llvm::Value* CreateIntBinop(cinder::Token::Type ty, llvm::Value* left,
                              llvm::Value* right);

  llvm::Value* CreateFltBinop(cinder::Token::Type ty, llvm::Value* left,
                              llvm::Value* right);

  llvm::Value* CreateLoad(llvm::Type* ty, llvm::Value* val,
                          const llvm::Twine& name = "");

  llvm::Value* CreatePreOp(cinder::types::Type* ty, cinder::Token::Type op,
                           llvm::Value* val, llvm::AllocaInst* alloca);
};

#endif
