#include "../include/expr.hpp"

Literal::Literal(ValueType value_type, TokenValue value)
    : value_type(value_type), value(value) {}

llvm::Value* Literal::Accept(ExprVisitor& visitor) {
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
    default:
      return "UNREACHABLE";
  }
}

Boolean::Boolean(bool boolean) : boolean(boolean) {}

llvm::Value* Boolean::Accept(ExprVisitor& visitor) {
  return visitor.VisitBoolean(*this);
}

std::string Boolean::ToString() {
  std::string temp = boolean ? "true" : "false";
  return "(" + temp + ")";
}

Variable::Variable(const std::string name) : name(name) {}

llvm::Value* Variable::Accept(ExprVisitor& visitor) {
  return visitor.VisitVariable(*this);
}

std::string Variable::ToString() {
  return "(" + name + ")";
}

Grouping::Grouping(std::unique_ptr<Expr> expr) : expr(std::move(expr)) {}

llvm::Value* Grouping::Accept(ExprVisitor& visitor) {
  return visitor.VisitGrouping(*this);
}

std::string Grouping::ToString() {
  return "(" + expr->ToString() + ")";
}

Binary::Binary(std::unique_ptr<Expr> left, std::unique_ptr<Expr> right,
               Token op)
    : left(std::move(left)), right(std::move(right)), op(op) {}

llvm::Value* Binary::Accept(ExprVisitor& visitor) {
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