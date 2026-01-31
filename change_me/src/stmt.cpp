#include "../include/stmt.hpp"
#include <memory>
#include <string>
#include "llvm/IR/Value.h"

using namespace llvm;

ModuleStmt::ModuleStmt(Token name, std::vector<std::unique_ptr<Stmt>> stmts)
    : name(name), stmts(std::move(stmts)) {}

Value* ModuleStmt::Accept(StmtVisitor& visitor) {
  return visitor.VisitModuleStmt(*this);
}

std::string ModuleStmt::ToString() {
  std::string temp = "Module: " + name.lexeme + "\n\n\n";
  for (auto& stmt : stmts) {
    temp += stmt->ToString();
  }
  return temp;
}

ExpressionStmt::ExpressionStmt(std::unique_ptr<Expr> expr)
    : expr(std::move(expr)) {}

Value* ExpressionStmt::Accept(StmtVisitor& visitor) {
  return visitor.VisitExpressionStmt(*this);
}

std::string ExpressionStmt::ToString() {
  return "Statement: " + expr->ToString();
}

FunctionProto::FunctionProto(Token name, Token return_type,
                             std::vector<FuncArg> args)
    : name(name), return_type(return_type), args(args) {}

Value* FunctionProto::Accept(StmtVisitor& visitor) {
  return visitor.VisitFunctionProto(*this);
}

std::string FunctionProto::ToString() {
  std::string temp = "Func: " + return_type.lexeme + " ";
  temp += name.lexeme;
  temp += " Args: ( ";
  for (auto& tok : args) {
    // temp += tok.type;
    // temp += " ";
    temp += tok.identifier.lexeme;
    temp += ", ";
  }
  temp += ")";
  return temp;
}

FunctionStmt::FunctionStmt(std::unique_ptr<Stmt> proto,
                           std::vector<std::unique_ptr<Stmt>> body)
    : proto(std::move(proto)), body(std::move(body)) {}

Value* FunctionStmt::Accept(StmtVisitor& visitor) {
  return visitor.VisitFunctionStmt(*this);
}

std::string FunctionStmt::ToString() {
  std::string temp = proto->ToString() + "\n";
  temp += "{\n";
  for (auto& stmt : body) {
    temp += stmt->ToString();
    temp += "\n";
  }
  return temp += "}\n";
}

ReturnStmt::ReturnStmt(std::unique_ptr<Expr> value) : value(std::move(value)) {}

Value* ReturnStmt::Accept(StmtVisitor& visitor) {
  return visitor.VisitReturnStmt(*this);
}

std::string ReturnStmt::ToString() {
  return "Return: " + value->ToString();
}

VarDeclarationStmt::VarDeclarationStmt(Token type, Token name,
                                       std::unique_ptr<Expr> value)
    : type(type), name(name), value(std::move(value)) {}

Value* VarDeclarationStmt::Accept(StmtVisitor& visitor) {
  return visitor.VisitVarDeclarationStmt(*this);
}

std::string VarDeclarationStmt::ToString() {
  std::string temp{};
  return "Var Declaration: " + name.lexeme + " = " + value->ToString();
}

IfStmt::IfStmt(std::unique_ptr<Expr> cond, std::unique_ptr<Stmt> then,
               std::unique_ptr<Stmt> otherwise)
    : cond(std::move(cond)),
      then(std::move(then)),
      otherwise(std::move(otherwise)) {}

Value* IfStmt::Accept(StmtVisitor& visitor) {
  return visitor.VisitIfStmt(*this);
}

std::string IfStmt::ToString() {
  std::string temp = "if: " + cond->ToString() + "\n";
  temp += "then: " + then->ToString() + "\n";
  temp += "else: " + otherwise->ToString() + "\n";
  return temp;
}

ForStmt::ForStmt(std::unique_ptr<Stmt> intializer,
                 std::unique_ptr<Expr> condition, std::unique_ptr<Expr> step,
                 std::vector<std::unique_ptr<Stmt>> body)
    : initializer(std::move(intializer)),
      condition(std::move(condition)),
      step(std::move(step)),
      body(std::move(body)) {}

Value* ForStmt::Accept(StmtVisitor& visitor) {
  return visitor.VisitForStmt(*this);
}

std::string ForStmt::ToString() {
  std::string temp = "for: " + initializer->ToString() + " ";
  temp += condition->ToString() + " " + step->ToString() + "\n";
  for (auto& stmt : body) {
    temp += stmt->ToString() + "\n";
  }
  return temp;
}

WhileStmt::WhileStmt(std::unique_ptr<Expr> condition,
                     std::vector<std::unique_ptr<Stmt>> body)
    : condition(std::move(condition)), body(std::move(body)) {}

Value* WhileStmt::Accept(StmtVisitor& visitor) {
  return visitor.VisitWhileStmt(*this);
}

std::string WhileStmt::ToString() {
  std::string temp = "while: " + condition->ToString() + "\n";
  for (auto& stmt : body) {
    temp += stmt->ToString() + "\n";
  }
  return temp;
}
