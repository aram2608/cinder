#ifndef LEXER_H
#define LEXER_H

#include "common.h"
#include "tokens.h"

#define LEXER_INIT 300
#define BUMP_CAP 50

/// @struct Lexer
/// @brief The main structure used during tokenization of source code
typedef struct {
  size_t start_pos;   /** The starting position for the current lexeme */
  size_t current_pos; /** The current positon in the lexeme */
  size_t capacity;    /** Capacity for our dyanmicaly sized token array */
  size_t line_count;

  size_t array_pos; /** Keeps track of array size for allocations */
  char* source_str; /** The source string provided */
  Token tokens[];   /** A flexible array member for the tokens */
} Lexer;

/**
 * @brief Method used to create a new lexer struct
 *
 * @param source_str A C string of the source code
 * @return A pointer to the newly created lexer
 */
Lexer* NewLexer(char* source_str);

/**
 * @brief Method used to create a new lexer struct
 *
 * Since the lexer uses a flexible array member, it needs to be resized
 * every now and then if the list of tokens ends up being too big
 *
 * @param lexer A pointer to the lexer
 * @param new_cap The new size for the lexer
 */
void ResizeLexer(Lexer* lexer, size_t new_cap);

/**
 * @brief Method to destroy a lexer struct and free resources
 * @param lexer A pointer to the lexer
 */
void DestroyLexer(Lexer* lexer);

/**
 * @brief Method to tokenize the source code
 * @param lexer A pointer to the lexer
 */
void ScanTokens(Lexer* lexer);

/**
 * @brief Method to scan an individual char
 * @param lexer A pointer to the lexer
 */
void Scan(Lexer* lexer);

/**
 * @brief Method to advance forward in the source string
 * @param lexer A pointer to the lexer
 *
 * @return The next character in the source code
 */
char Advance(Lexer* lexer);

/**
 * @brief Method to test if we are at the end of the soure cdoe
 * @param lexer A pointer to the lexer
 *
 * @return Either true or false
 */
bool IsEnd(Lexer* lexer);

/**
 * @brief Method to test if a current character is in the alphabet
 * @param c The char to be tested
 *
 * @return Either true or false
 */
bool IsAlpha(char c);

/**
 * @brief Method to test if a current character is numeric
 * @param c The char to be tested
 *
 * @return Either true or false
 */
bool IsNumeric(char c);

/**
 * @brief Method to test if a current character is alpha numeric
 * @param c The char to be tested
 *
 * @return Either true or false
 */
bool IsAlphaNumeric(char c);

/**
 * @brief Method to peek at the current character in the source code
 * @param lexer A pointer to the lexer
 *
 * @return The current character in the source code
 */
char PeekChar(Lexer* lexer);

/**
 * @brief Method to peek at the next character if possible
 * @param lexer A pointer to the lexer
 *
 * @return The next character in the source code
 */
char PeekNextChar(Lexer* lexer);

/**
 * @brief Method to add a token
 * @param lexer A pointer to the lexer
 * @param tokType The type of token to be added
 */
void AddToken(Lexer* lexer, TokenType tok_type);

/**
 * @brief Method to help during debugging, prints the tokens incrementally
 * @param lexer A pointer to the lexer
 */
void EmitTokens(Lexer* lexer);

/**
 * @brief A simple helper method to parse token types into string for debugging
 * @param tok_type The token to be formatted
 */
char* TokenTypeToString(TokenType tok_type);

/**
 * @brief Method to lex identifiers and key words
 * @param lexer
 */
void TokenizeIdentifier(Lexer* lexer);

#endif
