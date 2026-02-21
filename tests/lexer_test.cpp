#include "cinder/frontend/lexer.hpp"

#include <string>
#include <vector>

#include "cinder/frontend/tokens.hpp"
#include "gtest/gtest.h"

namespace {

std::vector<cinder::Token> TokenizeFromSource(const std::string& source) {
  Lexer lexer(source);
  lexer.ScanTokens();
  EXPECT_GT(lexer.GetTokens().size(), 0u);
  return lexer.GetTokens();
}

}  // namespace

TEST(LexerTest, LexKeyWords) {
  auto toks = TokenizeFromSource(R"(
    mod 
    main 
    def
    end
    if
    elif
    else 
    int32
    int64
    flt32
    flt64
    str
    bool
    struct
    for
    while
    return
    void
    extern
    import
    ...
    )");

  const cinder::Token::Type TYPES[] = {
      cinder::Token::Type::MOD,
      cinder::Token::Type::IDENTIFER,
      cinder::Token::Type::DEF,
      cinder::Token::Type::END,
      cinder::Token::Type::IF,
      cinder::Token::Type::ELSEIF,
      cinder::Token::Type::ELSE,
      cinder::Token::Type::INT32_SPECIFIER,
      cinder::Token::Type::INT64_SPECIFIER,
      cinder::Token::Type::FLT32_SPECIFIER,
      cinder::Token::Type::FLT64_SPECIFIER,
      cinder::Token::Type::STR_SPECIFIER,
      cinder::Token::Type::BOOL_SPECIFIER,
      cinder::Token::Type::STRUCT_SPECIFIER,
      cinder::Token::Type::FOR,
      cinder::Token::Type::WHILE,
      cinder::Token::Type::RETURN,
      cinder::Token::Type::VOID_SPECIFIER,
      cinder::Token::Type::EXTERN,
      cinder::Token::Type::IMPORT,
      cinder::Token::Type::ELLIPSIS,
  };

  for (auto it = toks.size(); it < toks.size(); it++) {
    ASSERT_EQ(toks[it].kind, TYPES[it]);
  }
}