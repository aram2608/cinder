#ifndef SYMBOL_H_
#define SYMBOL_H_

#include <string>
#include <unordered_map>
#include <vector>

#include "no_name_lang/ast/types.hpp"

using SymbolId = uint32_t;

struct SymbolInfo {
  SymbolId id;
  std::string name;
  types::Type* type;
  bool is_function = false;
};

class SemanticSymbols {
 public:
  SymbolId Declare(std::string name, types::Type* type, bool is_function);

  SymbolInfo* Lookup(const std::string& name);

 private:
  std::vector<SymbolInfo> symbols_;
  std::unordered_map<std::string, SymbolId> table_;
};

#endif
