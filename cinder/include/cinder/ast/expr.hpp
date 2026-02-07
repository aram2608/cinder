#ifndef EXPR_H_
#define EXPR_H_

#include <memory>
#include <vector>

#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Instructions.h"
#include "cinder/ast/types.hpp"
#include "cinder/frontend/tokens.hpp"

struct Literal;
struct Variable;
struct Grouping;
struct PreFixOp;
struct Binary;
struct CallExpr;
struct Assign;
struct Conditional;

struct ExprVisitor {
  virtual ~ExprVisitor() = default;
  virtual llvm::Value* Visit(Literal& expr) = 0;
  virtual llvm::Value* Visit(Variable& expr) = 0;
  virtual llvm::Value* Visit(Grouping& expr) = 0;
  virtual llvm::Value* Visit(PreFixOp& expr) = 0;
  virtual llvm::Value* Visit(Binary& expr) = 0;
  virtual llvm::Value* Visit(CallExpr& expr) = 0;
  virtual llvm::Value* Visit(Assign& expr) = 0;
  virtual llvm::Value* Visit(Conditional& expr) = 0;
};

/// @struct Expr
/// @brief Base class for the AST nodes for expressions
struct Expr {
  types::Type* type = nullptr;
  /// @brief Virtual destructor, set to default
  virtual ~Expr() = default;

  /**
   * @brief Method used to emply the visitor pattern
   * All derived classes must implement this method
   * @param visitor The expression visitor
   * @return The appropiate visitor method
   */
  virtual llvm::Value* Accept(ExprVisitor& visitor) = 0;

  /**
   * @brief Method to return the string representation of the node
   * @return The string represention
   */
  virtual std::string ToString() = 0;
};

/// @struct Literal
/// @brief Numeric Node, stores floats and ints as of now
struct Literal : Expr {
  TokenValue value; /**< Appropriate value for the given value type */
  explicit Literal(TokenValue value);

  /**
   * @brief Method used to emply the visitor pattern
   * All derived classes must implement this method
   * @param visitor The expression visitor
   * @return The appropiate visitor method
   */
  llvm::Value* Accept(ExprVisitor& visitor) override;

  /**
   * @brief Method to return the string representation of the node
   * @return The string represention
   */
  std::string ToString() override;
};

/// @struct Variable
/// @brief Variable node
struct Variable : Expr {
  Token name; /** Name of the variable */

  explicit Variable(Token name);

  /**
   * @brief Method used to emply the visitor pattern
   * All derived classes must implement this method
   * @param visitor The expression visitor
   * @return The appropiate visitor method
   */
  llvm::Value* Accept(ExprVisitor& visitor) override;

  /**
   * @brief Method to return the string representation of the node
   * @return The string represention
   */
  std::string ToString() override;
};

/// @struct Grouping
/// @brief Grouping node
struct Grouping : Expr {
  std::unique_ptr<Expr> expr; /** Expression inside of the paren, '( expr )' */

  explicit Grouping(std::unique_ptr<Expr> expr);

  /**
   * @brief Method used to emply the visitor pattern
   * All derived classes must implement this method
   * @param visitor The expression visitor
   * @return The appropiate visitor method
   */
  llvm::Value* Accept(ExprVisitor& visitor) override;

  /**
   * @brief Method to return the string representation of the node
   * @return The string represention
   */
  std::string ToString() override;
};

struct PreFixOp : Expr {
  Token op;
  Token name;

  PreFixOp(Token op, Token name);

  /**
   * @brief Method used to emply the visitor pattern
   * All derived classes must implement this method
   * @param visitor The expression visitor
   * @return The appropiate visitor method
   */
  llvm::Value* Accept(ExprVisitor& visitor) override;

  /**
   * @brief Method to return the string representation of the node
   * @return The string represention
   */
  std::string ToString() override;
};

/// @struct Binary
/// @brief Binary operation node
struct Binary : Expr {
  std::unique_ptr<Expr> left;  /**< The lhs expression */
  std::unique_ptr<Expr> right; /**< The rhs expression */
  Token op;                    /** The binary operator */

  Binary(std::unique_ptr<Expr> left, std::unique_ptr<Expr> right, Token op);

  /**
   * @brief Method used to emply the visitor pattern
   * All derived classes must implement this method
   * @param visitor The expression visitor
   * @return The appropiate visitor method
   */
  llvm::Value* Accept(ExprVisitor& visitor) override;

  /**
   * @brief Method to return the string representation of the node
   * @return The string represention
   */
  std::string ToString() override;
};

struct Conditional : Expr {
  std::unique_ptr<Expr> left;  /**< The lhs expression */
  std::unique_ptr<Expr> right; /**< The rhs expression */
  Token op;                    /**< The bianry operator */

  Conditional(std::unique_ptr<Expr> left, std::unique_ptr<Expr> right,
              Token op);

  /**
   * @brief Method used to emply the visitor pattern
   * All derived classes must implement this method
   * @param visitor The expression visitor
   * @return The appropiate visitor method
   */
  llvm::Value* Accept(ExprVisitor& visitor) override;

  /**
   * @brief Method to return the string representation of the node
   * @return The string represention
   */
  std::string ToString() override;
};

struct Assign : Expr {
  Token name;                  /**< Variable name */
  std::unique_ptr<Expr> value; /**< The variables new value */

  Assign(Token name, std::unique_ptr<Expr> value);

  /**
   * @brief Method used to emply the visitor pattern
   * All derived classes must implement this method
   * @param visitor The expression visitor
   * @return The appropiate visitor method
   */
  llvm::Value* Accept(ExprVisitor& visitor) override;

  /**
   * @brief Method to return the string representation of the node
   * @return The string represention
   */
  std::string ToString() override;
};

struct CallExpr : Expr {
  std::unique_ptr<Expr> callee; /**< The name of the callee as an expr */
  std::vector<std::unique_ptr<Expr>> args; /**< List of arguments in the call */

  CallExpr(std::unique_ptr<Expr> callee,
           std::vector<std::unique_ptr<Expr>> args);

  /**
   * @brief Method used to emply the visitor pattern
   * All derived classes must implement this method
   * @param visitor The expression visitor
   * @return The appropiate visitor method
   */
  llvm::Value* Accept(ExprVisitor& visitor) override;

  /**
   * @brief Method to return the string representation of the node
   * @return The string represention
   */
  std::string ToString() override;
};

#endif
