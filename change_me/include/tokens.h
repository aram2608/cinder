#ifndef TOKENS_H
#define TOKENS_H

#include "common.h"

typedef enum {
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
  TT_IF,   /** If statement */
  TT_THEN, /** Then branch */
  TT_ELSE, /** Else branch */
  TT_FOR,  /** For loop */

  // Containers
  TT_LPAREN,   /** "(" */
  TT_RPAREN,   /** ")" */
  TT_LBRACE,   /** "{" */
  TT_RBRACE,   /** "}" */
  TT_LBRACKET, /** "[" */
  TT_RBRACKET, /** "]" */
  TT_QUOTE,

  // Key words
  TT_IDENTIFER, /** Any series of characters */
  TT_DEF,       /** "def" keyword */

  // Misc
  TT_COLON,     /** ":" */
  TT_SEMICOLON, /** ";" */
  TT_ERROR,     /** Used during parsing for error handling */
  TT_EOF,       /** The end of the list of tokens */
  TT_COUNT,     /** The number of tokens available */
} TokenType;

typedef union {
  char* str;     /** String value */
  int number;    /** Integer value */
  float decimal; /** Floating point value */
} Value;

typedef enum {
  VT_INT,  /** Integer */
  VT_STR,  /** String */
  VT_FLT,  /** Float */
  VT_NULL, /** NULL */
} ValueType;

#define MAX_LEXEME_LENGTH 50
#define MAX_IDENTIFIER_LENGTH 255

typedef struct {
  TokenType token_type;
  size_t line_num;
  char lexeme[MAX_LEXEME_LENGTH];
  ValueType value_type;
  union {
    char str[MAX_IDENTIFIER_LENGTH];
    int num;
    float flt;
  } value;
} Token;

#endif
