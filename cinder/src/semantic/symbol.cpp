#include "cinder/semantic/symbol.hpp"

using namespace cinder;

SymbolId ResolvedSymbols::Declare(std::string name, types::Type* type,
                                  bool is_function) {
  SymbolId id = static_cast<SymbolId>(symbols_.size());
  symbols_.push_back({id, name, type, is_function});
  return id;
}

SymbolInfo* ResolvedSymbols::GetSymbolInfo(SymbolId id) {
  if (id >= symbols_.size()) {
    return nullptr;
  }
  return &symbols_[id];
}

const SymbolInfo* ResolvedSymbols::GetSymbolInfo(SymbolId id) const {
  if (id >= symbols_.size()) {
    return nullptr;
  }
  return &symbols_[id];
}

std::vector<SymbolInfo> ResolvedSymbols::GetSymbolTable() {
  return symbols_;
}