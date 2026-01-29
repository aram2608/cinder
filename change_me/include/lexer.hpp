#ifndef LEXER_H
#define LEXER_H

#include "common.hpp"
#include "tokens.hpp"

#define LEXER_INIT 300
#define BUMP_CAP 50

/// @struct Lexer
/// @brief The main structure used during tokenization of source code
struct Lexer {
  size_t start_pos;       /**< The starting position for the current lexeme */
  size_t current_pos;     /**< The current positon in the lexeme */
  size_t line_count;      /**< The line count */
  std::string source_str; /** The source string provided */
  std::vector<Token> tokens; /** A flexible array member for the tokens */

  /// @brief Constructor for std::string
  /// @param source_str Source string
  Lexer(std::string source_str);

  /// @brief Constructor for const char*
  /// @param source_str
  Lexer(const char* source_str);

  /// @brief Method to tokenize the source code
  void ScanTokens();

  /// @brief Method to scan an individual char
  void Scan();

  /**
   * @brief Method to advance forward in the source string
   * @return The next character in the source code
   */
  char Advance();

  /**
   * @brief Method to test if we are at the end of the soure cdoe
   * @return Either true or false
   */
  bool IsEnd();

  /**
   * @brief Method to test if a current character is in the alphabet
   * @param c The char to be tested
   * @return Either true or false
   */
  bool IsAlpha(char c);

  /**
   * @brief Method to test if a current character is numeric
   * @param c The char to be tested
   * @return Either true or false
   */
  bool IsNumeric(char c);

  /**
   * @brief Method to test if a current character is alpha numeric
   * @param c The char to be tested
   * @return Either true or false
   */
  bool IsAlphaNumeric(char c);

  /**
   * @brief Method to peek at the current character in the source code
   * @return The current character in the source code
   */
  char PeekChar();

  /**
   * @brief Method to peek at the next character if possible
   * @return The next character in the source code
   */
  char PeekNextChar();

  /**
   * @brief Method to test if a current chararacter matches the provided type
   * @param c The character to be tested against the current_pos
   */
  bool Match(char c);

  /**
   * @brief Method to add simple tokens
   * @param tok_type The type of token to be added
   */
  void AddToken(TokenType tok_type);

  /**
   * @brief Method to add complex tokens
   * @param tok_type
   * @param lexeme
   * @param value_type
   * @param value
   */
  void AddToken(TokenType tok_type, std::string lexeme, ValueType value_type,
                TokenValue value);

  void ParseComment();

  /**
   * @brief Method to help during debugging, prints the tokens incrementally
   */
  void EmitTokens();

  /**
   * @brief A simple helper method to parse token types into strings
   * @param tok_type The token to be formatted
   */
  std::string TokenToString(Token tok);

  void TokenizeString();

  /**
   * @brief Method to lex identifiers and key words
   */
  void TokenizeIdentifier();

  /**
   * @brief Method to lex numeric types
   */
  void TokenizeNumber();
};

#endif
