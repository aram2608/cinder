#ifndef EXPR_H_
#define EXPR_H_

#include <memory>
#include <optional>
#include <string>
#include <system_error>
#include <vector>

#include "cinder/ast/types.hpp"
#include "cinder/frontend/tokens.hpp"
#include "cinder/semantic/symbol.hpp"
#include "cinder/support/error_category.hpp"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Value.h"
#include "llvm/Support/ErrorOr.h"

struct Literal;
struct Variable;
struct MemberAccess;
struct Grouping;
struct PreFixOp;
struct Binary;
struct CallExpr;
struct Assign;
struct MemberAssign;
struct Conditional;

/** @brief Code generation visitor interface for expression nodes. */
struct CodegenExprVisitor {
  virtual ~CodegenExprVisitor() = default;
  virtual llvm::Value* Visit(Literal& expr) = 0;
  virtual llvm::Value* Visit(Variable& expr) = 0;
  virtual llvm::Value* Visit(MemberAccess& expr) = 0;
  virtual llvm::Value* Visit(Grouping& expr) = 0;
  virtual llvm::Value* Visit(PreFixOp& expr) = 0;
  virtual llvm::Value* Visit(Binary& expr) = 0;
  virtual llvm::Value* Visit(CallExpr& expr) = 0;
  virtual llvm::Value* Visit(Assign& expr) = 0;
  virtual llvm::Value* Visit(MemberAssign& expr) = 0;
  virtual llvm::Value* Visit(Conditional& expr) = 0;
};

/** @brief Semantic analysis visitor interface for expression nodes. */
struct SemanticExprVisitor {
  virtual ~SemanticExprVisitor() = default;
  virtual void Visit(Literal& expr) = 0;
  virtual void Visit(Variable& expr) = 0;
  virtual void Visit(MemberAccess& expr) = 0;
  virtual void Visit(Grouping& expr) = 0;
  virtual void Visit(PreFixOp& expr) = 0;
  virtual void Visit(Binary& expr) = 0;
  virtual void Visit(CallExpr& expr) = 0;
  virtual void Visit(Assign& expr) = 0;
  virtual void Visit(MemberAssign& expr) = 0;
  virtual void Visit(Conditional& expr) = 0;
};

struct ExprDumperVisitor {
  virtual ~ExprDumperVisitor() = default;
  virtual std::string Visit(Literal& expr) = 0;
  virtual std::string Visit(Variable& expr) = 0;
  virtual std::string Visit(MemberAccess& expr) = 0;
  virtual std::string Visit(Grouping& expr) = 0;
  virtual std::string Visit(PreFixOp& expr) = 0;
  virtual std::string Visit(Binary& expr) = 0;
  virtual std::string Visit(CallExpr& expr) = 0;
  virtual std::string Visit(Assign& expr) = 0;
  virtual std::string Visit(MemberAssign& expr) = 0;
  virtual std::string Visit(Conditional& expr) = 0;
};

/** @brief Abstract base class for all expression AST nodes. */
struct Expr {
  enum class ExprType {
    Literal,
    Variable,
    MemberAccess,
    Grouping,
    PreFix,
    Binary,
    Call,
    Assign,
    MemberAssign,
    Conditional,
    Unknown
  };
  cinder::types::Type* type = nullptr; /**< Resolved semantic type, if known. */
  std::optional<SymbolId> id = std::nullopt; /**< Bound symbol id, if any. */
  ExprType expr_type;

  Expr(ExprType type) : expr_type(type) {}

  /** @brief Virtual destructor for polymorphic ownership. */
  virtual ~Expr() = default;

  /**
   * @brief Accepts a codegen visitor.
   * @param visitor Codegen visitor.
   * @return Value produced by code generation.
   */
  virtual llvm::Value* Accept(CodegenExprVisitor& visitor) = 0;

  /**
   * @brief Accepts a semantic visitor.
   * @param visitor Semantic visitor.
   */
  virtual void Accept(SemanticExprVisitor& visitor) = 0;

  virtual std::string Accept(ExprDumperVisitor& visitor) = 0;

  /** @brief Returns whether this node is `Literal`. */
  bool IsLiteral();
  /** @brief Returns whether this node is `Variable`. */
  bool IsVariable();
  /** @brief Returns whether this node is `MemberAccess`. */
  bool IsMemberAccess();
  /** @brief Returns whether this node is `Grouping`. */
  bool IsGrouping();
  /** @brief Returns whether this node is `PreFix`. */
  bool IsPreFixOp();
  /** @brief Returns whether this node is `Binary`. */
  bool IsBinary();
  /** @brief Returns whether this node is `Call`. */
  bool IsCallExpr();
  /** @brief Returns whether this node is `Assign`. */
  bool IsAssign();
  /** @brief Returns whether this node is `MemberAssign`. */
  bool IsMemberAssign();
  /** @brief Returns whether this node is `Conditional`. */
  bool IsConditional();
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

/** @brief Literal expression node. */
struct Literal : Expr {
  cinder::TokenValue value; /**< Literal value payload. */

  explicit Literal(cinder::TokenValue value);

  /**
   * @brief Accepts a codegen visitor.
   * @param visitor Codegen visitor.
   * @return Value produced by code generation.
   */
  llvm::Value* Accept(CodegenExprVisitor& visitor) override;

  /**
   * @brief Accepts a semantic visitor.
   * @param visitor Semantic visitor.
   */
  void Accept(SemanticExprVisitor& visitor) override;

  std::string Accept(ExprDumperVisitor& visitor) override;
};

/** @brief Variable reference expression node. */
struct Variable : Expr {
  cinder::Token name; /**< Identifier token for the variable. */

  explicit Variable(cinder::Token name);

  /**
   * @brief Accepts a codegen visitor.
   * @param visitor Codegen visitor.
   * @return Value produced by code generation.
   */
  llvm::Value* Accept(CodegenExprVisitor& visitor) override;

  /** @brief Accepts a semantic visitor. */
  void Accept(SemanticExprVisitor& visitor) override;

  std::string Accept(ExprDumperVisitor& visitor) override;
};

/** @brief Member access expression node (`object.member`). */
struct MemberAccess : Expr {
  std::unique_ptr<Expr> object; /**< Object/base expression. */
  cinder::Token member;         /**< Accessed member identifier token. */
  std::optional<size_t> field_index = std::nullopt;

  MemberAccess(std::unique_ptr<Expr> object, cinder::Token member);

  /** @brief Accepts a codegen visitor. */
  llvm::Value* Accept(CodegenExprVisitor& visitor) override;

  /** @brief Accepts a semantic visitor. */
  void Accept(SemanticExprVisitor& visitor) override;

  std::string Accept(ExprDumperVisitor& visitor) override;
};

/** @brief Parenthesized expression node. */
struct Grouping : Expr {
  std::unique_ptr<Expr> expr; /**< Expression inside parentheses. */

  explicit Grouping(std::unique_ptr<Expr> expr);

  /**
   * @brief Accepts a codegen visitor.
   * @param visitor Codegen visitor.
   * @return Value produced by code generation.
   */
  llvm::Value* Accept(CodegenExprVisitor& visitor) override;

  /** @brief Accepts a semantic visitor. */
  void Accept(SemanticExprVisitor& visitor) override;

  std::string Accept(ExprDumperVisitor& visitor) override;
};

/** @brief Prefix increment/decrement expression node. */
struct PreFixOp : Expr {
  cinder::Token op;   /**< Prefix operator token. */
  cinder::Token name; /**< Target variable identifier token. */

  PreFixOp(cinder::Token op, cinder::Token name);

  /**
   * @brief Accepts a codegen visitor.
   * @param visitor Codegen visitor.
   * @return Value produced by code generation.
   */
  llvm::Value* Accept(CodegenExprVisitor& visitor) override;

  /** @brief Accepts a semantic visitor. */
  void Accept(SemanticExprVisitor& visitor) override;

  std::string Accept(ExprDumperVisitor& visitor) override;
};

/** @brief Binary arithmetic expression node. */
struct Binary : Expr {
  std::unique_ptr<Expr> left;  /**< Left-hand side expression. */
  std::unique_ptr<Expr> right; /**< Right-hand side expression. */
  cinder::Token op;            /**< Binary operator token. */

  Binary(std::unique_ptr<Expr> left, std::unique_ptr<Expr> right,
         cinder::Token op);

  /**
   * @brief Accepts a codegen visitor.
   * @param visitor Codegen visitor.
   * @return Value produced by code generation.
   */
  llvm::Value* Accept(CodegenExprVisitor& visitor) override;

  /** @brief Accepts a semantic visitor. */
  void Accept(SemanticExprVisitor& visitor) override;

  std::string Accept(ExprDumperVisitor& visitor) override;
};

/** @brief Binary comparison expression node. */
struct Conditional : Expr {
  std::unique_ptr<Expr> left;  /**< Left-hand side expression. */
  std::unique_ptr<Expr> right; /**< Right-hand side expression. */
  cinder::Token op;            /**< Comparison operator token. */

  Conditional(std::unique_ptr<Expr> left, std::unique_ptr<Expr> right,
              cinder::Token op);

  /**
   * @brief Accepts a codegen visitor.
   * @param visitor Codegen visitor.
   * @return Value produced by code generation.
   */
  llvm::Value* Accept(CodegenExprVisitor& visitor) override;

  /** @brief Accepts a semantic visitor. */
  void Accept(SemanticExprVisitor& visitor) override;

  std::string Accept(ExprDumperVisitor& visitor) override;
};

/** @brief Assignment expression node. */
struct Assign : Expr {
  cinder::Token name;          /**< Target variable identifier token. */
  std::unique_ptr<Expr> value; /**< Value expression to assign. */

  Assign(cinder::Token name, std::unique_ptr<Expr> value);

  /**
   * @brief Accepts a codegen visitor.
   * @param visitor Codegen visitor.
   * @return Value produced by code generation.
   */
  llvm::Value* Accept(CodegenExprVisitor& visitor) override;

  /** @brief Accepts a semantic visitor. */
  void Accept(SemanticExprVisitor& visitor) override;

  std::string Accept(ExprDumperVisitor& visitor) override;
};

/** @brief Struct member assignment expression node (`object.member = value`).
 */
struct MemberAssign : Expr {
  std::unique_ptr<MemberAccess> target;
  std::unique_ptr<Expr> value;
  std::optional<SymbolId> base_id = std::nullopt;

  MemberAssign(std::unique_ptr<MemberAccess> target,
               std::unique_ptr<Expr> value);

  llvm::Value* Accept(CodegenExprVisitor& visitor) override;
  void Accept(SemanticExprVisitor& visitor) override;
  std::string Accept(ExprDumperVisitor& visitor) override;
};

/** @brief Function call expression node. */
struct CallExpr : Expr {
  std::unique_ptr<Expr> callee;            /**< Callee expression. */
  std::vector<std::unique_ptr<Expr>> args; /**< Call argument expressions. */

  CallExpr(std::unique_ptr<Expr> callee,
           std::vector<std::unique_ptr<Expr>> args);

  /**
   * @brief Accepts a codegen visitor.
   * @param visitor Codegen visitor.
   * @return Value produced by code generation.
   */
  llvm::Value* Accept(CodegenExprVisitor& visitor) override;

  /** @brief Accepts a semantic visitor. */
  void Accept(SemanticExprVisitor& visitor) override;

  std::string Accept(ExprDumperVisitor& visitor) override;
};

#endif
