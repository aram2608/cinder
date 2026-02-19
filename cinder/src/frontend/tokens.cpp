#include "cinder/frontend/tokens.hpp"

using namespace cinder;

Token::Token(Token::Type kind, SourceLocation loc, std::string lexeme,
             std::optional<TokenValue> literal)
    : kind(kind), location(loc), lexeme(lexeme), literal(literal) {}

bool Token::IsLiteral() {
  return kind == Type::FLT_LITERAL || kind == Type::INT_LITERAL ||
         kind == Type::STR_LITERAL;
}

bool Token::IsInt() {
  return kind == Type::INT32_SPECIFIER || kind == Type::INT64_SPECIFIER;
}

bool Token::IsFloat() {
  return kind == Type::FLT32_SPECIFIER || kind == Type::FLT64_SPECIFIER;
}

bool Token::IsString() {
  return kind == Type::STR_SPECIFIER;
}

bool Token::IsVoid() {
  return kind == Type::VOID_SPECIFIER;
}

bool Token::IsPrimitive() {
  return IsFloat() || IsBool() || IsInt() || IsFloat() || IsString() ||
         IsVoid();
}

bool Token::IsStruct() {
  return kind == Type::STRUCT_SPECIFIER;
}

bool Token::IsIdentifier() {
  return kind == Type::IDENTIFER;
}

bool Token::IsBool() {
  return kind == Type::BOOL_SPECIFIER;
}

bool Token::IsFactor() {
  return kind == Type::STAR || kind == Type::SLASH;
}

bool Token::IsTerm() {
  return kind == Type::Plus || kind == Type::Minus;
}

bool Token::IsThisType(Type type) {
  return kind == type;
}

bool Token::IsEOF() {
  return kind == Type::EOF_;
}

bool Token::IsComparison() {
  return kind == Type::LESSER || kind == Type::LESSER_EQ ||
         kind == Type::GREATER || kind == Type::GREATER_EQ ||
         kind == Type::BANGEQ || kind == Type::EQEQ;
}