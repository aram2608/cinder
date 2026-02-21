#ifndef MODULE_LOADER_H_
#define MODULE_LOADER_H_

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "cinder/ast/stmt/stmt.hpp"

class ModuleLoader {
 public:
  struct LoadedModule {
    std::string file_path;
    std::unique_ptr<ModuleStmt> ast;
  };

  explicit ModuleLoader(std::vector<std::string> roots)
      : roots_(std::move(roots)) {}

  bool LoadEntrypoints(const std::vector<std::string>& entry_files);
  const std::vector<LoadedModule>& OrderedModules() const {
    return ordered_;
  }
  const std::string& LastError() const {
    return error_;
  }

 private:
  enum class Mark { Unvisited, Visiting, Visited };

  std::vector<std::string> roots_;
  std::vector<LoadedModule> ordered_;
  std::unordered_map<std::string, Mark> marks_;
  std::unordered_map<std::string, std::string> module_to_path_;
  std::string error_;

  bool LoadFileRecursive(const std::string& file_path,
                         std::vector<std::string>& stack);
  bool IndexModuleName(const std::string& file_path, const ModuleStmt& mod);
  std::string ResolveImportToPath(const std::string& mod_name) const;
};

#endif