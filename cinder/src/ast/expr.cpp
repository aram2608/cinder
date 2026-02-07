#include "cinder/ast/expr.hpp"

#include <sstream>
#include <variant>

using namespace llvm;

namespace {

std::string EscapeString(const std::string& value) {
  std::string escaped;
  escaped.reserve(value.size());
  for (char c : value) {
    switch (c) {
      case '\\':
        escaped += "\\\\";
        break;
      case '\n':
        escaped += "\\n";
        break;
      case '\r':
        escaped += "\\r";
        break;
      case '\t':
        escaped += "\\t";
        break;
      case '"':
        escaped += "\\\"";
        break;
      default:
        escaped += c;
        break;
    }
  }
  return escaped;
}

template <class... Ts>
struct overloaded : Ts... {
  using Ts::operator()...;
};
template <class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

static std::string LiteralValueToString(const TokenValue& value) {
  return std::visit(
      overloaded{[](bool v) -> std::string { return v ? "true" : "false"; },
                 [](const std::string& v) -> std::string {
                   return "\"" + EscapeString(v) + "\"";
                 },
                 [](auto v) -> std::string {
                   std::ostringstream out;
                   out << v;
                   return out.str();
                 }},
      value);
}

void AppendTreeBlock(std::string* out, const std::string& prefix, bool is_last,
                     const std::string& label, const std::string& child_tree) {
  *out += prefix + (is_last ? "`- " : "|- ") + label + "\n";
  const std::string child_prefix = prefix + (is_last ? "   " : "|  ");

  std::istringstream in(child_tree);
  std::string line;
  while (std::getline(in, line)) {
    *out += child_prefix + line + "\n";
  }
}

std::string RenderExpr(const Expr& expr);

std::string ExprNodeLabel(const Expr& expr) {
  if (const auto* literal = dynamic_cast<const Literal*>(&expr)) {
    return "Literal " + LiteralValueToString(literal->value);
  }
  if (const auto* variable = dynamic_cast<const Variable*>(&expr)) {
    return "Variable " + variable->name.lexeme;
  }
  if (dynamic_cast<const Grouping*>(&expr)) {
    return "Grouping";
  }
  if (const auto* prefix = dynamic_cast<const PreFixOp*>(&expr)) {
    return "PreFixOp " + prefix->op.lexeme + " " + prefix->name.lexeme;
  }
  if (const auto* binary = dynamic_cast<const Binary*>(&expr)) {
    return "Binary " + binary->op.lexeme;
  }
  if (const auto* assign = dynamic_cast<const Assign*>(&expr)) {
    return "Assign " + assign->name.lexeme;
  }
  if (const auto* conditional = dynamic_cast<const Conditional*>(&expr)) {
    return "Conditional " + conditional->op.lexeme;
  }
  if (dynamic_cast<const CallExpr*>(&expr)) {
    return "CallExpr";
  }
  return "Expr";
}

static std::string RenderExpr(const Expr& expr) {
  std::string out = ExprNodeLabel(expr) + "\n";

  if (const auto* grouping = dynamic_cast<const Grouping*>(&expr)) {
    AppendTreeBlock(&out, "", true, "expr", RenderExpr(*grouping->expr));
    out.pop_back();
    return out;
  }

  if (const auto* binary = dynamic_cast<const Binary*>(&expr)) {
    AppendTreeBlock(&out, "", false, "left", RenderExpr(*binary->left));
    AppendTreeBlock(&out, "", true, "right", RenderExpr(*binary->right));
    out.pop_back();
    return out;
  }

  if (const auto* assign = dynamic_cast<const Assign*>(&expr)) {
    AppendTreeBlock(&out, "", true, "value", RenderExpr(*assign->value));
    out.pop_back();
    return out;
  }

  if (const auto* conditional = dynamic_cast<const Conditional*>(&expr)) {
    AppendTreeBlock(&out, "", false, "left", RenderExpr(*conditional->left));
    AppendTreeBlock(&out, "", true, "right", RenderExpr(*conditional->right));
    out.pop_back();
    return out;
  }

  if (const auto* call = dynamic_cast<const CallExpr*>(&expr)) {
    const bool has_args = !call->args.empty();
    AppendTreeBlock(&out, "", !has_args, "callee", RenderExpr(*call->callee));
    for (size_t i = 0; i < call->args.size(); ++i) {
      const bool is_last = (i + 1) == call->args.size();
      AppendTreeBlock(&out, "", is_last, "arg[" + std::to_string(i) + "]",
                      RenderExpr(*call->args[i]));
    }
    out.pop_back();
    return out;
  }

  if (out.back() == '\n') {
    out.pop_back();
  }
  return out;
}

}  // namespace

Literal::Literal(TokenValue value) : value(value) {}

Value* Literal::Accept(ExprVisitor& visitor) {
  return visitor.Visit(*this);
}

std::string Literal::ToString() {
  return RenderExpr(*this);
}

Variable::Variable(Token name) : name(name) {}

Value* Variable::Accept(ExprVisitor& visitor) {
  return visitor.Visit(*this);
}

std::string Variable::ToString() {
  return RenderExpr(*this);
}

Grouping::Grouping(std::unique_ptr<Expr> expr) : expr(std::move(expr)) {}

Value* Grouping::Accept(ExprVisitor& visitor) {
  return visitor.Visit(*this);
}

std::string Grouping::ToString() {
  return RenderExpr(*this);
}

PreFixOp::PreFixOp(Token op, Token name) : op(op), name(name) {}

Value* PreFixOp::Accept(ExprVisitor& visitor) {
  return visitor.Visit(*this);
}

std::string PreFixOp::ToString() {
  return RenderExpr(*this);
}

Binary::Binary(std::unique_ptr<Expr> left, std::unique_ptr<Expr> right,
               Token op)
    : left(std::move(left)), right(std::move(right)), op(op) {}

Value* Binary::Accept(ExprVisitor& visitor) {
  return visitor.Visit(*this);
}

std::string Binary::ToString() {
  return RenderExpr(*this);
}

Assign::Assign(Token name, std::unique_ptr<Expr> value)
    : name(name), value(std::move(value)) {}

Value* Assign::Accept(ExprVisitor& visitor) {
  return visitor.Visit(*this);
}

std::string Assign::ToString() {
  return RenderExpr(*this);
}

Conditional::Conditional(std::unique_ptr<Expr> left,
                         std::unique_ptr<Expr> right, Token op)
    : left(std::move(left)), right(std::move(right)), op(op) {}

Value* Conditional::Accept(ExprVisitor& visitor) {
  return visitor.Visit(*this);
}

std::string Conditional::ToString() {
  return RenderExpr(*this);
}

CallExpr::CallExpr(std::unique_ptr<Expr> callee,
                   std::vector<std::unique_ptr<Expr>> args)
    : callee(std::move(callee)), args(std::move(args)) {}

Value* CallExpr::Accept(ExprVisitor& visitor) {
  return visitor.Visit(*this);
}

std::string CallExpr::ToString() {
  return RenderExpr(*this);
}
