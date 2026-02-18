#ifndef PARSER_H_
#define PARSER_H_

#include <initializer_list>
#include <memory>
#include <string>
#include <vector>

#include "cinder/ast/expr/expr.hpp"
#include "cinder/ast/stmt/stmt.hpp"
#include "cinder/frontend/tokens.hpp"

/**
 * @brief Recursive-descent parser that converts tokens into an AST.
 *
 * The parser consumes a flat token stream and produces a `ModuleStmt` root.
 */
struct Parser {
  std::vector<cinder::Token> tokens_; /**< Input token stream. */
  size_t current_tok_; /**< Index of the next token to consume. */

  /**
   * @brief Creates a parser for a token sequence.
   * @param tokens Tokens to parse.
   */
  explicit Parser(std::vector<cinder::Token> tokens);

  /** @brief Parses a full translation unit. */
  std::unique_ptr<Stmt> Parse();

  /** @brief Parses the top-level module declaration and contents. */
  std::unique_ptr<Stmt> ParseModule();

  /** @brief Parses a function prototype signature. */
  std::unique_ptr<Stmt> FunctionPrototype();

  /** @brief Parses either an extern prototype or a full function definition. */
  std::unique_ptr<Stmt> ExternFunction();

  /** @brief Parses a function definition or falls back to a statement. */
  std::unique_ptr<Stmt> Function();

  /** @brief Parses a single statement. */
  std::unique_ptr<Stmt> Statement();

  /** @brief Parses a `while` statement. */
  std::unique_ptr<Stmt> WhileStatement();

  /** @brief Parses a `for` statement. */
  std::unique_ptr<Stmt> ForStatement();

  /** @brief Parses an `if` statement with optional `else` branch. */
  std::unique_ptr<Stmt> IfStatement();

  /** @brief Parses a `return` statement. */
  std::unique_ptr<Stmt> ReturnStatement();

  /** @brief Parses a variable declaration statement. */
  std::unique_ptr<Stmt> VarDeclaration();

  /** @brief Parses an expression statement terminated by `;`. */
  std::unique_ptr<Stmt> ExpressionStatement();

  /** @brief Parses an expression root. */
  std::unique_ptr<Expr> Expression();

  /** @brief Parses assignment expressions. */
  std::unique_ptr<Expr> Assignment();

  /** @brief Parses comparison expressions. */
  std::unique_ptr<Expr> Comparison();

  /** @brief Parses additive (`+`, `-`) expressions. */
  std::unique_ptr<Expr> Term();

  /** @brief Parses multiplicative (`*`, `/`) expressions. */
  std::unique_ptr<Expr> Factor();

  /** @brief Parses prefix increment/decrement expressions. */
  std::unique_ptr<Expr> PreIncrement();

  /** @brief Parses call expressions. */
  std::unique_ptr<Expr> Call();

  /** @brief Parses expression atoms (literals, identifiers, groupings). */
  std::unique_ptr<Expr> Atom();

  /**
   * @brief Tests current token against any type in `types`.
   *
   * Advances the token position if a match is made
   *
   * @param types Candidate token kinds.
   * @return `true` if a match was found and consumed.
   */
  bool MatchType(std::initializer_list<cinder::Token::Type> types);

  /** @brief Convenience alias for token predicate member pointers. */
  using TokenMethod = bool (cinder::Token::*)();

  /**
   * @brief Tests current token through a token predicate member function.
   *
   * Advances the token position if a match is made. This method is an
   * alternative to the above method, instead using std::invoke from the
   * functional header to invoke the function pointer given the current token.
   * The token struct offers some testing methods which helps clean up some of
   * the longer matching patterns.
   *
   * @param func Predicate on `cinder::Token` (for example `&Token::IsLiteral`).
   * @return `true` if predicate succeeds and token is consumed.
   */
  bool MatchType(TokenMethod func);

  /**
   * @brief Checks whether the current token has kind `type`.
   * @param type Token kind to compare.
   * @return `true` when the current token matches `type`.
   */
  bool CheckType(cinder::Token::Type type);

  /** @brief Returns the current token without consuming it. */
  cinder::Token Peek();

  /** @brief Returns the most recently consumed token. */
  cinder::Token Previous();

  /** @brief Returns whether the parser reached the EOF token. */
  bool IsEnd();

  /**
   * @brief Consumes the current token and advances.
   *
   * Has a boundary check
   * @return The consumed token.
   */
  cinder::Token Advance();

  /**
   * @brief Consumes `type` or emits a parse error.
   *
   * The provided token type must match the current token
   * This method advances to the next token
   *
   * @param type Required token kind.
   * @param message Error message used when the token does not match.
   * @return The consumed token when matched.
   */
  cinder::Token Consume(cinder::Token::Type type, std::string message);

  /** @brief Prints statements as formatted AST debug output. */
  void EmitAST(std::vector<std::unique_ptr<Stmt>> statements);

  /** @brief Prints a statement as formatted AST debug output. */
  void EmitAST(std::unique_ptr<Stmt> statement);
};

#endif
