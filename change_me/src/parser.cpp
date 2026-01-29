#include "../include/parser.hpp"

Parser::Parser(std::vector<Token> tokens) : tokens(tokens), current_tok(0) {}

std::unique_ptr<Stmt> Parser::Parse() {
  return ParseModule();
}

std::unique_ptr<Stmt> Parser::ParseModule() {
  Consume(TT_MOD, "expected module at start of translation unit");
  Token name = Consume(TT_IDENTIFER, "expected identifier after module");
  Consume(TT_SEMICOLON, "';' expected after statement");
  std::vector<std::unique_ptr<Stmt>> statements;
  while (!IsEnd()) {
    statements.push_back(ExternFunction());
  }
  return std::make_unique<ModuleStmt>(name, std::move(statements));
}

std::unique_ptr<Stmt> Parser::FunctionPrototype() {
  Token name = Consume(TT_IDENTIFER, "expected identifier after 'def'");
  Consume(TT_LPAREN, "expected '(' after function name");

  std::vector<FuncArg> args;
  if (!CheckType(TT_RPAREN)) {
    do {
      if (args.size() >= 255) {
        std::cout << "error: exceeded maximum number of arguments: 255\n";
        exit(1);
      }
      Token type = Advance();
      Token identifier = Consume(TT_IDENTIFER, "expected arg name");
      if (type.token_type == TT_INT32_SPECIFIER) {
        args.emplace_back(VT_INT32, identifier);
      } else if (type.token_type == TT_FLT_SPECIFIER) {
        args.emplace_back(VT_FLT, identifier);
      } else {
        std::cout << "type not allowed\n";
        exit(1);
      }
    } while (MatchType({TT_COMMA}));
  }
  Consume(TT_RPAREN, "expected ')' after end of function declaration");
  Consume(TT_ARROW, "expected '->' prior to the return type");
  Token return_type = Advance();
  return std::make_unique<FunctionProto>(name, return_type, args);
}

std::unique_ptr<Stmt> Parser::ExternFunction() {
  if (MatchType({TT_EXTERN})) {
    return FunctionPrototype();
  }
  return Function();
}

std::unique_ptr<Stmt> Parser::Function() {
  if (MatchType({TT_DEF})) {
    std::unique_ptr<Stmt> proto = FunctionPrototype();
    std::vector<std::unique_ptr<Stmt>> stmts;
    while (!(Peek().token_type == TT_END) && !IsEnd()) {
      stmts.push_back(Statement());
    }
    Consume(TT_END, "expected end after a function body");
    return std::make_unique<FunctionStmt>(std::move(proto), std::move(stmts));
  }
  return Statement();
}

std::unique_ptr<Stmt> Parser::Statement() {
  if (MatchType({TT_INT32_SPECIFIER, TT_INT64_SPECIFIER, TT_FLT_SPECIFIER,
                 TT_BOOL_SPECIFIER})) {
    return VarDeclaration();
  }
  if (MatchType({TT_RETURN})) {
    return ReturnStatement();
  }
  return ExpressionStatement();
}

std::unique_ptr<Stmt> Parser::ReturnStatement() {
  if (CheckType(TT_SEMICOLON)) {
    Advance();
    std::unique_ptr<Expr> void_literal =
        std::make_unique<Literal>(VT_VOID, Void{});
    return std::make_unique<ReturnStmt>(std::move(void_literal));
  }
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
    std::cout << "variables must be instantiated\n";
    exit(1);
  }
  Consume(TT_SEMICOLON, "expected ';' after variable declaration");
  return std::make_unique<VarDeclarationStmt>(specifier, var,
                                              std::move(initializer));
}

std::unique_ptr<Stmt> Parser::ExpressionStatement() {
  std::unique_ptr<Expr> expr = Expression();
  Consume(TT_SEMICOLON, "Expected ';' after statement");
  return std::make_unique<ExpressionStmt>(std::move(expr));
}

std::unique_ptr<Expr> Parser::Expression() {
  return Assignment();
}

std::unique_ptr<Expr> Parser::Assignment() {
  std::unique_ptr<Expr> expr = Comparison();
  if (MatchType({TT_EQ})) {
    Token eq = Previous();
    std::unique_ptr<Expr> value = Assignment();
    return std::make_unique<Assign>(std::move(expr), std::move(value));
  }
  return expr;
}

std::unique_ptr<Expr> Parser::Comparison() {
  std::unique_ptr<Expr> expr = Term();
  while (MatchType(
      {TT_LESSER, TT_LESSER_EQ, TT_GREATER, TT_GREATER_EQ, TT_BANGEQ})) {
    Token op = Previous();
    std::unique_ptr<Expr> right = Term();
    expr = std::make_unique<Conditional>(std::move(expr), std::move(right), op);
  }
  return expr;
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
  std::unique_ptr<Expr> expr = PreIncrement();
  while (MatchType({TT_STAR, TT_SLASH})) {
    Token op = Previous();
    std::unique_ptr<Expr> right = PreIncrement();
    expr = std::make_unique<Binary>(std::move(expr), std::move(right), op);
  }
  return expr;
}

std::unique_ptr<Expr> Parser::PreIncrement() {
  if (MatchType({TT_PLUS_PLUS, TT_MINUS_MINUS})) {
    Token op = Previous();
    std::unique_ptr<Expr> var = Call();
    return std::make_unique<PreFixOp>(op, std::move(var));
  }
  return Call();
}

std::unique_ptr<Expr> Parser::Call() {
  std::unique_ptr<Expr> expr = Atom();

  if (MatchType({TT_LPAREN})) {
    const size_t MAX_ARGS = 255;
    std::vector<std::unique_ptr<Expr>> args;

    if (!CheckType(TT_RPAREN)) {
      do {
        if (args.size() >= MAX_ARGS) {
          std::cout << "max arguments reached\n";
          exit(1);
        }
        args.push_back(Expression());
      } while (MatchType({TT_COMMA}));
    }

    Consume(TT_RPAREN, "expected ')' after call");
    expr = std::make_unique<CallExpr>(std::move(expr), std::move(args));
  }

  return expr;
}

std::unique_ptr<Expr> Parser::Atom() {
  if (MatchType({TT_INT, TT_FLT, TT_STR})) {
    return std::make_unique<Literal>(Previous().value_type, Previous().value);
  }

  if (MatchType({TT_TRUE})) {
    return std::make_unique<BoolLiteral>(true);
  }

  if (MatchType({TT_FALSE})) {
    return std::make_unique<BoolLiteral>(false);
  }

  if (MatchType({TT_IDENTIFER})) {
    return std::make_unique<Variable>(Previous());
  }

  if (MatchType({TT_LPAREN})) {
    std::unique_ptr<Expr> expr = Expression();
    Consume(TT_RPAREN, ErrorMessageFormatLn("Expected ')' after grouping"));
    return std::make_unique<Grouping>(std::move(expr));
  }
  std::cout << ErrorMessageFormatLn("Expected expression: " + Peek().lexeme);
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

bool Parser::CheckType(TokenType type) {
  if (IsEnd()) {
    return false;
  }
  return Peek().token_type == type;
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
  std::cout << ErrorMessageFormatLn(message);
  exit(1);
}

void Parser::EmitAST(std::vector<std::unique_ptr<Stmt>> statements) {
  for (std::unique_ptr<Stmt>& stmt : statements) {
    std::cout << stmt->ToString() << "\n";
  }
}

void Parser::EmitAST(std::unique_ptr<Stmt> statement) {
  std::cout << statement->ToString() << "\n";
}

std::string Parser::ErrorMessageFormatLn(std::string message) {
  return message + "\nLine: " + std::to_string(Peek().line_num) + "\n";
}

std::string Parser::GetTokenTypeString(Token tok) {
  switch (tok.value_type) {
    case VT_FLT:
      return "flt";
    case VT_INT32:
      return "int32";
    case VT_STR:
      return "str";
    case VT_VOID:
      return "void";
    default:
      return tok.lexeme;
  }
}