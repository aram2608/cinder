#include "../include/expr.hpp"
#include "expr.hpp"

Expr::~Expr() = default;

Numeric::Numeric(ValueType value_type, Value value)
    : value_type(value_type), value(value) {}
std::string Numeric::ToString() {
  switch (value_type) {
    case VT_INT:
      return std::to_string(std::get<int>(value));
    case VT_FLT:
      return std::to_string(std::get<float>(value));
    default:
      return "UNREACHABLE";
  }
}

Boolean::Boolean(bool boolean) : boolean(boolean) {}
std::string Boolean::ToString() {
  std::string temp = boolean ? "true" : "false";
  return "(" + temp + ")";
}

Binary::Binary(std::unique_ptr<Expr> left, std::unique_ptr<Expr> right,
               Token op)
    : left(std::move(left)), right(std::move(right)), op(op) {}

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

Variable::Variable(const std::string name) : name(name) {}

std::string Variable::ToString() {
  return "(" + name + ")";
}