#ifndef CODEGEN_CONTEXT_H_
#define CODEGEN_CONTEXT_H_

#include <memory>

#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "no_name_lang/codegen/codegen_bindings.hpp"

class CodegenContext {
  std::unique_ptr<llvm::LLVMContext> llvm_ctx_;
  std::unique_ptr<llvm::Module> module_;
  std::unique_ptr<llvm::IRBuilder<>> builder_;
  BindingMap bindings;

 public:
  explicit CodegenContext(const std::string& module_name);

  llvm::LLVMContext& GetContext();
  llvm::Module& GetModule();
  llvm::IRBuilder<>& GetBuilder();
};

#endif
