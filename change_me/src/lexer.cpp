#include "../include/lexer.hpp"

static const std::unordered_map<std::string, TokenType> key_words = {
    {"int", TT_INT_SPECIFIER}, {"float", TT_FLT_SPECIFIER},
    {"str", TT_STR_SPECIFIER}, {"def", TT_DEF},
    {"end", TT_END},           {"if", TT_IF},
    {"elif", TT_ELSEIF},       {"else", TT_ELSE},
    {"for", TT_FOR},
};

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
      AddToken(TT_QUOTE);
      break;
    case ':':
      AddToken(TT_COLON);
      break;
    case ';':
      AddToken(TT_SEMICOLON);
      break;
    // BINOPS
    case '+':
      AddToken(TT_PLUS);
      break;
    case '-':
      AddToken(TT_MINUS);
      break;
    case '/':
      AddToken(TT_SLASH);
      break;
    case '%':
      AddToken(TT_MODULO);
      break;
    case '*':
      AddToken(TT_STAR);
      break;
    case '!':
      if (PeekNextChar() == '=') {
        AddToken(TT_BANGEQ);
      } else {
        AddToken(TT_BANG);
      }
      break;
    case '=':
      if (PeekNextChar() == '=') {
        AddToken(TT_EQEQ);
      } else {
        AddToken(TT_EQ);
      }
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
  return source_str[current_pos] == '\0';
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

void Lexer::AddToken(TokenType tok_type) {
  tokens.emplace_back(tok_type, line_count);
}

void Lexer::AddToken(TokenType tok_type, std::string lexeme,
                     ValueType value_type, Value value) {
  tokens.emplace_back(tok_type, line_count, lexeme, value_type, value);
}

void Lexer::EmitTokens() {
  for (auto it = tokens.begin(); it != tokens.end(); it++) {
    printf("%s\n", TokenTypeToString(it->token_type));
  }
}

void Lexer::TokenizeIdentifier() {
  while (!IsEnd() && IsAlphaNumeric(PeekChar())) {
    Advance();
  }
  size_t index = current_pos - start_pos;
  std::string temp = source_str.substr(start_pos, index);

  auto match = key_words.find(temp);
  if (match != key_words.end()) {
    AddToken(match->second);
    return;
  }
  // AddToken(TT_IDENTIFER, temp, VT_INT, std::stoi(temp));
}

// TODO: Implement floats
void Lexer::TokenizeNumber() {
  while (!IsEnd() && IsNumeric(PeekChar())) {
    Advance();
  }
  size_t index = current_pos - start_pos;
  std::string temp = source_str.substr(start_pos, index);
  // AddToken(TT_IDENTIFER, temp, VT_INT, std::stoi(temp));
}

const char* Lexer::TokenTypeToString(TokenType tok_type) {
  switch (tok_type) {
    case TT_QUOTE:
      return "\"";
    case TT_PLUS:
      return "+";
    case TT_MINUS:
      return "-";
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
    case TT_IF:
      return "IF";
    case TT_ELSEIF:
      return "ELIF";
    case TT_ELSE:
      return "ELSE";
    case TT_FOR:
      return "FOR";
    case TT_IDENTIFER:
      return "IDENTIFIER";
    case TT_DEF:
      return "DEF";
    case TT_END:
      return "END";
    case TT_ERROR:
      return "ERROR";
    case TT_EOF:
      return "EOF";
    case TT_INT_SPECIFIER:
      return "INT";
    case TT_FLT_SPECIFIER:
      return "FLOAT";
    case TT_STR_SPECIFIER:
      return "STR";
    case TT_COUNT:
      return "Number of tokens";
    default:
      assert(0 && "Unreachable");
  }
}
