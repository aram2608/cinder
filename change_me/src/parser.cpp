#include "../include/parser.hpp"

Parser::Parser(std::vector<Token> tokens) : tokens(tokens), current_tok(0) {}

std::vector<std::unique_ptr<Expr>> Parser::Parse() {
  std::vector<std::unique_ptr<Expr>> expressions;
  while (!IsEnd()) {
    expressions = ParseExpressions();
  }
  return expressions;
}

std::vector<std::unique_ptr<Expr>> Parser::ParseExpressions() {
  std::vector<std::unique_ptr<Expr>> expressions;
  while (!IsEnd()) {
    expressions.push_back(Expression());
  }
  return expressions;
}

std::unique_ptr<Expr> Parser::Expression() {
  return Term();
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
  if (MatchType({TT_INT, TT_FLT, TT_STR})) {
    return std::make_unique<Literal>(Previous().value_type, Previous().value);
  }
  if (MatchType({TT_LPAREN})) {
    std::unique_ptr<Expr> expr = Expression();
    Consume(TT_RPAREN, ErrorMessageFormat("Expected ')' after grouping"));
    return std::make_unique<Grouping>(std::move(expr));
  }
  printf("%s\n",
         ErrorMessageFormat("Expected expression: " + Peek().lexeme).c_str());
  exit(1);
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

void Parser::Consume(TokenType type, std::string message) {
  if (Peek().token_type == type) {
    Advance();
    return;
  }
  printf("%s\n", ErrorMessageFormat(message).c_str());
  exit(1);
}

void Parser::EmitAST(std::vector<std::unique_ptr<Expr>> expressions) {
  for (std::unique_ptr<Expr>& expr : expressions) {
    printf("%s\n", expr->ToString().c_str());
  }
}

std::string Parser::ErrorMessageFormat(std::string message) {
  return message + "\nLine: " + std::to_string(Peek().line_num) + "\n";
}