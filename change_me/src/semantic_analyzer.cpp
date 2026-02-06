#include "../include/semantic_analyzer.hpp"

#include <variant>
#include <vector>

#include "../include/errors.hpp"
#include "../include/utils.hpp"

/// Global error handler
static errs::RawOutStream errors{};

SemanticAnalyzer::SemanticAnalyzer(TypeContext* tc)
    : types(tc), scope(nullptr), current_return(nullptr) {}

llvm::Value* SemanticAnalyzer::Visit(ModuleStmt& stmt) {
  std::shared_ptr<Scope> prev = this->scope;
  std::shared_ptr<Scope> global_scope = std::make_shared<Scope>(this->scope);

  this->scope = global_scope;
  for (auto& s : stmt.stmts) {
    Resolve(*s);
  }

  this->scope = prev;
  return nullptr;
}

llvm::Value* SemanticAnalyzer::Visit(FunctionProto& stmt) {
  types::Type* ret = ResolveType(stmt.return_type);
  stmt.resolved_type = ret;

  std::vector<types::Type*> params;
  for (auto& arg : stmt.args) {
    types::Type* arg_type = ResolveArgType(arg.type_token);
    arg.resolved_type = arg_type;
    params.push_back(arg_type);
  }

  Declare(stmt.name.lexeme, types->Function(ret, params, stmt.is_variadic));
  return nullptr;
}

llvm::Value* SemanticAnalyzer::Visit(FunctionStmt& stmt) {
  Resolve(*stmt.proto);
  FunctionProto* proto = dynamic_cast<FunctionProto*>(stmt.proto.get());
  current_return = ResolveType(proto->return_type);

  std::shared_ptr<Scope> function_scope = std::make_shared<Scope>(this->scope);
  std::shared_ptr<Scope> previous_scope = this->scope;
  this->scope = function_scope;

  for (auto& arg : proto->args) {
    types::Type* arg_type = ResolveArgType(arg.type_token);
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
  if (stmt.otherwise) {
    Resolve(*stmt.otherwise);
  }
  return nullptr;
}

llvm::Value* SemanticAnalyzer::Visit(ExpressionStmt& stmt) {
  Resolve(*stmt.expr);
  return nullptr;
}

llvm::Value* SemanticAnalyzer::Visit(ReturnStmt& stmt) {
  if (!current_return) {
    errs::ErrorOutln(errors, "Return statement outside function body");
    return nullptr;
  }

  if (!stmt.value) {
    if (current_return->kind != types::TypeKind::Void) {
      errs::ErrorOutln(errors,
                       "Return value does not match current return type");
    }
    return nullptr;
  }

  Resolve(*stmt.value);
  if (stmt.value->type->kind != current_return->kind) {
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

  types::Type* declared_type = ResolveType(stmt.type);
  Resolve(*stmt.value);

  if (stmt.value->type->kind != declared_type->kind) {
    errs::ErrorOutln(errors,
                     "Type mismatch in variable declaration:", stmt.name.lexeme,
                     "at line:", stmt.name.line_num);
    return nullptr;
  }

  stmt.value->type = declared_type;
  Declare(stmt.name.lexeme, declared_type);
  return nullptr;
}

llvm::Value* SemanticAnalyzer::Visit(Variable& expr) {
  auto* sym = scope->Lookup(expr.name.lexeme);
  if (!sym) {
    errs::ErrorOutln(errors, "Undeclared variable:", expr.name.lexeme);
    return nullptr;
  }
  expr.type = sym->type;
  return nullptr;
}

llvm::Value* SemanticAnalyzer::Visit(Binary& expr) {
  Resolve(*expr.left);
  Resolve(*expr.right);

  if (expr.left->type->kind != expr.right->type->kind) {
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
    return nullptr;
  }
  if (sym->type->kind != expr.value->type->kind) {
    errs::ErrorOutln(errors, "Type mismatch in assignment:", expr.name.lexeme);
  }
  expr.type = sym->type;
  return nullptr;
}

llvm::Value* SemanticAnalyzer::Visit(PreFixOp& expr) {
  auto* sym = scope->Lookup(expr.name.lexeme);
  if (!sym) {
    errs::ErrorOutln(errors, "Variable is not defined:", expr.name.lexeme);
    return nullptr;
  }

  if (sym->type->kind != types::TypeKind::Int &&
      sym->type->kind != types::TypeKind::Float) {
    errs::ErrorOutln(
        errors, "Prefix operator requires numeric operand:", expr.name.lexeme);
  }

  expr.type = sym->type;
  return nullptr;
}

llvm::Value* SemanticAnalyzer::Visit(Conditional& expr) {
  Resolve(*expr.left);
  Resolve(*expr.right);
  if (expr.left->type->kind != expr.right->type->kind) {
    errs::ErrorOutln(errors, "Type mismatch:", expr.op.lexeme, "at line",
                     expr.op.line_num);
  }
  expr.type = types->Bool();
  return nullptr;
}

llvm::Value* SemanticAnalyzer::Visit(Grouping& expr) {
  Resolve(*expr.expr);
  return nullptr;
}

llvm::Value* SemanticAnalyzer::Visit(CallExpr& expr) {
  Variable* callee = dynamic_cast<Variable*>(expr.callee.get());
  if (!callee) {
    errs::ErrorOutln(errors,
                     "Only direct function calls are currently supported");
    return nullptr;
  }

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

  size_t num_params = func_type->params.size();
  size_t num_args = expr.args.size();

  if (func_type->is_variadic) {
    if (num_args < num_params) {
      errs::ErrorOutln(errors, "Too few arguments for variadic function:",
                       callee->name.lexeme);
      return nullptr;
    }
  } else if (num_args != num_params) {
    errs::ErrorOutln(errors,
                     "Argument count mismatch for:", callee->name.lexeme);
    return nullptr;
  }

  /// TODO: make sure this works, im not entirely sure if it will or not
  for (size_t i = 0; i < num_args; i++) {
    Resolve(*expr.args[i]);
    if (i < num_params) {
      if (expr.args[i]->type->kind != func_type->params[i]->kind) {
        errs::ErrorOutln(errors, "Type mismatch in fixed argument", i);
      }
    } else {
      // This should come in handy later when types get extended
      VariadicPromotion(expr.args[i].get());
    }
  }

  expr.type = func_type->return_type;
  return nullptr;
}

/// TODO: Extend literal types
llvm::Value* SemanticAnalyzer::Visit(Literal& expr) {
  if (std::holds_alternative<int>(expr.value)) {
    expr.type = types->Int32();
  } else if (std::holds_alternative<float>(expr.value)) {
    expr.type = types->Float32();
  } else if (std::holds_alternative<std::string>(expr.value)) {
    expr.type = types->String();
  } else if (std::holds_alternative<bool>(expr.value)) {
    expr.type = types->Bool();
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

void SemanticAnalyzer::VariadicPromotion(Expr* expr) {
  switch (expr->type->kind) {
    case types::TypeKind::Float:
      expr->type = types->Float32();
      break;
    case types::TypeKind::Int:
      expr->type = types->Int32();
      break;
    case types::TypeKind::Bool:
      expr->type = types->Int32();
      break;
    case types::TypeKind::Function:
    case types::TypeKind::String:
    case types::TypeKind::Struct:
    case types::TypeKind::Void:
    default:
      break;
  }
}
