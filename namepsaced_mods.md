Patch Plan
1. Extend AST for qualified identifiers  
- Update Variable to optionally hold a module qualifier token (e.g. math in math.add) in cinder/include/cinder/ast/expr/expr.hpp and cinder/src/ast/expr/expr.cpp.  
- Keep unqualified Variable(name) working for locals and same-module references.
2. Parse module.symbol in expressions  
- In cinder/src/frontend/parser.cpp, update Atom() so after an identifier it accepts optional DOT IDENTIFIER and builds a qualified Variable.  
- No grammar rewrite needed; this is a small targeted change.
3. (Optional but recommended) track import aliases later  
- For v1, keep import math; only.  
- If you want aliases (import math as m;), extend ImportStmt in cinder/include/cinder/ast/stmt/stmt.hpp and parser import logic later as phase 2.
4. Add module-aware semantic lookup  
- In cinder/include/cinder/semantic/semantic_analyzer.hpp + cinder/src/semantic/semantic_analyzer.cpp, add analyzer state: current module name and imported module set per module.  
- Build a small map at start of AnalyzeProgram: module_name -> imported module names.  
- Resolve symbols with rules:
  - local vars: existing lexical scope lookup unchanged
  - unqualified function call: current module first
  - qualified call (math.add): resolve only if math is imported
- Emit clear diagnostics for:
  - unknown module qualifier
  - module not imported
  - symbol not exported/found in that module
5. Namespace function declarations semantically  
- When declaring function symbols in first pass, use a canonical semantic name like module.add (or module::add) internally.  
- Extern declarations (like printf) should remain unmangled/global in semantic naming.
6. Mangle LLVM symbol names in codegen  
- In cinder/include/cinder/codegen/codegen.hpp + cinder/src/codegen/codegen.cpp, track current module during Visit(ModuleStmt&).  
- In Visit(FunctionProto&), emit LLVM function name as:
  - extern: printf (unchanged)
  - module function: module__name
- Calls will still work via symbol IDs/bindings; this just prevents cross-module collisions in IR/linking.
7. Keep import statements as ordering/visibility metadata  
- ModuleLoader stays mostly unchanged; it already handles dependency order and cycles in cinder/src/frontend/module_loader.cpp.  
- Semantic pass enforces visibility; loader enforces discoverability/order.
8. Add regression tests  
- Add files under tests/:
  - positive: import math; + math.add(...)
  - negative: using math.add without import
  - negative: unknown foo.bar
  - collision: two modules both define add, both callable via qualification
- Run:
  - cmake --build build
  - ./build/bin/cinder --compile -o /tmp/out ./tests/<new>.ci