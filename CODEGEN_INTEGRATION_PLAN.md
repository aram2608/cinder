# Integrating Recent Semantic Additions Into Codegen

## What changed recently

From `semantic/` and `codegen/`, the important new pieces are:

- `SemanticEnv` (`semantic_env.hpp/.cpp`): lexical scopes map names to `SymbolId`.
- `SemanticSymbols` (`symbol.hpp/.cpp`): global symbol storage (`SymbolInfo`) keyed by `SymbolId`.
- `CodegenContext` (`codegen_context.hpp/.cpp`): owns LLVM context/module/builder.
- `BindingMap` (`codegen_bindings.hpp`): maps `SymbolId -> Binding` (`alloca_ptr`, `value`, `function`).

These parts are directionally compatible, but they are not wired together yet.

## Current gap

Codegen still largely uses legacy/static state and name-based assumptions, while
semantic now resolves identity using `SymbolId`.

Key mismatch:

- semantic resolves `name -> SymbolId -> SymbolInfo`
- codegen bindings are prepared for `SymbolId`
- AST currently does not carry symbol identity from semantic to codegen

Result: codegen cannot reliably use the new `BindingMap` yet.

## Recommended integration path

### 1) Carry symbol identity on AST nodes during semantic analysis

Add `SymbolId` fields (or optional IDs) to nodes that need lookup in codegen:

- `Variable`
- `Assign`
- `VarDeclarationStmt`
- `FunctionProto`
- optionally function args (`FuncArg`) if you want direct parameter binding

During semantic pass, assign IDs when resolving/declaring symbols.

Why: this removes string lookups during codegen and gives stable identity across
shadowed scopes.

### 2) Expose semantic results to codegen

Provide a small semantic result object from `SemanticAnalyzer`, e.g.:

- diagnostics status
- optional accessor to `SymbolInfo` by `SymbolId`

At minimum, codegen needs the `SymbolId` written into AST and a way to fail fast
if semantic errors exist.

### 3) Replace compiler static LLVM globals with `CodegenContext`

In `Compiler`, add a member:

- `std::unique_ptr<CodegenContext> cg_`

Then replace uses of:

- `TheContext` -> `cg_->GetContext()`
- `TheModule` -> `cg_->GetModule()`
- `Builder` -> `cg_->GetBuilder()`

Why: this aligns codegen with the new architecture and avoids global mutable
state.

### 4) Make `BindingMap` the single source of truth for generated values

Store bindings in `CodegenContext` keyed by `SymbolId`.

Suggested usage:

- `VarDeclarationStmt`: create alloca + store init value, then bind by var ID
- `Variable`: load from `bindings[var_id].alloca_ptr` (or return constant/value)
- `Assign`: store into `bindings[var_id].alloca_ptr`
- `FunctionProto`: create `llvm::Function`, bind by function ID
- `CallExpr`: resolve callee via function symbol ID, not by name string

### 5) Add lightweight codegen scope tracking

`BindingMap` alone is flat. Add scope ownership tracking to clean up locals after
blocks:

- stack of vectors: `std::vector<std::vector<SymbolId>> codegen_scopes_`
- on declaration, push ID to current scope
- on scope exit, erase those IDs from `BindingMap`

This mirrors semantic scoping behavior and prevents accidental stale captures.

### 6) Restore end-to-end statement lowering order

`Compiler::Visit(ModuleStmt&)` currently initializes state but does not lower all
statements. Re-enable traversal so IR is actually emitted.

Also ensure currently stubbed methods return values safely (`FunctionStmt`,
`FunctionProto`, `Variable`, `Assign`, `PreFixOp`) before relying on them.

## Suggested implementation sequence

1. Add AST `SymbolId` fields and populate them in semantic pass.
2. Stop codegen when semantic diagnostics report errors.
3. Introduce `CodegenContext` member in `Compiler` and remove static LLVM globals.
4. Implement symbol-ID-based bindings for var/function declaration and lookup.
5. Re-enable module/function body traversal and verify IR output.
6. Add scope cleanup for bindings and verify shadowing behavior.

## Fast verification checklist

- `def f(int32: x) -> int32 return x; end` returns loaded parameter value.
- Local shadowing emits correct loads/stores by distinct `SymbolId`s.
- Calls resolve via function symbol IDs (including extern like `printf`).
- `--emit-llvm` prints valid module and `verifyFunction`/`verifyModule` pass.
- Semantic error prevents codegen emission.

## Minimal data flow target

```text
Parser AST
  -> SemanticAnalyzer assigns types + SymbolId on nodes
  -> Compiler/codegen reads SymbolId
  -> CodegenContext::bindings[SymbolId] stores LLVM values/allocas/functions
  -> IR emission with stable scoped symbol identity
```
