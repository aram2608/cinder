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

struct ExpressionStmt;
struct FunctionStmt;
struct FunctionProto;
struct ReturnStmt;
struct VarDeclarationStmt;

struct StmtVisitor {
  virtual ~StmtVisitor() = default;

  virtual llvm::Value* VisitExpressionStmt(ExpressionStmt& stmt) = 0;
  virtual llvm::Value* VisitFunctionStmt(FunctionStmt& stmt) = 0;
  virtual llvm::Value* VisitReturnStmt(ReturnStmt& stmt) = 0;
  virtual llvm::Value* VisitVarDeclarationStmt(VarDeclarationStmt& stmt) = 0;
  virtual llvm::Value* VisitFunctionProto(FunctionProto& stmt) = 0;
};

struct Stmt {
  virtual ~Stmt();
  virtual llvm::Value* Accept(StmtVisitor& visitor) = 0;
  virtual std::string ToString() = 0;
};

struct ExpressionStmt : Stmt {
  std::unique_ptr<Expr> expr;

  ExpressionStmt(std::unique_ptr<Expr> expr);
  llvm::Value* Accept(StmtVisitor& visitor) override;
  std::string ToString() override;
};

struct FunctionProto : Stmt {
  Token name;
  Token return_type;
  std::vector<FuncArg> args;

  FunctionProto(Token name, Token return_type, std::vector<FuncArg> args);
  llvm::Value* Accept(StmtVisitor& visitor) override;
  std::string ToString() override;
};

struct FunctionStmt : Stmt {
  std::unique_ptr<Stmt> proto;
  std::vector<std::unique_ptr<Stmt>> body;

  FunctionStmt(std::unique_ptr<Stmt> proto,
               std::vector<std::unique_ptr<Stmt>> body);
  llvm::Value* Accept(StmtVisitor& visitor) override;
  std::string ToString() override;
};

struct ReturnStmt : Stmt {
  std::unique_ptr<Expr> value;

  ReturnStmt(std::unique_ptr<Expr> value);
  llvm::Value* Accept(StmtVisitor& visitor) override;
  std::string ToString() override;
};

struct VarDeclarationStmt : Stmt {
  Token type;
  Token name;
  std::unique_ptr<Expr> value;

  VarDeclarationStmt(Token type, Token name, std::unique_ptr<Expr> value);
  llvm::Value* Accept(StmtVisitor& visitor) override;
  std::string ToString() override;
};

#endif