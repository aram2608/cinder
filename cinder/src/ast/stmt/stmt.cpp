#include "cinder/ast/stmt/stmt.hpp"

#include <memory>
#include <sstream>
#include <string>

#include "llvm/IR/Value.h"

using namespace cinder;

using namespace llvm;

namespace {

void AppendDiagram(std::string* out, const std::string& prefix, bool is_last,
                   const std::string& label, const std::string& diagram) {
  *out += prefix + (is_last ? "`- " : "|- ") + label + "\n";
  const std::string child_prefix = prefix + (is_last ? "   " : "|  ");

  std::istringstream in(diagram);
  std::string line;
  while (std::getline(in, line)) {
    *out += child_prefix + line + "\n";
  }
}

std::string RenderStmt(const Stmt& stmt);

std::string StmtNodeLabel(const Stmt& stmt) {
  if (const auto* module = dynamic_cast<const ModuleStmt*>(&stmt)) {
    return "Module " + module->name.lexeme;
  }
  if (dynamic_cast<const ExpressionStmt*>(&stmt)) {
    return "ExpressionStmt";
  }
  if (const auto* proto = dynamic_cast<const FunctionProto*>(&stmt)) {
    return "FunctionProto " + proto->name.lexeme + " -> " +
           proto->return_type.lexeme;
  }
  if (dynamic_cast<const FunctionStmt*>(&stmt)) {
    return "FunctionStmt";
  }
  if (dynamic_cast<const ReturnStmt*>(&stmt)) {
    return "ReturnStmt";
  }
  if (const auto* decl = dynamic_cast<const VarDeclarationStmt*>(&stmt)) {
    return "VarDeclaration " + decl->type.lexeme + " " + decl->name.lexeme;
  }
  if (dynamic_cast<const IfStmt*>(&stmt)) {
    return "IfStmt";
  }
  if (dynamic_cast<const ForStmt*>(&stmt)) {
    return "ForStmt";
  }
  if (dynamic_cast<const WhileStmt*>(&stmt)) {
    return "WhileStmt";
  }
  return "Stmt";
}

std::string RenderStmt(const Stmt& stmt) {
  std::string out = StmtNodeLabel(stmt) + "\n";

  if (const auto* module = dynamic_cast<const ModuleStmt*>(&stmt)) {
    for (size_t i = 0; i < module->stmts.size(); ++i) {
      const bool is_last = (i + 1) == module->stmts.size();
      AppendDiagram(&out, "", is_last, "stmt[" + std::to_string(i) + "]",
                    RenderStmt(*module->stmts[i]));
    }
  } else if (const auto* expr_stmt =
                 dynamic_cast<const ExpressionStmt*>(&stmt)) {
    AppendDiagram(&out, "", true, "expr", expr_stmt->expr->ToString());
  } else if (const auto* proto = dynamic_cast<const FunctionProto*>(&stmt)) {
    for (size_t i = 0; i < proto->args.size(); ++i) {
      const auto& arg = proto->args[i];
      const std::string arg_line =
          arg.type_token.lexeme + " " + arg.identifier.lexeme;
      const bool is_last = (i + 1) == proto->args.size() && !proto->is_variadic;
      AppendDiagram(&out, "", is_last, "arg[" + std::to_string(i) + "]",
                    arg_line);
    }
    if (proto->is_variadic) {
      AppendDiagram(&out, "", true, "variadic", "...");
    }
  } else if (const auto* fn = dynamic_cast<const FunctionStmt*>(&stmt)) {
    const bool has_body = !fn->body.empty();
    AppendDiagram(&out, "", !has_body, "proto", RenderStmt(*fn->proto));
    for (size_t i = 0; i < fn->body.size(); ++i) {
      const bool is_last = (i + 1) == fn->body.size();
      AppendDiagram(&out, "", is_last, "body[" + std::to_string(i) + "]",
                    RenderStmt(*fn->body[i]));
    }
  } else if (const auto* ret = dynamic_cast<const ReturnStmt*>(&stmt)) {
    if (ret->value) {
      AppendDiagram(&out, "", true, "value", ret->value->ToString());
    }
  } else if (const auto* decl =
                 dynamic_cast<const VarDeclarationStmt*>(&stmt)) {
    AppendDiagram(&out, "", true, "value", decl->value->ToString());
  } else if (const auto* if_stmt = dynamic_cast<const IfStmt*>(&stmt)) {
    const bool has_else = static_cast<bool>(if_stmt->otherwise);
    AppendDiagram(&out, "", false, "condition", if_stmt->cond->ToString());
    AppendDiagram(&out, "", !has_else, "then", RenderStmt(*if_stmt->then));
    if (has_else) {
      AppendDiagram(&out, "", true, "else", RenderStmt(*if_stmt->otherwise));
    }
  } else if (const auto* for_stmt = dynamic_cast<const ForStmt*>(&stmt)) {
    const bool has_step = static_cast<bool>(for_stmt->step);
    const bool has_body = !for_stmt->body.empty();

    bool initializer_is_last = !for_stmt->condition && !has_step && !has_body;
    AppendDiagram(&out, "", initializer_is_last, "initializer",
                  RenderStmt(*for_stmt->initializer));

    if (for_stmt->condition) {
      const bool condition_is_last = !has_step && !has_body;
      AppendDiagram(&out, "", condition_is_last, "condition",
                    for_stmt->condition->ToString());
    }

    if (for_stmt->step) {
      const bool step_is_last = !has_body;
      AppendDiagram(&out, "", step_is_last, "step", for_stmt->step->ToString());
    }

    for (size_t i = 0; i < for_stmt->body.size(); ++i) {
      const bool is_last = (i + 1) == for_stmt->body.size();
      AppendDiagram(&out, "", is_last, "body[" + std::to_string(i) + "]",
                    RenderStmt(*for_stmt->body[i]));
    }
  } else if (const auto* while_stmt = dynamic_cast<const WhileStmt*>(&stmt)) {
    const bool has_body = !while_stmt->body.empty();
    AppendDiagram(&out, "", !has_body, "condition",
                  while_stmt->condition->ToString());
    for (size_t i = 0; i < while_stmt->body.size(); ++i) {
      const bool is_last = (i + 1) == while_stmt->body.size();
      AppendDiagram(&out, "", is_last, "body[" + std::to_string(i) + "]",
                    RenderStmt(*while_stmt->body[i]));
    }
  }

  if (out.back() == '\n') {
    out.pop_back();
  }
  return out;
}

}  // namespace

bool Stmt::IsModule() {
  return stmt_type == StmtType::Module;
}
bool Stmt::IsExpression() {
  return stmt_type == StmtType::Expression;
}
bool Stmt::IsFunction() {
  return stmt_type == StmtType::Function;
}
bool Stmt::IsFunctionP() {
  return stmt_type == StmtType::FunctionProto;
}
bool Stmt::IsReturn() {
  return stmt_type == StmtType::Return;
}
bool Stmt::IsVarDeclaration() {
  return stmt_type == StmtType::VarDeclaration;
}
bool Stmt::IsIf() {
  return stmt_type == StmtType::If;
}
bool Stmt::IsFor() {
  return stmt_type == StmtType::For;
}
bool Stmt::IsWhile() {
  return stmt_type == StmtType::While;
}
bool Stmt::HasID() {
  return id.has_value();
}
SymbolId Stmt::GetID() {
  return id.value();
}

ModuleStmt::ModuleStmt(Token name, std::vector<std::unique_ptr<Stmt>> stmts)
    : Stmt(StmtType::Module), name(name), stmts(std::move(stmts)) {}

Value* ModuleStmt::Accept(StmtVisitor& visitor) {
  return visitor.Visit(*this);
}

void ModuleStmt::Accept(SemanticStmtVisitor& visitor) {
  visitor.Visit(*this);
}

std::string ModuleStmt::ToString() {
  return RenderStmt(*this);
}

ExpressionStmt::ExpressionStmt(std::unique_ptr<Expr> expr)
    : Stmt(StmtType::Expression), expr(std::move(expr)) {}

Value* ExpressionStmt::Accept(StmtVisitor& visitor) {
  return visitor.Visit(*this);
}

void ExpressionStmt::Accept(SemanticStmtVisitor& visitor) {
  visitor.Visit(*this);
}

std::string ExpressionStmt::ToString() {
  return RenderStmt(*this);
}

FunctionProto::FunctionProto(Token name, Token return_type,
                             std::vector<FuncArg> args, bool is_variadic)
    : Stmt(StmtType::FunctionProto),
      name(name),
      return_type(return_type),
      args(args),
      is_variadic(is_variadic) {}

Value* FunctionProto::Accept(StmtVisitor& visitor) {
  return visitor.Visit(*this);
}

void FunctionProto::Accept(SemanticStmtVisitor& visitor) {
  visitor.Visit(*this);
}

std::string FunctionProto::ToString() {
  return RenderStmt(*this);
}

FunctionStmt::FunctionStmt(std::unique_ptr<Stmt> proto,
                           std::vector<std::unique_ptr<Stmt>> body)
    : Stmt(StmtType::Function),
      proto(std::move(proto)),
      body(std::move(body)) {}

Value* FunctionStmt::Accept(StmtVisitor& visitor) {
  return visitor.Visit(*this);
}

void FunctionStmt::Accept(SemanticStmtVisitor& visitor) {
  visitor.Visit(*this);
}

std::string FunctionStmt::ToString() {
  return RenderStmt(*this);
}

ReturnStmt::ReturnStmt(Token ret_token, std::unique_ptr<Expr> value)
    : Stmt(StmtType::Return), ret_token(ret_token), value(std::move(value)) {}

Value* ReturnStmt::Accept(StmtVisitor& visitor) {
  return visitor.Visit(*this);
}

void ReturnStmt::Accept(SemanticStmtVisitor& visitor) {
  visitor.Visit(*this);
}

std::string ReturnStmt::ToString() {
  return RenderStmt(*this);
}

VarDeclarationStmt::VarDeclarationStmt(Token type, Token name,
                                       std::unique_ptr<Expr> value)
    : Stmt(StmtType::VarDeclaration),
      type(type),
      name(name),
      value(std::move(value)) {}

Value* VarDeclarationStmt::Accept(StmtVisitor& visitor) {
  return visitor.Visit(*this);
}

void VarDeclarationStmt::Accept(SemanticStmtVisitor& visitor) {
  visitor.Visit(*this);
}

std::string VarDeclarationStmt::ToString() {
  return RenderStmt(*this);
}

IfStmt::IfStmt(std::unique_ptr<Expr> cond, std::unique_ptr<Stmt> then,
               std::unique_ptr<Stmt> otherwise)
    : Stmt(StmtType::If),
      cond(std::move(cond)),
      then(std::move(then)),
      otherwise(std::move(otherwise)) {}

Value* IfStmt::Accept(StmtVisitor& visitor) {
  return visitor.Visit(*this);
}

void IfStmt::Accept(SemanticStmtVisitor& visitor) {
  visitor.Visit(*this);
}

std::string IfStmt::ToString() {
  return RenderStmt(*this);
}

ForStmt::ForStmt(std::unique_ptr<Stmt> intializer,
                 std::unique_ptr<Expr> condition, std::unique_ptr<Expr> step,
                 std::vector<std::unique_ptr<Stmt>> body)
    : Stmt(StmtType::For),
      initializer(std::move(intializer)),
      condition(std::move(condition)),
      step(std::move(step)),
      body(std::move(body)) {}

Value* ForStmt::Accept(StmtVisitor& visitor) {
  return visitor.Visit(*this);
}

void ForStmt::Accept(SemanticStmtVisitor& visitor) {
  visitor.Visit(*this);
}

std::string ForStmt::ToString() {
  return RenderStmt(*this);
}

WhileStmt::WhileStmt(std::unique_ptr<Expr> condition,
                     std::vector<std::unique_ptr<Stmt>> body)
    : Stmt(StmtType::While),
      condition(std::move(condition)),
      body(std::move(body)) {}

Value* WhileStmt::Accept(StmtVisitor& visitor) {
  return visitor.Visit(*this);
}

void WhileStmt::Accept(SemanticStmtVisitor& visitor) {
  visitor.Visit(*this);
}

std::string WhileStmt::ToString() {
  return RenderStmt(*this);
}
