#ifndef CODEGEN_BINDINGS_H_
#define CODEGEN_BINDINGS_H_

#include <unordered_map>

#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "cinder/semantic/symbol.hpp"

struct Binding {
  llvm::AllocaInst* alloca_ptr = nullptr;
  llvm::Value* value = nullptr;
  llvm::Function* function = nullptr;
};

using BindingMap = std::unordered_map<SymbolId, Binding>;

#endif
