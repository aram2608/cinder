#include "../include/expr.hpp"
using namespace llvm;

Literal::Literal(TokenValue value) : value(value) {}

Value* Literal::Accept(ExprVisitor& visitor) {
  return visitor.Visit(*this);
}

std::string Literal::ToString() {
  return " ";
}

BoolLiteral::BoolLiteral(bool boolean) : boolean(boolean) {}

Value* BoolLiteral::Accept(ExprVisitor& visitor) {
  return visitor.Visit(*this);
}

std::string BoolLiteral::ToString() {
  std::string temp = boolean ? "true" : "false";
  return temp;
}

Variable::Variable(Token name) : name(name) {}

Value* Variable::Accept(ExprVisitor& visitor) {
  return visitor.Visit(*this);
}

std::string Variable::ToString() {
  return name.lexeme;
}

Grouping::Grouping(std::unique_ptr<Expr> expr) : expr(std::move(expr)) {}

Value* Grouping::Accept(ExprVisitor& visitor) {
  return visitor.Visit(*this);
}

std::string Grouping::ToString() {
  return "(" + expr->ToString() + ")";
}

PreFixOp::PreFixOp(Token op, Token name) : op(op), name(name) {}

Value* PreFixOp::Accept(ExprVisitor& visitor) {
  return visitor.Visit(*this);
}

std::string PreFixOp::ToString() {
  std::string temp = "PreFixOp: " + op.lexeme;
  return temp;
}

Binary::Binary(std::unique_ptr<Expr> left, std::unique_ptr<Expr> right,
               Token op)
    : left(std::move(left)), right(std::move(right)), op(op) {}

Value* Binary::Accept(ExprVisitor& visitor) {
  return visitor.Visit(*this);
}

std::string Binary::ToString() {
  std::string temp_op{};
  switch (op.type) {
    case TT_PLUS:
      temp_op = "+";
      break;
    case TT_MINUS:
      temp_op = "-";
      break;
    case TT_SLASH:
      temp_op = "/";
      break;
    case TT_STAR:
      temp_op = "*";
      break;
    default:
      temp_op = "UNREACHABLE";
  }
  return left->ToString() + " " + temp_op + " " + right->ToString();
}

Assign::Assign(Token name, std::unique_ptr<Expr> value)
    : name(name), value(std::move(value)) {}

Value* Assign::Accept(ExprVisitor& visitor) {
  return visitor.Visit(*this);
}

std::string Assign::ToString() {
  return "Assignment: " + name.lexeme + " = " + value->ToString();
}

Conditional::Conditional(std::unique_ptr<Expr> left,
                         std::unique_ptr<Expr> right, Token op)
    : left(std::move(left)), right(std::move(right)), op(op) {}

Value* Conditional::Accept(ExprVisitor& visitor) {
  return visitor.Visit(*this);
}

std::string Conditional::ToString() {
  std::string temp_op{};
  switch (op.type) {
    case TT_LESSER:
      temp_op = "<";
      break;
    case TT_LESSER_EQ:
      temp_op = "<=";
      break;
    case TT_GREATER:
      temp_op = ">";
      break;
    case TT_GREATER_EQ:
      temp_op = ">=";
      break;
    default:
      temp_op = "UNREACHABLE";
  }
  return left->ToString() + " " + temp_op + " " + right->ToString();
}

CallExpr::CallExpr(std::unique_ptr<Expr> callee,
                   std::vector<std::unique_ptr<Expr>> args)
    : callee(std::move(callee)), args(std::move(args)) {}

Value* CallExpr::Accept(ExprVisitor& visitor) {
  return visitor.Visit(*this);
}

std::string CallExpr::ToString() {
  std::string temp = callee->ToString() + "(";
  for (auto& arg : args) {
    temp += arg->ToString();
  }
  return temp += ")";
}
