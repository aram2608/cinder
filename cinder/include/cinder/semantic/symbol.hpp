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
  cinder::types::Type* type;
  bool is_function = false;
};

class ResolvedSymbols {
 public:
  SymbolId Declare(std::string name, cinder::types::Type* type,
                   bool is_function);

  SymbolInfo* GetSymbolInfo(SymbolId id);
  const SymbolInfo* GetSymbolInfo(SymbolId id) const;
  std::vector<SymbolInfo> GetSymbolTable();

 private:
  std::vector<SymbolInfo> symbols_;
};

#endif
