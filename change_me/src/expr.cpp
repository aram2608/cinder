#include "../include/expr.hpp"
using namespace llvm;

Literal::Literal(ValueType value_type, TokenValue value)
    : value_type(value_type), value(value) {}

Value* Literal::Accept(ExprVisitor& visitor) {
  return visitor.VisitLiteral(*this);
}

std::string Literal::ToString() {
  switch (value_type) {
    case VT_INT:
      return std::to_string(std::get<int>(value));
    case VT_FLT:
      return std::to_string(std::get<float>(value));
    case VT_STR:
      return std::get<std::string>(value);
    case VT_VOID:
      return "void";
    default:
      return "UNREACHABLE";
  }
}

Boolean::Boolean(bool boolean) : boolean(boolean) {}

Value* Boolean::Accept(ExprVisitor& visitor) {
  return visitor.VisitBoolean(*this);
}

std::string Boolean::ToString() {
  std::string temp = boolean ? "true" : "false";
  return "(" + temp + ")";
}

Variable::Variable(Token name) : name(name) {}

Value* Variable::Accept(ExprVisitor& visitor) {
  return visitor.VisitVariable(*this);
}

std::string Variable::ToString() {
  return "(" + name.lexeme + ")";
}

Grouping::Grouping(std::unique_ptr<Expr> expr) : expr(std::move(expr)) {}

Value* Grouping::Accept(ExprVisitor& visitor) {
  return visitor.VisitGrouping(*this);
}

std::string Grouping::ToString() {
  return "(" + expr->ToString() + ")";
}

Binary::Binary(std::unique_ptr<Expr> left, std::unique_ptr<Expr> right,
               Token op)
    : left(std::move(left)), right(std::move(right)), op(op) {}

Value* Binary::Accept(ExprVisitor& visitor) {
  return visitor.VisitBinary(*this);
}

std::string Binary::ToString() {
  std::string temp_op{};
  switch (op.token_type) {
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
  std::string temp{};
  temp +=
      "(" + left->ToString() + " " + temp_op + " " + right->ToString() + ")";
  return temp;
}

Assign::Assign(std::unique_ptr<Expr> name, std::unique_ptr<Expr> value)
    : name(std::move(name)), value(std::move(value)) {}

Value* Assign::Accept(ExprVisitor& visitor) {
  return visitor.VisitAssignment(*this);
}

std::string Assign::ToString() {
  std::string temp = "Assignment: " + name->ToString() + " ";
  return temp + value->ToString();
}

CallExpr::CallExpr(std::unique_ptr<Expr> callee,
                   std::vector<std::unique_ptr<Expr>> args)
    : callee(std::move(callee)), args(std::move(args)) {}

Value* CallExpr::Accept(ExprVisitor& visitor) {
  return visitor.VisitCall(*this);
}

std::string CallExpr::ToString() {
  std::string temp = callee->ToString();
  for (auto& arg : args) {
    temp += arg->ToString();
  }
  return temp;
}
