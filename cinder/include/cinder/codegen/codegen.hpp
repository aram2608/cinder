#ifndef COMPILER_H_
#define COMPILER_H_

#include <memory>

#include "cinder/ast/expr.hpp"
#include "cinder/ast/stmt.hpp"
#include "cinder/ast/types.hpp"
#include "cinder/codegen/codegen_bindings.hpp"
#include "cinder/codegen/codegen_context.hpp"
#include "cinder/codegen/codegen_opts.hpp"
#include "cinder/semantic/semantic_analyzer.hpp"
#include "cinder/semantic/type_context.hpp"
#include "cinder/support/diagnostic.hpp"
#include "cinder/support/environment.hpp"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/DIBuilder.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/PassManager.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Value.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Linker/Linker.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/StandardInstrumentations.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Transforms/Scalar/GVN.h"
#include "llvm/Transforms/Scalar/Reassociate.h"

/**
 * @struct Compiler
 * @brief The main compiler structure
 * Employs the visitor method to resolve the different nodes of the AST
 */
struct Codegen : ExprVisitor, StmtVisitor {
  /// TODO:  make this a vector or a map of mods to handle imports
  std::unique_ptr<Stmt> mod; /**< module */

  CodegenOpts opts;                     /**< The compiler options */
  std::unique_ptr<CodegenContext> ctx_; /**< Codegen ctx, stores IR objs */
  BindingMap ir_bindings_;              /**< Stores the llvm IR instructions */
  DiagnosticEngine diagnose_;           /**< Engine to handle erros/warnings/ */
  TypeContext types_;                   /**< Stores type information */
  SemanticAnalyzer pass_;               /**< Semantic analyzer */

  Codegen(std::unique_ptr<Stmt> mod, CodegenOpts opts);

  void AddPrintf();
  bool Generate();
  void GenerateIR();
  void EmitLLVM();
  void CompileRun();
  void CompileBinary(llvm::TargetMachine* target_machine);

  using ExprVisitor::Visit;
  using StmtVisitor::Visit;

  // Statements
  llvm::Value* Visit(ModuleStmt& stmt) override;
  llvm::Value* Visit(ForStmt& stmt) override;
  llvm::Value* Visit(WhileStmt& stmt) override;
  llvm::Value* Visit(IfStmt& stmt) override;
  llvm::Value* Visit(ExpressionStmt& stmt) override;
  llvm::Value* Visit(FunctionStmt& stmt) override;
  llvm::Value* Visit(FunctionProto& stmt) override;
  llvm::Value* Visit(ReturnStmt& stmt) override;
  llvm::Value* Visit(VarDeclarationStmt& stmt) override;

  // Expressions
  llvm::Value* Visit(Assign& expr) override;
  llvm::Value* Visit(Conditional& expr) override;
  llvm::Value* Visit(Binary& expr) override;
  llvm::Value* Visit(PreFixOp& expr) override;
  llvm::Value* Visit(CallExpr& expr) override;
  llvm::Value* Visit(Grouping& expr) override;
  llvm::Value* Visit(Variable& expr) override;
  llvm::Value* Visit(Literal& expr) override;

  void SemanticPass(ModuleStmt& mod);

  llvm::Value* EmitInteger(Literal& expr);

  llvm::Type* ResolveArgType(types::Type* type);
  llvm::Type* ResolveType(types::Type* type);
};

#endif
