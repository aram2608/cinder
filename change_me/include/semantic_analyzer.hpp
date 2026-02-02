#ifndef SEMANTIC_ANALYZER_H_
#define SEMANTIC_ANALYZER_H_

#include "common.hpp"
#include "expr.hpp"
#include "llvm/IR/Value.h"
#include "stmt.hpp"
#include "tokens.hpp"
#include "types.hpp"
#include "utils.hpp"

struct TypeContext {
  types::IntType int32{32, true};
  types::FloatType float32{32};
  types::Type voidTy{types::TypeKind::Void};
  /// TODO: Extend types
  /// BoolType
  /// FunctionType

  std::unordered_map<types::Type*, std::unique_ptr<types::PointerType>> ptrs;

  types::Type* Int32() {
    return &int32;
  }
  types::Type* Float32() {
    return &float32;
  }
  types::Type* Void() {
    return &voidTy;
  }

  types::Type* PointerTo(types::Type* base) {
    auto& p = ptrs[base];
    if (!p) {
      p = std::make_unique<types::PointerType>(base);
    }
    return p.get();
  }
};

struct Symbol {
  types::Type* type;
};

struct Scope {
  std::unordered_map<std::string, Symbol> table;
  Scope* parent;

  explicit Scope(Scope* parent = nullptr) : parent(parent) {}

  void Declare(const std::string& name, types::Type* type) {
    table[name] = Symbol{type};
  }

  Symbol* Lookup(const std::string& name) {
    if (auto it = table.find(name); it != table.end()) {
      return &it->second;
    }
    return parent ? parent->Lookup(name) : nullptr;
  }
};

struct SemanticAnalyzer : ExprVisitor, StmtVisitor {
  SemanticAnalyzer(TypeContext& tc) : types(tc), scope(nullptr) {}

  void Analyze(Expr& expr) {
    expr.Accept(*this);
  }

  // Statements
  llvm::Value* Visit(ModuleStmt& stmt) override;
  llvm::Value* Visit(ForStmt& stmt) override;
  llvm::Value* Visit(WhileStmt& stmt) override;
  llvm::Value* Visit(IfStmt& stmt) override;
  llvm::Value* Visit(ExpressionStmt& stmt) override;
  llvm::Value* Visit(FunctionStmt& stmt) override;
  llvm::Value* Visit(FunctionProto& stmt) override;
  llvm::Value* Visit(ReturnStmt& stmt) override;
  llvm::Value* Visit(VarDeclarationStmt& stmt) override;

  // Expressions
  llvm::Value* Visit(Literal& expr) override;
  llvm::Value* Visit(BoolLiteral& expr) override;
  llvm::Value* Visit(Variable& expr) override;
  llvm::Value* Visit(Binary& expr) override;
  llvm::Value* Visit(Assign& expr) override;
  llvm::Value* Visit(CallExpr& expr) override;

  TypeContext& types;
  Scope* scope;
};

#endif