#include "no_name_lang/semantic/symbol.hpp"

SymbolId SemanticSymbols::Declare(std::string name, types::Type* type,
                                  bool is_function) {
  SymbolId id = static_cast<SymbolId>(symbols_.size());
  symbols_.push_back({id, name, type, is_function});
  table_[name] = id;
  return id;
}

SymbolInfo* SemanticSymbols::Lookup(const std::string& name) {
  auto it = table_.find(name);
  if (it == table_.end()) return nullptr;
  return &symbols_[it->second];
}