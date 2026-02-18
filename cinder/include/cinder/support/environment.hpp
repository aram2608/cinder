#ifndef SEMANTIC_ENV_H_
#define SEMANTIC_ENV_H_

#include <string>
#include <unordered_map>
#include <vector>

#include "cinder/semantic/symbol.hpp"

/**
 * @brief Lexical scope stack mapping names to resolved symbol ids.
 *
 * Scopes are searched from innermost to outermost during lookup.
 */
class Environment {
 public:
  /** @brief Pushes a new innermost scope. */
  void PushScope();
  /** @brief Pops the innermost scope if present. */
  void PopScope();

  /**
   * @brief Declares `name` in the current scope.
   * @param name Symbol name.
   * @param id Resolved symbol id.
   * @return `false` if already declared in current scope, else `true`.
   */
  bool DeclareLocal(const std::string& name, SymbolId id);

  /**
   * @brief Looks up a symbol id by name from innermost outward.
   * @param name Symbol name.
   * @return Pointer to symbol id when found; otherwise `nullptr`.
   */
  SymbolId* Lookup(const std::string& name);

  /**
   * @brief Checks whether `name` exists in the current scope only.
   * @param name Symbol name.
   * @return `true` when present in current scope.
   */
  bool IsDeclaredInCurrentScope(const std::string& name) const;

 private:
  std::vector<std::unordered_map<std::string, SymbolId>>
      scopes_; /**< Scope stack from outermost to innermost. */
};

#endif
