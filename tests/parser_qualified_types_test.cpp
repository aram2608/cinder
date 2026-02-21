#include <memory>
#include <string>

#include "cinder/ast/stmt/stmt.hpp"
#include "cinder/frontend/lexer.hpp"
#include "cinder/frontend/parser.hpp"
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

}  // namespace

TEST(ParserQualifiedTypeTest, ParsesQualifiedVarDeclarationType) {
  auto module = ParseModuleFromSource(R"(
mod main;
def main() -> int32
  math.Vector2: p = math.Vector2(10, 20);
  return 0;
end
)");

  ASSERT_EQ(module->stmts.size(), 1u);

  auto* fn = dynamic_cast<FunctionStmt*>(module->stmts[0].get());
  ASSERT_NE(fn, nullptr);
  ASSERT_FALSE(fn->body.empty());

  auto* decl = dynamic_cast<VarDeclarationStmt*>(fn->body[0].get());
  ASSERT_NE(decl, nullptr);
  EXPECT_EQ(decl->type.kind, cinder::Token::Type::IDENTIFER);
  EXPECT_EQ(decl->type.lexeme, "math.Vector2");
}

TEST(ParserQualifiedTypeTest, ParsesQualifiedFunctionArgAndReturnType) {
  auto module = ParseModuleFromSource(R"(
mod main;
def make(math.Vector2 p) -> math.Vector2
  return p;
end
)");

  ASSERT_EQ(module->stmts.size(), 1u);

  auto* fn = dynamic_cast<FunctionStmt*>(module->stmts[0].get());
  ASSERT_NE(fn, nullptr);

  std::error_code ec;
  auto* proto = fn->proto->CastTo<FunctionProto>(ec);
  ASSERT_EQ(ec.value(), 0);
  ASSERT_NE(proto, nullptr);
  ASSERT_EQ(proto->args.size(), 1u);

  EXPECT_EQ(proto->args[0].type_token.kind, cinder::Token::Type::IDENTIFER);
  EXPECT_EQ(proto->args[0].type_token.lexeme, "math.Vector2");
  EXPECT_EQ(proto->return_type.kind, cinder::Token::Type::IDENTIFER);
  EXPECT_EQ(proto->return_type.lexeme, "math.Vector2");
}
