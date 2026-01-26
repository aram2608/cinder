#include "../include/lexer.hpp"

static const std::unordered_map<std::string, TokenType> key_words = {
    {"int", TT_INT_SPECIFIER},
    {"flt", TT_FLT_SPECIFIER},
    {"str", TT_STR_SPECIFIER},
    {"bool", TT_BOOL_SPECIFIER},
    {"def", TT_DEF},
    {"end", TT_END},
    {"if", TT_IF},
    {"elif", TT_ELSEIF},
    {"else", TT_ELSE},
    {"for", TT_FOR},
    {"true", TT_TRUE},
    {"false", TT_FALSE},
    {"return", TT_RETURN},
    {"void", TT_VOID_SPECIFIER},
    {"extern", TT_EXTERN},
    {"mod", TT_MOD},
    {"import", TT_IMPORT},
};

Lexer::Lexer(std::string source_str)
    : start_pos(0), current_pos(0), line_count(0), source_str(source_str) {}

Lexer::Lexer(const char* source_str)
    : start_pos(0), current_pos(0), line_count(0), source_str(source_str) {}

void Lexer::ScanTokens() {
  while (!IsEnd()) {
    start_pos = current_pos;
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
      line_count++;
      break;
    case '\r':
      break;
    case '\t':
      break;
    case ' ':
      break;
    case '\0':
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
      if (Match('/')) {
        while (PeekChar() != '\n') {
          Advance();
        }
        break;
      }
      AddToken(TT_SLASH);
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
    return source_str[current_pos++];
  }
  return '\0';
}

bool Lexer::IsEnd() {
  return current_pos >= source_str.length();
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
  return source_str[current_pos];
}

char Lexer::PeekNextChar() {
  if (!IsEnd()) {
    return source_str[current_pos + 1];
  }
  return '\0';
}

bool Lexer::Match(char c) {
  if (IsEnd()) {
    return false;
  }

  if (PeekChar() == c) {
    current_pos++;
    return true;
  }
  return false;
}

void Lexer::AddToken(TokenType tok_type) {
  size_t index = current_pos - start_pos;
  std::string temp = source_str.substr(start_pos, index);
  tokens.emplace_back(tok_type, line_count, temp, VT_NULL, temp);
}

void Lexer::AddToken(TokenType tok_type, std::string lexeme,
                     ValueType value_type, TokenValue value) {
  tokens.emplace_back(tok_type, line_count, lexeme, value_type, value);
}

void Lexer::EmitTokens() {
  for (auto it = tokens.begin(); it != tokens.end(); it++) {
    std::cout << TokenToString(*it) << "\n";
  }
}

void Lexer::TokenizeString() {
  while (!IsEnd() && PeekChar() != '"') {
    Advance();
  }
  if (IsEnd()) {
    std::cout << std::to_string(start_pos) << "\n";
    exit(1);
  }
  Advance();
  size_t index = current_pos - start_pos - 2;
  std::string value = source_str.substr(start_pos + 1, index);
  AddToken(TT_STR, value, VT_STR, value);
}

void Lexer::TokenizeIdentifier() {
  while (!IsEnd() && IsAlphaNumeric(PeekChar())) {
    Advance();
  }
  size_t index = current_pos - start_pos;
  std::string temp = source_str.substr(start_pos, index);

  auto match = key_words.find(temp);
  if (match != key_words.end()) {
    AddToken(match->second, temp, VT_NULL, temp);
    return;
  }
  AddToken(TT_IDENTIFER, temp, VT_NULL, temp);
}

// TODO: Implement floats
void Lexer::TokenizeNumber() {
  while (!IsEnd() && IsNumeric(PeekChar())) {
    Advance();
  }
  if (PeekChar() == '.' && IsNumeric(PeekNextChar())) {
    Advance();
    while (!IsEnd() && IsNumeric(PeekChar())) {
      Advance();
    }
    size_t index = current_pos - start_pos;
    std::string temp = source_str.substr(start_pos, index);
    AddToken(TT_FLT, temp, VT_FLT, std::stof(temp));
  } else {
    size_t index = current_pos - start_pos;
    std::string temp = source_str.substr(start_pos, index);
    AddToken(TT_INT, temp, VT_INT, std::stoi(temp));
  }
}

std::string Lexer::TokenToString(Token tok) {
  switch (tok.token_type) {
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
    case TT_IDENTIFER:
      return "IDENTIFIER: " + tok.lexeme;
    case TT_DEF:
      return "DEF";
    case TT_END:
      return "END";
    case TT_EOF:
      return "EOF";
    case TT_INT_SPECIFIER:
      return "INT TYPE";
    case TT_FLT_SPECIFIER:
      return "FLOAT TYPE";
    case TT_STR_SPECIFIER:
      return "STR TYPE";
    case TT_BOOL_SPECIFIER:
      return "BOOL TYPE";
    case TT_INT:
      return "INT LITERAL: " + tok.lexeme;
    case TT_FLT:
      return "FLT LITERAL: " + tok.lexeme;
    case TT_STR:
      return "STR LITERAL: " + tok.lexeme;
    case TT_COUNT:
      return "Number of tokens";
    default:
      std::cout << "UNREACHABLE " << tok.token_type << "\n";
      exit(1);
  }
}
