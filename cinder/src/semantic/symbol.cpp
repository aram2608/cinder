#include "cinder/semantic/symbol.hpp"

SymbolId SemanticSymbols::Declare(std::string name, types::Type* type,
                                  bool is_function) {
  SymbolId id = static_cast<SymbolId>(symbols_.size());
  symbols_.push_back({id, name, type, is_function});
  return id;
}

SymbolInfo* SemanticSymbols::Get(SymbolId id) {
  if (id >= symbols_.size()) {
    return nullptr;
  }
  return &symbols_[id];
}

const SymbolInfo* SemanticSymbols::Get(SymbolId id) const {
  if (id >= symbols_.size()) {
    return nullptr;
  }
  return &symbols_[id];
}
