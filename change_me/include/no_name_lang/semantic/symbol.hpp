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

#endif
