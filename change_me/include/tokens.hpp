#ifndef TOKENS_H
#define TOKENS_H

#include <optional>

#include "common.hpp"
#include "utils.hpp"

/// @enum TokenType
/// @brief The token types allowed during lexical analysis
enum TokenType {
  // Binops
  TT_PLUS, /** "+" */
  TT_PLUS_PLUS,
  TT_MINUS, /** "-" */
  TT_MINUS_MINUS,
  TT_MODULO, /** "%" */
  TT_STAR,   /** "*" */
  TT_SLASH,  /** "/" */
  TT_BANG,   /** "!" */
  TT_BANGEQ, /** "!=" */
  TT_EQ,     /** "=" */
  TT_EQEQ,   /** "==" */
  TT_GREATER,
  TT_LESSER,
  TT_GREATER_EQ,
  TT_LESSER_EQ,

  TT_ARROW, /** "->" */
  TT_EXTERN,

  // Control flow
  TT_IF,     /** If statement */
  TT_ELSEIF, /** Then branch */
  TT_ELSE,   /** Else branch */
  TT_FOR,    /** For loop */
  TT_WHILE,
  TT_TRUE,
  TT_FALSE,
  TT_RETURN,
  // Containers
  TT_MOD,
  TT_IMPORT,
  TT_LPAREN,   /** "(" */
  TT_RPAREN,   /** ")" */
  TT_LBRACE,   /** "{" */
  TT_RBRACE,   /** "}" */
  TT_LBRACKET, /** "[" */
  TT_RBRACKET, /** "]" */
  TT_QUOTE,
  TT_COMMA, /**< "," */

  // Key words and identifiers
  TT_IDENTIFER, /** Any series of characters */
  TT_DEF,       /** "def" keyword */
  TT_END,       /** "end" keyword */

  // Types
  TT_BOOL_SPECIFIER,
  TT_INT32_SPECIFIER,
  TT_INT64_SPECIFIER,
  TT_FLT_SPECIFIER,
  TT_STR_SPECIFIER,
  TT_VOID_SPECIFIER,
  // Types
  TT_INT_LITERAL,
  TT_FLT_LITERAL,
  TT_STR_LITERAL,
  // Delimiters
  TT_COLON,     /** ":" */
  TT_SEMICOLON, /** ";" */

  // Misc
  TT_EOF,   /** The end of the list of tokens */
  TT_COUNT, /** The number of tokens available */
};

/// @enum ValueType
/// @brief Enum type used to store the underling value types
/// TODO: Fix the typing system so it is more ergonomic
enum ValueType {
  VT_INT32, /** Integer */
  VT_INT64,
  VT_STR, /** String */
  VT_FLT, /** Float */
  VT_VOID,
  VT_NULL, /** NULL */
};

using TokenValue = std::variant<std::string, int, float>;

/**
 * @struct Token
 * @brief The structure used to store source string info during lexical analysis
 * A simple POD
 */
struct Token {
  TokenType type;
  size_t line_num;
  std::string lexeme;
  std::optional<TokenValue> literal;
  Token(TokenType type, size_t line_num, std::string lexeme,
        std::optional<TokenValue> literal = std::nullopt)
      : type(type), line_num(line_num), lexeme(lexeme), literal(literal) {}
};

/// @struct FuncArg
/// @brief A simple data container to store function arguments
struct FuncArg {
  ValueType type;   /**< The argument type */
  Token identifier; /**< The identifer for the argument */
  FuncArg(ValueType type, Token identifier)
      : type(type), identifier(identifier) {}
};

#endif
