#include "../include/stmt.hpp"

using namespace llvm;

Stmt::~Stmt() = default;

ExpressionStmt::ExpressionStmt(std::unique_ptr<Expr> expr)
    : expr(std::move(expr)) {}

Value* ExpressionStmt::Accept(StmtVisitor& visitor) {
  return visitor.VisitExpressionStmt(*this);
}

std::string ExpressionStmt::ToString() {
  return "Statement: " + expr->ToString();
}

FunctionStmt::FunctionStmt(Token name, Token return_type,
                           std::vector<Token> args,
                           std::vector<std::unique_ptr<Stmt>> body)
    : name(name), return_type(return_type), args(args), body(std::move(body)) {}

Value* FunctionStmt::Accept(StmtVisitor& visitor) {
  return visitor.VisitFunctionStmt(*this);
}

std::string FunctionStmt::ToString() {
  std::string temp = return_type.lexeme + " ";
  temp += name.lexeme;
  temp += " Args( ";
  for (auto& tok : args) {
    temp += tok.lexeme;
    temp += " ";
  }
  temp += ") {\n";
  for (auto& stmt : body) {
    temp += stmt->ToString();
    temp += "\n";
  }
  temp += "}";
  return temp;
}

ReturnStmt::ReturnStmt(std::unique_ptr<Expr> value) : value(std::move(value)) {}

Value* ReturnStmt::Accept(StmtVisitor& visitor) {
  return visitor.VisitReturnStmt(*this);
}

std::string ReturnStmt::ToString() {
  return "Return: " + value->ToString();
}

VarDeclarationStmt::VarDeclarationStmt(Token name, std::unique_ptr<Expr> value)
    : name(name), value(std::move(value)) {}

Value* VarDeclarationStmt::Accept(StmtVisitor& visitor) {
  return visitor.VisitVarDeclarationStmt(*this);
}
std::string VarDeclarationStmt::ToString() {
  std::string temp{};
  return "Var Declaration: " + name.lexeme + " = " + value->ToString();
}