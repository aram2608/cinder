#include "../include/symbol_table.hpp"

#include "../include/errors.hpp"

static errs::RawOutStream errors{};

void SymbolTable::Declare(std::string name, InternalSymbol symb) {
  symbols[name] = symb;
}

InternalSymbol* SymbolTable::LookUp(std::string name) {
  if (auto it = symbols.find(name); it != symbols.end()) {
    return &it->second;
  }
  return parent ? parent->LookUp(name) : nullptr;
}