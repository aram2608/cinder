#ifndef AST_DUMPER_H_
#define AST_DUMPER_H_

#include <memory>
#include <vector>

#include "cinder/ast/expr/expr.hpp"
#include "cinder/ast/stmt/stmt.hpp"

namespace cinder {

struct AstDumper : ExprDumperVisitor, StmtDumperVisitor {
  void RenderProgram(const std::vector<std::unique_ptr<Stmt>>& prog);

  using ExprDumperVisitor::Visit;
  using StmtDumperVisitor::Visit;
  std::string Visit(Literal& expr) override;
  std::string Visit(Variable& expr) override;
  std::string Visit(Grouping& expr) override;
  std::string Visit(PreFixOp& expr) override;
  std::string Visit(Binary& expr) override;
  std::string Visit(CallExpr& expr) override;
  std::string Visit(Assign& expr) override;
  std::string Visit(Conditional& expr) override;

  std::string Visit(ExpressionStmt& stmt) override;
  std::string Visit(FunctionStmt& stmt) override;
  std::string Visit(ReturnStmt& stmt) override;
  std::string Visit(VarDeclarationStmt& stmt) override;
  std::string Visit(FunctionProto& stmt) override;
  std::string Visit(ModuleStmt& stmt) override;
  std::string Visit(IfStmt& stmt) override;
  std::string Visit(ForStmt& stmt) override;
  std::string Visit(WhileStmt& stmt) override;
  std::string Visit(ImportStmt& stmt) override;
};

}  // namespace cinder

#endif
