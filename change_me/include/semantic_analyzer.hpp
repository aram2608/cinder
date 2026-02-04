#ifndef SEMANTIC_ANALYZER_H_
#define SEMANTIC_ANALYZER_H_

#include "common.hpp"
#include "errors.hpp"
#include "expr.hpp"
#include "llvm/IR/Value.h"
#include "stmt.hpp"
#include "tokens.hpp"
#include "types.hpp"
#include "utils.hpp"

struct TypeContext {
  types::IntType int32{32};
  types::FloatType float32{32};
  types::Type voidTy{types::TypeKind::Void};
  types::BoolType int1{1};

  /// TODO: Extend types
  /// FunctionType
  /// StructType

  types::Type* Int32();
  types::Type* Float32();
  types::Type* Void();
  types::Type* Bool();
  types::Type* Function(types::Type* ret, std::vector<types::Type*> params);
};

struct Symbol {
  types::Type* type;
};

struct Scope {
  std::unordered_map<std::string, Symbol> table;
  std::shared_ptr<Scope> parent;

  explicit Scope(std::shared_ptr<Scope> parent = nullptr);
  void Declare(const std::string& name, types::Type* type);
  Symbol* Lookup(const std::string& name);
};

struct SemanticAnalyzer : ExprVisitor, StmtVisitor {
  SemanticAnalyzer(TypeContext& tc);

  /// We need to declare we are using these so the compiler knows which methods
  /// are available
  using ExprVisitor::Visit;
  using StmtVisitor::Visit;

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

  types::Type* ResolveArgType(Token type);
  types::Type* ResolveType(Token type);

  TypeContext& types;
  std::shared_ptr<Scope> scope;
};

#endif