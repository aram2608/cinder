#ifndef TOKENS_H
#define TOKENS_H

#include <optional>
#include <string>
#include <variant>

#include "cinder/ast/types.hpp"

namespace cinder {

using TokenValue = std::variant<std::string, int, float, bool>;

/**
 * @struct Token
 * @brief The structure used to store source string info during lexical analysis
 * A simple POD
 */
struct Token {
  enum class Type {
    // Binops
    PLUS, /** "+" */
    PLUS_PLUS,
    MINUS, /** "-" */
    MINUS_MINUS,
    MODULO, /** "%" */
    STAR,   /** "*" */
    SLASH,  /** "/" */
    BANG,   /** "!" */
    BANGEQ, /** "!=" */
    EQ,     /** "=" */
    EQEQ,   /** "==" */
    GREATER,
    LESSER,
    GREATER_EQ,
    LESSER_EQ,

    ARROW, /** "->" */
    EXTERN,

    // Control flow
    IF,     /** If statement */
    ELSEIF, /** Then branch */
    ELSE,   /** Else branch */
    FOR,    /** For loop */
    WHILE,
    TRUE,
    FALSE,
    RETURN,
    // Containers
    MOD,
    IMPORT,
    LPAREN,   /** "(" */
    RPAREN,   /** ")" */
    LBRACE,   /** "{" */
    RBRACE,   /** "}" */
    LBRACKET, /** "[" */
    RBRACKET, /** "]" */
    QUOTE,
    COMMA, /**< "," */
    ELLIPSIS,
    DOT,
    // Key words and identifiers
    IDENTIFER, /** Any series of characters */
    DEF,       /** "def" keyword */
    END,       /** "end" keyword */
    // Types
    BOOL_SPECIFIER,
    INT32_SPECIFIER,
    INT64_SPECIFIER,
    FLT32_SPECIFIER,
    FLT64_SPECIFIER,
    STR_SPECIFIER,
    VOID_SPECIFIER,
    STRUCT_SPECIFIER,
    // Types
    INT_LITERAL,
    FLT_LITERAL,
    STR_LITERAL,
    // Delimiters
    COLON,     /** ":" */
    SEMICOLON, /** ";" */

    // Misc
    EOF_,  /** The end of the list of tokens */
    COUNT, /** The number of tokens available */
  };
  Token::Type kind;
  size_t line_num;
  std::string lexeme;
  std::optional<TokenValue> literal;
  Token(Token::Type kind, size_t line_num, std::string lexeme,
        std::optional<TokenValue> literal = std::nullopt);

  bool IsLiteral();
  bool IsInt();
  bool IsFloat();
  bool IsString();
  bool IsVoid();
  bool IsPrimitive();
  bool IsStruct();
  bool IsIdentifier();
  bool IsBool();
  bool IsTerm();
  bool IsFactor();
  bool IsComparison();
  bool IsThisType(Type type);
  bool IsEOF();
};

/// @struct FuncArg
/// @brief A simple data container to store function arguments
struct FuncArg {
  Token type_token; /**< The argument type */
  Token identifier; /**< The identifer for the argument */
  cinder::types::Type* resolved_type;
  FuncArg(Token type_token, Token identifier)
      : type_token(type_token), identifier(identifier) {}
};

}

#endif
