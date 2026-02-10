#ifndef CODEGEN_BINDINGS_H_
#define CODEGEN_BINDINGS_H_

#include <memory>
#include <unordered_map>
#include <vector>

#include "cinder/semantic/symbol.hpp"
#include "cinder/support/error_category.hpp"
#include "llvm/IR/Argument.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Type.h"

struct Binding {
  enum class BindType { Func, Var, None };
  BindType type_ = BindType::None;

  explicit Binding(BindType type) : type_(type) {}
  virtual ~Binding() = default;

  bool IsFunction() const;
  bool IsVariable() const;

  template <typename T>
  T* CastTo(std::error_code& ec) {
    T* p = dynamic_cast<T*>(this);
    if (!p) {
      ec = Errors::BadCast;
    }
    return p;
  }
};

struct VarBinding : Binding {
  llvm::AllocaInst* alloca_ptr = nullptr;
  VarBinding() : Binding(BindType::Var) {}
};

struct FuncBinding : Binding {
  llvm::Function* function = nullptr;
  std::vector<llvm::Argument*> args;

  FuncBinding() : Binding(BindType::Func) {}
};

using BindingMap = std::unordered_map<SymbolId, std::unique_ptr<Binding>>;

#endif
