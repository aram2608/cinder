#ifndef PARSER_H_
#define PARSER_H_

#include "common.hpp"
#include "tokens.hpp"
#include "expr.hpp"

struct Parser {
  std::vector<Token> tokens;
  size_t current_tok;
  Parser(std::vector<Token> tokens);

  std::vector<std::unique_ptr<Expr>> Parse();
  std::vector<std::unique_ptr<Expr>> ParseExpressions();
  std::unique_ptr<Expr> Expression();
  std::unique_ptr<Expr> Term();
  std::unique_ptr<Expr> Factor();
  std::unique_ptr<Expr> Atom();
  bool MatchType(std::initializer_list<TokenType> types);
  Token Peek();
  Token Previous();
  bool IsEnd();
  Token Advance();
  void Consume(TokenType type, std::string message);
  void EmitAST(std::vector<std::unique_ptr<Expr>> expressions);
  std::string ErrorMessageFormat(std::string message);
};

#endif