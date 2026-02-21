# AGENTS Guide for `cinder`
This file is for autonomous coding agents operating in this repository.
Keep changes minimal, scoped, and aligned with existing conventions.

## 1) Project Snapshot
- Language: C++20
- Build system: CMake (`CMakeLists.txt`) with optional presets (`CMakePresets.json`)
- Main binary: `build/bin/cinder`
- Core target: `cinder_core` (static library)
- Unit tests: GoogleTest executable `cinder_unit_tests` discovered by CTest
- Compiler smoke tests: run `cinder` against `tests/*.ci`
- Formatting authority: `.clang-format`

## 2) Key Paths
- `CMakeLists.txt`: top-level options, diagnostics, test enablement, compile commands link
- `CMakePresets.json`: configure/build/test presets (`debug`, `release`, `debug-tests`, etc.)
- `BUILD.md`: canonical command documentation
- `cinder/CMakeLists.txt`: LLVM/Clang package discovery
- `cinder/src/CMakeLists.txt`: flags, compile definitions, links, subdirectories
- `cinder/include/cinder/**`: public headers
- `cinder/src/**`: implementation
- `tests/CMakeLists.txt`: unit test target and `gtest_discover_tests`
- `tests/*.ci`: language examples used for compile/semantic/codegen checks
- `Makefile`: convenience wrappers (`test`, `fib`, `ast`, `semantic`, `llvm`, `cinder`)
- `cinder/vendor/**`: vendored third-party code; avoid edits unless explicitly requested

## 3) Build Commands
Run from repository root: `/Users/ja1473/cinder`.

### Direct CMake
- Configure: `cmake -S . -B build`
- Build: `cmake --build build`
- Configure+build: `cmake -S . -B build && cmake --build build`

### Presets
- Debug: `cmake --preset debug && cmake --build --preset build-debug`
- Release: `cmake --preset release && cmake --build --preset build-release`
- Debug + tests: `cmake --preset debug-tests && cmake --build --preset build-debug-tests`

### clang-tidy (optional)
- `cmake -S . -B build -DCINDER_ENABLE_CLANG_TIDY=ON`
- `cmake --build build`

### Make convenience wrapper
- `make cinder`

### Build notes
- `CMAKE_CXX_STANDARD` is pinned to 20.
- Exceptions are disabled at compile level (`-fno-exceptions`).
- Warnings in debug builds include `-Wall -Wextra -pedantic`.
- `compile_commands.json` is exported and symlinked at repo root.

## 4) Lint / Formatting Commands
There is no dedicated lint target by default.
Primary style enforcement is clang-format + compiler warnings (+ optional clang-tidy).

### Formatting
- Format all tracked files: `./format.sh`
- Format one file: `clang-format -i cinder/src/frontend/parser.cpp`
- Quick review of touched C++/tests: `git diff -- cinder/src cinder/include tests`

### `.clang-format` baseline
- `BasedOnStyle: Google`
- `IndentWidth: 2`
- `ColumnLimit: 80`
- `BreakBeforeBraces: Attach`
- `PointerAlignment: Left`
- `SortIncludes: true`

## 5) Test Commands
Use both unit tests and `.ci` compiler runs.

### Build tests
- `cmake -S . -B build -DCINDER_BUILD_TESTS=ON`
- `cmake --build build`

### Run all unit tests
- `ctest --test-dir build --output-on-failure`

### Run a single unit test (important)
- CTest filtering: `ctest --test-dir build --output-on-failure -R lexer`
- Direct gtest filter: `./build/bin/cinder_unit_tests --gtest_filter='*Lexer*'`

### Preset-based test run
- `cmake --preset debug-tests`
- `cmake --build --preset build-debug-tests`
- `ctest --preset test-debug`

### Run a single `.ci` test file (important)
- `./build/bin/cinder --compile -o /tmp/out ./tests/fib.ci`

### Useful `.ci` smoke commands
- `./build/bin/cinder --compile -o /tmp/out ./tests/test.ci`
- `./build/bin/cinder --compile -o /tmp/out ./tests/bad_op.ci`
- `./build/bin/cinder --emit-ast -o /tmp/out ./tests/test.ci`
- `./build/bin/cinder --test-semantic -o /tmp/out ./tests/test.ci`
- `./build/bin/cinder --emit-llvm -o /tmp/test.ll ./tests/test.ci`

### Makefile shortcuts for smoke runs
- `make test`
- `make fib`
- `make ast`
- `make semantic`
- `make llvm`

## 6) Code Style Guidelines
Follow existing local file conventions first.
When unclear, use the rules below.

### Includes and dependencies
- Keep includes minimal and used.
- Prefer project headers with `"cinder/..."` style.
- Let clang-format sort includes; do not hand-sort just for churn.
- Avoid dependency or include reordering unrelated to the requested task.

### Formatting
- Run clang-format on every touched C++ file before finishing.
- Use 2-space indentation and attached braces.
- Keep lines near 80 columns unless readability clearly improves.
- Avoid broad whitespace-only edits.

### Types and ownership
- Prefer `std::unique_ptr` for ownership-heavy AST/IR structures.
- Use references for required non-owning parameters.
- Raw pointers are acceptable for established non-owning links.
- Use `std::optional` when optional state must be represented explicitly.
- Preserve existing visitor-style interfaces and ownership boundaries.

### Naming conventions
- Types/classes/enums: `PascalCase`.
- Core methods/functions are frequently `PascalCase`.
- Local variables: lowercase, commonly `snake_case`.
- Member fields often use trailing underscore (for example `ctx_`, `tokens_`).
- Match naming already present in the file/module you edit.

### Error handling and diagnostics
- Do not introduce exception-based control flow.
- Reuse existing diagnostics paths instead of ad-hoc stderr prints.
- Use `DiagnosticEngine` where semantic/codegen diagnostics already flow.
- Use `ostream::ErrorOutln` for CLI/parser-oriented diagnostics.
- Prefer actionable messages with location context when available.
- Return status (`bool`, nullable, optional) consistent with surrounding APIs.

### Change hygiene
- Keep patches focused; avoid broad refactors unless asked.
- Preserve public API shape unless the task explicitly requires API changes.
- Do not modify `cinder/vendor/**` unless explicitly requested.
- If build/test workflows change, update `BUILD.md` and `AGENTS.md` together.

## 7) Cursor / Copilot Rules
Scanned locations:
- `.cursor/rules/`
- `.cursorrules`
- `.github/copilot-instructions.md`

Current status in this repo:
- No Cursor rules found.
- No Copilot instruction file found.

If these files are added later, merge their requirements into this guide.
