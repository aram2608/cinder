#include "cinder/frontend/lexer.hpp"

#include <cassert>
#include <iostream>
#include <string>
#include <unordered_map>

#include "cinder/frontend/tokens.hpp"

/// TODO: Fix the typing system so it is more ergonomic to work with
/// Handling the types at parse time and IR gen time is a bit awkward
static const std::unordered_map<std::string, TokenType> key_words = {
    {"int32", TT_INT32_SPECIFIER},
    {"int64", TT_INT64_SPECIFIER},
    {"flt32", TT_FLT32_SPECIFIER},
    {"str", TT_STR_SPECIFIER},
    {"bool", TT_BOOL_SPECIFIER},
    {"def", TT_DEF},
    {"end", TT_END},
    {"if", TT_IF},
    {"elif", TT_ELSEIF},
    {"else", TT_ELSE},
    {"for", TT_FOR},
    {"while", TT_WHILE},
    {"true", TT_TRUE},
    {"false", TT_FALSE},
    {"return", TT_RETURN},
    {"void", TT_VOID_SPECIFIER},
    {"extern", TT_EXTERN},
    {"mod", TT_MOD},
    {"import", TT_IMPORT},
    {"...", TT_ELLIPSIS},
};

Lexer::Lexer(std::string source_str_)
    : start_pos_(0),
      current_pos_(0),
      line_count_(1),
      source_str_(source_str_) {}

Lexer::Lexer(const char* source_str_)
    : start_pos_(0),
      current_pos_(0),
      line_count_(0),
      source_str_(source_str_) {}

void Lexer::ScanTokens() {
  while (!IsEnd()) {
    start_pos_ = current_pos_;
    Scan();
  }
  AddToken(TT_EOF);
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
      line_count_++;
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
      AddToken(TT_COMMA);
      break;
    case ':':
      AddToken(TT_COLON);
      break;
    case ';':
      AddToken(TT_SEMICOLON);
      break;
    // BINOPS
    case '+':
      AddToken(Match('+') ? TT_PLUS_PLUS : TT_PLUS);
      break;
    case '-':
      if (Match('>')) {
        AddToken(TT_ARROW);
      } else if (Match('-')) {
        AddToken(TT_MINUS_MINUS);
      } else {
        AddToken(TT_MINUS);
      }
      break;
    case '/':
      Match('/') ? ParseComment() : AddToken(TT_SLASH);
      break;
    case '%':
      AddToken(TT_MODULO);
      break;
    case '*':
      AddToken(TT_STAR);
      break;
    case '>':
      AddToken(Match('=') ? TT_GREATER_EQ : TT_GREATER);
      break;
    case '<':
      AddToken(Match('=') ? TT_LESSER_EQ : TT_LESSER);
      break;
    case '!':
      AddToken(Match('=') ? TT_BANGEQ : TT_BANG);
      break;
    case '=':
      AddToken(Match('=') ? TT_EQEQ : TT_EQ);
      break;
    case '[':
      AddToken(TT_LBRACKET);
      break;
    case ']':
      AddToken(TT_RBRACKET);
      break;
    case '(':
      AddToken(TT_LPAREN);
      break;
    case ')':
      AddToken(TT_RPAREN);
      break;
    case '{':
      AddToken(TT_LBRACE);
      break;
    case '}':
      AddToken(TT_RBRACE);
      break;
    default:
      assert(0 && "Unreachable");
  }
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

void Lexer::AddToken(TokenType tok_type) {
  size_t index = current_pos_ - start_pos_;
  std::string temp = source_str_.substr(start_pos_, index);
  tokens_.emplace_back(tok_type, line_count_, temp);
}

void Lexer::AddToken(TokenType tok_type, std::string lexeme,
                     std::optional<TokenValue> value) {
  tokens_.emplace_back(tok_type, line_count_, lexeme, value);
}

void Lexer::AddToken(TokenType tok_type, std::string lexeme) {
  tokens_.emplace_back(tok_type, line_count_, lexeme);
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
    std::cout << "Unterminated string at " << std::to_string(line_count_)
              << "\n";
    exit(1);
  }
  Advance();
  size_t index = current_pos_ - start_pos_ - 2;
  std::string value = source_str_.substr(start_pos_ + 1, index);
  EscapeCharacters(value);
  std::optional<TokenValue> literal;
  literal.emplace(std::in_place_type<std::string>, value);
  AddToken(TT_STR_LITERAL, value, literal);
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
  AddToken(TT_IDENTIFER, temp);
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
    AddToken(TT_FLT_LITERAL, temp, literal);
  } else {
    size_t index = current_pos_ - start_pos_;
    std::string temp = source_str_.substr(start_pos_, index);
    std::optional<TokenValue> literal;
    literal.emplace(std::in_place_type<int>, std::stoi(temp));
    AddToken(TT_INT_LITERAL, temp, literal);
  }
}

void Lexer::TokenizeDot() {
  if (PeekChar() == '.' && PeekNextChar() == '.') {
    Advance();
    Advance();
    AddToken(TT_ELLIPSIS);
  } else {
    AddToken(TT_DOT);
  }
}

bool Lexer::LookAheadOkay() {
  return current_pos_ + 1 < source_str_.size();
}

std::string Lexer::TokenToString(Token tok) {
  switch (tok.type) {
    case TT_QUOTE:
      return "\"";
    case TT_PLUS:
      return "+";
    case TT_PLUS_PLUS:
      return "++";
    case TT_MINUS:
      return "-";
    case TT_MINUS_MINUS:
      return "--";
    case TT_MODULO:
      return "%";
    case TT_STAR:
      return "*";
    case TT_SLASH:
      return "/";
    case TT_BANG:
      return "!";
    case TT_BANGEQ:
      return "!=";
    case TT_EQ:
      return "=";
    case TT_EQEQ:
      return "==";
    case TT_LESSER:
      return ">";
    case TT_LESSER_EQ:
      return ">=";
    case TT_GREATER:
      return "<";
    case TT_GREATER_EQ:
      return "<=";
    case TT_ARROW:
      return "->";
    case TT_LPAREN:
      return "(";
    case TT_RPAREN:
      return ")";
    case TT_LBRACE:
      return "{";
    case TT_RBRACE:
      return "}";
    case TT_LBRACKET:
      return "[";
    case TT_RBRACKET:
      return "]";
    case TT_COLON:
      return ":";
    case TT_SEMICOLON:
      return ";";
    case TT_COMMA:
      return ",";
    case TT_MOD:
      return "MOD";
    case TT_TRUE:
      return "true";
    case TT_FALSE:
      return "false";
    case TT_IF:
      return "IF";
    case TT_ELSEIF:
      return "ELIF";
    case TT_ELSE:
      return "ELSE";
    case TT_RETURN:
      return "RETURN";
    case TT_EXTERN:
      return "EXTERN";
    case TT_FOR:
      return "FOR";
    case TT_WHILE:
      return "WHILE";
    case TT_IDENTIFER:
      return "IDENTIFIER: " + tok.lexeme;
    case TT_DEF:
      return "DEF";
    case TT_END:
      return "END";
    case TT_EOF:
      return "EOF";
    case TT_ELLIPSIS:
      return "ELIPSIS";
    case TT_INT32_SPECIFIER:
      return "INT32 TYPE";
    case TT_INT64_SPECIFIER:
      return "INT64 TYPE";
    case TT_FLT32_SPECIFIER:
      return "FLOAT32 TYPE";
    case TT_FLT64_SPECIFIER:
      return "FLOAT64 TYPE";
    case TT_STR_SPECIFIER:
      return "STR TYPE";
    case TT_BOOL_SPECIFIER:
      return "BOOL TYPE";
    case TT_INT_LITERAL:
      return "INT LITERAL: " + tok.lexeme;
    case TT_FLT_LITERAL:
      return "FLT LITERAL: " + tok.lexeme;
    case TT_STR_LITERAL:
      return "STR LITERAL: " + tok.lexeme;
    case TT_COUNT:
      return "Number of tokens_";
    default:
      std::cout << "UNREACHABLE " << tok.type << " " << tok.lexeme << "\n";
      exit(1);
  }
}

std::vector<Token> Lexer::GetTokens() {
  return tokens_;
}
