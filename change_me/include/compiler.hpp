#ifndef COMPILER_H_
#define COMPILER_H_

#include "common.hpp"
#include "expr.hpp"
#include "stmt.hpp"
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

struct Compiler : ExprVisitor, StmtVisitor {
  std::vector<std::unique_ptr<Stmt>> statements;
  std::unordered_map<std::string, llvm::Value*> symbol_table;

  Compiler(std::vector<std::unique_ptr<Stmt>> statements);

  void Compile();

  llvm::Value* VisitExpressionStmt(ExpressionStmt& stmt) override;
  llvm::Value* VisitFunctionStmt(FunctionStmt& stmt) override;
  llvm::Value* VisitReturnStmt(ReturnStmt& stmt) override;
  llvm::Value* VisitVarDeclarationStmt(VarDeclarationStmt& stmt) override;

  llvm::Value* VisitLiteral(Literal& expr) override;
  llvm::Value* VisitBoolean(Boolean& expr) override;
  llvm::Value* VisitVariable(Variable& expr) override;
  llvm::Value* VisitGrouping(Grouping& expr) override;
  llvm::Value* VisitBinary(Binary& expr) override;
};

#endif