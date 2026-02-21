#include "cinder/ast/ast_dumper.hpp"

#include <iostream>
#include <sstream>

using namespace cinder;

static std::string EscapeString(const std::string& value) {
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
struct overload : Ts... {
  using Ts::operator()...;
};
template <class... Ts>
overload(Ts...) -> overload<Ts...>;

static std::string LiteralValueToString(const TokenValue& value) {
  return std::visit(
      overload{[](bool v) -> std::string { return v ? "true" : "false"; },
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

static void TrimTrailingNewline(std::string* text) {
  if (!text->empty() && text->back() == '\n') {
    text->pop_back();
  }
}

static void AppendTreeBlock(std::string* out, const std::string& prefix,
                            bool is_last, const std::string& label,
                            const std::string& child_tree) {
  *out += prefix + (is_last ? "`- " : "|- ") + label + "\n";
  const std::string child_prefix = prefix + (is_last ? "   " : "|  ");

  std::istringstream in(child_tree);
  std::string line;
  while (std::getline(in, line)) {
    *out += child_prefix + line + "\n";
  }
}

void AstDumper::RenderProgram(const std::vector<std::unique_ptr<Stmt>>& prog) {
  for (const auto& stmt : prog) {
    std::cout << stmt->Accept(*this) << "\n";
  }
}

std::string AstDumper::Visit(Literal& expr) {
  return "Literal " + LiteralValueToString(expr.value);
}

std::string AstDumper::Visit(Variable& expr) {
  return "Variable " + expr.name.lexeme;
}

std::string AstDumper::Visit(Grouping& expr) {
  std::string out = "Grouping\n";
  AppendTreeBlock(&out, "", true, "expr", expr.expr->Accept(*this));
  TrimTrailingNewline(&out);
  return out;
}

std::string AstDumper::Visit(PreFixOp& expr) {
  return "PrefixOp " + expr.op.lexeme + " " + expr.name.lexeme;
}

std::string AstDumper::Visit(Binary& expr) {
  std::string out = "Binary " + expr.op.lexeme + "\n";
  AppendTreeBlock(&out, "", false, "left", expr.left->Accept(*this));
  AppendTreeBlock(&out, "", true, "right", expr.right->Accept(*this));
  TrimTrailingNewline(&out);
  return out;
}

std::string AstDumper::Visit(CallExpr& expr) {
  std::string out = "CallExpr\n";
  const bool has_args = !expr.args.empty();
  AppendTreeBlock(&out, "", !has_args, "callee", expr.callee->Accept(*this));
  for (size_t i = 0; i < expr.args.size(); ++i) {
    const bool is_last = (i + 1) == expr.args.size();
    AppendTreeBlock(&out, "", is_last, "arg[" + std::to_string(i) + "]",
                    expr.args[i]->Accept(*this));
  }
  TrimTrailingNewline(&out);
  return out;
}

std::string AstDumper::Visit(Assign& expr) {
  std::string out = "Assign " + expr.name.lexeme + "\n";
  AppendTreeBlock(&out, "", true, "value", expr.value->Accept(*this));
  TrimTrailingNewline(&out);
  return out;
}

std::string AstDumper::Visit(Conditional& expr) {
  std::string out = "Conditional " + expr.op.lexeme + "\n";
  AppendTreeBlock(&out, "", false, "left", expr.left->Accept(*this));
  AppendTreeBlock(&out, "", true, "right", expr.right->Accept(*this));
  TrimTrailingNewline(&out);
  return out;
}

std::string AstDumper::Visit(ExpressionStmt& stmt) {
  std::string out = "ExpressionStmt\n";
  AppendTreeBlock(&out, "", true, "expr", stmt.expr->Accept(*this));
  TrimTrailingNewline(&out);
  return out;
}

std::string AstDumper::Visit(FunctionStmt& stmt) {
  std::string out = "FunctionStmt\n";
  const bool has_body = !stmt.body.empty();
  AppendTreeBlock(&out, "", !has_body, "proto", stmt.proto->Accept(*this));
  for (size_t i = 0; i < stmt.body.size(); ++i) {
    const bool is_last = (i + 1) == stmt.body.size();
    AppendTreeBlock(&out, "", is_last, "body[" + std::to_string(i) + "]",
                    stmt.body[i]->Accept(*this));
  }
  TrimTrailingNewline(&out);
  return out;
}

std::string AstDumper::Visit(ReturnStmt& stmt) {
  std::string out = "ReturnStmt\n";
  if (stmt.value) {
    AppendTreeBlock(&out, "", true, "value", stmt.value->Accept(*this));
  }
  TrimTrailingNewline(&out);
  return out;
}

std::string AstDumper::Visit(VarDeclarationStmt& stmt) {
  std::string out =
      "VarDeclaration " + stmt.type.lexeme + " " + stmt.name.lexeme + "\n";
  AppendTreeBlock(&out, "", true, "value", stmt.value->Accept(*this));
  TrimTrailingNewline(&out);
  return out;
}

std::string AstDumper::Visit(FunctionProto& stmt) {
  std::string out = "FunctionProto " + stmt.name.lexeme + " -> " +
                    stmt.return_type.lexeme + "\n";
  for (size_t i = 0; i < stmt.args.size(); ++i) {
    const auto& arg = stmt.args[i];
    const std::string arg_line =
        arg.type_token.lexeme + " " + arg.identifier.lexeme;
    const bool is_last = (i + 1) == stmt.args.size() && !stmt.is_variadic;
    AppendTreeBlock(&out, "", is_last, "arg[" + std::to_string(i) + "]",
                    arg_line);
  }
  if (stmt.is_variadic) {
    AppendTreeBlock(&out, "", true, "variadic", "...");
  }
  TrimTrailingNewline(&out);
  return out;
}

std::string AstDumper::Visit(ModuleStmt& stmt) {
  std::string out = "Module " + stmt.name.lexeme + "\n";
  for (size_t i = 0; i < stmt.stmts.size(); ++i) {
    const bool is_last = (i + 1) == stmt.stmts.size();
    AppendTreeBlock(&out, "", is_last, "stmt[" + std::to_string(i) + "]",
                    stmt.stmts[i]->Accept(*this));
  }
  TrimTrailingNewline(&out);
  return out;
}

std::string AstDumper::Visit(IfStmt& stmt) {
  std::string out = "IfStmt\n";

  const bool has_else = static_cast<bool>(stmt.otherwise);
  AppendTreeBlock(&out, "", false, "condition", stmt.cond->Accept(*this));
  AppendTreeBlock(&out, "", !has_else, "then", stmt.then->Accept(*this));
  if (has_else) {
    AppendTreeBlock(&out, "", true, "else", stmt.otherwise->Accept(*this));
  }

  TrimTrailingNewline(&out);
  return out;
}

std::string AstDumper::Visit(ForStmt& stmt) {
  std::string out = "ForStmt\n";

  const bool has_step = static_cast<bool>(stmt.step);
  const bool has_body = !stmt.body.empty();

  bool initializer_is_last = !stmt.condition && !has_step && !has_body;
  AppendTreeBlock(&out, "", initializer_is_last, "initializer",
                  stmt.initializer->Accept(*this));

  if (stmt.condition) {
    const bool condition_is_last = !has_step && !has_body;
    AppendTreeBlock(&out, "", condition_is_last, "condition",
                    stmt.condition->Accept(*this));
  }
  if (stmt.step) {
    const bool step_is_last = !has_body;
    AppendTreeBlock(&out, "", step_is_last, "step", stmt.step->Accept(*this));
  }
  for (size_t i = 0; i < stmt.body.size(); ++i) {
    const bool is_last = (i + 1) == stmt.body.size();
    AppendTreeBlock(&out, "", is_last, "body[" + std::to_string(i) + "]",
                    stmt.body[i]->Accept(*this));
  }
  TrimTrailingNewline(&out);
  return out;
}

std::string AstDumper::Visit(WhileStmt& stmt) {
  std::string out = "WhileStmt\n";
  const bool has_body = !stmt.body.empty();
  AppendTreeBlock(&out, "", !has_body, "condition",
                  stmt.condition->Accept(*this));
  for (size_t i = 0; i < stmt.body.size(); ++i) {
    const bool is_last = (i + 1) == stmt.body.size();
    AppendTreeBlock(&out, "", is_last, "body[" + std::to_string(i) + "]",
                    stmt.body[i]->Accept(*this));
  }
  TrimTrailingNewline(&out);
  return out;
}
std::string AstDumper::Visit(ImportStmt& stmt) {
  return "ImportStmt " + stmt.mod_name.lexeme;
}
