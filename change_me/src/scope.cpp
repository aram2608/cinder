#include "../include/scope.hpp"

#include "../include/errors.hpp"

static errs::RawOutStream errors{};

Scope::Scope(std::shared_ptr<Scope> parent) : parent(parent) {}

void Scope::Declare(const std::string& name, types::Type* type) {
  if (table.find(name) != table.end()) {
    errs::ErrorOutln(errors, "Redefinition of variable:", name);
  }
  table[name] = Symbol{type};
}

Symbol* Scope::Lookup(const std::string& name) {
  if (auto it = table.find(name); it != table.end()) {
    return &it->second;
  }
  return parent ? parent->Lookup(name) : nullptr;
}