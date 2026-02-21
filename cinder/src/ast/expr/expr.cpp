#include "cinder/ast/expr/expr.hpp"

#include <sstream>
#include <string>
#include <system_error>
#include <variant>

#include "cinder/semantic/symbol.hpp"

using namespace cinder;

using namespace llvm;

bool Expr::IsLiteral() {
  return expr_type == ExprType::Literal;
}
bool Expr::IsVariable() {
  return expr_type == ExprType::Variable;
}
bool Expr::IsGrouping() {
  return expr_type == ExprType::Grouping;
}
bool Expr::IsPreFixOp() {
  return expr_type == ExprType::PreFix;
}
bool Expr::IsBinary() {
  return expr_type == ExprType::Binary;
}
bool Expr::IsCallExpr() {
  return expr_type == ExprType::Call;
}
bool Expr::IsAssign() {
  return expr_type == ExprType::Assign;
}
bool Expr::IsConditional() {
  return expr_type == ExprType::Conditional;
}
bool Expr::HasID() {
  return id.has_value();
}
SymbolId Expr::GetID() {
  return id.value();
}

Literal::Literal(TokenValue value) : Expr(ExprType::Literal), value(value) {}

Value* Literal::Accept(CodegenExprVisitor& visitor) {
  return visitor.Visit(*this);
}

void Literal::Accept(SemanticExprVisitor& visitor) {
  visitor.Visit(*this);
}

std::string Literal::Accept(ExprDumperVisitor& visitor) {
  return visitor.Visit(*this);
}

Variable::Variable(Token name) : Expr(ExprType::Variable), name(name) {}

Value* Variable::Accept(CodegenExprVisitor& visitor) {
  return visitor.Visit(*this);
}

void Variable::Accept(SemanticExprVisitor& visitor) {
  return visitor.Visit(*this);
}

std::string Variable::Accept(ExprDumperVisitor& visitor) {
  return visitor.Visit(*this);
}

Grouping::Grouping(std::unique_ptr<Expr> expr)
    : Expr(ExprType::Grouping), expr(std::move(expr)) {}

Value* Grouping::Accept(CodegenExprVisitor& visitor) {
  return visitor.Visit(*this);
}

void Grouping::Accept(SemanticExprVisitor& visitor) {
  return visitor.Visit(*this);
}

std::string Grouping::Accept(ExprDumperVisitor& visitor) {
  return visitor.Visit(*this);
}

PreFixOp::PreFixOp(Token op, Token name)
    : Expr(ExprType::PreFix), op(op), name(name) {}

Value* PreFixOp::Accept(CodegenExprVisitor& visitor) {
  return visitor.Visit(*this);
}

void PreFixOp::Accept(SemanticExprVisitor& visitor) {
  return visitor.Visit(*this);
}

std::string PreFixOp::Accept(ExprDumperVisitor& visitor) {
  return visitor.Visit(*this);
}

Binary::Binary(std::unique_ptr<Expr> left, std::unique_ptr<Expr> right,
               Token op)
    : Expr(ExprType::Binary),
      left(std::move(left)),
      right(std::move(right)),
      op(op) {}

Value* Binary::Accept(CodegenExprVisitor& visitor) {
  return visitor.Visit(*this);
}

void Binary::Accept(SemanticExprVisitor& visitor) {
  return visitor.Visit(*this);
}

std::string Binary::Accept(ExprDumperVisitor& visitor) {
  return visitor.Visit(*this);
}

Assign::Assign(Token name, std::unique_ptr<Expr> value)
    : Expr(ExprType::Assign), name(name), value(std::move(value)) {}

Value* Assign::Accept(CodegenExprVisitor& visitor) {
  return visitor.Visit(*this);
}

void Assign::Accept(SemanticExprVisitor& visitor) {
  return visitor.Visit(*this);
}

std::string Assign::Accept(ExprDumperVisitor& visitor) {
  return visitor.Visit(*this);
}

Conditional::Conditional(std::unique_ptr<Expr> left,
                         std::unique_ptr<Expr> right, Token op)
    : Expr(ExprType::Conditional),
      left(std::move(left)),
      right(std::move(right)),
      op(op) {}

Value* Conditional::Accept(CodegenExprVisitor& visitor) {
  return visitor.Visit(*this);
}

void Conditional::Accept(SemanticExprVisitor& visitor) {
  visitor.Visit(*this);
}

std::string Conditional::Accept(ExprDumperVisitor& visitor) {
  return visitor.Visit(*this);
}

CallExpr::CallExpr(std::unique_ptr<Expr> callee,
                   std::vector<std::unique_ptr<Expr>> args)
    : Expr(ExprType::Call), callee(std::move(callee)), args(std::move(args)) {}

Value* CallExpr::Accept(CodegenExprVisitor& visitor) {
  return visitor.Visit(*this);
}

void CallExpr::Accept(SemanticExprVisitor& visitor) {
  visitor.Visit(*this);
}

std::string CallExpr::Accept(ExprDumperVisitor& visitor) {
  return visitor.Visit(*this);
}
