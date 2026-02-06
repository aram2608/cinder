# Snippets for Architecture Refactors

This file contains implementation snippets for the architecture items in
`TODO.md`, with emphasis on a cleaner semantic-analysis-to-codegen pipeline.

## 1) Phase-Oriented Compilation Pipeline

```cpp
// pipeline.hpp
#pragma once

#include <memory>

#include "ast.hpp"
#include "diagnostics.hpp"
#include "ir_codegen.hpp"
#include "lexer.hpp"
#include "parser.hpp"
#include "semantic_analyzer.hpp"

struct PipelineResult {
  std::unique_ptr<ModuleStmt> ast;
  bool ok = false;
};

class Pipeline {
 public:
  explicit Pipeline(DiagnosticEngine* diag) : diag_(diag) {}

  PipelineResult Run(const std::string& source) {
    Lexer lexer{source};
    auto tokens = lexer.ScanTokens(diag_);
    if (diag_->HasErrors()) {
      return {};
    }

    Parser parser{tokens};
    auto ast = parser.Parse(diag_);
    if (!ast || diag_->HasErrors()) {
      return {};
    }

    SemanticAnalyzer sem{diag_};
    sem.Analyze(*ast);
    if (diag_->HasErrors()) {
      return {};
    }

    return {.ast = std::move(ast), .ok = true};
  }

 private:
  DiagnosticEngine* diag_;
};
```

## 2) Non-Fatal Diagnostics Instead of `exit(1)`

```cpp
// diagnostics.hpp
#pragma once

#include <string>
#include <vector>

struct SourceLoc {
  size_t line = 0;
  size_t column = 0;
};

struct Diagnostic {
  enum class Severity { Error, Warning } severity;
  std::string message;
  SourceLoc loc;
};

class DiagnosticEngine {
 public:
  void Error(SourceLoc loc, std::string msg) {
    diags_.push_back({Diagnostic::Severity::Error, std::move(msg), loc});
    error_count_++;
  }

  void Warning(SourceLoc loc, std::string msg) {
    diags_.push_back({Diagnostic::Severity::Warning, std::move(msg), loc});
  }

  bool HasErrors() const { return error_count_ > 0; }
  const std::vector<Diagnostic>& All() const { return diags_; }

 private:
  size_t error_count_ = 0;
  std::vector<Diagnostic> diags_;
};
```

## 3) Semantic Analyzer Should Not Return `llvm::Value*`

```cpp
// semantic_analyzer.hpp
#pragma once

#include "diagnostics.hpp"
#include "expr.hpp"
#include "semantic_scope.hpp"
#include "stmt.hpp"
#include "type_context.hpp"

class SemanticAnalyzer : ExprVisitor, StmtVisitor {
 public:
  explicit SemanticAnalyzer(DiagnosticEngine* diag)
      : diag_(diag), scope_(nullptr), current_return_(nullptr) {}

  void Analyze(ModuleStmt& mod) {
    Visit(mod);
  }

 private:
  // Statements
  void Visit(ModuleStmt& stmt) override;
  void Visit(FunctionStmt& stmt) override;
  void Visit(FunctionProto& stmt) override;
  void Visit(ReturnStmt& stmt) override;
  void Visit(IfStmt& stmt) override;
  void Visit(ForStmt& stmt) override;
  void Visit(WhileStmt& stmt) override;
  void Visit(VarDeclarationStmt& stmt) override;
  void Visit(ExpressionStmt& stmt) override;

  // Expressions
  void Visit(Variable& expr) override;
  void Visit(Binary& expr) override;
  void Visit(Assign& expr) override;
  void Visit(Grouping& expr) override;
  void Visit(Conditional& expr) override;
  void Visit(PreFixOp& expr) override;
  void Visit(CallExpr& expr) override;
  void Visit(Literal& expr) override;

  TypeContext tc_;
  DiagnosticEngine* diag_;
  std::shared_ptr<Scope> scope_;
  types::Type* current_return_;
};
```

## 4) Stable Type Ownership + Interning

```cpp
// type_context.hpp
#pragma once

#include <memory>
#include <vector>

#include "types.hpp"

class TypeContext {
 public:
  types::IntType* Int32() { return &int32_; }
  types::IntType* Int64() { return &int64_; }
  types::FloatType* Float32() { return &flt32_; }
  types::BoolType* Bool() { return &bool_; }
  types::Type* Void() { return &void_; }
  types::StringType* String() { return &str_; }

  types::FunctionType* Function(types::Type* ret,
                                std::vector<types::Type*> params,
                                bool variadic) {
    function_pool_.push_back(std::make_unique<types::FunctionType>(
        ret, std::move(params), variadic));
    return function_pool_.back().get();
  }

 private:
  types::IntType int32_{32, true};
  types::IntType int64_{64, true};
  types::FloatType flt32_{32};
  types::BoolType bool_{1};
  types::Type void_{types::TypeKind::Void};
  types::StringType str_{};

  std::vector<std::unique_ptr<types::FunctionType>> function_pool_;
};
```

## 5) Structural Type Compatibility Helper

```cpp
// type_utils.hpp
#pragma once

#include "types.hpp"

inline bool SameType(const types::Type* a, const types::Type* b) {
  if (!a || !b) return false;
  if (a->kind != b->kind) return false;

  if (a->kind == types::TypeKind::Int) {
    auto* ia = static_cast<const types::IntType*>(a);
    auto* ib = static_cast<const types::IntType*>(b);
    return ia->bits == ib->bits && ia->is_signed == ib->is_signed;
  }

  if (a->kind == types::TypeKind::Function) {
    auto* fa = static_cast<const types::FunctionType*>(a);
    auto* fb = static_cast<const types::FunctionType*>(b);
    if (!SameType(fa->return_type, fb->return_type)) return false;
    if (fa->is_variadic != fb->is_variadic) return false;
    if (fa->params.size() != fb->params.size()) return false;
    for (size_t i = 0; i < fa->params.size(); i++) {
      if (!SameType(fa->params[i], fb->params[i])) return false;
    }
    return true;
  }

  return true;
}
```

## 6) Single Symbol Identity Across Phases

```cpp
// symbol.hpp
#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "types.hpp"

using SymbolId = uint32_t;

struct SymbolInfo {
  SymbolId id;
  std::string name;
  types::Type* type;
  bool is_function = false;
};

class SemanticSymbols {
 public:
  SymbolId Declare(std::string name, types::Type* type, bool is_function) {
    SymbolId id = static_cast<SymbolId>(symbols_.size());
    symbols_.push_back({id, name, type, is_function});
    table_[name] = id;
    return id;
  }

  SymbolInfo* Lookup(const std::string& name) {
    auto it = table_.find(name);
    if (it == table_.end()) return nullptr;
    return &symbols_[it->second];
  }

 private:
  std::vector<SymbolInfo> symbols_;
  std::unordered_map<std::string, SymbolId> table_;
};
```

```cpp
// codegen_bindings.hpp
#pragma once

#include <unordered_map>

#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"

#include "symbol.hpp"

struct Binding {
  llvm::AllocaInst* alloca_ptr = nullptr;
  llvm::Value* value = nullptr;
  llvm::Function* function = nullptr;
};

using BindingMap = std::unordered_map<SymbolId, Binding>;
```

## 7) Semantic Annotation on AST

```cpp
// expr.hpp (conceptual extension)
struct Expr {
  types::Type* type = nullptr;
  SymbolId resolved_symbol = static_cast<SymbolId>(-1);
  virtual ~Expr() = default;
};

// In semantic pass:
void SemanticAnalyzer::Visit(Variable& expr) {
  SymbolInfo* sym = symbols_.Lookup(expr.name.lexeme);
  if (!sym) {
    diag_->Error({expr.name.line_num, 0}, "undefined variable: " + expr.name.lexeme);
    return;
  }
  expr.type = sym->type;
  expr.resolved_symbol = sym->id;
}
```

## 8) Codegen Context (No Global LLVM Statics)

```cpp
// codegen_context.hpp
#pragma once

#include <memory>

#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"

#include "codegen_bindings.hpp"

struct CodegenContext {
  std::unique_ptr<llvm::LLVMContext> llvm_ctx;
  std::unique_ptr<llvm::Module> module;
  std::unique_ptr<llvm::IRBuilder<>> builder;
  BindingMap bindings;

  explicit CodegenContext(const std::string& module_name)
      : llvm_ctx(std::make_unique<llvm::LLVMContext>()),
        module(std::make_unique<llvm::Module>(module_name, *llvm_ctx)),
        builder(std::make_unique<llvm::IRBuilder<>>(*llvm_ctx)) {}
};
```

## 9) Function Args as Allocas (Consistent Mutability)

```cpp
// in Compiler::Visit(FunctionStmt&)
for (auto& arg : fn->args()) {
  llvm::AllocaInst* slot = builder->CreateAlloca(arg.getType(), nullptr,
                                                 std::string(arg.getName()));
  builder->CreateStore(&arg, slot);

  SymbolId id = /* from semantic annotation */;
  ctx.bindings[id] = Binding{.alloca_ptr = slot, .value = nullptr,
                             .function = nullptr};
}
```

## 10) Variable Read/Write Through Bindings

```cpp
llvm::Value* Compiler::Visit(Variable& expr) {
  Binding& b = ctx_.bindings.at(expr.resolved_symbol);
  if (b.alloca_ptr) {
    return ctx_.builder->CreateLoad(b.alloca_ptr->getAllocatedType(),
                                    b.alloca_ptr, "load." + expr.name.lexeme);
  }
  if (b.function) {
    return b.function;
  }
  return b.value;
}

llvm::Value* Compiler::Visit(Assign& expr) {
  Binding& b = ctx_.bindings.at(expr.resolved_symbol);
  llvm::Value* rhs = expr.value->Accept(*this);
  ctx_.builder->CreateStore(rhs, b.alloca_ptr);
  return rhs;
}
```

## 11) Safer Link Step with Exit-Code Checks

```cpp
bool LinkBinary(const std::string& object_path,
                const std::string& out_path,
                const std::vector<std::string>& linker_flags,
                DiagnosticEngine* diag) {
  std::string cmd = "clang \"" + object_path + "\"";
  for (const std::string& f : linker_flags) {
    cmd += " " + f;
  }
  cmd += " -o \"" + out_path + "\"";

  int rc = std::system(cmd.c_str());
  if (rc != 0) {
    diag->Error({0, 0}, "link step failed with code " + std::to_string(rc));
    return false;
  }
  return true;
}
```

## 12) Parser/lexer crash guards

```cpp
// parser.cpp
std::unique_ptr<Stmt> Parser::WhileStatement() {
  std::unique_ptr<Expr> condition = Expression();
  std::vector<std::unique_ptr<Stmt>> body;
  while (!CheckType(TT_END) && !IsEnd()) {
    body.push_back(Statement());
  }
  Consume(TT_END, "'end' expected after loop");
  return std::make_unique<WhileStmt>(std::move(condition), std::move(body));
}
```

```cpp
// lexer.cpp
char Lexer::PeekChar() {
  if (IsEnd()) return '\0';
  return source_str[current_pos];
}

char Lexer::PeekNextChar() {
  if (current_pos + 1 >= source_str.size()) return '\0';
  return source_str[current_pos + 1];
}

void Lexer::ParseComment() {
  while (!IsEnd() && PeekChar() != '\n') {
    Advance();
  }
}
```

## 13) Driver skeleton tying it together

```cpp
int CompileSource(const std::string& source,
                  const CompilerOptions& opts,
                  DiagnosticEngine* diag) {
  Pipeline pipeline{diag};
  PipelineResult result = pipeline.Run(source);
  if (!result.ok) {
    return 1;
  }

  CodeGenerator cg{diag};
  if (!cg.Generate(*result.ast, opts)) {
    return 1;
  }

  return diag->HasErrors() ? 1 : 0;
}
```

## 14) Migration strategy (incremental)

```text
Step 1: Add DiagnosticEngine, keep existing APIs, route fatal errors through it.
Step 2: Make SemanticAnalyzer return void (or semantic result), remove LLVM includes.
Step 3: Add SymbolId annotations on AST variables/calls.
Step 4: Introduce CodegenContext and remove file-level LLVM globals.
Step 5: Replace name lookups in codegen with SymbolId bindings.
Step 6: Add structural SameType checks and int64 support end-to-end.
```
