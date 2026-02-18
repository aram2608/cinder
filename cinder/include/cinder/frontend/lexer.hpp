#ifndef LEXER_H
#define LEXER_H

#include <cstddef>
#include <vector>

#include "cinder/frontend/tokens.hpp"

#define LEXER_INIT 300
#define BUMP_CAP 50

/**
 * @brief Converts source text into a stream of lexical tokens.
 *
 * `Lexer` performs a single left-to-right pass over the input and stores the
 * produced tokens in insertion order. Location metadata is attached to each
 * token as it is emitted.
 */
class Lexer {
  size_t start_pos_;   /**< Byte offset where the current lexeme starts. */
  size_t current_pos_; /**< Current byte offset in the source string. */

  size_t line_;   /**< 1-based source line counter. */
  size_t column_; /**< 1-based source column counter. */

  std::string source_str_;            /**< Input source text. */
  std::vector<cinder::Token> tokens_; /**< Tokens emitted during scanning. */

 public:
  /**
   * @brief Creates a lexer from an owned source string.
   * @param source_str Source program text.
   */
  Lexer(std::string source_str);

  /**
   * @brief Creates a lexer from a C string.
   * @param source_str Null-terminated source program text.
   */
  Lexer(const char* source_str);

  /** @brief Scans the full input and appends an explicit EOF token. */
  void ScanTokens();

  /**
   * @brief Returns a copy of all produced tokens.
   * @return Token vector in source order.
   */
  std::vector<cinder::Token> GetTokens();

  /** @brief Prints a human-readable token stream for debugging. */
  void EmitTokens();

 private:
  /** @brief Scans a single lexical unit starting at `current_pos_`. */
  void Scan();

  /**
   * @brief Consumes and returns the current character.
   * @return The consumed character, or `'\0'` at end-of-input.
   */
  char Advance();

  /**
   * @brief Checks whether scanning reached the end of the source.
   * @return `true` when no characters remain; otherwise `false`.
   */
  bool IsEnd();

  /**
   * @brief Checks whether a character is alphabetic or underscore.
   * @param c Character to test.
   * @return `true` if `c` is `[A-Za-z_]`; otherwise `false`.
   */
  bool IsAlpha(char c);

  /**
   * @brief Checks whether a character is an ASCII digit.
   * @param c Character to test.
   * @return `true` if `c` is `[0-9]`; otherwise `false`.
   */
  bool IsNumeric(char c);

  /**
   * @brief Checks whether a character is alphanumeric or underscore.
   * @param c Character to test.
   * @return `true` when `IsAlpha(c)` or `IsNumeric(c)` is true.
   */
  bool IsAlphaNumeric(char c);

  /**
   * @brief Returns the current character without consuming it.
   * @return Current character, or `'\0'` at end-of-input.
   */
  char PeekChar();

  /**
   * @brief Returns the character one position ahead without consuming.
   * @return Lookahead character, or `'\0'` when unavailable.
   */
  char PeekNextChar();

  /**
   * @brief Conditionally consumes the current character when it matches `c`.
   * @param c Expected character.
   * @return `true` if a match was consumed; otherwise `false`.
   */
  bool Match(char c);

  /**
   * @brief Emits a token whose lexeme is sliced from the current source range.
   * @param tok_type Token kind to emit.
   */
  void AddToken(cinder::Token::Type tok_type);

  /**
   * @brief Emits a token with an explicit lexeme.
   * @param tok_type Token kind to emit.
   * @param lexeme Lexeme text to store in the token.
   */
  void AddToken(cinder::Token::Type tok_type, std::string lexeme);

  /**
   * @brief Emits a token with explicit lexeme and literal payload.
   * @param tok_type Token kind to emit.
   * @param lexeme Lexeme text to store in the token.
   * @param value Optional literal value associated with the lexeme.
   */
  void AddToken(cinder::Token::Type tok_type, std::string lexeme,
                std::optional<cinder::TokenValue> value);

  /** @brief Consumes a single-line comment. */
  void ParseComment();

  /** @brief Scans and emits a string literal token. */
  void TokenizeString();

  /**
   * @brief Decodes supported backslash escapes in a string literal payload.
   * @param str String contents to unescape in place.
   */
  void EscapeCharacters(std::string& str);

  /** @brief Scans an identifier or reserved keyword token. */
  void TokenizeIdentifier();

  /** @brief Scans an integer or floating-point literal token. */
  void TokenizeNumber();

  /** @brief Scans either `.` or `...` tokens. */
  void TokenizeDot();

  /**
   * @brief Checks whether `PeekNextChar()` is safe to call.
   * @return `true` when one-character lookahead is available.
   */
  bool LookAheadOkay();

  /**
   * @brief Formats a token for debugging output.
   * @param tok Token to format.
   * @return Human-readable representation of `tok`.
   */
  std::string TokenToString(cinder::Token tok);
};

#endif
