#include "../include/module_builder.hpp"

ModuleBuilder::ModuleBuilder(std::unique_ptr<llvm::Module> mod,
                             std::unique_ptr<llvm::IRBuilder<>> builder,
                             std::unique_ptr<llvm::FunctionPassManager> fpm) {}