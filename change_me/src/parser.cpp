#include "../include/parser.hpp"

Parser::Parser(std::vector<Token> tokens) : tokens(tokens), current_tok(0) {}

std::vector<std::unique_ptr<Stmt>> Parser::Parse() {
  std::vector<std::unique_ptr<Stmt>> statements;
  while (!IsEnd()) {
    statements.push_back(Function());
  }
  EmitAST(std::move(statements));
  return statements;
}

std::unique_ptr<Stmt> Parser::Function() {
  if (MatchType({TT_DEF})) {
    Token name = Consume(TT_IDENTIFER, "expected identifier after 'def'");
    Consume(TT_LPAREN, "expected '(' after function name");

    std::vector<Token> args;
    if (!(Peek().token_type == TT_RPAREN)) {
      do {
        if (args.size() >= 255) {
          printf("error: exceeded maximum number of arguments: 255\n");
          exit(1);
        }
        // TODO: Fix this logic
        // We need to type check each argument
        args.push_back(Consume(TT_IDENTIFER, "expected arg name"));
      } while (0);
    }
    Consume(TT_RPAREN, "expected ')' after end of function declaration");
    Consume(TT_ARROW, "expected '->' prior to the return type");
    Token return_type = Advance();
    std::vector<std::unique_ptr<Stmt>> stmts;
    while (!(Peek().token_type == TT_END) && !IsEnd()) {
      stmts.push_back(Statement());
    }
    Consume(TT_END, "expected end after a function body");
    return std::make_unique<FunctionStmt>(name, return_type, args,
                                          std::move(stmts));
  }
  return Statement();
}

std::unique_ptr<Stmt> Parser::Statement() {
  if (MatchType({TT_PRINT})) {
  }
  if (MatchType({TT_INT_SPECIFIER, TT_FLT_SPECIFIER})) {
    return VarDeclaration();
  }
  if (MatchType({TT_RETURN})) {
    return ReturnStatement();
  }
  return ExpressionStatement();
}

std::unique_ptr<Stmt> Parser::ReturnStatement() {
  std::unique_ptr<Expr> expr = Expression();
  Consume(TT_SEMICOLON, "expected ';' after return statement");
  return std::make_unique<ReturnStmt>(std::move(expr));
}

std::unique_ptr<Stmt> Parser::VarDeclaration() {
  Token specifier = Previous();
  Consume(TT_COLON, "expected ':' after type specifier");
  Token var = Advance();
  std::unique_ptr<Expr> initializer;
  if (MatchType({TT_EQ})) {
    initializer = Expression();
  } else {
    printf("variables must be instantiated\n");
    exit(1);
  }
  Consume(TT_SEMICOLON, "expected ';' after variable declaration");
  return std::make_unique<VarDeclarationStmt>(var, std::move(initializer));
}

std::unique_ptr<Stmt> Parser::ExpressionStatement() {
  std::unique_ptr<Expr> expr = Expression();
  Consume(TT_SEMICOLON, "Expected ';' after statement");
  return std::make_unique<ExpressionStmt>(std::move(expr));
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

  if (MatchType({TT_TRUE})) {
    return std::make_unique<Boolean>(true);
  }

  if (MatchType({TT_FALSE})) {
    return std::make_unique<Boolean>(false);
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

Token Parser::Consume(TokenType type, std::string message) {
  if (Peek().token_type == type) {
    return Advance();
  }
  printf("%s\n", ErrorMessageFormat(message).c_str());
  exit(1);
}

void Parser::EmitAST(std::vector<std::unique_ptr<Stmt>> statements) {
  for (std::unique_ptr<Stmt>& stmt : statements) {
    printf("%s\n", stmt->ToString().c_str());
  }
}

std::string Parser::ErrorMessageFormat(std::string message) {
  return message + "\nLine: " + std::to_string(Peek().line_num) + "\n";
}

std::string Parser::GetTokenTypeString(Token tok) {
  switch (tok.value_type) {
    case VT_FLT:
      return "flt";
    case VT_INT:
      return "int";
    case VT_STR:
      return "str";
    default:
      return tok.lexeme;
  }
}