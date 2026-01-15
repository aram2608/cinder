#include "../include/parser.hpp"

Parser::Parser(std::vector<Token> tokens) : tokens(tokens), current_tok(0) {}

void Parser::Parse() {
  while (!IsEnd()) {
    Expression();
  }
}

std::unique_ptr<Expr> Parser::Expression() {
  std::vector<std::unique_ptr<Expr>> expressions;
  while (!IsEnd()) {
    expressions.push_back(Term());
  }
  EmitAST(std::move(expressions));
  return nullptr;
}

std::unique_ptr<Expr> Parser::Term() {
  std::unique_ptr<Expr> expr = Factor();
  while (MatchType({TT_PLUS, TT_MINUS})) {
    Token op = Previous();
    std::unique_ptr<Expr> right = Factor();
    expr = std::make_unique<Binary>(std::move(expr), std::move(right), op);
  }
  return expr;
}

std::unique_ptr<Expr> Parser::Factor() {
  std::unique_ptr<Expr> expr = Atom();
  while (MatchType({TT_STAR, TT_SLASH})) {
    Token op = Previous();
    std::unique_ptr<Expr> right = Atom();
    expr = std::make_unique<Binary>(std::move(expr), std::move(right), op);
  }
  return expr;
}

std::unique_ptr<Expr> Parser::Atom() {
  if (MatchType({TT_INT, TT_FLT})) {
    return std::make_unique<Numeric>(Previous().value_type, Previous().value);
  }

  throw std::exception{};
}

bool Parser::MatchType(std::initializer_list<TokenType> types) {
  for (TokenType type : types) {
    if (Peek().token_type == type) {
      Advance();
      return true;
    }
  }
  return false;
}

Token Parser::Peek() {
  return tokens[current_tok];
}

Token Parser::Previous() {
  return tokens[current_tok - 1];
}

bool Parser::IsEnd() {
  return tokens[current_tok].token_type == TT_EOF;
}

Token Parser::Advance() {
  if (!IsEnd()) {
    return tokens[current_tok++];
  }
  return tokens[current_tok];
}

void Parser::EmitAST(std::vector<std::unique_ptr<Expr>> expressions) {
  for (std::unique_ptr<Expr>& expr : expressions) {
    printf("%s\n", expr->ToString().c_str());
  }
}