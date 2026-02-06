#ifndef SEMANTIC_ANALYZER_H_
#define SEMANTIC_ANALYZER_H_

#include <cstddef>
#include <memory>

#include "expr.hpp"
#include "llvm/IR/Value.h"
#include "semantic_scope.hpp"
#include "stmt.hpp"
#include "tokens.hpp"
#include "type_context.hpp"
#include "types.hpp"

class SemanticAnalyzer : ExprVisitor, StmtVisitor {
  TypeContext* types;
  std::shared_ptr<Scope> scope;
  types::Type* current_return;

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
  llvm::Value* Visit(Variable& expr) override;
  llvm::Value* Visit(Binary& expr) override;
  llvm::Value* Visit(Assign& expr) override;
  llvm::Value* Visit(Grouping& expr) override;
  llvm::Value* Visit(Conditional& expr) override;
  llvm::Value* Visit(PreFixOp& expr) override;
  llvm::Value* Visit(CallExpr& expr) override;
  llvm::Value* Visit(Literal& expr) override;

  types::Type* ResolveArgType(Token type);
  types::Type* ResolveType(Token type);

  llvm::Value* Resolve(Stmt& stmt);
  llvm::Value* Resolve(Expr& expr);
  void Declare(std::string name, types::Type* type);
  void VariadicPromotion(Expr* expr);

 public:
  SemanticAnalyzer(TypeContext* tc);
  void Analyze(ModuleStmt& mod);
};

#endif