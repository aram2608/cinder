#ifndef MODULE_LOADER_H_
#define MODULE_LOADER_H_

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "cinder/ast/stmt/stmt.hpp"

/**
 * @brief Loads modules from entry files and resolves import dependencies.
 *
 * The loader parses each module, recursively follows `import` statements,
 * records modules in dependency order, and reports resolution/cycle errors.
 */
class ModuleLoader {
 public:
  /** @brief Parsed module bundle retained by the loader. */
  struct LoadedModule {
    std::string file_path;           /**< Resolved source file path. */
    std::unique_ptr<ModuleStmt> ast; /**< Owned parsed module AST. */
  };

  /**
   * @brief Constructs a loader with import search roots.
   * @param roots Directories searched when resolving `import <name>`.
   */
  explicit ModuleLoader(std::vector<std::string> roots)
      : roots_(std::move(roots)) {}

  /**
   * @brief Loads entry files and all transitive imports.
   * @param entry_files Initial source files to compile.
   * @return `true` when all modules are resolved and parsed successfully.
   */
  bool LoadEntrypoints(const std::vector<std::string>& entry_files);

  /**
   * @brief Returns parsed modules in dependency order.
   *
   * Imported modules appear before modules that depend on them.
   */
  const std::vector<LoadedModule>& OrderedModules() const {
    return ordered_;
  }

  /** @brief Returns the most recent loader error message. */
  const std::string& LastError() const {
    return error_;
  }

 private:
  /** @brief DFS visitation marks for cycle detection. */
  enum class Mark { Unvisited, Visiting, Visited };

  std::vector<std::string> roots_;    /**< Import search roots. */
  std::vector<LoadedModule> ordered_; /**< Dependency-ordered modules. */
  std::unordered_map<std::string, Mark> marks_; /**< DFS state by file path. */
  std::unordered_map<std::string, std::string>
      module_to_path_; /**< Declared module name to file path map. */
  std::string error_;  /**< Last encountered error. */

  /**
   * @brief Recursively loads one file and its imports.
   * @param file_path Candidate module file path.
   * @param stack Current DFS stack for cycle reporting.
   */
  bool LoadFileRecursive(const std::string& file_path,
                         std::vector<std::string>& stack);

  /**
   * @brief Records a parsed module name and validates uniqueness.
   * @param file_path Source file declaring the module.
   * @param mod Parsed module AST node.
   */
  bool IndexModuleName(const std::string& file_path, const ModuleStmt& mod);

  /**
   * @brief Resolves an import name to a source file path.
   * @param mod_name Module name from `import` statement.
   * @return Resolved file path or empty string when unresolved.
   */
  std::string ResolveImportToPath(const std::string& mod_name) const;
};

#endif
