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

/** @brief Polymorphic base for AST-symbol to LLVM-entity bindings. */
struct Binding {
  enum class BindType { Func, Var, None };
  BindType type_ = BindType::None;

  explicit Binding(BindType type) : type_(type) {}
  virtual ~Binding() = default;

  /** @brief Returns whether this binding stores a function. */
  bool IsFunction() const;
  /** @brief Returns whether this binding stores a variable allocation. */
  bool IsVariable() const;

  /**
   * @brief Dynamically casts this binding to `T` with error-code reporting.
   * @tparam T Target binding type.
   * @param ec Set to `Errors::BadCast` on failure.
   * @return Pointer to `T` on success; otherwise `nullptr`.
   */
  template <typename T>
  T* CastTo(std::error_code& ec) {
    T* p = dynamic_cast<T*>(this);
    if (!p) {
      ec = Errors::BadCast;
    }
    return p;
  }
};

/** @brief Binding for local/global variables represented by an alloca slot. */
struct VarBinding : Binding {
  llvm::AllocaInst* alloca_ptr = nullptr; /**< Backing alloca instruction. */
  VarBinding() : Binding(BindType::Var) {}
};

/** @brief Binding for function symbols represented by an LLVM function. */
struct FuncBinding : Binding {
  llvm::Function* function = nullptr; /**< LLVM function handle. */
  std::vector<llvm::Argument*> args;  /**< Optional cached argument handles. */

  FuncBinding() : Binding(BindType::Func) {}
};

/** @brief Symbol-id keyed map of codegen bindings. */
using BindingMap = std::unordered_map<SymbolId, std::unique_ptr<Binding>>;

#endif
