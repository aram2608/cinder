#ifndef SYMBOL_TABLE_H_
#define SYMBOL_TABLE_H_

#include <memory>
#include <string>
#include <unordered_map>

#include "llvm/IR/Argument.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Value.h"
#include "llvm/Transforms/Scalar/GVN.h"
#include "llvm/Transforms/Scalar/Reassociate.h"

struct InternalSymbol {
  llvm::AllocaInst* alloca_ptr;
  llvm::Value* value;
  InternalSymbol(llvm::AllocaInst* alloca_ptr = nullptr,
                 llvm::Value* value = nullptr)
      : alloca_ptr(alloca_ptr), value(value) {}
};

struct SymbolTable {
  std::unordered_map<std::string, InternalSymbol> symbols;
  std::shared_ptr<SymbolTable> parent;

  explicit SymbolTable(std::shared_ptr<SymbolTable> parent = nullptr)
      : parent(parent) {}

  void Declare(std::string name, InternalSymbol symb);
  InternalSymbol* LookUp(std::string name);
};

#endif