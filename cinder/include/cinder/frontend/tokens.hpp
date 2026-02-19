#ifndef TOKENS_H
#define TOKENS_H

#include <optional>
#include <string>
#include <variant>

#include "cinder/ast/types.hpp"

namespace cinder {

/** @brief 1-based source position for a token in input text. */
struct SourceLocation {
  size_t offset = 0;
  size_t line = 1;
  size_t column = 1;
};

/** @brief Literal payload type used by lexical tokens and literal AST nodes. */
using TokenValue = std::variant<std::string, int, float, bool>;

/**
 * @brief Represents a lexical token produced by the lexer.
 *
 * A token stores its kind, source location, lexeme text, and optional literal
 * payload for literal tokens.
 */
struct Token {
  enum class Type {
    // Binops
    Plus, /** "+" */
    PlusPlus,
    Minus, /** "-" */
    MinusMinus,
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
  Token::Type kind;                  /**< Token category. */
  SourceLocation location;           /**< Source position of the token. */
  std::string lexeme;                /**< Source spelling for the token. */
  std::optional<TokenValue> literal; /**< Parsed literal value, if present. */

  /**
   * @brief Constructs a token.
   * @param kind Token category.
   * @param loc Source location metadata.
   * @param lexeme Source spelling for this token.
   * @param literal Optional parsed literal value.
   */
  Token(Token::Type kind, SourceLocation loc, std::string lexeme,
        std::optional<TokenValue> literal = std::nullopt);

  /** @brief Returns whether this token is a literal token kind. */
  bool IsLiteral();
  /** @brief Returns whether this token is an integer type specifier. */
  bool IsInt();
  /** @brief Returns whether this token is a floating-point type specifier. */
  bool IsFloat();
  /** @brief Returns whether this token is a string type specifier. */
  bool IsString();
  /** @brief Returns whether this token is a void type specifier. */
  bool IsVoid();
  /** @brief Returns whether this token is a primitive type specifier. */
  bool IsPrimitive();
  /** @brief Returns whether this token is a struct type specifier. */
  bool IsStruct();
  /** @brief Returns whether this token is an identifier token. */
  bool IsIdentifier();
  /** @brief Returns whether this token is a bool type specifier. */
  bool IsBool();
  /** @brief Returns whether this token is a term-level operator (`+` or `-`).
   */
  bool IsTerm();
  /** @brief Returns whether this token is a factor-level operator (`*` or `/`).
   */
  bool IsFactor();
  /** @brief Returns whether this token is a comparison operator. */
  bool IsComparison();
  /** @brief Returns whether this token exactly matches `type`. */
  bool IsThisType(Type type);
  /** @brief Returns whether this token is the end-of-file sentinel. */
  bool IsEOF();
};

struct TokenTypeHash {
  std::size_t operator()(Token::Type t) const noexcept {
    return std::hash<std::underlying_type_t<Token::Type>>{}(
        static_cast<std::underlying_type_t<Token::Type>>(t));
  }
};

/** @brief Parsed function argument metadata from a function prototype. */
struct FuncArg {
  Token type_token; /**< Declared argument type token. */
  Token identifier; /**< Argument identifier token. */
  cinder::types::Type*
      resolved_type; /**< Resolved semantic type (if analyzed). */

  /**
   * @brief Constructs an unresolved function argument record.
   * @param type_token Declared type token.
   * @param identifier Argument identifier token.
   */
  FuncArg(Token type_token, Token identifier)
      : type_token(type_token), identifier(identifier) {}
};

}  // namespace cinder

#endif
