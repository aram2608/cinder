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
#include "llvm/IR/Verifier.h"
#include "llvm/Support/FileSystem.h"
#include <llvm/Linker/Linker.h>
#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/PassManager.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/StandardInstrumentations.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Transforms/InstCombine/InstCombine.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/Scalar/GVN.h"
#include "llvm/Transforms/Scalar/Reassociate.h"
#include "llvm/Transforms/Scalar/SimplifyCFG.h"

struct Compiler : ExprVisitor, StmtVisitor {
  std::unique_ptr<Stmt> mod;
  std::unordered_map<std::string, llvm::AllocaInst*> symbol_table;
  std::unordered_map<std::string, llvm::Argument*> argument_table;
  std::unordered_map<std::string, size_t> func_table;
  std::string out_path;
  std::string compiled_program;
  bool emit_llvm;
  bool run;
  bool compile;

  Compiler(std::unique_ptr<Stmt> mod, std::string out_path, bool emit_llvm,
           bool run, bool compile);

  void Compile();

  llvm::Value* VisitModuleStmt(ModuleStmt& stmt) override;
  llvm::Value* VisitExpressionStmt(ExpressionStmt& stmt) override;
  llvm::Value* VisitFunctionStmt(FunctionStmt& stmt) override;
  llvm::Value* VisitFunctionProto(FunctionProto& stmt) override;
  llvm::Value* VisitReturnStmt(ReturnStmt& stmt) override;
  llvm::Value* VisitVarDeclarationStmt(VarDeclarationStmt& stmt) override;

  llvm::Value* VisitAssignment(Assign& expr) override;
  llvm::Value* VisitConditional(Conditional& expr) override;
  llvm::Value* VisitBinary(Binary& expr) override;
  llvm::Value* VisitPreIncrement(PreFixOp& expr) override;
  llvm::Value* VisitCall(CallExpr& expr) override;
  llvm::Value* VisitGrouping(Grouping& expr) override;
  llvm::Value* VisitVariable(Variable& expr) override;
  llvm::Value* VisitBoolean(BoolLiteral& expr) override;
  llvm::Value* VisitLiteral(Literal& expr) override;
};

#endif