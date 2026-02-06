#include "../include/symbol_table.hpp"

#include "../include/errors.hpp"

static errs::RawOutStream errors{};

void SymbolTable::Declare(std::string name, InternalSymbol symb) {
  symbols[name] = symb;
}

InternalSymbol* SymbolTable::LookUp(std::string name) {
  auto symb = symbols.find(name);
  return &symb->second;
}