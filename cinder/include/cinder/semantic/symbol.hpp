#ifndef SYMBOL_H_
#define SYMBOL_H_

#include <cstdint>
#include <string>
#include <vector>

#include "cinder/ast/types.hpp"

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

  SymbolInfo* Get(SymbolId id);
  const SymbolInfo* Get(SymbolId id) const;

 private:
  std::vector<SymbolInfo> symbols_;
};

#endif
