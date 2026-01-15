#ifndef TOKENS_H
#define TOKENS_H

#include "common.hpp"

enum TokenType {
  // Binops
  TT_PLUS,   /** "+" */
  TT_MINUS,  /** "-" */
  TT_MODULO, /** "%" */
  TT_STAR,   /** "*" */
  TT_SLASH,  /** "/" */
  TT_BANG,   /** "!" */
  TT_BANGEQ, /** "!=" */
  TT_EQ,     /** "=" */
  TT_EQEQ,   /** "==" */
  // Control flow
  TT_IF,     /** If statement */
  TT_ELSEIF, /** Then branch */
  TT_ELSE,   /** Else branch */
  TT_FOR,    /** For loop */
  // Containers
  TT_LPAREN,   /** "(" */
  TT_RPAREN,   /** ")" */
  TT_LBRACE,   /** "{" */
  TT_RBRACE,   /** "}" */
  TT_LBRACKET, /** "[" */
  TT_RBRACKET, /** "]" */
  TT_QUOTE,
  // Key words and idnetifiers
  TT_IDENTIFER, /** Any series of characters */
  TT_DEF,       /** "def" keyword */
  TT_END,       /** "end" keyword */
  TT_INT_SPECIFIER,
  TT_FLT_SPECIFIER,
  TT_STR_SPECIFIER,
  // Types
  TT_INT,
  TT_FLT,
  TT_STR,
  // Delimiters
  TT_COLON,     /** ":" */
  TT_SEMICOLON, /** ";" */

  // Misc
  TT_ERROR, /** Used during parsing for error handling */
  TT_EOF,   /** The end of the list of tokens */
  TT_COUNT, /** The number of tokens available */
};

enum ValueType {
  VT_INT,  /** Integer */
  VT_STR,  /** String */
  VT_FLT,  /** Float */
  VT_NULL, /** NULL */
};

using Value = std::variant<std::string, int, float>;

struct Token {
  Token(TokenType token_type, size_t line_num)
      : token_type(token_type), line_num(line_num) {}
  Token(TokenType token_type, size_t line_num, std::string lexeme,
        ValueType value_type, Value value)
      : token_type(token_type),
        line_num(line_num),
        lexeme(lexeme),
        value_type(value_type),
        value(value) {}
  TokenType token_type;
  size_t line_num;
  std::string lexeme;
  ValueType value_type;
  Value value;
};

#endif
