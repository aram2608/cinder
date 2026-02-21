#ifndef DEBUG_INFO_CONTEXT_H_
#define DEBUG_INFO_CONTEXT_H_

#include <memory>
#include <optional>
#include <vector>

#include "cinder/ast/stmt/stmt.hpp"
#include "cinder/ast/types.hpp"
#include "cinder/frontend/tokens.hpp"
#include "llvm/IR/DIBuilder.h"
#include "llvm/IR/DebugInfoMetadata.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"

class DebugInfoContext {
  llvm::LLVMContext& llvm_ctx_;
  llvm::IRBuilder<>& builder_;
  std::unique_ptr<llvm::DIBuilder> di_builder_;
  llvm::DICompileUnit* di_compile_unit_ = nullptr;
  llvm::DIFile* di_file_ = nullptr;
  llvm::DIScope* di_scope_ = nullptr;

 public:
  DebugInfoContext(llvm::LLVMContext& llvm_ctx, llvm::IRBuilder<>& builder);

  void Init(bool debug, llvm::Module& module,
            std::vector<ModuleStmt*>& modules);
  void Finalize();

  llvm::DIBuilder* GetBuilder();
  llvm::DIFile* GetFile();
  llvm::DIScope* GetScope();
  void SetScope(llvm::DIScope* scope);

  llvm::DIType* ResolveType(cinder::types::Type* type);
  void SetLocation(const cinder::SourceLocation& loc);
  void EmitValue(llvm::Value* value, llvm::DILocalVariable* variable,
                 const cinder::SourceLocation& loc);
  void CreateLexicalBlock(std::optional<cinder::SourceLocation> loc);
};

#endif
