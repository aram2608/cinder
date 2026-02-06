#include "../include/semantic_analyzer.hpp"

#include "../include/errors.hpp"
#include "../include/utils.hpp"
#include "stmt.hpp"
#include "tokens.hpp"
#include "types.hpp"

/// Global error handler
static errs::RawOutStream errors{};

SemanticAnalyzer::SemanticAnalyzer(TypeContext* tc)
    : types(tc), scope(nullptr), current_return(nullptr) {}

llvm::Value* SemanticAnalyzer::Visit(ModuleStmt& stmt) {
  for (auto& s : stmt.stmts) {
    Resolve(*s);
  }
  return nullptr;
}

llvm::Value* SemanticAnalyzer::Visit(FunctionProto& stmt) {
  types::Type* ret = ResolveType(stmt.return_type);

  std::vector<types::Type*> params;
  for (auto& arg : stmt.args) {
    params.push_back(ResolveArgType(arg.type));
  }

  Declare(stmt.name.lexeme, types->Function(ret, params));
  return nullptr;
}

llvm::Value* SemanticAnalyzer::Visit(FunctionStmt& stmt) {
  FunctionProto* proto = dynamic_cast<FunctionProto*>(stmt.proto.get());
  current_return = ResolveType(proto->return_type);

  std::shared_ptr<Scope> function_scope = std::make_shared<Scope>(this->scope);
  std::shared_ptr<Scope> previous_scope = this->scope;
  this->scope = function_scope;

  for (auto& arg : proto->args) {
    types::Type* arg_type = ResolveArgType(arg.type);
    Declare(arg.identifier.lexeme, arg_type);
  }

  for (auto& s : stmt.body) {
    Resolve(*s);
  }

  this->scope = previous_scope;
  return nullptr;
}

llvm::Value* SemanticAnalyzer::Visit(ForStmt& stmt) {
  std::shared_ptr<Scope> for_scope = std::make_shared<Scope>(this->scope);
  std::shared_ptr<Scope> previous_scope = this->scope;
  this->scope = for_scope;

  Resolve(*stmt.initializer);
  Resolve(*stmt.condition);

  for (auto& s : stmt.body) {
    Resolve(*s);
  }

  this->scope = previous_scope;
  return nullptr;
}

llvm::Value* SemanticAnalyzer::Visit(WhileStmt& stmt) {
  Resolve(*stmt.condition);
  for (auto& s : stmt.body) {
    Resolve(*s);
  }
  return nullptr;
}

llvm::Value* SemanticAnalyzer::Visit(IfStmt& stmt) {
  Resolve(*stmt.cond);
  Resolve(*stmt.then);
  Resolve(*stmt.otherwise);
  return nullptr;
}

llvm::Value* SemanticAnalyzer::Visit(ExpressionStmt& stmt) {
  Resolve(*stmt.expr);
  return nullptr;
}

llvm::Value* SemanticAnalyzer::Visit(ReturnStmt& stmt) {
  Resolve(*stmt.value);
  if (stmt.value->type != current_return) {
    errs::ErrorOutln(errors, "Return value does not match current return type");
  }
  return nullptr;
}

llvm::Value* SemanticAnalyzer::Visit(VarDeclarationStmt& stmt) {
  auto* sym = scope->Lookup(stmt.name.lexeme);

  if (sym) {
    errs::ErrorOutln(errors, "Variable already declared:", stmt.name.lexeme,
                     "at line:", stmt.name.line_num);
    return nullptr;
  }

  stmt.value->type = ResolveType(stmt.type);
  Declare(stmt.name.lexeme, stmt.value->type);
  return nullptr;
}

llvm::Value* SemanticAnalyzer::Visit(Variable& expr) {
  auto* sym = scope->Lookup(expr.name.lexeme);
  if (!sym) {
    errs::ErrorOutln(errors, "Undeclared variable:", expr.name.lexeme);
  }
  expr.type = sym->type;
  return nullptr;
}

llvm::Value* SemanticAnalyzer::Visit(Binary& expr) {
  Resolve(*expr.left);
  Resolve(*expr.right);

  if (expr.left->type != expr.right->type) {
    errs::ErrorOutln(errors, "Type mismatch:", expr.op.lexeme, "at line",
                     expr.op.line_num);
  }

  switch (expr.op.type) {
    case TT_PLUS:
    case TT_MINUS:
    case TT_STAR:
    case TT_SLASH:
      expr.type = expr.left->type;
      break;
    case TT_EQEQ:
    case TT_BANGEQ:
      expr.type = types->Bool();
      break;
    default:
      UNREACHABLE(VisitBinary, "Unknown operation: " + expr.op.lexeme);
  }

  return nullptr;
}

llvm::Value* SemanticAnalyzer::Visit(Assign& expr) {
  Resolve(*expr.value);
  auto* sym = scope->Lookup(expr.name.lexeme);
  if (!sym) {
    errs::ErrorOutln(errors,
                     "Assignment to undelcared variable:", expr.name.lexeme);
  }
  if (sym->type != expr.value->type) {
    errs::ErrorOutln(errors, "Type mismatch in assignment:", expr.name.lexeme);
  }
  expr.type = sym->type;
  return nullptr;
}

llvm::Value* SemanticAnalyzer::Visit(PreFixOp& expr) {
  return nullptr;
}

llvm::Value* SemanticAnalyzer::Visit(Conditional& expr) {
  return nullptr;
}

llvm::Value* SemanticAnalyzer::Visit(Grouping& expr) {
  Resolve(*expr.expr);
  return nullptr;
}

llvm::Value* SemanticAnalyzer::Visit(CallExpr& expr) {
  Variable* callee = dynamic_cast<Variable*>(expr.callee.get());
  Symbol* symbol = scope->Lookup(callee->name.lexeme);

  if (!symbol) {
    errs::ErrorOutln(errors, "Undefined function:", callee->name.lexeme);
    return nullptr;
  }

  auto* func_type = dynamic_cast<types::FunctionType*>(symbol->type);
  if (!func_type) {
    errs::ErrorOutln(errors, "Symbol is not callable:", callee->name.lexeme);
    return nullptr;
  }

  /// TODO: Find a way to add variadic functions
  if (expr.args.size() != func_type->params.size()) {
    errs::ErrorOutln(errors,
                     "Argument count mismatch for:", callee->name.lexeme);
    return nullptr;
  }

  /// TODO: make sure this works, im not entirely sure if it will or not
  for (size_t it = 0; it < expr.args.size(); it++) {
    Resolve(*expr.args[it]);
    if (expr.args[it]->type != func_type->params[it]) {
      errs::ErrorOutln(errors, "Type mismatch in argument ");
    }
  }

  expr.type = func_type->return_type;
  return nullptr;
}

llvm::Value* SemanticAnalyzer::Visit(BoolLiteral& expr) {
  expr.type = types->Bool();
  return nullptr;
}

/// TODO: Extend literal types
llvm::Value* SemanticAnalyzer::Visit(Literal& expr) {
  if (std::holds_alternative<int>(expr.value)) {
    expr.type = types->Int32();
  } else if (std::holds_alternative<float>(expr.value)) {
    expr.type = types->Float32();
  } else {
    UNREACHABLE(VisitLiteral, "Unknown value type");
  }
  return nullptr;
}

types::Type* SemanticAnalyzer::ResolveArgType(Token type) {
  switch (type.type) {
    case TT_INT32_SPECIFIER:
      return types->Int32();
    case TT_FLT_SPECIFIER:
      return types->Float32();
    case TT_BOOL_SPECIFIER:
      return types->Bool();
    case TT_STR_SPECIFIER:
      return types->String();
    case TT_INT64_SPECIFIER:
    // Not valid in args
    case TT_VOID_SPECIFIER:
    default:
      errs::ErrorOutln(errors, "Invalid type:", type.lexeme, "at line",
                       type.line_num);
  }
  return nullptr;
}

types::Type* SemanticAnalyzer::ResolveType(Token type) {
  switch (type.type) {
    case TT_INT32_SPECIFIER:
      return types->Int32();
    case TT_FLT_SPECIFIER:
      return types->Float32();
    case TT_VOID_SPECIFIER:
      return types->Void();
    case TT_STR_SPECIFIER:
      return types->String();
    case TT_BOOL_SPECIFIER:
      return types->Bool();
    case TT_INT64_SPECIFIER:
    default:
      errs::ErrorOutln(errors, "Invalid type:", type.lexeme, "at line",
                       type.line_num);
  }
  return nullptr;
}

llvm::Value* SemanticAnalyzer::Resolve(Stmt& stmt) {
  return stmt.Accept(*this);
}

llvm::Value* SemanticAnalyzer::Resolve(Expr& expr) {
  return expr.Accept(*this);
}

void SemanticAnalyzer::Declare(std::string name, types::Type* type) {
  scope->Declare(name, type);
}

void SemanticAnalyzer::Analyze(ModuleStmt& mod) {
  Resolve(mod);
}