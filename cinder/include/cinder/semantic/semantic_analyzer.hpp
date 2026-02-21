#ifndef SEMANTIC_ANALYZER_H_
#define SEMANTIC_ANALYZER_H_

#include <optional>
#include <string>
#include <vector>

#include "cinder/ast/expr/expr.hpp"
#include "cinder/ast/stmt/stmt.hpp"
#include "cinder/frontend/tokens.hpp"
#include "cinder/semantic/symbol.hpp"
#include "cinder/semantic/type_context.hpp"
#include "cinder/support/diagnostic.hpp"
#include "cinder/support/environment.hpp"

/**
 * @brief Performs semantic analysis over AST expressions and statements.
 *
 * This pass resolves symbol declarations/usages, computes expression types,
 * validates function calls and return statements, and collects diagnostics.
 */
class SemanticAnalyzer : SemanticExprVisitor, SemanticStmtVisitor {
  TypeContext& types_;                 /**< Canonical type provider. */
  ResolvedSymbols symbols_;            /**< Table of resolved symbols. */
  Environment env_;                    /**< Lexical scope stack. */
  cinder::types::Type* current_return; /**< Active function return type. */
  DiagnosticEngine diagnose_;          /**< Collected diagnostics. */

  /// We need to declare we are using these so the compiler knows which methods
  /// are available
  using SemanticExprVisitor::Visit;
  using SemanticStmtVisitor::Visit;

  /** @name Statement visitor overrides */
  ///@{
  void Visit(ModuleStmt& stmt) override;
  void Visit(ImportStmt& stmt) override;
  void Visit(ForStmt& stmt) override;
  void Visit(WhileStmt& stmt) override;
  void Visit(IfStmt& stmt) override;
  void Visit(ExpressionStmt& stmt) override;
  void Visit(FunctionStmt& stmt) override;
  void Visit(FunctionProto& stmt) override;
  void Visit(ReturnStmt& stmt) override;
  void Visit(VarDeclarationStmt& stmt) override;
  ///@}

  /** @name Expression visitor overrides */
  ///@{
  void Visit(Variable& expr) override;
  void Visit(Binary& expr) override;
  void Visit(Assign& expr) override;
  void Visit(Grouping& expr) override;
  void Visit(Conditional& expr) override;
  void Visit(PreFixOp& expr) override;
  void Visit(CallExpr& expr) override;
  void Visit(Literal& expr) override;
  ///@}

  /** @brief Resolves a function-argument type token. */
  cinder::types::Type* ResolveArgType(cinder::Token type);
  /** @brief Resolves a general declared type token. */
  cinder::types::Type* ResolveType(cinder::Token type);

  /** @brief Dispatches semantic analysis on a statement node. */
  void Resolve(Stmt& stmt);
  /** @brief Dispatches semantic analysis on an expression node. */
  void Resolve(Expr& expr);
  /** @brief Looks up symbol info by source name in the current environment. */
  SymbolInfo* LookupSymbol(const std::string& name);
  /** @brief Pushes a new lexical scope. */
  void BeginScope();
  /** @brief Pops the current lexical scope. */
  void EndScope();

  /**
   * @brief Declares a symbol in the current scope.
   * @param name Symbol name.
   * @param type Resolved type.
   * @param is_function Whether symbol represents a function.
   * @param loc Source location used for diagnostics.
   * @return Declared symbol id, or `std::nullopt` on redeclaration.
   */
  std::optional<SymbolId> Declare(std::string name, cinder::types::Type* type,
                                  bool is_function = false, SourceLoc loc = {});

  /**
   * @brief Applies default promotions for variadic call arguments.
   * @param expr Argument expression to potentially promote.
   */
  void VariadicPromotion(Expr* expr);

 public:
  /**
   * @brief Constructs the semantic analyzer.
   * @param types Canonical type context shared with later passes.
   */
  SemanticAnalyzer(TypeContext& types);

  /**
   * @brief Runs semantic analysis on a module AST.
   * @param mod Module node to analyze.
   */
  void Analyze(ModuleStmt& mod);

  /**
   * @brief Runs semantic analysis over a dependency-ordered module set.
   * @param modules Module nodes to analyze.
   */
  void AnalyzeProgram(const std::vector<ModuleStmt*>& modules);

  /** @brief Returns whether any error diagnostics were emitted. */
  bool HadError();

  /** @brief Prints collected diagnostics to stderr. */
  void DumpErrors();

  /** @brief Emits debug diagnostics for the resolved symbol table. */
  void DebugSymbols();
};

#endif
