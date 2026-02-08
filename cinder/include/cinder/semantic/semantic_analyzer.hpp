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

class SemanticAnalyzer : SemanticExprVisitor, SemanticStmtVisitor {
  TypeContext& types_;
  ResolvedSymbols symbols_;
  Environment env_;
  cinder::types::Type* current_return;
  DiagnosticEngine diagnose_;

  /// We need to declare we are using these so the compiler knows which methods
  /// are available
  using SemanticExprVisitor::Visit;
  using SemanticStmtVisitor::Visit;

  // Statements
  void Visit(ModuleStmt& stmt) override;
  void Visit(ForStmt& stmt) override;
  void Visit(WhileStmt& stmt) override;
  void Visit(IfStmt& stmt) override;
  void Visit(ExpressionStmt& stmt) override;
  void Visit(FunctionStmt& stmt) override;
  void Visit(FunctionProto& stmt) override;
  void Visit(ReturnStmt& stmt) override;
  void Visit(VarDeclarationStmt& stmt) override;

  // Expressions
  void Visit(Variable& expr) override;
  void Visit(Binary& expr) override;
  void Visit(Assign& expr) override;
  void Visit(Grouping& expr) override;
  void Visit(Conditional& expr) override;
  void Visit(PreFixOp& expr) override;
  void Visit(CallExpr& expr) override;
  void Visit(Literal& expr) override;

  cinder::types::Type* ResolveArgType(Token type);
  cinder::types::Type* ResolveType(Token type);

  void Resolve(Stmt& stmt);
  void Resolve(Expr& expr);
  SymbolInfo* LookupSymbol(const std::string& name);
  void BeginScope();
  void EndScope();
  std::optional<SymbolId> Declare(std::string name, cinder::types::Type* type,
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
