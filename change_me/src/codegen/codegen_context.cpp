#include "no_name_lang/codegen/codegen_context.hpp"

CodegenContext::CodegenContext(const std::string& module_name)
    : llvm_ctx_(std::make_unique<llvm::LLVMContext>()),
      module_(std::make_unique<llvm::Module>(module_name, *llvm_ctx_)),
      builder_(std::make_unique<llvm::IRBuilder<>>(*llvm_ctx_)) {}

llvm::LLVMContext& CodegenContext::GetContext() {
  return *llvm_ctx_;
}

llvm::Module& CodegenContext::GetModule() {
  return *module_;
}

llvm::IRBuilder<>& CodegenContext::GetBuilder() {
  return *builder_;
}
