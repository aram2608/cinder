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
#include "llvm/Linker/Linker.h"
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
#include "llvm/IR/DIBuilder.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/StandardInstrumentations.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Transforms/InstCombine/InstCombine.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/Scalar/GVN.h"
#include "llvm/Transforms/Scalar/Reassociate.h"
#include "llvm/Transforms/Scalar/SimplifyCFG.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetOptions.h"
#include "llvm/TargetParser/Host.h"
#include "llvm/IR/LegacyPassManager.h"

/// @enum @class CompilerMode
/// @brief The compilation mode, whether to emit LLVM, run, or compile
enum class CompilerMode {
  EMIT_LLVM,
  RUN,
  COMPILE,
};

/**
 * @struct CompilerOptions
 * @brief A simple data container for the compiler options
 * Populated during CLI argument parse time and used during the final stages of
 * compilation.
 */
struct CompilerOptions {
  std::string out_path; /**< The desired outpath */
  CompilerMode mode;    /** The compiler mode, can emit-llvm, run, or compile */
  std::string linker_flags; /**< Flags to pass to the clang linker */
  bool debug_info; /**< TODO: Implement debug information, its a bit of a ritual
                    */
  CompilerOptions(std::string out_path, CompilerMode mode, bool debug_info,
                  std::vector<std::string> linker_flags_list);
};

/**
 * @struct Compiler
 * @brief The main compiler structure
 * Employs the visitor method to resolve the different nodes of the AST
 */
struct Compiler : ExprVisitor, StmtVisitor {
  std::unique_ptr<Stmt>
      mod; /**< TODO: make this a vector or a map of mods to handle imports */
  std::unordered_map<std::string, llvm::AllocaInst*>
      symbol_table; /**< Symbol table to store local vars */
  std::unordered_map<std::string, llvm::Argument*>
      argument_table; /**< Table to store function arguments */
  /// TODO: Fix function arity, currently we store it in the table so we can
  /// check at call time but that is restrictive for variadic funcs. I think I
  /// can probably just ignore the arity and have clang yell at the user
  std::unordered_map<std::string, size_t>
      func_table;       /**< Table to store the funcs in a module */
  CompilerOptions opts; /**< The compiler options */
  std::unique_ptr<llvm::LLVMContext>
      context; /**< TODO: Let the compiler store the context */

  Compiler(std::unique_ptr<Stmt> mod, CompilerOptions opts);

  void AddPrintf();
  bool Compile();
  void GenerateIR();
  void EmitLLVM();
  void CompileRun();
  void CompileBinary(llvm::TargetMachine* target_machine);

  llvm::Value* VisitModuleStmt(ModuleStmt& stmt) override;
  llvm::Value* VisitForStmt(ForStmt& stmt) override;
  llvm::Value* VisitIfStmt(IfStmt& stmt) override;
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

  std::unique_ptr<llvm::Module> CreateModule(ModuleStmt& stmt,
                                             llvm::LLVMContext& ctx);
};

#endif