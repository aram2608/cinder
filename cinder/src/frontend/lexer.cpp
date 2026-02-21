#include "cinder/frontend/lexer.hpp"

#include <cassert>
#include <iostream>
#include <string>
#include <unordered_map>

#include "cinder/frontend/tokens.hpp"

using namespace cinder;

/// TODO: Fix the typing system so it is more ergonomic to work with
/// Handling the types at parse time and IR gen time is a bit awkward
static const std::unordered_map<std::string, Token::Type> key_words = {
    {"int32", Token::Type::INT32_SPECIFIER},
    {"int64", Token::Type::INT64_SPECIFIER},
    {"flt32", Token::Type::FLT32_SPECIFIER},
    {"flt64", Token::Type::FLT64_SPECIFIER},
    {"str", Token::Type::STR_SPECIFIER},
    {"bool", Token::Type::BOOL_SPECIFIER},
    {"struct", Token::Type::STRUCT_SPECIFIER},
    {"def", Token::Type::DEF},
    {"end", Token::Type::END},
    {"if", Token::Type::IF},
    {"elif", Token::Type::ELSEIF},
    {"else", Token::Type::ELSE},
    {"for", Token::Type::FOR},
    {"while", Token::Type::WHILE},
    {"true", Token::Type::TRUE},
    {"false", Token::Type::FALSE},
    {"return", Token::Type::RETURN},
    {"void", Token::Type::VOID_SPECIFIER},
    {"extern", Token::Type::EXTERN},
    {"mod", Token::Type::MOD},
    {"import", Token::Type::IMPORT},
    {"...", Token::Type::ELLIPSIS},
};

Lexer::Lexer(std::string source_str_)
    : start_pos_(0),
      current_pos_(0),
      line_(1),
      column_(1),
      source_str_(source_str_) {}

Lexer::Lexer(const char* source_str_)
    : start_pos_(0),
      current_pos_(0),
      line_(1),
      column_(1),
      source_str_(source_str_) {}

void Lexer::ScanTokens() {
  while (!IsEnd()) {
    start_pos_ = current_pos_;
    Scan();
  }
  AddToken(Token::Type::EOF_);
}

void Lexer::Scan() {
  char c = Advance();

  if (IsAlpha(c)) {
    TokenizeIdentifier();
    return;
  }

  if (IsNumeric(c)) {
    TokenizeNumber();
    return;
  }

  switch (c) {
    case '\n':
      line_++;
      column_ = 1;
      break;
    case '\r':
      break;
    case '\t':
      break;
    case ' ':
      break;
    case '\0':
      break;
    case '.':
      TokenizeDot();
      break;
    case '"':
      TokenizeString();
      break;
    case ',':
      AddToken(Token::Type::COMMA);
      break;
    case ':':
      AddToken(Token::Type::COLON);
      break;
    case ';':
      AddToken(Token::Type::SEMICOLON);
      break;
    // BINOPS
    case '+':
      AddToken(Match('+') ? Token::Type::PlusPlus : Token::Type::Plus);
      break;
    case '-':
      if (Match('>')) {
        AddToken(Token::Type::ARROW);
      } else if (Match('-')) {
        AddToken(Token::Type::MinusMinus);
      } else {
        AddToken(Token::Type::Minus);
      }
      break;
    case '/':
      Match('/') ? ParseComment() : AddToken(Token::Type::SLASH);
      break;
    case '%':
      AddToken(Token::Type::MODULO);
      break;
    case '*':
      AddToken(Token::Type::STAR);
      break;
    case '>':
      AddToken(Match('=') ? Token::Type::GREATER_EQ : Token::Type::GREATER);
      break;
    case '<':
      AddToken(Match('=') ? Token::Type::LESSER_EQ : Token::Type::LESSER);
      break;
    case '!':
      AddToken(Match('=') ? Token::Type::BANGEQ : Token::Type::BANG);
      break;
    case '=':
      AddToken(Match('=') ? Token::Type::EQEQ : Token::Type::EQ);
      break;
    case '[':
      AddToken(Token::Type::LBRACKET);
      break;
    case ']':
      AddToken(Token::Type::RBRACKET);
      break;
    case '(':
      AddToken(Token::Type::LPAREN);
      break;
    case ')':
      AddToken(Token::Type::RPAREN);
      break;
    case '{':
      AddToken(Token::Type::LBRACE);
      break;
    case '}':
      AddToken(Token::Type::RBRACE);
      break;
    default:
      assert(0 && "Unreachable");
  }
  ++column_;
}

char Lexer::Advance() {
  if (!IsEnd()) {
    return source_str_[current_pos_++];
  }
  return '\0';
}

bool Lexer::IsEnd() {
  return current_pos_ >= source_str_.length();
}

bool Lexer::IsAlpha(char c) {
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c == '_');
}

bool Lexer::IsNumeric(char c) {
  return (c >= '0' && c <= '9');
}

bool Lexer::IsAlphaNumeric(char c) {
  return IsAlpha(c) || IsNumeric(c);
}

char Lexer::PeekChar() {
  if (!IsEnd()) {
    return source_str_[current_pos_];
  }
  return '\0';
}

char Lexer::PeekNextChar() {
  if (!IsEnd() && LookAheadOkay()) {
    return source_str_[current_pos_ + 1];
  }
  return '\0';
}

bool Lexer::Match(char c) {
  if (IsEnd()) {
    return false;
  }

  if (PeekChar() == c) {
    current_pos_++;
    return true;
  }
  return false;
}

void Lexer::AddToken(Token::Type tok_type) {
  size_t index = current_pos_ - start_pos_;
  std::string temp = source_str_.substr(start_pos_, index);
  SourceLocation loc{current_pos_, line_, column_};
  tokens_.emplace_back(tok_type, loc, temp);
}

void Lexer::AddToken(Token::Type tok_type, std::string lexeme,
                     std::optional<TokenValue> value) {
  SourceLocation loc{current_pos_, line_, column_};

  tokens_.emplace_back(tok_type, loc, lexeme, value);
}

void Lexer::AddToken(Token::Type tok_type, std::string lexeme) {
  SourceLocation loc{current_pos_, line_, column_};
  tokens_.emplace_back(tok_type, loc, lexeme);
}

void Lexer::ParseComment() {
  while (!IsEnd() && PeekChar() != '\n') {
    Advance();
  }
}

void Lexer::EmitTokens() {
  for (auto it = tokens_.begin(); it != tokens_.end(); it++) {
    std::cout << TokenToString(*it) << "\n";
  }
}

void Lexer::TokenizeString() {
  while (!IsEnd() && PeekChar() != '"') {
    Advance();
  }
  if (IsEnd()) {
    std::cout << "Unterminated string at " << std::to_string(line_) << "\n";
    exit(1);
  }
  Advance();
  size_t index = current_pos_ - start_pos_ - 2;
  std::string value = source_str_.substr(start_pos_ + 1, index);
  EscapeCharacters(value);
  std::optional<TokenValue> literal;
  literal.emplace(std::in_place_type<std::string>, value);
  AddToken(Token::Type::STR_LITERAL, value, literal);
}

void Lexer::EscapeCharacters(std::string& str) {
  std::string temp = "";
  for (auto c = str.begin(); c != str.end(); c++) {
    if (*c == '\\') {
      c++;
      switch (*c) {
        case '"':
          temp += '"';
          break;
        case 'n':
          temp += '\n';
          break;
        case 't':
          temp += '\t';
          break;
        default:
          temp += *c;
          break;
      }
    } else {
      temp += *c;
    }
  }
  str = temp;
}

void Lexer::TokenizeIdentifier() {
  while (!IsEnd() && IsAlphaNumeric(PeekChar())) {
    Advance();
  }
  size_t index = current_pos_ - start_pos_;
  std::string temp = source_str_.substr(start_pos_, index);

  auto match = key_words.find(temp);
  if (match != key_words.end()) {
    AddToken(match->second, temp);
    return;
  }
  AddToken(Token::Type::IDENTIFER, temp);
}

void Lexer::TokenizeNumber() {
  while (!IsEnd() && IsNumeric(PeekChar())) {
    Advance();
  }
  if (PeekChar() == '.' && IsNumeric(PeekNextChar())) {
    Advance();
    while (!IsEnd() && IsNumeric(PeekChar())) {
      Advance();
    }
    size_t index = current_pos_ - start_pos_;
    std::string temp = source_str_.substr(start_pos_, index);
    std::optional<TokenValue> literal;
    literal.emplace(std::in_place_type<float>, std::stof(temp));
    AddToken(Token::Type::FLT_LITERAL, temp, literal);
  } else {
    size_t index = current_pos_ - start_pos_;
    std::string temp = source_str_.substr(start_pos_, index);
    std::optional<TokenValue> literal;
    literal.emplace(std::in_place_type<int>, std::stoi(temp));
    AddToken(Token::Type::INT_LITERAL, temp, literal);
  }
}

void Lexer::TokenizeDot() {
  if (PeekChar() == '.' && PeekNextChar() == '.') {
    Advance();
    Advance();
    AddToken(Token::Type::ELLIPSIS);
  } else {
    AddToken(Token::Type::DOT);
  }
}

bool Lexer::LookAheadOkay() {
  return current_pos_ + 1 < source_str_.size();
}

std::string Lexer::TokenToString(Token tok) {
  switch (tok.kind) {
    case Token::Type::QUOTE:
      return "\"";
    case Token::Type::Plus:
      return "+";
    case Token::Type::PlusPlus:
      return "++";
    case Token::Type::Minus:
      return "-";
    case Token::Type::MinusMinus:
      return "--";
    case Token::Type::MODULO:
      return "%";
    case Token::Type::STAR:
      return "*";
    case Token::Type::SLASH:
      return "/";
    case Token::Type::BANG:
      return "!";
    case Token::Type::BANGEQ:
      return "!=";
    case Token::Type::EQ:
      return "=";
    case Token::Type::EQEQ:
      return "==";
    case Token::Type::LESSER:
      return ">";
    case Token::Type::LESSER_EQ:
      return ">=";
    case Token::Type::GREATER:
      return "<";
    case Token::Type::GREATER_EQ:
      return "<=";
    case Token::Type::ARROW:
      return "->";
    case Token::Type::LPAREN:
      return "(";
    case Token::Type::RPAREN:
      return ")";
    case Token::Type::LBRACE:
      return "{";
    case Token::Type::RBRACE:
      return "}";
    case Token::Type::LBRACKET:
      return "[";
    case Token::Type::RBRACKET:
      return "]";
    case Token::Type::COLON:
      return ":";
    case Token::Type::SEMICOLON:
      return ";";
    case Token::Type::COMMA:
      return ",";
    case Token::Type::MOD:
      return "MOD";
    case Token::Type::TRUE:
      return "true";
    case Token::Type::FALSE:
      return "false";
    case Token::Type::IF:
      return "IF";
    case Token::Type::ELSEIF:
      return "ELIF";
    case Token::Type::ELSE:
      return "ELSE";
    case Token::Type::RETURN:
      return "RETURN";
    case Token::Type::EXTERN:
      return "EXTERN";
    case Token::Type::FOR:
      return "FOR";
    case Token::Type::WHILE:
      return "WHILE";
    case Token::Type::IDENTIFER:
      return "IDENTIFIER: " + tok.lexeme;
    case Token::Type::DEF:
      return "DEF";
    case Token::Type::END:
      return "END";
    case Token::Type::EOF_:
      return "EOF";
    case Token::Type::ELLIPSIS:
      return "ELIPSIS";
    case Token::Type::INT32_SPECIFIER:
      return "INT32 TYPE";
    case Token::Type::INT64_SPECIFIER:
      return "INT64 TYPE";
    case Token::Type::FLT32_SPECIFIER:
      return "FLOAT32 TYPE";
    case Token::Type::FLT64_SPECIFIER:
      return "FLOAT64 TYPE";
    case Token::Type::STR_SPECIFIER:
      return "STR TYPE";
    case Token::Type::BOOL_SPECIFIER:
      return "BOOL TYPE";
    case Token::Type::INT_LITERAL:
      return "INT LITERAL: " + tok.lexeme;
    case Token::Type::FLT_LITERAL:
      return "FLT LITERAL: " + tok.lexeme;
    case Token::Type::STR_LITERAL:
      return "STR LITERAL: " + tok.lexeme;
    case Token::Type::COUNT:
      return "Number of tokens_";
    default:
      std::cout << "UNREACHABLE " << (int)tok.kind << " " << tok.lexeme << "\n";
      exit(1);
  }
}

std::vector<Token> Lexer::GetTokens() {
  return tokens_;
}
