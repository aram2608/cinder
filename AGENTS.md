# AGENTS.md - Development Guidelines for no-name-lang

This document provides essential guidelines for agentic coding agents working on this C++ compiler project.

## Project Overview

This is a C++17/20 compiler for a custom programming language with Ruby/C-like syntax, using LLVM as the backend. The project implements a recursive descent parser with AST traversal using the visitor pattern.

## Build System and Commands

### Building
```bash
# Build the main compiler
make -C /Users/ja1473/no-name-lang

# Alternative: Build explicitly
make -C /Users/ja1473/no-name-lang $(BUILD_DIR)/$(TARGET)
```

### Testing and Development Commands
```bash
# Lexical analysis (token output)
./build/main --emit-tokens test.changeme

# Parse and show AST
./build/main --emit-ast test.changeme

# Generate LLVM IR
./build/main --emit-llvm -o test.ll test.changeme

# Compile to executable
./build/main --compile -o test test.changeme

# Quick test commands (via Makefile)
make -C /Users/ja1473/no-name-lang LLVM   # Generate LLVM IR from test file
make -C /Users/ja1473/no-name-lang TEST   # Compile test file to executable
```

### Build Configuration
- **Compiler**: `clang++`
- **Standard**: C++17/20
- **Flags**: `-g -Wall -Wextra -pedantic -Wno-unused-parameter`
- **LLVM Integration**: Uses `llvm-config` for flags and libraries
- **No Exceptions**: Compiled with `-DCXXOPTS_NO_EXCEPTIONS`

## Code Style Guidelines

### Formatting (from .clang-format)
- **Style**: Google-based
- **Indentation**: 2 spaces (no tabs)
- **Line Limit**: 80 characters
- **Braces**: Attached to closing brace (same line)
- **Pointers**: Left alignment (`Type* ptr` not `Type *ptr`)
- **Includes**: Sorted alphabetically

### Naming Conventions
- **Classes/Structs**: `PascalCase` (e.g., `Compiler`, `ExprVisitor`)
- **Functions/Methods**: `PascalCase` for class methods, `camelCase` for standalone functions
- **Variables**: `snake_case` (e.g., `line_count`, `source_str`)
- **Constants**: `UPPER_SNAKE_CASE` (e.g., `TT_PLUS`, `VT_INT32`)
- **Enums**: `PascalCase` with `UPPER_SNAKE_CASE` members
- **Files**: `snake_case.cpp` and `snake_case.hpp`

### Include Organization
```cpp
// System headers first
#include <memory>
#include <vector>

// LLVM headers
#include "llvm/IR/IRBuilder.h"

// Local headers with relative paths from source location
#include "../include/compiler.hpp"
#include "expr.hpp"
```

## Architecture and Patterns

### Core Components
- **Lexer**: Tokenizes source code into predefined token types
- **Parser**: Recursive descent parser building AST
- **AST Nodes**: Expression and statement nodes with visitor pattern
- **Semantic Analyzer**: Type checking and semantic validation
- **Compiler**: LLVM IR generation and compilation
- **Symbol Table**: Variable and function scope management

### Design Patterns
- **Visitor Pattern**: Used for AST traversal (`ExprVisitor`, `StmtVisitor`)
- **RAII**: Extensive use of smart pointers (`std::unique_ptr`, `std::shared_ptr`)
- **Factory Methods**: For AST node creation
- **Builder Pattern**: For complex LLVM IR construction

### Error Handling
- **Custom Error System**: Uses `RawOutStream` and template-based error reporting
- **No Exceptions**: Project compiled without exception support
- **Exit on Error**: Immediate program termination on compilation errors
- **Error Location**: Include line/column information when possible

## Language Design Context

### Supported Types
- `int32`, `int64` (signed integers)
- `flt` (floating point)
- `str` (strings)
- `bool` (booleans)
- `void` (for functions)

### Syntax Characteristics
- Function declarations: `def func_name(type: param) -> return_type`
- Variable declarations: `int32: variable = value;`
- Module system: `mod main;`
- External functions: `extern printf(str fmt, ...) -> int32`
- Control structures: `if-else-end`, `for`, `while`, `return`

## Development Guidelines

### Memory Management
- Use smart pointers exclusively for AST nodes
- Prefer `std::unique_ptr` for ownership
- Use `std::shared_ptr` only when sharing is necessary
- Never use raw pointers for AST node management

### LLVM Integration
- Use `llvm-config` for proper linking flags
- Follow LLVM coding conventions when writing IR generation code
- Always verify LLVM modules before compilation
- Handle LLVM errors gracefully with proper error messages

### Code Organization
- Header files in `change_me/include/`
- Source files in `change_me/src/`
- Vendor libraries in `change_me/vendor/`
- Build output in `build/`

### Testing Approach
- Currently uses manual testing with `test.changeme`
- Use CLI flags for component testing:
  - `--emit-tokens` for lexer testing
  - `--emit-ast` for parser testing
  - `--emit-llvm` for compiler testing
- No automated test framework currently implemented

## Common Issues to Avoid

### LLVM Integration
- Don't forget to verify modules before code generation
- Properly manage LLVM context lifetime
- Use correct calling conventions for external functions

### Parser Development
- Handle all error cases with meaningful messages
- Maintain proper token lookahead
- Ensure AST node ownership is clear

### Memory Safety
- Never delete pointers managed by smart pointers
- Be careful with reference cycles in shared_ptr
- Use weak_ptr where appropriate to break cycles

## Current Limitations (from TODO.md)
- LLVM code generation needs cleanup
- JIT compiler has segmentation faults
- Function arity problems (no variadic support)
- Import/module resolution not implemented
- No standard library (printf, etc.)

## External Dependencies
- **LLVM**: Core dependency for IR generation
- **cxxopts**: Header-only CLI parsing (in `vendor/`)
- **QBE**: Backend compiler reference (in `qbe-1.2/`)

## File Extensions
- Source files: `.cpp`
- Header files: `.hpp`
- Language files: `.changeme` (subject to change)
- LLVM IR: `.ll`