#ifndef SEMANTIC_ANALYZER_H_
#define SEMANTIC_ANALYZER_H_

#include "expr.hpp"
#include "llvm/IR/Value.h"
#include "scope.hpp"
#include "stmt.hpp"
#include "tokens.hpp"
#include "type_context.hpp"
#include "types.hpp"

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