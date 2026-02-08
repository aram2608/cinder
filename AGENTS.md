# AGENTS.md
Guidance for coding agents working in this repository.

## Project Snapshot
- Language: C++20.
- Build system: CMake.
- Main binary: `build/bin/cinder`.
- Primary code: `cinder/include`, `cinder/src`.
- Sample inputs used as tests: `tests/test.ci`, `tests/fib.ci`.
- Third-party code: `cinder/vendor` (do not edit unless asked).

## Cursor and Copilot Rules
- `.cursor/rules/`: not present.
- `.cursorrules`: not present.
- `.github/copilot-instructions.md`: not present.
- If any of these are added later, treat them as high-priority instructions.

## Dependencies
- CMake >= 3.21.
- C++20-capable compiler.
- LLVM development packages discoverable by CMake.
- Clang package is optional; build works without it.

## Build Commands
Configure once:

```bash
cmake -S . -B build
```

Build:

```bash
cmake --build build
```

Rebuild from clean state:

```bash
rm -rf build && cmake -S . -B build && cmake --build build
```

## Lint and Format Commands
This repo has formatting tooling but no dedicated lint target.

Apply formatting:

```bash
./format.sh
```

Check formatting only:

```bash
git ls-files "*.cpp" "*.hpp" | xargs clang-format --dry-run --Werror
```

Formatting source of truth: `.clang-format`.

## Test Commands
There is no `ctest` or unit test harness yet.
Use compiler smoke tests against `.ci` inputs.

Makefile wrappers:

```bash
make ast
make llvm
make test
```

Important caveat:
- `make semantic` currently fails because `--test-semantic` is not a valid CLI option.

Direct CLI commands:

```bash
./build/bin/cinder --emit-tokens ./tests/test.ci
./build/bin/cinder --emit-ast -o /tmp/out ./tests/test.ci
./build/bin/cinder --emit-llvm -o test.ll ./tests/test.ci
./build/bin/cinder --compile -o test ./tests/test.ci
```

### Running a Single Test File
Run any one `.ci` file directly:

```bash
./build/bin/cinder --emit-ast -o /tmp/out ./tests/fib.ci
```

Reusable template:

```bash
TEST_FILE=./tests/fib.ci
./build/bin/cinder --emit-ast -o /tmp/out "$TEST_FILE"
```

For semantic/codegen-focused checks, swap `--emit-ast` with `--emit-llvm` or `--compile`.

## Recommended Agent Workflow
1. Configure build dir: `cmake -S . -B build`.
2. Build after edits: `cmake --build build`.
3. Run one targeted single-file smoke test.
4. Format touched C++ files.

## Code Style Guidelines

### Formatting
- Follow `.clang-format` (Google base, 2-space indent, 80-column limit).
- Use spaces, not tabs.
- Keep braces attached (K&R style).
- Prefer short, single-purpose functions.
- Prefer early return over deep nesting.

### Includes and Imports
- Keep includes sorted (clang-format enforces this).
- Use project includes as `cinder/...` when possible.
- Keep STL includes explicit; do not rely on transitive includes.
- In headers, prefer forward declarations when practical.
- Do not include unused headers.

### Types and Ownership
- Use `std::unique_ptr` for AST ownership.
- Use references (`T&`) for required non-null parameters.
- Use raw pointers for non-owning links only.
- Use `std::optional` for maybe-present values.
- Keep visitor interfaces returning `llvm::Value*`.

### Naming
- Types/structs/classes: `PascalCase`.
- Member fields: trailing underscore (`tokens_`, `current_tok_`).
- Token constants: `TT_*` uppercase enum names.
- Match local file naming style for methods/functions.
- Do not mix naming styles in a single touched file.

### Error Handling
- Prefer `DiagnosticEngine` for semantic and analysis errors.
- Use `UNREACHABLE(...)` for impossible internal states.
- Avoid adding new ad-hoc `std::cout` + `exit(1)` paths where diagnostics exist.
- For expected failures, prefer returning status/results over aborting.

### AST and Semantic Conventions
- Construct AST nodes with `std::make_unique<...>`.
- Ensure semantic passes set `type` and `id` where applicable.
- Preserve scope balance (`BeginScope` / `EndScope`).
- Use symbol/environment helpers for declaration and lookup.
- Keep parser token handling consistent with current `Consume/MatchType` patterns.

### CMake Conventions
- Keep warning flags aligned across targets.
- Prefer target-scoped CMake APIs (`target_*`).
- Guard platform-specific options.
- Do not introduce global compile flags unless required.

## Verification Expectations
- Minimum: successful build + one relevant `.ci` smoke test.
- Parser/Lexer changes: run `--emit-tokens` and `--emit-ast`.
- Semantic changes: run `--emit-ast` and `--emit-llvm`.
- Codegen changes: run `--emit-llvm` and `--compile`.

## Files to Avoid Touching by Default
- `cinder/vendor/**`.
- `build/**` outputs.
- Generated artifacts (`*.ll`, compiled binaries) unless requested.

## Maintenance Note
- If you add/rename commands or test flows, update this file in the same change.
