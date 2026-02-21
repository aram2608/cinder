# AGENTS Guide for `cinder`
This file is for autonomous coding agents working in this repository.
Keep edits minimal, scoped, and consistent with surrounding code.
## 1) Project Snapshot
- Language: C++20
- Build system: CMake (root `CMakeLists.txt`)
- Main executable: `build/bin/cinder`
- Core library target: `cinder_core` (static)
- Primary tests: compiler runs over `.ci` files in `tests/`
- Formatting: `.clang-format` and `format.sh`
- Toolchain: LLVM required, Clang package optional
## 2) Important Paths
- `CMakeLists.txt`: top-level configuration, diagnostics, compile_commands symlink
- `cinder/CMakeLists.txt`: LLVM/Clang package discovery, source subtree entry
- `cinder/src/CMakeLists.txt`: targets, warnings, no-exception flags, link setup
- `cinder/include/cinder/**`: public headers
- `cinder/src/**`: implementation
- `cinder/vendor/**`: third-party code (do not edit unless asked)
- `tests/*.ci`: integration-style test inputs (`test.ci`, `fib.ci`, `bad_op.ci`)
- `Makefile`: convenience wrappers for common compiler runs
- `format.sh`: batch clang-format helper
## 3) Build Commands
Run from repo root: `/Users/ja1473/cinder`
### Configure
```bash
cmake -S . -B build
```
### Build
```bash
cmake --build build
```
### Configure + build (recommended first run)
```bash
cmake -S . -B build && cmake --build build
```
### Convenience target
```bash
make cinder
```
### Optional: enable clang-tidy during build
```bash
cmake -S . -B build -DCINDER_ENABLE_CLANG_TIDY=ON
cmake --build build
```
Notes:
- Build outputs: `build/bin` and `build/lib`
- `compile_commands.json` is exported and symlinked at repo root
- Configure prints compiler, target, sysroot, and include diagnostics
## 4) Lint / Formatting Commands
There is no dedicated lint target by default.
Formatting is the primary enforced style check.
### Format all tracked C++ files
```bash
./format.sh
```
### Format one file
```bash
clang-format -i cinder/src/frontend/parser.cpp
```
### Useful verification
```bash
git diff -- cinder/src cinder/include
```
Formatting defaults from `.clang-format`:
- BasedOnStyle: Google
- IndentWidth: 2
- ColumnLimit: 80
- BreakBeforeBraces: Attach
- PointerAlignment: Left
- SortIncludes: true
## 5) Test Commands
There is no `ctest` suite currently.
Treat tests as executable runs over `.ci` inputs.
### Build before running tests
```bash
cmake --build build
```
### Canonical compile test
```bash
./build/bin/cinder --compile -o test ./tests/test.ci
```
### AST mode
```bash
./build/bin/cinder --emit-ast -o test ./tests/test.ci
```
### Semantic mode
```bash
./build/bin/cinder --test-semantic -o test ./tests/test.ci
```
### LLVM IR emission
```bash
./build/bin/cinder --emit-llvm -o test.ll ./tests/test.ci
```
### Run a single specific test file (important)
```bash
./build/bin/cinder --compile -o /tmp/out ./tests/fib.ci
```
### Additional focused case (negative behavior)
```bash
./build/bin/cinder --compile -o /tmp/out ./tests/bad_op.ci
```
### Makefile shortcuts
```bash
make test
make fib
make ast
make semantic
make llvm
```
Minimum regression set for parser/semantic/codegen changes:
- `./tests/test.ci`
- `./tests/fib.ci`
## 6) Compiler and Build Constraints
- C++ exceptions are disabled (`-fno-exceptions`)
- Warning set includes `-Wall -Wextra -pedantic`
- `-Wno-unused-parameter` is currently enabled
- `CXXOPTS_NO_EXCEPTIONS` is defined for targets
- Keep portability in mind for LLVM/Clang package discovery differences
## 7) Code Style Guidelines
Follow existing local patterns first; use these defaults when uncertain.
### Includes and dependencies
- Keep includes minimal and used
- Standard library includes first
- Project headers (`"cinder/..."`) before external headers where practical
- Rely on clang-format include sorting; avoid manual churn-only reorderings
### Formatting and structure
- Run clang-format on touched C++ files
- Use 2-space indentation and attached braces
- Keep lines around 80 columns unless readability clearly improves
- Avoid unrelated whitespace-only changes in untouched code
### Naming
- Types, enums, classes: `PascalCase`
- Many methods use `PascalCase` (`Parse`, `Visit`, `Generate`, `ResolveType`)
- Local variables: lower-case, often `snake_case`
- Member fields commonly end with trailing underscore (`ctx_`, `tokens_`)
- Match naming conventions of the file you are editing
### Types and ownership
- Prefer `std::unique_ptr` for AST node ownership
- Use references for required non-owning parameters
- Raw pointers are acceptable for established non-owning links
- Use `std::optional` for optional value-state where already used
- Preserve visitor-based AST/semantic/codegen interfaces
### Error handling and diagnostics
- Do not introduce exception-based flow
- Prefer existing diagnostics mechanisms:
  - `DiagnosticEngine` for semantic/codegen diagnostics
  - `ostream::ErrorOutln` for direct CLI/parser-facing errors
- Return `bool`, `nullptr`, or error-like status objects as established
- Diagnostics should be actionable and location-aware when possible
## 8) Change Scope Guidelines for Agents
- Keep patches focused; avoid broad refactors unless requested
- Do not edit `cinder/vendor/**` unless explicitly asked
- Preserve public API shape unless task requires breaking changes
- Update docs/comments only when behavior or contracts actually change
- If toolchain/config behavior is changed, include verification commands
## 9) Cursor/Copilot Rule Files
Repository scan checked:
- `.cursor/rules/`
- `.cursorrules`
- `.github/copilot-instructions.md`
Current status:
- No Cursor rules found
- No Copilot instruction file found
If any of these files are added later, update this AGENTS guide accordingly.
