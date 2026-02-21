#include <memory>
#include <string>
#include <vector>

#include "cinder/ast/stmt/stmt.hpp"
#include "cinder/frontend/lexer.hpp"
#include "cinder/frontend/parser.hpp"
#include "cinder/semantic/semantic_analyzer.hpp"
#include "cinder/semantic/type_context.hpp"
#include "gtest/gtest.h"

namespace {

std::unique_ptr<ModuleStmt> ParseModuleFromSource(const std::string& source) {
  Lexer lexer(source);
  lexer.ScanTokens();
  Parser parser(lexer.GetTokens());

  std::unique_ptr<Stmt> root = parser.Parse();
  auto* mod = dynamic_cast<ModuleStmt*>(root.release());
  EXPECT_NE(mod, nullptr);
  return std::unique_ptr<ModuleStmt>(mod);
}

TEST(SemanticQualifiedTypeTest, AcceptsQualifiedStructTypeDeclaration) {
  auto math = ParseModuleFromSource(R"(
mod math;

struct Vector2
  int32: x;
  int32: y;
end

def sum(Vector2 p) -> int32
  return p.x + p.y;
end
)");

  auto main = ParseModuleFromSource(R"(
mod main;
import math;

def main() -> int32
  math.Vector2: p = math.Vector2(1, 2);
  int32: result = math.sum(p);
  return result;
end
)");

  std::vector<ModuleStmt*> modules{math.get(), main.get()};
  TypeContext types;
  SemanticAnalyzer analyzer(types);
  analyzer.AnalyzeProgram(modules);

  EXPECT_FALSE(analyzer.HadError());
}

TEST(SemanticQualifiedTypeTest, RejectsUnknownQualifiedType) {
  auto math = ParseModuleFromSource(R"(
mod math;

struct Vector2
  int32: x;
  int32: y;
end
)");

  auto main = ParseModuleFromSource(R"(
mod main;

def main() -> int32
  math.Missing: p = math.Vector2(1, 2);
  return 0;
end
)");

  std::vector<ModuleStmt*> modules{math.get(), main.get()};
  TypeContext types;
  SemanticAnalyzer analyzer(types);
  analyzer.AnalyzeProgram(modules);

  EXPECT_TRUE(analyzer.HadError());
}

TEST(SemanticQualifiedTypeTest, RejectsStructConstructorArgTypeMismatch) {
  auto math = ParseModuleFromSource(R"(
mod math;

struct Vector2
  int32: x;
  int32: y;
end

def sum(Vector2 p) -> int32
  return p.x + p.y;
end
)");

  auto main = ParseModuleFromSource(R"(
mod main;
import math;

def main() -> int32
  math.Vector2: p = math.Vector2("bad", 2);
  return math.sum(p);
end
)");

  std::vector<ModuleStmt*> modules{math.get(), main.get()};
  TypeContext types;
  SemanticAnalyzer analyzer(types);
  analyzer.AnalyzeProgram(modules);

  EXPECT_TRUE(analyzer.HadError());
}

}  // namespace
