#ifndef CODEGEN_BINDINGS_H_
#define CODEGEN_BINDINGS_H_

#include <unordered_map>
#include <vector>

#include "cinder/semantic/symbol.hpp"
#include "llvm/IR/Argument.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"

struct Binding {
  llvm::AllocaInst* alloca_ptr = nullptr;
  llvm::Value* value = nullptr;
  llvm::Function* function = nullptr;
  std::vector<llvm::Argument*> args;
};

using BindingMap = std::unordered_map<SymbolId, Binding>;

#endif
