#ifndef STMT_H_
#define STMT_H_

#include <memory>
#include <string>

// #include "common.hpp"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Value.h"
#include "cinder/ast/expr.hpp"
#include "cinder/ast/types.hpp"
#include "cinder/frontend/tokens.hpp"

struct ModuleStmt;
struct ExpressionStmt;
struct FunctionStmt;
struct FunctionProto;
struct ReturnStmt;
struct VarDeclarationStmt;
struct IfStmt;
struct ForStmt;
struct WhileStmt;

/// @struct StmtVisitor
/// @brief The statement visitor interface
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
};

/// @struct Stmt
/// @brief The abstract class used to define stmts
struct Stmt {
  virtual ~Stmt() = default;

  /**
   * @brief Method used to emply the visitor pattern
   * All derived classes must implement this method
   * @param visitor The expression visitor
   * @return The appropiate visitor method
   */
  virtual llvm::Value* Accept(StmtVisitor& visitor) = 0;

  /**
   * @brief Method to return the string representation of the node
   * @return The string represention
   */
  virtual std::string ToString() = 0;
};

/// @struct ModuleStmt
/// @brief The top level statement in a program
struct ModuleStmt : Stmt {
  Token name;                               /**< Name of the module */
  std::vector<std::unique_ptr<Stmt>> stmts; /**< Statements in the module */

  ModuleStmt(Token name, std::vector<std::unique_ptr<Stmt>> stmts);

  /**
   * @brief Method used to emply the visitor pattern
   * All derived classes must implement this method
   * @param visitor The expression visitor
   * @return The appropiate visitor method
   */
  llvm::Value* Accept(StmtVisitor& visitor) override;

  /**
   * @brief Method to return the string representation of the node
   * @return The string represention
   */
  std::string ToString() override;
};

/// @struct ExpressionStmt
/// @brief Basic expression statement node
struct ExpressionStmt : Stmt {
  std::unique_ptr<Expr> expr; /*< The underlying expression to evalute */

  ExpressionStmt(std::unique_ptr<Expr> expr);

  /**
   * @brief Method used to emply the visitor pattern
   * All derived classes must implement this method
   * @param visitor The expression visitor
   * @return The appropiate visitor method
   */
  llvm::Value* Accept(StmtVisitor& visitor) override;

  /**
   * @brief Method to return the string representation of the node
   * @return The string represention
   */
  std::string ToString() override;
};

/// @struct FunctionProto
/// @brief Function prototype node
struct FunctionProto : Stmt {
  Token name;                /**< The name of the function */
  Token return_type;         /**< The return type of the method */
  std::vector<FuncArg> args; /**< The function arguments */
  bool is_variadic;
  types::Type* resolved_type;

  FunctionProto(Token name, Token return_type, std::vector<FuncArg> args,
                bool is_variadic);

  /**
   * @brief Method used to emply the visitor pattern
   * All derived classes must implement this method
   * @param visitor The expression visitor
   * @return The appropiate visitor method
   */
  llvm::Value* Accept(StmtVisitor& visitor) override;

  /**
   * @brief Method to return the string representation of the node
   * @return The string represention
   */
  std::string ToString() override;
};

/// @struct FunctionStmt
/// @brief Function statement node
struct FunctionStmt : Stmt {
  std::unique_ptr<Stmt> proto;
  std::vector<std::unique_ptr<Stmt>> body;

  FunctionStmt(std::unique_ptr<Stmt> proto,
               std::vector<std::unique_ptr<Stmt>> body);

  /**
   * @brief Method used to emply the visitor pattern
   * All derived classes must implement this method
   * @param visitor The expression visitor
   * @return The appropiate visitor method
   */
  llvm::Value* Accept(StmtVisitor& visitor) override;

  /**
   * @brief Method to return the string representation of the node
   * @return The string represention
   */
  std::string ToString() override;
};

/// @struct ReturnStmt
/// @brief Return stmt node
struct ReturnStmt : Stmt {
  Token ret_token;
  std::unique_ptr<Expr> value;

  ReturnStmt(Token ret_toke, std::unique_ptr<Expr> value);

  /**
   * @brief Method used to emply the visitor pattern
   * All derived classes must implement this method
   * @param visitor The expression visitor
   * @return The appropiate visitor method
   */
  llvm::Value* Accept(StmtVisitor& visitor) override;

  /**
   * @brief Method to return the string representation of the node
   * @return The string represention
   */
  std::string ToString() override;
};

/// @struct VarDeclarationStmt
/// @brief Variable declaration node
struct VarDeclarationStmt : Stmt {
  Token type;
  Token name;
  std::unique_ptr<Expr> value;

  VarDeclarationStmt(Token type, Token name, std::unique_ptr<Expr> value);

  /**
   * @brief Method used to emply the visitor pattern
   * All derived classes must implement this method
   * @param visitor The expression visitor
   * @return The appropiate visitor method
   */
  llvm::Value* Accept(StmtVisitor& visitor) override;

  /**
   * @brief Method to return the string representation of the node
   * @return The string represention
   */
  std::string ToString() override;
};

/// @IfStmt
/// @brief If statement node
struct IfStmt : Stmt {
  std::unique_ptr<Expr> cond;
  std::unique_ptr<Stmt> then;
  std::unique_ptr<Stmt> otherwise;

  IfStmt(std::unique_ptr<Expr> cond, std::unique_ptr<Stmt> then,
         std::unique_ptr<Stmt> otherwise);

  /**
   * @brief Method used to emply the visitor pattern
   * All derived classes must implement this method
   * @param visitor The expression visitor
   * @return The appropiate visitor method
   */
  llvm::Value* Accept(StmtVisitor& visitor) override;

  /**
   * @brief Method to return the string representation of the node
   * @return The string represention
   */
  std::string ToString() override;
};

/// @struct ForStmt
/// @brief For statement node
struct ForStmt : Stmt {
  std::unique_ptr<Stmt> initializer;
  std::unique_ptr<Expr> condition;
  std::unique_ptr<Expr> step;
  std::vector<std::unique_ptr<Stmt>> body;

  ForStmt(std::unique_ptr<Stmt> initializer, std::unique_ptr<Expr> condition,
          std::unique_ptr<Expr> step, std::vector<std::unique_ptr<Stmt>> body);

  /**
   * @brief Method used to emply the visitor pattern
   * All derived classes must implement this method
   * @param visitor The expression visitor
   * @return The appropiate visitor method
   */
  llvm::Value* Accept(StmtVisitor& visitor) override;

  /**
   * @brief Method to return the string representation of the node
   * @return The string represention
   */
  std::string ToString() override;
};

/// @struct WhileStmt
/// @brief While stmt node
struct WhileStmt : Stmt {
  std::unique_ptr<Expr> condition;
  std::vector<std::unique_ptr<Stmt>> body;

  WhileStmt(std::unique_ptr<Expr> condition,
            std::vector<std::unique_ptr<Stmt>> body);

  llvm::Value* Accept(StmtVisitor& visitor) override;
  std::string ToString() override;
};

#endif
