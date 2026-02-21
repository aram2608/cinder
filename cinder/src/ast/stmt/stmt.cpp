#include "cinder/ast/stmt/stmt.hpp"

#include <memory>
#include <sstream>
#include <string>

#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Value.h"

using namespace cinder;

using namespace llvm;

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
bool Stmt::IsImport() {
  return stmt_type == StmtType::Import;
}
bool Stmt::IsStruct() {
  return stmt_type == StmtType::Struct;
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

std::string ModuleStmt::Accept(StmtDumperVisitor& visitor) {
  return visitor.Visit(*this);
}

ExpressionStmt::ExpressionStmt(std::unique_ptr<Expr> expr)
    : Stmt(StmtType::Expression), expr(std::move(expr)) {}

Value* ExpressionStmt::Accept(StmtVisitor& visitor) {
  return visitor.Visit(*this);
}

std::string ExpressionStmt::Accept(StmtDumperVisitor& visitor) {
  return visitor.Visit(*this);
}

void ExpressionStmt::Accept(SemanticStmtVisitor& visitor) {
  visitor.Visit(*this);
}

FunctionProto::FunctionProto(Token name, Token return_type,
                             std::vector<FuncArg> args, bool is_variadic,
                             bool is_extern)
    : Stmt(StmtType::FunctionProto),
      name(name),
      return_type(return_type),
      args(args),
      is_variadic(is_variadic),
      is_extern(is_extern) {}

Value* FunctionProto::Accept(StmtVisitor& visitor) {
  return visitor.Visit(*this);
}

std::string FunctionProto::Accept(StmtDumperVisitor& visitor) {
  return visitor.Visit(*this);
}

void FunctionProto::Accept(SemanticStmtVisitor& visitor) {
  visitor.Visit(*this);
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

std::string FunctionStmt::Accept(StmtDumperVisitor& visitor) {
  return visitor.Visit(*this);
}

ReturnStmt::ReturnStmt(Token ret_token, std::unique_ptr<Expr> value)
    : Stmt(StmtType::Return), ret_token(ret_token), value(std::move(value)) {}

Value* ReturnStmt::Accept(StmtVisitor& visitor) {
  return visitor.Visit(*this);
}

void ReturnStmt::Accept(SemanticStmtVisitor& visitor) {
  visitor.Visit(*this);
}

std::string ReturnStmt::Accept(StmtDumperVisitor& visitor) {
  return visitor.Visit(*this);
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

std::string VarDeclarationStmt::Accept(StmtDumperVisitor& visitor) {
  return visitor.Visit(*this);
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

std::string IfStmt::Accept(StmtDumperVisitor& visitor) {
  return visitor.Visit(*this);
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

std::string ForStmt::Accept(StmtDumperVisitor& visitor) {
  return visitor.Visit(*this);
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

std::string WhileStmt::Accept(StmtDumperVisitor& visitor) {
  return visitor.Visit(*this);
}

ImportStmt::ImportStmt(cinder::Token mod_name)
    : Stmt(StmtType::Import), mod_name(mod_name) {}

llvm::Value* ImportStmt::Accept(StmtVisitor& visitor) {
  return visitor.Visit(*this);
}

void ImportStmt::Accept(SemanticStmtVisitor& visitor) {
  visitor.Visit(*this);
}

std::string ImportStmt::Accept(StmtDumperVisitor& visitor) {
  return visitor.Visit(*this);
}

StructStmt::StructStmt(cinder::Token name, std::vector<cinder::FuncArg> fields)
    : Stmt(StmtType::Struct), name(name), fields(std::move(fields)) {}

llvm::Value* StructStmt::Accept(StmtVisitor& visitor) {
  return visitor.Visit(*this);
}

void StructStmt::Accept(SemanticStmtVisitor& visitor) {
  visitor.Visit(*this);
}

std::string StructStmt::Accept(StmtDumperVisitor& visitor) {
  return visitor.Visit(*this);
}
