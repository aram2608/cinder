#include "../include/lexer.h"

Lexer* NewLexer(char* source_str) {
  Lexer* lexer = (Lexer*)malloc(sizeof(Lexer) + LEXER_INIT * sizeof(Token));
  lexer->source_str = (char*)malloc(strlen(source_str));
  strcpy(lexer->source_str, source_str);

  lexer->array_pos = 0;
  lexer->line_count = 0;
  lexer->current_pos = 0;
  lexer->start_pos = 0;
  lexer->capacity = LEXER_INIT;
  free(source_str);

  return lexer;
}

void ResizeLexer(Lexer* lexer, size_t new_cap) {
  Lexer* temp =
      (Lexer*)realloc(lexer, sizeof(Lexer) + new_cap * (sizeof(Token)));
  if (temp == NULL) {
    perror("failed to resize lexer");
    abort();
  }

  lexer = temp;
  lexer->capacity = new_cap;
}

void ScanTokens(Lexer* lexer) {
  while (!IsEnd(lexer)) {
    if (lexer->array_pos < lexer->capacity) {
      lexer->start_pos = lexer->current_pos;
      Scan(lexer);
    } else {
      ResizeLexer(lexer, lexer->capacity + BUMP_CAP);
    }
  }
  AddToken(lexer, TT_EOF);
}

void Scan(Lexer* lexer) {
  char c = Advance(lexer);

  if (IsAlpha(c)) {
    TokenizeIdentifier(lexer);
    return;
  }

  if (IsNumeric(c)) {
    printf("TODO: Implement numbers\n");
    return;
  }

  switch (c) {
    case '\n':
      lexer->line_count++;
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
      AddToken(lexer, TT_QUOTE);
      break;
    case ':':
      AddToken(lexer, TT_COLON);
      break;
    case ';':
      AddToken(lexer, TT_SEMICOLON);
      break;
    // BINOPS
    case '+':
      AddToken(lexer, TT_PLUS);
      break;
    case '-':
      AddToken(lexer, TT_MINUS);
      break;
    case '/':
      AddToken(lexer, TT_SLASH);
      break;
    case '%':
      AddToken(lexer, TT_MODULO);
      break;
    case '*':
      AddToken(lexer, TT_STAR);
      break;
    case '!':
      if (PeekNextChar(lexer) == '=') {
        AddToken(lexer, TT_BANGEQ);
      } else {
        AddToken(lexer, TT_BANG);
      }
      break;
    case '=':
      if (PeekNextChar(lexer) == '=') {
        AddToken(lexer, TT_EQEQ);
      } else {
        AddToken(lexer, TT_EQ);
      }
    case '[':
      AddToken(lexer, TT_LBRACKET);
      break;
    case ']':
      AddToken(lexer, TT_RBRACKET);
      break;
    case '(':
      AddToken(lexer, TT_LPAREN);
      break;
    case ')':
      AddToken(lexer, TT_RPAREN);
      break;
    case '{':
      AddToken(lexer, TT_LBRACE);
      break;
    case '}':
      AddToken(lexer, TT_RBRACE);
      break;
    default:
      assert(0 && "Unreachable");
  }
}

char Advance(Lexer* lexer) {
  if (!IsEnd(lexer)) {
    return lexer->source_str[lexer->current_pos++];
  }
  return '\0';
}

bool IsEnd(Lexer* lexer) {
  return lexer->source_str[(lexer->current_pos)] == '\0';
}

bool IsAlpha(char c) {
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c == '_');
}

bool IsNumeric(char c) {
  return (c >= '0' && c <= '9');
}

bool IsAlphaNumeric(char c) {
  return IsAlpha(c) || IsNumeric(c);
}

char PeekChar(Lexer* lexer) {
  return lexer->source_str[lexer->current_pos];
}

char PeekNextChar(Lexer* lexer) {
  if (!IsEnd(lexer)) {
    int index = lexer->current_pos + 1;
    return lexer->source_str[index];
  }
  return '\0';
}

void AddToken(Lexer* lexer, TokenType tok_type) {
  Token tok = {0};
  tok.token_type = tok_type;
  tok.line_num = lexer->line_count;
  lexer->tokens[lexer->array_pos++] = tok;
}

void EmitTokens(Lexer* lexer) {
  for (size_t it = 0; it <= lexer->array_pos - 1; it++) {
    TokenType temp = lexer->tokens[it].token_type;
    printf("%s\n", TokenTypeToString(temp));
  }
}

void TokenizeIdentifier(Lexer* lexer) {
  while(!IsEnd(lexer) && IsAlphaNumeric(PeekChar(lexer))) {
    Advance(lexer);
  }
  char buff[MAX_LEXEME_LENGTH];
  size_t length = lexer->current_pos - lexer->start_pos;
  strncpy(buff, lexer->source_str + lexer->start_pos, length);
  buff[length] = '\0';
  printf("lexeme: %s\n", buff);
}

char* TokenTypeToString(TokenType tok_type) {
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
    case TT_IF:
      return "IF";
    case TT_THEN:
      return "THEN";
    case TT_ELSE:
      return "ELSE";
    case TT_FOR:
      return "FOR";
    case TT_IDENTIFER:
      return "IDENTIFIER";
    case TT_DEF:
      return "DEF";
    case TT_COLON:
      return ":";
    case TT_SEMICOLON:
      return ";";
    case TT_ERROR:
      return "EROR";
    case TT_EOF:
      return "EOF";
    case TT_COUNT:
      return "Number of tokens";
    default:
      assert(0 && "Unreachable");
  }
}

void DestroyLexer(Lexer* lexer) {
  free((lexer)->source_str);
  free(lexer);
  lexer = NULL;
}
