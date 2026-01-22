#ifndef PARSER_H_
#define PARSER_H_

#include "common.hpp"
#include "tokens.hpp"
#include "expr.hpp"
#include "stmt.hpp"

struct Parser {
  std::vector<Token> tokens;
  size_t current_tok;
  Parser(std::vector<Token> tokens);

  std::vector<std::unique_ptr<Stmt>> Parse();

  std::unique_ptr<Stmt> FunctionPrototype();
  std::unique_ptr<Stmt> ExternFunction();
  std::unique_ptr<Stmt> Function();
  std::unique_ptr<Stmt> Statement();
  std::unique_ptr<Stmt> ReturnStatement();
  std::unique_ptr<Stmt> VarDeclaration();
  std::unique_ptr<Stmt> ExpressionStatement();
  std::unique_ptr<Expr> Expression();
  std::unique_ptr<Expr> Term();
  std::unique_ptr<Expr> Factor();
  std::unique_ptr<Expr> Call();
  std::unique_ptr<Expr> Atom();
  bool MatchType(std::initializer_list<TokenType> types);
  Token Peek();
  bool CheckType(TokenType type);
  Token Previous();
  bool IsEnd();
  Token Advance();
  Token Consume(TokenType type, std::string message);
  void EmitAST(std::vector<std::unique_ptr<Stmt>> statement);
  std::string ErrorMessageFormat(std::string message);
  std::string GetTokenTypeString(Token type);
};

#endif