#include "cinder/frontend/module_loader.hpp"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <system_error>

#include "cinder/ast/stmt/stmt.hpp"
#include "cinder/frontend/lexer.hpp"
#include "cinder/frontend/parser.hpp"

static std::string ReadEntireFile(std::string file_path) {
  std::fstream file{file_path};

  if (!file.is_open()) {
    return "";
  }

  std::string content((std::istreambuf_iterator<char>(file)),
                      std::istreambuf_iterator<char>());

  file.close();
  return content;
}

bool ModuleLoader::LoadEntrypoints(
    const std::vector<std::string>& entry_files) {
  ordered_.clear();
  marks_.clear();
  module_to_path_.clear();
  error_.clear();

  std::vector<std::string> stack;
  for (const auto& entry : entry_files) {
    if (!LoadFileRecursive(entry, stack)) {
      return false;
    }
  }
  return true;
}

bool ModuleLoader::LoadFileRecursive(const std::string& file_path,
                                     std::vector<std::string>& stack) {
  std::string normalized_path = file_path;
  {
    std::error_code ec;
    auto normalized = std::filesystem::weakly_canonical(file_path, ec);
    if (!ec) {
      normalized_path = normalized.string();
    }
  }

  auto mark_it = marks_.find(normalized_path);
  if (mark_it != marks_.end()) {
    if (mark_it->second == Mark::Visited) return true;
    if (mark_it->second == Mark::Visiting) {
      std::ostringstream cycle;
      cycle << "Import cycle detected: ";
      auto start = stack.begin();
      for (; start != stack.end(); ++start) {
        if (*start == normalized_path) {
          break;
        }
      }

      for (auto it = start; it != stack.end(); ++it) {
        if (it != start) {
          cycle << " -> ";
        }
        cycle << *it;
      }
      if (start != stack.end()) {
        cycle << " -> " << normalized_path;
      } else {
        cycle << normalized_path;
      }

      error_ = cycle.str();
      return false;
    }
  }

  marks_[normalized_path] = Mark::Visiting;
  stack.push_back(normalized_path);

  // read file, lex, parse
  std::string source = ReadEntireFile(normalized_path);
  if (source.empty()) {
    error_ = "Could not read module file: " + normalized_path;
    return false;
  }

  Lexer lexer{source};
  lexer.ScanTokens();
  Parser parser{lexer.GetTokens()};
  std::unique_ptr<Stmt> root = parser.Parse();

  auto casted = root->CastTo<ModuleStmt>();
  if (std::error_code ec = casted.getError()) {
    error_ = "Root is not a module for file: " + normalized_path;
    return false;
  }

  ModuleStmt* module = casted.get();

  if (!IndexModuleName(normalized_path, *module)) return false;

  for (auto& s : module->stmts) {
    auto* imp = dynamic_cast<ImportStmt*>(s.get());
    if (!imp) continue;
    std::string dep_path = ResolveImportToPath(imp->mod_name.lexeme);
    if (dep_path.empty()) {
      std::filesystem::path sibling =
          std::filesystem::path(normalized_path).parent_path() /
          (imp->mod_name.lexeme + ".ci");
      std::error_code ec;
      if (std::filesystem::exists(sibling, ec) && !ec) {
        dep_path = sibling.string();
      }
    }
    if (dep_path.empty()) {
      error_ = "Could not resolve import '" + imp->mod_name.lexeme + "' from " +
               normalized_path;
      return false;
    }
    if (!LoadFileRecursive(dep_path, stack)) return false;
  }

  marks_[normalized_path] = Mark::Visited;
  stack.pop_back();

  ordered_.push_back(
      {normalized_path,
       std::unique_ptr<ModuleStmt>(static_cast<ModuleStmt*>(root.release()))});
  return true;
}

bool ModuleLoader::IndexModuleName(const std::string& file_path,
                                   const ModuleStmt& mod) {
  auto it = module_to_path_.find(mod.name.lexeme);
  if (it != module_to_path_.end() && it->second != file_path) {
    error_ = "Duplicate module name '" + mod.name.lexeme + "' in " +
             it->second + " and " + file_path;
    return false;
  }
  module_to_path_[mod.name.lexeme] = file_path;
  return true;
}

std::string ModuleLoader::ResolveImportToPath(
    const std::string& mod_name) const {
  auto indexed = module_to_path_.find(mod_name);
  if (indexed != module_to_path_.end()) {
    return indexed->second;
  }

  const std::string filename = mod_name + ".ci";
  for (const auto& root : roots_) {
    std::filesystem::path candidate = std::filesystem::path(root) / filename;
    std::error_code ec;
    if (std::filesystem::exists(candidate, ec) && !ec) {
      return candidate.string();
    }
  }

  return "";
}
