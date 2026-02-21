#include "cinder/frontend/parser.hpp"

#include <functional>
#include <iostream>
#include <optional>

#include "cinder/frontend/tokens.hpp"
#include "cinder/support/raw_outstream.hpp"

using namespace cinder;

static ostream::RawOutStream errors{2};

Parser::Parser(std::vector<Token> tokens) : tokens_(tokens), current_tok_(0) {}

std::unique_ptr<Stmt> Parser::Parse() {
  return ParseModule();
}

std::unique_ptr<Stmt> Parser::ParseModule() {
  Consume(Token::Type::MOD, "expected module at start of translation unit");
  Token name =
      Consume(Token::Type::IDENTIFER, "expected identifier after module");
  Consume(Token::Type::SEMICOLON, "';' expected after statement");
  std::vector<std::unique_ptr<Stmt>> statements;

  while (MatchType({Token::Type::IMPORT})) {
    statements.push_back(ImportStatement());
  }

  while (!IsEnd()) {
    statements.push_back(ExternFunction());
  }
  return std::make_unique<ModuleStmt>(name, std::move(statements));
}

std::unique_ptr<Stmt> Parser::FunctionPrototype() {
  Token name =
      Consume(Token::Type::IDENTIFER, "expected identifier after 'def'");
  Consume(Token::Type::LPAREN, "expected '(' after function name");

  std::vector<FuncArg> args;
  bool is_variadic = false;
  if (!CheckType(Token::Type::RPAREN)) {
    do {
      if (args.size() >= 255) {
        ostream::ErrorOutln(errors,
                            "Exceeded maximum number of arguments: 255");
      }
      if (MatchType({Token::Type::ELLIPSIS})) {
        is_variadic = true;
        break;
      }
      Token type = Advance();
      Token identifier = Consume(Token::Type::IDENTIFER, "expected arg name");
      args.emplace_back(type, identifier);
    } while (MatchType({Token::Type::COMMA}));
  }
  Consume(Token::Type::RPAREN,
          "expected ')' after end of function declaration");
  Consume(Token::Type::ARROW, "expected '->' prior to the return type");
  Token return_type = Advance();
  return std::make_unique<FunctionProto>(name, return_type, args, is_variadic);
}

std::unique_ptr<Stmt> Parser::ExternFunction() {
  if (MatchType({Token::Type::EXTERN})) {
    return FunctionPrototype();
  }
  return Function();
}

std::unique_ptr<Stmt> Parser::Function() {
  if (MatchType({Token::Type::DEF})) {
    std::unique_ptr<Stmt> proto = FunctionPrototype();
    std::vector<std::unique_ptr<Stmt>> stmts;
    while (!CheckType(Token::Type::END) && !IsEnd()) {
      stmts.push_back(Statement());
    }
    Consume(Token::Type::END, "expected end after a function definition");
    return std::make_unique<FunctionStmt>(std::move(proto), std::move(stmts));
  }
  return Statement();
}

std::unique_ptr<Stmt> Parser::Statement() {
  if (MatchType(&Token::IsPrimitive)) {
    return VarDeclaration();
  }
  if (MatchType({Token::Type::RETURN})) {
    return ReturnStatement();
  }
  if (MatchType({Token::Type::IF})) {
    return IfStatement();
  }
  if (MatchType({Token::Type::FOR})) {
    return ForStatement();
  }
  if (MatchType({Token::Type::WHILE})) {
    return WhileStatement();
  }
  return ExpressionStatement();
}

std::unique_ptr<Stmt> Parser::ImportStatement() {
  Token mod_name =
      Consume(Token::Type::IDENTIFER, "expected module name after import");
  Consume(Token::Type::SEMICOLON, "expected ';' after import declaration");
  return std::make_unique<ImportStmt>(mod_name);
}

std::unique_ptr<Stmt> Parser::WhileStatement() {
  std::unique_ptr<Expr> condition = Expression();
  std::vector<std::unique_ptr<Stmt>> body;
  while (!CheckType(Token::Type::END)) {
    body.push_back(Statement());
  }
  Consume(Token::Type::END, "'end' expected after loop");
  return std::make_unique<WhileStmt>(std::move(condition), std::move(body));
}

std::unique_ptr<Stmt> Parser::ForStatement() {
  std::unique_ptr<Stmt> initializer = Statement();
  std::unique_ptr<Expr> condition = Expression();
  Consume(Token::Type::SEMICOLON, "';' expected after condition");
  std::unique_ptr<Expr> step = Expression();
  std::vector<std::unique_ptr<Stmt>> body;
  while (!CheckType(Token::Type::END) && !IsEnd()) {
    body.push_back(Statement());
  }
  Consume(Token::Type::END, "expected 'end' after the loop");
  return std::make_unique<ForStmt>(std::move(initializer), std::move(condition),
                                   std::move(step), std::move(body));
}

std::unique_ptr<Stmt> Parser::IfStatement() {
  std::unique_ptr<Expr> condition = Expression();
  std::unique_ptr<Stmt> then = Statement();
  std::unique_ptr<Stmt> otherwise =
      MatchType({Token::Type::ELSE}) ? Statement() : nullptr;
  Consume(Token::Type::END, "Expected 'end' after if statement");
  return std::make_unique<IfStmt>(std::move(condition), std::move(then),
                                  std::move(otherwise));
}

std::unique_ptr<Stmt> Parser::ReturnStatement() {
  Token tok = Previous();
  if (CheckType(Token::Type::SEMICOLON)) {
    Advance();
    return std::make_unique<ReturnStmt>(tok, nullptr);
  }
  std::unique_ptr<Expr> expr = Expression();
  Consume(Token::Type::SEMICOLON, "expected ';' after return statement");
  return std::make_unique<ReturnStmt>(tok, std::move(expr));
}

std::unique_ptr<Stmt> Parser::VarDeclaration() {
  Token specifier = Previous();
  Consume(Token::Type::COLON, "expected ':' after type specifier");
  Token var = Advance();
  std::unique_ptr<Expr> initializer;
  if (MatchType({Token::Type::EQ})) {
    initializer = Expression();
  } else {
    std::cout << "variables must be instantiated\n";
    exit(1);
  }
  Consume(Token::Type::SEMICOLON, "expected ';' after variable declaration");
  return std::make_unique<VarDeclarationStmt>(specifier, var,
                                              std::move(initializer));
}

std::unique_ptr<Stmt> Parser::ExpressionStatement() {
  std::unique_ptr<Expr> expr = Expression();
  Consume(Token::Type::SEMICOLON, "Expected ';' after expression statement");
  return std::make_unique<ExpressionStmt>(std::move(expr));
}

std::unique_ptr<Expr> Parser::Expression() {
  return Assignment();
}

std::unique_ptr<Expr> Parser::Assignment() {
  std::unique_ptr<Expr> expr = Comparison();
  if (MatchType({Token::Type::EQ})) {
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
  while (MatchType(&Token::IsComparison)) {
    Token op = Previous();
    std::unique_ptr<Expr> right = Term();
    expr = std::make_unique<Conditional>(std::move(expr), std::move(right), op);
  }
  return expr;
}

std::unique_ptr<Expr> Parser::Term() {
  std::unique_ptr<Expr> expr = Factor();
  while (MatchType(&Token::IsTerm)) {
    Token op = Previous();
    std::unique_ptr<Expr> right = Factor();
    expr = std::make_unique<Binary>(std::move(expr), std::move(right), op);
  }
  return expr;
}

std::unique_ptr<Expr> Parser::Factor() {
  std::unique_ptr<Expr> expr = PreIncrement();
  while (MatchType(&Token::IsFactor)) {
    Token op = Previous();
    std::unique_ptr<Expr> right = PreIncrement();
    expr = std::make_unique<Binary>(std::move(expr), std::move(right), op);
  }
  return expr;
}

std::unique_ptr<Expr> Parser::PreIncrement() {
  if (MatchType({Token::Type::PlusPlus, Token::Type::MinusMinus})) {
    Token op = Previous();
    Token name = Advance();
    return std::make_unique<PreFixOp>(op, name);
  }
  return Call();
}

std::unique_ptr<Expr> Parser::Call() {
  std::unique_ptr<Expr> expr = Atom();

  if (MatchType({Token::Type::LPAREN})) {
    const size_t MAX_ARGS = 255;
    std::vector<std::unique_ptr<Expr>> args;
    if (!CheckType(Token::Type::RPAREN)) {
      do {
        if (args.size() >= MAX_ARGS) {
          std::cout << "max number of allowed arguments reached\n";
          exit(1);
        }
        args.push_back(Expression());
      } while (MatchType({Token::Type::COMMA}));
    }

    Consume(Token::Type::RPAREN, "expected ')' after call");
    expr = std::make_unique<CallExpr>(std::move(expr), std::move(args));
  }

  return expr;
}

std::unique_ptr<Expr> Parser::Atom() {
  // if (MatchType({Token::Type::INT_LITERAL, Token::Type::FLT_LITERAL,
  //                Token::Type::STR_LITERAL})) {
  //   return std::make_unique<Literal>(Previous().literal.value());
  // }

  if (MatchType(&Token::IsLiteral)) {
    return std::make_unique<Literal>(Previous().literal.value());
  }

  if (MatchType({Token::Type::TRUE})) {
    return std::make_unique<Literal>(true);
  }

  if (MatchType({Token::Type::FALSE})) {
    return std::make_unique<Literal>(false);
  }

  if (MatchType({Token::Type::IDENTIFER})) {
    return std::make_unique<Variable>(Previous());
  }

  if (MatchType({Token::Type::LPAREN})) {
    std::unique_ptr<Expr> expr = Expression();
    Consume(Token::Type::RPAREN, "Expected ')' after grouping");
    return std::make_unique<Grouping>(std::move(expr));
  }
  ostream::ErrorOutln(errors, "Expected expression:", Peek().lexeme);
  return nullptr;
}

bool Parser::MatchType(std::initializer_list<Token::Type> types) {
  for (Token::Type type : types) {
    if (Peek().kind == type) {
      Advance();
      return true;
    }
  }
  return false;
}

bool Parser::MatchType(TokenMethod func) {
  if (std::invoke(func, Peek())) {
    Advance();
    return true;
  }
  return false;
}

Token Parser::Peek() {
  return tokens_[current_tok_];
}

bool Parser::CheckType(Token::Type type) {
  if (IsEnd()) {
    return false;
  }
  return Peek().kind == type;
}

Token Parser::Previous() {
  return tokens_[current_tok_ - 1];
}

bool Parser::IsEnd() {
  return tokens_[current_tok_].IsEOF();
}

Token Parser::Advance() {
  if (!IsEnd()) {
    return tokens_[current_tok_++];
  }
  return tokens_[current_tok_];
}

Token Parser::Consume(Token::Type type, std::string message) {
  if (Peek().kind == type) {
    return Advance();
  }
  ostream::ErrorOutln(errors, message);
  // Not used, simply to suppress compiler warning
  Token tok{type, {0, 0, 0}, "", std::nullopt};
  return tok;
}
