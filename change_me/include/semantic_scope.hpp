#ifndef SCOPE_H_
#define SCOPE_H_

#include <unordered_map>

#include "types.hpp"

struct Symbol {
  types::Type* type;
};

struct Scope {
  std::unordered_map<std::string, Symbol> table;
  std::shared_ptr<Scope> parent;

  explicit Scope(std::shared_ptr<Scope> parent = nullptr);
  void Declare(const std::string name, types::Type* type);
  Symbol* Lookup(const std::string name);
};

#endif