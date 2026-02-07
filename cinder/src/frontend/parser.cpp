#include "cinder/frontend/parser.hpp"

#include "cinder/ast/expr.hpp"
#include "cinder/frontend/tokens.hpp"
#include "cinder/support/raw_outstream.hpp"
// #include "cinder/support/utils.hpp"

static ostream::RawOutStream errors{2};

Parser::Parser(std::vector<Token> tokens) : tokens_(tokens), current_tok_(0) {}

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
  bool is_variadic = false;
  if (!CheckType(TT_RPAREN)) {
    do {
      if (args.size() >= 255) {
        ostream::ErrorOutln(errors,
                            "Exceeded maximum number of arguments: 255");
      }
      if (MatchType({TT_ELLIPSIS})) {
        is_variadic = true;
        break;
      }
      Token type = Advance();
      Token identifier = Consume(TT_IDENTIFER, "expected arg name");
      args.emplace_back(type, identifier);
    } while (MatchType({TT_COMMA}));
  }
  Consume(TT_RPAREN, "expected ')' after end of function declaration");
  Consume(TT_ARROW, "expected '->' prior to the return type");
  Token return_type = Advance();
  return std::make_unique<FunctionProto>(name, return_type, args, is_variadic);
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
    while (!CheckType(TT_END) && !IsEnd()) {
      stmts.push_back(Statement());
    }
    Consume(TT_END, "expected end after a function definition");
    return std::make_unique<FunctionStmt>(std::move(proto), std::move(stmts));
  }
  return Statement();
}

std::unique_ptr<Stmt> Parser::Statement() {
  if (MatchType({TT_INT32_SPECIFIER, TT_INT64_SPECIFIER, TT_FLT_SPECIFIER,
                 TT_BOOL_SPECIFIER, TT_STR_SPECIFIER})) {
    return VarDeclaration();
  }
  if (MatchType({TT_RETURN})) {
    return ReturnStatement();
  }
  if (MatchType({TT_IF})) {
    return IfStatement();
  }
  if (MatchType({TT_FOR})) {
    return ForStatement();
  }
  if (MatchType({TT_WHILE})) {
    return WhileStatement();
  }
  return ExpressionStatement();
}

std::unique_ptr<Stmt> Parser::WhileStatement() {
  std::unique_ptr<Expr> condition = Expression();
  std::vector<std::unique_ptr<Stmt>> body;
  while (!CheckType(TT_END)) {
    body.push_back(Statement());
  }
  Consume(TT_END, "'end' expected after loop");
  return std::make_unique<WhileStmt>(std::move(condition), std::move(body));
}

std::unique_ptr<Stmt> Parser::ForStatement() {
  std::unique_ptr<Stmt> initializer = Statement();
  std::unique_ptr<Expr> condition = Expression();
  Consume(TT_SEMICOLON, "';' expected after condition");
  std::unique_ptr<Expr> step = Expression();
  std::vector<std::unique_ptr<Stmt>> body;
  while (!CheckType(TT_END) && !IsEnd()) {
    body.push_back(Statement());
  }
  Consume(TT_END, "expected 'end' after the loop");
  return std::make_unique<ForStmt>(std::move(initializer), std::move(condition),
                                   std::move(step), std::move(body));
}

std::unique_ptr<Stmt> Parser::IfStatement() {
  std::unique_ptr<Expr> condition = Expression();
  std::unique_ptr<Stmt> then = Statement();
  std::unique_ptr<Stmt> otherwise =
      MatchType({TT_ELSE}) ? Statement() : nullptr;
  Consume(TT_END, "Expected 'end' after if statement");
  return std::make_unique<IfStmt>(std::move(condition), std::move(then),
                                  std::move(otherwise));
}

std::unique_ptr<Stmt> Parser::ReturnStatement() {
  Token tok = Previous();
  if (CheckType(TT_SEMICOLON)) {
    Advance();
    return std::make_unique<ReturnStmt>(tok, nullptr);
  }
  std::unique_ptr<Expr> expr = Expression();
  Consume(TT_SEMICOLON, "expected ';' after return statement");
  return std::make_unique<ReturnStmt>(tok, std::move(expr));
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
  Consume(TT_SEMICOLON, "Expected ';' after expression statement");
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
    if (auto* var = dynamic_cast<Variable*>(expr.get())) {
      return std::make_unique<Assign>(var->name, std::move(value));
    }
  }
  return expr;
}

std::unique_ptr<Expr> Parser::Comparison() {
  std::unique_ptr<Expr> expr = Term();
  while (MatchType({TT_LESSER, TT_LESSER_EQ, TT_GREATER, TT_GREATER_EQ,
                    TT_BANGEQ, TT_EQEQ})) {
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
    Token name = Advance();
    return std::make_unique<PreFixOp>(op, name);
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
          std::cout << "max number of allowed arguments reached\n";
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
  if (MatchType({TT_INT_LITERAL, TT_FLT_LITERAL, TT_STR_LITERAL})) {
    return std::make_unique<Literal>(Previous().literal.value());
  }

  if (MatchType({TT_TRUE})) {
    return std::make_unique<Literal>(true);
  }

  if (MatchType({TT_FALSE})) {
    return std::make_unique<Literal>(false);
  }

  if (MatchType({TT_IDENTIFER})) {
    return std::make_unique<Variable>(Previous());
  }

  if (MatchType({TT_LPAREN})) {
    std::unique_ptr<Expr> expr = Expression();
    Consume(TT_RPAREN, "Expected ')' after grouping");
    return std::make_unique<Grouping>(std::move(expr));
  }
  ostream::ErrorOutln(errors, "Expected expression:", Peek().lexeme);
  return nullptr;
}

bool Parser::MatchType(std::initializer_list<TokenType> types) {
  for (TokenType type : types) {
    if (Peek().type == type) {
      Advance();
      return true;
    }
  }
  return false;
}

Token Parser::Peek() {
  return tokens_[current_tok_];
}

bool Parser::CheckType(TokenType type) {
  if (IsEnd()) {
    return false;
  }
  return Peek().type == type;
}

Token Parser::Previous() {
  return tokens_[current_tok_ - 1];
}

bool Parser::IsEnd() {
  return tokens_[current_tok_].type == TT_EOF;
}

Token Parser::Advance() {
  if (!IsEnd()) {
    return tokens_[current_tok_++];
  }
  return tokens_[current_tok_];
}

Token Parser::Consume(TokenType type, std::string message) {
  if (Peek().type == type) {
    return Advance();
  }
  ostream::ErrorOutln(errors, message);
  // Not used, simply to suppress compiler warning
  return {type, 0, "", std::nullopt};
}

void Parser::EmitAST(std::vector<std::unique_ptr<Stmt>> statements) {
  for (std::unique_ptr<Stmt>& stmt : statements) {
    std::cout << stmt->ToString() << "\n";
  }
}

void Parser::EmitAST(std::unique_ptr<Stmt> statement) {
  std::cout << statement->ToString() << "\n";
}
