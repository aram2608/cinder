#ifndef EXPR_H_
#define EXPR_H_

#include "common.hpp"
#include "tokens.hpp"
#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Verifier.h"

struct Literal;
struct Boolean;
struct Variable;
struct Grouping;
struct Binary;

struct ExprVisitor {
  virtual ~ExprVisitor() = default;
  virtual llvm::Value* VisitLiteral(Literal& expr) = 0;
  virtual llvm::Value* VisitBoolean(Boolean& expr) = 0;
  virtual llvm::Value* VisitVariable(Variable& expr) = 0;
  virtual llvm::Value* VisitGrouping(Grouping& expr) = 0;
  virtual llvm::Value* VisitBinary(Binary& expr) = 0;
};

/// @struct Expr
/// @brief Base class for the AST nodes for expressions
struct Expr {
  /// @brief Virtual destructor, set to default
  virtual ~Expr() = default;

  /**
   * @brief Method used to emply the visitor pattern
   * All derived classes must implement this method
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
  ValueType value_type;
  TokenValue value;

  Literal(ValueType value_type, TokenValue value);
  llvm::Value* Accept(ExprVisitor& visitor) override;
  std::string ToString() override;
};

/// @struct Boolean
/// @brief Boolean node
struct Boolean : Expr {
  bool boolean;

  Boolean(bool boolean);
  llvm::Value* Accept(ExprVisitor& visitor) override;
  std::string ToString() override;
};

/// @struct Variable
/// @brief Variable node
struct Variable : Expr {
  std::string name;

  Variable(const std::string name);
  llvm::Value* Accept(ExprVisitor& visitor) override;
  std::string ToString() override;
};

/// @struct Grouping
/// @brief Grouping node
struct Grouping : Expr {
  std::unique_ptr<Expr> expr;

  Grouping(std::unique_ptr<Expr> expr);
  llvm::Value* Accept(ExprVisitor& visitor) override;
  std::string ToString() override;
};

/// @struct Binary
/// @brief Binary operation node
struct Binary : Expr {
  std::unique_ptr<Expr> left;
  std::unique_ptr<Expr> right;
  Token op;

  Binary(std::unique_ptr<Expr> left, std::unique_ptr<Expr> right, Token op);
  llvm::Value* Accept(ExprVisitor& visitor) override;
  std::string ToString() override;
};

#endif