#ifndef SEMANTIC_ENV_H_
#define SEMANTIC_ENV_H_

#include <string>
#include <unordered_map>
#include <vector>

#include "cinder/semantic/symbol.hpp"

class Environment {
 public:
  void PushScope();
  void PopScope();

  bool DeclareLocal(const std::string& name, SymbolId id);
  SymbolId* Lookup(const std::string& name);
  bool IsDeclaredInCurrentScope(const std::string& name) const;

 private:
  std::vector<std::unordered_map<std::string, SymbolId>> scopes_;
};

#endif
