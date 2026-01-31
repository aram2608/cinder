#ifndef EXPR_H_
#define EXPR_H_

#include "common.hpp"
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
#include "tokens.hpp"

struct Literal;
struct BoolLiteral;
struct Variable;
struct Grouping;
struct PreFixOp;
struct Binary;
struct CallExpr;
struct Assign;
struct Conditional;

struct ExprVisitor {
  virtual ~ExprVisitor() = default;
  virtual llvm::Value* VisitLiteral(Literal& expr) = 0;
  virtual llvm::Value* VisitBoolean(BoolLiteral& expr) = 0;
  virtual llvm::Value* VisitVariable(Variable& expr) = 0;
  virtual llvm::Value* VisitGrouping(Grouping& expr) = 0;
  virtual llvm::Value* VisitPreIncrement(PreFixOp& expr) = 0;
  virtual llvm::Value* VisitBinary(Binary& expr) = 0;
  virtual llvm::Value* VisitCall(CallExpr& expr) = 0;
  virtual llvm::Value* VisitAssignment(Assign& expr) = 0;
  virtual llvm::Value* VisitConditional(Conditional& expr) = 0;
};

/// @struct Expr
/// @brief Base class for the AST nodes for expressions
struct Expr {
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
  ValueType value_type; /**< Value type for the literal */
  TokenValue value;     /**< Appropriate value for the given value type */

  Literal(ValueType value_type, TokenValue value);

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

/// @struct Boolean
/// @brief Boolean node
struct BoolLiteral : Expr {
  bool boolean; /** The boolean value */

  explicit BoolLiteral(bool boolean);

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
  Token op; /**< The operator token */
  std::unique_ptr<Expr>
      var; /**< The parsed variable, should resolve to Variable */

  PreFixOp(Token op, std::unique_ptr<Expr> var);

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
  std::unique_ptr<Expr> name;  /**< Variable name, should resolve to Variable */
  std::unique_ptr<Expr> value; /**< The variables new value */

  Assign(std::unique_ptr<Expr> name, std::unique_ptr<Expr> value);

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