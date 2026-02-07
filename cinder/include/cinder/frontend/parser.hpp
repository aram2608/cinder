#ifndef PARSER_H_
#define PARSER_H_

// #include "common.hpp"
#include <initializer_list>
#include <iostream>
#include <memory>
#include <vector>

#include "cinder/ast/expr.hpp"
#include "cinder/ast/stmt.hpp"
#include "cinder/frontend/tokens.hpp"

/// @struct Parser
/// @brief Main class for parsing tokens in a program
struct Parser {
  std::vector<Token> tokens_; /**< The vector of tokens */
  size_t current_tok_;        /**< The current token position */

  explicit Parser(std::vector<Token> tokens);

  /// @brief Top level method to parse a program
  /// @return A pointer to a Module statement
  std::unique_ptr<Stmt> Parse();

  /// @brief Method to parse a module
  /// @return A pointer to the module
  std::unique_ptr<Stmt> ParseModule();

  /// @brief Method to parse a function prototype
  /// @return A pointer to the function prototype
  std::unique_ptr<Stmt> FunctionPrototype();

  /// @brief Method to parse an externally declared function
  /// @return A pointer to the extern function
  std::unique_ptr<Stmt> ExternFunction();

  /// @brief Method to parse a function
  /// @return A pointer to the function
  std::unique_ptr<Stmt> Function();

  /// @brief Top level method to parse statements
  /// @return A pointer to a statement
  std::unique_ptr<Stmt> Statement();

  std::unique_ptr<Stmt> WhileStatement();

  /// @brief Method to parse a for statement
  /// @return A pointer to a for statement
  std::unique_ptr<Stmt> ForStatement();

  /// @brief Method to parse an if statement
  /// @return A pointer to an if statement
  std::unique_ptr<Stmt> IfStatement();

  /// @brief Method to parse return statements
  /// @return A pointer to a return statement
  std::unique_ptr<Stmt> ReturnStatement();

  /// @brief Method to parse a variable declaration
  /// @return A pointer to a variable declaration
  std::unique_ptr<Stmt> VarDeclaration();

  /// @brief Method to parse an expression statement
  /// @return A pointer to the expression statement
  std::unique_ptr<Stmt> ExpressionStatement();

  /// @brief Top level method to parse expressions
  /// @return A pointer to an expression
  std::unique_ptr<Expr> Expression();

  /// @brief Method to parse an assignment
  /// @return A pointer to the assignment
  std::unique_ptr<Expr> Assignment();

  /// @brief Method to parse a comparison
  /// @return A pointer to the comparison
  std::unique_ptr<Expr> Comparison();

  /// @brief Method to parse addition and subtraction
  /// @return A pointer to the binary operation
  std::unique_ptr<Expr> Term();

  /// @brief Method to parse mulitplication and division
  /// @return A pointer to the binary operation
  std::unique_ptr<Expr> Factor();

  /// @brief Method to parse a pre increment operation
  /// @return A pointer to the pre increment operation
  std::unique_ptr<Expr> PreIncrement();

  /// @brief Method to parse a function call
  /// @return A pointer to the call
  std::unique_ptr<Expr> Call();

  /// @brief Method to parse language primitives
  /// @return A pointer to the primitive
  std::unique_ptr<Expr> Atom();

  /**
   * @brief Method to match a list of tokens to the current token
   *
   * Advances the token position if a match is made
   *
   * @param types An initializer list of token types
   * @return True or False depending if the match is made
   */
  bool MatchType(std::initializer_list<TokenType> types);

  /**
   * @brief Method to check the underlying type of a token
   * @param type An token type
   * @return True or False depending if the match is made
   */
  bool CheckType(TokenType type);

  /// @brief Peeks at the current token, does not advance
  /// @return The current token
  Token Peek();

  /// @brief Method to return the previous token
  /// @return The previous token
  Token Previous();

  /// @brief Method to check if we are at the end of the token list
  /// @return True or False
  bool IsEnd();

  /**
   * @brief Method to Advance forward in the list of tokens
   *
   * Has a boundary check
   * @return The next token in the list
   */
  Token Advance();

  /**
   * @brief Method to consume a token type, errors out if not matched
   *
   * The provided token type must match the current token
   * This method advances to the next token
   *
   * @param type The token type to best tested
   * @param message An error message to error out with
   * @return The next token
   */
  Token Consume(TokenType type, std::string message);

  /// @brief Method to print out the ast
  /// @param statements The statements to be printed
  void EmitAST(std::vector<std::unique_ptr<Stmt>> statements);

  /// @brief Method to print out the ast
  /// @param statement The statement to be printed
  void EmitAST(std::unique_ptr<Stmt> statement);
};

#endif
