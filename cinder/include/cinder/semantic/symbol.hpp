#ifndef SYMBOL_H_
#define SYMBOL_H_

#include <cstdint>
#include <string>
#include <vector>

#include "cinder/ast/types.hpp"

using SymbolId = uint32_t;

/** @brief Immutable symbol metadata produced by semantic resolution. */
struct SymbolInfo {
  SymbolId id;               /**< Stable symbol identifier. */
  std::string name;          /**< Source-level symbol name. */
  cinder::types::Type* type; /**< Resolved symbol type. */
  bool is_function = false;  /**< True when symbol denotes a function. */
};

/** @brief Symbol table storing all resolved symbols in declaration order. */
class ResolvedSymbols {
 public:
  /**
   * @brief Declares a new symbol and returns its id.
   * @param name Symbol name.
   * @param type Resolved symbol type.
   * @param is_function Whether the symbol represents a function.
   * @return Newly assigned symbol id.
   */
  SymbolId Declare(std::string name, cinder::types::Type* type,
                   bool is_function);

  /** @brief Looks up mutable symbol metadata by id. */
  SymbolInfo* GetSymbolInfo(SymbolId id);
  /** @brief Looks up immutable symbol metadata by id. */
  const SymbolInfo* GetSymbolInfo(SymbolId id) const;
  /** @brief Returns a copy of the full symbol table. */
  std::vector<SymbolInfo> GetSymbolTable();

 private:
  std::vector<SymbolInfo> symbols_;
};

#endif
