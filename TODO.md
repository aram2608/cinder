# TODO (Architecture + Bug Audit)

This list is prioritized by impact (P0 highest). It combines architectural
issues and concrete bug spots found in `change_me/`.

## P0 - Crashes / correctness blockers

- [ ] **Fix parser loop EOF bug in while parsing**
  - `Parser::WhileStatement()` uses `while (!(CheckType(TT_END) && !IsEnd()))`.
  - At EOF this condition stays true and can recurse into invalid parse paths.
  - File: `change_me/src/parser.cpp:93`

- [ ] **Harden lexer bounds checks (`PeekChar` / `PeekNextChar`)**
  - `PeekChar()` directly indexes `source_str[current_pos]` with no `IsEnd()` guard.
  - `PeekNextChar()` checks `!IsEnd()` but still reads `current_pos + 1`, which is
    out-of-range at the last character.
  - `ParseComment()` calls `PeekChar()` before checking `IsEnd()`.
  - File: `change_me/src/lexer.cpp:169`, `change_me/src/lexer.cpp:173`, `change_me/src/lexer.cpp:207`

- [ ] **Fix `if` / `else` AST printing null deref regressions across AST nodes**
  - `ReturnStmt`/`IfStmt` stringification was partially fixed; audit all `ToString()`
    paths for nullable children and optional syntax forms.
  - File: `change_me/src/stmt.cpp`

- [ ] **Make codegen fail loudly when linker step fails**
  - `CompileBinary()` ignores `system()` exit codes and always deletes temp object.
  - This can silently produce no output binary even when compile command "succeeds".
  - File: `change_me/src/compiler.cpp:137`

- [ ] **Remove unsafe `dynamic_cast` assumptions in semantic/codegen call paths**
  - `CallExpr` assumes callee shape in multiple places; continue hardening and
    produce stable diagnostics instead of undefined behavior.
  - Files: `change_me/src/semantic_analyzer.cpp`, `change_me/src/compiler.cpp`

## P1 - Architecture debt (semantic <-> codegen coupling)

- [ ] **Decouple semantic analysis from LLVM types**
  - `SemanticAnalyzer` currently returns `llvm::Value*` and includes LLVM headers.
  - Semantic pass should be LLVM-agnostic and return semantic results only.
  - File: `change_me/include/semantic_analyzer.hpp`

- [ ] **Unify scope/symbol systems**
  - There are two parallel scope implementations:
    - semantic: `Scope` (`name -> type`)
    - codegen: `SymbolTable` (`name -> alloca/value`)
  - This duplicates lookup semantics and causes phase drift.
  - Files: `change_me/include/semantic_scope.hpp`, `change_me/include/symbol_table.hpp`

- [ ] **Eliminate global LLVM state (`TheContext`, `TheModule`, `Builder`, ...)**
  - Globals in `compiler.cpp` conflict with instance ownership and make lifetime
    bugs likely (already noted in comments/TODOs).
  - Move all LLVM state into `Compiler` or a dedicated `CodegenContext`.
  - File: `change_me/src/compiler.cpp:34`

- [ ] **Replace hard exits with diagnostic accumulation**
  - `errs::ErrorOutln()` exits immediately; parser/lexer also call `exit(1)`.
  - This prevents multi-error reporting and makes phase boundaries fragile.
  - Files: `change_me/include/errors.hpp`, `change_me/src/parser.cpp`, `change_me/src/lexer.cpp`

- [ ] **Split frontend phases explicitly**
  - Define: parse -> semantic/type check -> IR lowering -> backend/link.
  - Avoid semantic mutation patterns that hide invalid AST state until codegen.
  - Files: `change_me/src/compiler.cpp`, `change_me/src/semantic_analyzer.cpp`

## P1 - Type system gaps / inconsistencies

- [ ] **Implement `int64` end-to-end (or remove token support)**
  - Lexer/parser accept `int64`, but semantic `ResolveType` rejects it and codegen
    always maps integer types to i32.
  - Files: `change_me/src/semantic_analyzer.cpp:322`, `change_me/src/compiler.cpp:639`

- [ ] **Stop leaking function type allocations**
  - `TypeContext::Function()` allocates with `new` and never frees.
  - Introduce type interning/arena ownership.
  - File: `change_me/src/type_context.cpp:25`

- [ ] **Add structural type equality helpers**
  - Some checks compare kind-only; future function/struct types need structural
    compatibility checks.
  - Files: `change_me/src/semantic_analyzer.cpp`, `change_me/include/types.hpp`

- [ ] **Audit variadic promotion semantics**
  - Promotion currently rewrites AST expression type in place and has TODO notes.
  - Clarify C ABI behavior and ensure lowering aligns with promoted LLVM arg types.
  - File: `change_me/src/semantic_analyzer.cpp:269`

## P1 - IR generation correctness

- [ ] **Ensure variable declarations preserve declared type semantics**
  - Initializer and declared type checks exist, but codegen still allocates based on
    expression type in some paths; normalize around declared type annotations.
  - Files: `change_me/src/semantic_analyzer.cpp`, `change_me/src/compiler.cpp`

- [ ] **Fix function argument representation in codegen symbol table**
  - Function args are stored as raw `llvm::Argument*` with `alloca_ptr == nullptr`.
  - This makes read/write semantics inconsistent with local vars.
  - Prefer alloca+store in entry block for mutable locals.
  - File: `change_me/src/compiler.cpp:324`

- [ ] **Implement/verify `CompileRun()` or remove mode until ready**
  - Currently stubbed with `IMPLEMENT(CompileRun)`.
  - File: `change_me/src/compiler.cpp:133`

## P2 - Parser / language design quality

- [ ] **Add block statement node for `if` branches**
  - `if` currently stores single `Stmt` for then/else; comments indicate this is
    known awkwardness.
  - File: `change_me/src/compiler.cpp:284`, `change_me/include/stmt.hpp`

- [ ] **Tighten top-level grammar**
  - Module parser loops through `ExternFunction()` and allows mixed statements in
    places that should likely be declaration-only.
  - File: `change_me/src/parser.cpp:16`

- [ ] **Normalize CLI option handling**
  - Output flag uses short key checks (`"o"`) while command examples use long form.
  - Clean up option names/help and ensure behavior is deterministic.
  - File: `change_me/src/main.cpp`

## P2 - Diagnostics and developer tooling

- [ ] **Replace `assert(0 && "Unreachable")` lexer fatal with proper diagnostic**
  - Assertions are poor UX for compiler users.
  - File: `change_me/src/lexer.cpp:141`

- [ ] **Fix token debug output inversions for relational operators**
  - `TT_LESSER` prints `>` and `TT_GREATER` prints `<` (and eq variants inverted).
  - File: `change_me/src/lexer.cpp:337`

- [ ] **Add regression tests for recently fixed segfault paths**
  - Cases: nested literal in call arg inside var init, `return;` in void fn,
    `if` without else, assignment/readback, prefix ops.
  - Add a simple script-based test harness if no framework is adopted.

- [ ] **Create CI checks for build + smoke tests**
  - At minimum: compile compiler, run token/ast/llvm emit on fixture set, and run
    one full `--compile` link path.

## Suggested execution order

1. P0 parser/lexer bounds + linker exit-code handling.
2. P1 semantic/codegen decoupling design doc + symbol table unification plan.
3. P1 type-system cleanup (`int64`, type ownership, type equality helpers).
4. P1/P2 IR and parser ergonomics refactors (block stmt, arg storage model).
5. P2 diagnostics + regression/CI hardening.
