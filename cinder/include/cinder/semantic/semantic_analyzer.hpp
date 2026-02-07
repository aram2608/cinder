#ifndef SEMANTIC_ANALYZER_H_
#define SEMANTIC_ANALYZER_H_

#include <cstddef>
#include <memory>
#include <optional>
#include <ratio>

#include "cinder/ast/expr.hpp"
#include "cinder/ast/stmt.hpp"
#include "cinder/frontend/tokens.hpp"
#include "cinder/semantic/symbol.hpp"
#include "cinder/semantic/type_context.hpp"
#include "cinder/support/diagnostic.hpp"
#include "cinder/support/environment.hpp"
#include "llvm/IR/Value.h"

class SemanticAnalyzer : ExprVisitor, StmtVisitor {
  TypeContext& types_;
  ResolvedSymbols symbols_;
  Environment env_;
  types::Type* current_return;
  DiagnosticEngine diagnose_;

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
  SymbolInfo* LookupSymbol(const std::string& name);
  void BeginScope();
  void EndScope();
  std::optional<SymbolId> Declare(std::string name, types::Type* type,
                                  bool is_function = false, SourceLoc loc = {});
  void VariadicPromotion(Expr* expr);

 public:
  SemanticAnalyzer(TypeContext& types);
  void Analyze(ModuleStmt& mod);
  bool HadError();
  void DumpErrors();
  void DebugSymbols();
};

#endif
