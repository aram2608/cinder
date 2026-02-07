#include "cinder/semantic/semantic_env.hpp"

void SemanticEnv::PushScope() {
  scopes_.emplace_back();
}

void SemanticEnv::PopScope() {
  if (!scopes_.empty()) {
    scopes_.pop_back();
  }
}

bool SemanticEnv::DeclareLocal(const std::string& name, SymbolId id) {
  if (scopes_.empty()) {
    PushScope();
  }

  auto& current = scopes_.back();
  if (current.find(name) != current.end()) {
    return false;
  }

  current[name] = id;
  return true;
}

SymbolId* SemanticEnv::Lookup(const std::string& name) {
  for (auto it = scopes_.rbegin(); it != scopes_.rend(); ++it) {
    auto lookup = it->find(name);
    if (lookup != it->end()) {
      return &lookup->second;
    }
  }
  return nullptr;
}

bool SemanticEnv::IsDeclaredInCurrentScope(const std::string& name) const {
  if (scopes_.empty()) {
    return false;
  }

  const auto& current = scopes_.back();
  return current.find(name) != current.end();
}
