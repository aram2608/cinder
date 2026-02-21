#ifndef COMPILER_H_
#define COMPILER_H_

#include <memory>
#include <unordered_map>
#include <vector>

#include "cinder/ast/expr/expr.hpp"
#include "cinder/ast/stmt/stmt.hpp"
#include "cinder/ast/types.hpp"
#include "cinder/codegen/codegen_bindings.hpp"
#include "cinder/codegen/codegen_context.hpp"
#include "cinder/codegen/codegen_opts.hpp"
#include "cinder/driver/clang_driver.hpp"
#include "cinder/semantic/semantic_analyzer.hpp"
#include "cinder/semantic/type_context.hpp"
#include "cinder/support/diagnostic.hpp"
#include "cinder/support/environment.hpp"
#include "llvm/BinaryFormat/Dwarf.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/DIBuilder.h"
#include "llvm/IR/DebugInfoMetadata.h"
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
 * @brief LLVM code generation driver for a parsed module AST.
 *
 * `Codegen` runs semantic analysis, lowers AST nodes to LLVM IR through
 * visitor dispatch, and emits/links output according to `CodegenOpts`.
 */
struct Codegen : CodegenExprVisitor, StmtVisitor {
  std::vector<ModuleStmt*> modules_; /**< Dependency-ordered modules. */

  CodegenOpts opts; /**< Backend options. */
  std::unique_ptr<CodegenContext>
      ctx_;                /**< Owned LLVM context and module state. */
  BindingMap ir_bindings_; /**< Symbol-to-IR binding table. */
  std::unordered_map<std::string, llvm::StructType*> struct_types_;
  std::unordered_map<SymbolId, llvm::DILocalVariable*> di_locals_;
  // std::unique_ptr<llvm::DIBuilder> di_builder_;
  // llvm::DICompileUnit* di_compile_unit_ = nullptr;
  // llvm::DIFile* di_file_ = nullptr;
  // llvm::DIScope* di_scope_ = nullptr;
  DiagnosticEngine diagnose_; /**< Internal diagnostic reporter. */
  TypeContext types_;         /**< Canonical semantic types. */
  SemanticAnalyzer pass_;     /**< Semantic analysis pass. */

  /**
   * @brief Creates a codegen driver for parsed modules.
   * @param modules Module AST nodes to lower.
   * @param opts Backend options.
   */
  Codegen(std::vector<ModuleStmt*> modules, CodegenOpts opts);

  /** @brief Runs full backend flow according to configured mode. */
  bool Generate();
  /** @brief Lowers AST into in-memory LLVM IR. */
  void GenerateIR();
  /** @brief Writes LLVM IR text to `opts.out_path`. */
  void EmitLLVM();
  /** @brief Compiles and executes output (currently not implemented). */
  void CompileRun();
  /** @brief Emits object code and links final binary. */
  void CompileBinary(llvm::TargetMachine* target_machine);

  using CodegenExprVisitor::Visit;
  using StmtVisitor::Visit;

  /** @name Statement visitor overrides */
  ///@{
  llvm::Value* Visit(ModuleStmt& stmt) override;
  llvm::Value* Visit(ImportStmt& stmt) override;
  llvm::Value* Visit(ForStmt& stmt) override;
  llvm::Value* Visit(WhileStmt& stmt) override;
  llvm::Value* Visit(IfStmt& stmt) override;
  llvm::Value* Visit(ExpressionStmt& stmt) override;
  llvm::Value* Visit(FunctionStmt& stmt) override;
  llvm::Value* Visit(FunctionProto& stmt) override;
  llvm::Value* Visit(ReturnStmt& stmt) override;
  llvm::Value* Visit(VarDeclarationStmt& stmt) override;
  llvm::Value* Visit(StructStmt& stmt) override;
  ///@}

  /** @name Expression visitor overrides */
  ///@{
  llvm::Value* Visit(Assign& expr) override;
  llvm::Value* Visit(MemberAssign& expr) override;
  llvm::Value* Visit(Conditional& expr) override;
  llvm::Value* Visit(Binary& expr) override;
  llvm::Value* Visit(PreFixOp& expr) override;
  llvm::Value* Visit(CallExpr& expr) override;
  llvm::Value* Visit(MemberAccess& expr) override;
  llvm::Value* Visit(Grouping& expr) override;
  llvm::Value* Visit(Variable& expr) override;
  llvm::Value* Visit(Literal& expr) override;
  ///@}

  /**
   * @brief Runs semantic analysis and reports diagnostics on failure.
   * @param modules Module AST nodes.
   * @return `true` on success, `false` when semantic errors are present.
   */
  bool SemanticPass(const std::vector<ModuleStmt*>& modules);

  /** @brief Emits an integer constant literal value. */
  llvm::Value* EmitInteger(Literal& expr);

  llvm::Type* ResolveType(cinder::types::Type* type, bool allow_void);
  /** @brief Maps semantic function-argument types to LLVM types. */
  llvm::Type* ResolveArgType(cinder::types::Type* type);
  /** @brief Maps semantic types to LLVM storage/value types. */
  llvm::Type* ResolveType(cinder::types::Type* type);

  void InitAllTargets();
};

#endif
