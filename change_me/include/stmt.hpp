#ifndef STMT_H_
#define STMT_H_

#include "common.hpp"
#include "expr.hpp"
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

struct ModuleStmt;
struct ExpressionStmt;
struct FunctionStmt;
struct FunctionProto;
struct ReturnStmt;
struct VarDeclarationStmt;
struct ForStmt;

struct StmtVisitor {
  virtual ~StmtVisitor() = default;

  virtual llvm::Value* VisitExpressionStmt(ExpressionStmt& stmt) = 0;
  virtual llvm::Value* VisitFunctionStmt(FunctionStmt& stmt) = 0;
  virtual llvm::Value* VisitReturnStmt(ReturnStmt& stmt) = 0;
  virtual llvm::Value* VisitVarDeclarationStmt(VarDeclarationStmt& stmt) = 0;
  virtual llvm::Value* VisitFunctionProto(FunctionProto& stmt) = 0;
  virtual llvm::Value* VisitModuleStmt(ModuleStmt& stmt) = 0;
  // virtual llvm::Value* VisitForStmt(ForStmt& stmt) = 0;
};

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

struct ModuleStmt : Stmt {
  Token name;
  std::vector<std::unique_ptr<Stmt>> stmts;

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

struct ExpressionStmt : Stmt {
  std::unique_ptr<Expr> expr;

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

struct FunctionProto : Stmt {
  Token name;
  Token return_type;
  std::vector<FuncArg> args;

  FunctionProto(Token name, Token return_type, std::vector<FuncArg> args);

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

struct ReturnStmt : Stmt {
  std::unique_ptr<Expr> value;

  ReturnStmt(std::unique_ptr<Expr> value);

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

struct ForStmt : Stmt {
  std::unique_ptr<Stmt> intializer;
  std::unique_ptr<Stmt> condition;
  std::unique_ptr<Expr> increment;
  std::vector<std::unique_ptr<Stmt>> body;

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

#endif