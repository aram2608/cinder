#ifndef STMT_H_
#define STMT_H_

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "cinder/ast/expr/expr.hpp"
#include "cinder/ast/types.hpp"
#include "cinder/frontend/tokens.hpp"
#include "cinder/semantic/symbol.hpp"
#include "cinder/support/error_category.hpp"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Value.h"

struct ModuleStmt;
struct ExpressionStmt;
struct FunctionStmt;
struct FunctionProto;
struct ReturnStmt;
struct VarDeclarationStmt;
struct IfStmt;
struct ForStmt;
struct WhileStmt;
struct ImportStmt;

/** @brief Code generation visitor interface for statement nodes. */
struct StmtVisitor {
  virtual ~StmtVisitor() = default;
  virtual llvm::Value* Visit(ExpressionStmt& stmt) = 0;
  virtual llvm::Value* Visit(FunctionStmt& stmt) = 0;
  virtual llvm::Value* Visit(ReturnStmt& stmt) = 0;
  virtual llvm::Value* Visit(VarDeclarationStmt& stmt) = 0;
  virtual llvm::Value* Visit(FunctionProto& stmt) = 0;
  virtual llvm::Value* Visit(ModuleStmt& stmt) = 0;
  virtual llvm::Value* Visit(IfStmt& stmt) = 0;
  virtual llvm::Value* Visit(ForStmt& stmt) = 0;
  virtual llvm::Value* Visit(WhileStmt& stmt) = 0;
  virtual llvm::Value* Visit(ImportStmt& stmt) = 0;
};

/** @brief Semantic analysis visitor interface for statement nodes. */
struct SemanticStmtVisitor {
  virtual ~SemanticStmtVisitor() = default;
  virtual void Visit(ExpressionStmt& stmt) = 0;
  virtual void Visit(FunctionStmt& stmt) = 0;
  virtual void Visit(ReturnStmt& stmt) = 0;
  virtual void Visit(VarDeclarationStmt& stmt) = 0;
  virtual void Visit(FunctionProto& stmt) = 0;
  virtual void Visit(ModuleStmt& stmt) = 0;
  virtual void Visit(IfStmt& stmt) = 0;
  virtual void Visit(ForStmt& stmt) = 0;
  virtual void Visit(WhileStmt& stmt) = 0;
  virtual void Visit(ImportStmt& stmt) = 0;
};

struct StmtDumperVisitor {
  virtual ~StmtDumperVisitor() = default;
  virtual std::string Visit(ExpressionStmt& stmt) = 0;
  virtual std::string Visit(ReturnStmt& stmt) = 0;
  virtual std::string Visit(FunctionStmt& stmt) = 0;
  virtual std::string Visit(VarDeclarationStmt& stmt) = 0;
  virtual std::string Visit(FunctionProto& stmt) = 0;
  virtual std::string Visit(ModuleStmt& stmt) = 0;
  virtual std::string Visit(IfStmt& stmt) = 0;
  virtual std::string Visit(ForStmt& stmt) = 0;
  virtual std::string Visit(WhileStmt& stmt) = 0;
  virtual std::string Visit(ImportStmt& stmt) = 0;
};

/** @brief Abstract base class for all statement AST nodes. */
struct Stmt {
  enum class StmtType {
    Module,
    Expression,
    Function,
    FunctionProto,
    Return,
    VarDeclaration,
    If,
    For,
    While,
    Import,
  };

  StmtType stmt_type;
  Stmt(StmtType type) : stmt_type(type) {};

  virtual ~Stmt() = default;
  std::optional<SymbolId> id = std::nullopt; /**< Bound symbol id, if any. */

  /**
   * @brief Accepts a code generation visitor.
   * @param visitor Codegen visitor.
   * @return Value produced by code generation.
   */
  virtual llvm::Value* Accept(StmtVisitor& visitor) = 0;

  /**
   * @brief Accepts a semantic analysis visitor.
   * @param visitor Semantic visitor.
   */
  virtual void Accept(SemanticStmtVisitor& visitor) = 0;

  virtual std::string Accept(StmtDumperVisitor& visitor) = 0;

  /** @brief Returns whether this node is `ModuleStmt`. */
  bool IsModule();
  /** @brief Returns whether this node is `ExpressionStmt`. */
  bool IsExpression();
  /** @brief Returns whether this node is `FunctionStmt`. */
  bool IsFunction();
  /** @brief Returns whether this node is `FunctionProto`. */
  bool IsFunctionP();
  /** @brief Returns whether this node is `ReturnStmt`. */
  bool IsReturn();
  /** @brief Returns whether this node is `VarDeclarationStmt`. */
  bool IsVarDeclaration();
  /** @brief Returns whether this node is `IfStmt`. */
  bool IsIf();
  /** @brief Returns whether this node is `ForStmt`. */
  bool IsFor();
  /** @brief Returns whether this node is `WhileStmt`. */
  bool IsWhile();
  /** @brief Returns whether this node is `ImportStmt`. */
  bool IsImport();

  /** @brief Returns whether this node's id contains a value. */
  bool HasID();
  /** @brief Returns the underlying symbol id. */
  SymbolId GetID();

  template <typename T>
  llvm::ErrorOr<T*> CastTo() {
    T* p = dynamic_cast<T*>(this);
    if (!p) {
      return make_error_code(Errors::BadCast);
    }
    return p;
  }

  /**
   * @brief Dynamically casts this node to `T` with error-code reporting.
   * @tparam T Target node type.
   * @param ec Set to `Errors::BadCast` on failure.
   * @return Pointer to `T` on success; otherwise `nullptr`.
   */
  template <typename T>
  T* CastTo(std::error_code& ec) {
    T* p = dynamic_cast<T*>(this);
    if (!p) {
      ec = Errors::BadCast;
      return nullptr;
    }
    return p;
  }
};

/** @brief Root AST node for a translation unit/module. */
struct ModuleStmt : Stmt {
  cinder::Token name;                       /**< Module identifier token. */
  std::vector<std::unique_ptr<Stmt>> stmts; /**< Top-level statements. */

  ModuleStmt(cinder::Token name, std::vector<std::unique_ptr<Stmt>> stmts);

  /**
   * @brief Accepts a code generation visitor.
   * @param visitor Codegen visitor.
   * @return Value produced by code generation.
   */
  llvm::Value* Accept(StmtVisitor& visitor) override;
  void Accept(SemanticStmtVisitor& visitor) override;

  std::string Accept(StmtDumperVisitor& visitor) override;
};

/** @brief Statement wrapper around an expression. */
struct ExpressionStmt : Stmt {
  std::unique_ptr<Expr> expr; /**< Underlying expression to evaluate. */

  ExpressionStmt(std::unique_ptr<Expr> expr);

  /**
   * @brief Accepts a code generation visitor.
   * @param visitor Codegen visitor.
   * @return Value produced by code generation.
   */
  llvm::Value* Accept(StmtVisitor& visitor) override;
  void Accept(SemanticStmtVisitor& visitor) override;

  std::string Accept(StmtDumperVisitor& visitor) override;
};

/** @brief Function signature statement node. */
struct FunctionProto : Stmt {
  cinder::Token name;                /**< Function identifier token. */
  cinder::Token return_type;         /**< Return type token. */
  std::vector<cinder::FuncArg> args; /**< Function parameter list. */
  bool is_variadic; /**< True when prototype accepts varargs. */

  FunctionProto(cinder::Token name, cinder::Token return_type,
                std::vector<cinder::FuncArg> args, bool is_variadic);

  /**
   * @brief Accepts a code generation visitor.
   * @param visitor Codegen visitor.
   * @return Value produced by code generation.
   */
  llvm::Value* Accept(StmtVisitor& visitor) override;
  void Accept(SemanticStmtVisitor& visitor) override;

  std::string Accept(StmtDumperVisitor& visitor) override;
};

/** @brief Function definition statement node. */
struct FunctionStmt : Stmt {
  std::unique_ptr<Stmt> proto;             /**< Function prototype node. */
  std::vector<std::unique_ptr<Stmt>> body; /**< Function body statements. */

  FunctionStmt(std::unique_ptr<Stmt> proto,
               std::vector<std::unique_ptr<Stmt>> body);

  /**
   * @brief Accepts a code generation visitor.
   * @param visitor Codegen visitor.
   * @return Value produced by code generation.
   */
  llvm::Value* Accept(StmtVisitor& visitor) override;
  void Accept(SemanticStmtVisitor& visitor) override;

  std::string Accept(StmtDumperVisitor& visitor) override;
};

/** @brief Return statement node. */
struct ReturnStmt : Stmt {
  cinder::Token ret_token;     /**< `return` token. */
  std::unique_ptr<Expr> value; /**< Optional returned expression. */

  ReturnStmt(cinder::Token ret_token, std::unique_ptr<Expr> value);

  /**
   * @brief Accepts a code generation visitor.
   * @param visitor Codegen visitor.
   * @return Value produced by code generation.
   */
  llvm::Value* Accept(StmtVisitor& visitor) override;
  void Accept(SemanticStmtVisitor& visitor) override;

  std::string Accept(StmtDumperVisitor& visitor) override;
};

/** @brief Variable declaration statement node. */
struct VarDeclarationStmt : Stmt {
  cinder::Token type;          /**< Declared type token. */
  cinder::Token name;          /**< Variable identifier token. */
  std::unique_ptr<Expr> value; /**< Initializer expression. */

  VarDeclarationStmt(cinder::Token type, cinder::Token name,
                     std::unique_ptr<Expr> value);

  /**
   * @brief Accepts a code generation visitor.
   * @param visitor Codegen visitor.
   * @return Value produced by code generation.
   */
  llvm::Value* Accept(StmtVisitor& visitor) override;
  void Accept(SemanticStmtVisitor& visitor) override;

  std::string Accept(StmtDumperVisitor& visitor) override;
};

/** @brief If/else statement node. */
struct IfStmt : Stmt {
  std::unique_ptr<Expr> cond;      /**< Condition expression. */
  std::unique_ptr<Stmt> then;      /**< Then-branch statement. */
  std::unique_ptr<Stmt> otherwise; /**< Optional else-branch statement. */

  IfStmt(std::unique_ptr<Expr> cond, std::unique_ptr<Stmt> then,
         std::unique_ptr<Stmt> otherwise);

  /**
   * @brief Accepts a code generation visitor.
   * @param visitor Codegen visitor.
   * @return Value produced by code generation.
   */
  llvm::Value* Accept(StmtVisitor& visitor) override;
  void Accept(SemanticStmtVisitor& visitor) override;

  std::string Accept(StmtDumperVisitor& visitor) override;
};

/** @brief For-loop statement node. */
struct ForStmt : Stmt {
  std::unique_ptr<Stmt> initializer;       /**< Loop initializer statement. */
  std::unique_ptr<Expr> condition;         /**< Loop continuation condition. */
  std::unique_ptr<Expr> step;              /**< Optional step expression. */
  std::vector<std::unique_ptr<Stmt>> body; /**< Loop body statements. */

  ForStmt(std::unique_ptr<Stmt> initializer, std::unique_ptr<Expr> condition,
          std::unique_ptr<Expr> step, std::vector<std::unique_ptr<Stmt>> body);

  /**
   * @brief Accepts a code generation visitor.
   * @param visitor Codegen visitor.
   * @return Value produced by code generation.
   */
  llvm::Value* Accept(StmtVisitor& visitor) override;
  void Accept(SemanticStmtVisitor& visitor) override;

  std::string Accept(StmtDumperVisitor& visitor) override;
};

/** @brief While-loop statement node. */
struct WhileStmt : Stmt {
  std::unique_ptr<Expr> condition;         /**< Loop continuation condition. */
  std::vector<std::unique_ptr<Stmt>> body; /**< Loop body statements. */

  WhileStmt(std::unique_ptr<Expr> condition,
            std::vector<std::unique_ptr<Stmt>> body);

  /**
   * @brief Accepts a code generation visitor.
   * @param visitor Codegen visitor.
   * @return Value produced by code generation.
   */
  llvm::Value* Accept(StmtVisitor& visitor) override;
  /** @brief Accepts a semantic analysis visitor. */
  void Accept(SemanticStmtVisitor& visitor) override;
  std::string Accept(StmtDumperVisitor& visitor) override;
};

struct ImportStmt : Stmt {
  cinder::Token mod_name;

  ImportStmt(cinder::Token mod_name);

  /**
   * @brief Accepts a code generation visitor.
   * @param visitor Codegen visitor.
   * @return Value produced by code generation.
   */
  llvm::Value* Accept(StmtVisitor& visitor) override;
  /** @brief Accepts a semantic analysis visitor. */
  void Accept(SemanticStmtVisitor& visitor) override;
  std::string Accept(StmtDumperVisitor& visitor) override;
};

#endif
