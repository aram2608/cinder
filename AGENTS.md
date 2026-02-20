# AGENTS Guide for `cinder`
This guide is for autonomous coding agents working in this repository.
Prefer minimal, targeted changes and follow existing local patterns.

## 1) Project Snapshot
- Language: C++20
- Build system: CMake
- Main binary: `build/bin/cinder`
- Tests: integration-style `.ci` files in `tests/`
- Formatting: `.clang-format` + `format.sh`
- Toolchain: LLVM + Clang packages discovered by CMake

## 2) Repository Layout
- `CMakeLists.txt` (root configure/build entry)
- `cinder/CMakeLists.txt` (LLVM/Clang discovery + subdirs)
- `cinder/include/cinder/**` (public headers)
- `cinder/src/**` (compiler implementation)
- `cinder/vendor/**` (third-party; avoid edits unless requested)
- `tests/*.ci` (sample programs used as tests)
- `Makefile` (convenience targets)
- `format.sh` (mass formatting helper)

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

### Configure + build
```bash
cmake -S . -B build && cmake --build build
```

### Convenience wrapper
```bash
make cinder
```

- `compile_commands.json` is exported by CMake.
- Root configure prints compiler/toolchain diagnostics.
- Outputs land under `build/bin` and `build/lib`.

## 4) Lint / Formatting Commands
There is no configured clang-tidy or dedicated lint target.
Formatting is the primary automated style check.

### Format all tracked C++ files
```bash
./format.sh
```

### Format one file
```bash
clang-format -i cinder/src/frontend/parser.cpp
```

Formatting authority is `.clang-format`:
- BasedOnStyle: Google
- 2-space indentation
- 80 column limit
- pointer alignment left

## 5) Test Commands
No `ctest` or unit-test framework is currently configured.
“Tests” are compiler runs against `.ci` inputs.

### Build first
```bash
cmake --build build
```

### Canonical test run (compile)
```bash
./build/bin/cinder --compile -o test ./tests/test.ci
```

### AST output mode
```bash
./build/bin/cinder --emit-ast -o test ./tests/test.ci
```

### Semantic mode
```bash
./build/bin/cinder --test-semantic -o test ./tests/test.ci
```

### LLVM IR mode
```bash
./build/bin/cinder --emit-llvm -o test.ll ./tests/test.ci
```

### Single-test equivalent (run one file)
```bash
./build/bin/cinder --compile -o /tmp/out ./tests/fib.ci
```

If your change affects parser/semantic/codegen, run at least:
- `./tests/test.ci`
- `./tests/fib.ci`

## 6) Code Style Guidelines
Follow nearby code first; these are default conventions.

### Includes
- Standard library includes first.
- Project includes (`"cinder/..."`) next.
- LLVM/Clang includes after project headers when practical.
- Do not keep unused includes.

### Formatting
- Enforce `.clang-format` settings.
- Use 2-space indentation and attached braces.
- Keep lines near 80 chars unless readability is harmed.

### Naming
- Types/structs/classes: `PascalCase`.
- Methods often `PascalCase` (`Visit`, `Generate`, `ResolveType`).
- Locals: lower case or `snake_case`.
- Member fields frequently end with trailing underscore (`ctx_`, `tokens_`).
- Preserve naming style already present in edited file.

### Types and Ownership
- Use `std::unique_ptr` for AST ownership.
- Use references for required non-owning params.
- Raw pointers are acceptable for non-owning links where established.
- Use `std::optional` for nullable/value-maybe fields.
- Preserve visitor-based interfaces (AST/semantic/codegen).

### Error Handling
- Do not introduce C++ exceptions (`-fno-exceptions` is enabled).
- Keep existing patterns:
  - `DiagnosticEngine` for semantic diagnostics
  - `ostream::ErrorOutln` for direct CLI/compiler errors
  - `bool` / `nullptr` / error-code based failure returns
- Emit clear, actionable diagnostics with location when available.

## 7) Toolchain + clangd Notes
- Configure step prints compiler path, ID, version, and include diagnostics.
- `build/compile_commands.json` is generated and linked at repo root.
- If clangd is stale after toolchain changes, reconfigure and reload index.

## 8) Cursor/Copilot Rule Files
Checked for:
- `.cursor/rules/`
- `.cursorrules`
- `.github/copilot-instructions.md`

Current status:
- No Cursor or Copilot rule files found in this repository.
- If these files are added later, update this document immediately.
