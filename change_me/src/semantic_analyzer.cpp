#include "../include/semantic_analyzer.hpp"

/// Global error handler
static errs::RawOutStream errors{};

types::Type* TypeContext::Int32() {
  return &int32;
}

types::Type* TypeContext::Float32() {
  return &float32;
}

types::Type* TypeContext::Void() {
  return &voidTy;
}

types::Type* TypeContext::Bool() {
  return &int1;
}

/// Leaks memory, fix somehow, maybe an arena
types::Type* TypeContext::Function(types::Type* ret,
                                   std::vector<types::Type*> params) {
  return new types::FunctionType{ret, params};
}

Scope::Scope(std::shared_ptr<Scope> parent) : parent(parent) {}

void Scope::Declare(const std::string& name, types::Type* type) {
  if (table.find(name) != table.end()) {
    errs::ErrorOutln(errors, "Redefinition of variable:", name);
  }
  table[name] = Symbol{type};
}

Symbol* Scope::Lookup(const std::string& name) {
  if (auto it = table.find(name); it != table.end()) {
    return &it->second;
  }
  return parent ? parent->Lookup(name) : nullptr;
}

SemanticAnalyzer::SemanticAnalyzer(TypeContext& tc)
    : types(tc), scope(nullptr) {}

llvm::Value* SemanticAnalyzer::Visit(ModuleStmt& stmt) {
  for (auto& s : stmt.stmts) {
    s->Accept(*this);
  }
  return nullptr;
}

llvm::Value* SemanticAnalyzer::Visit(FunctionProto& stmt) {
  types::Type* ret;
  switch (stmt.return_type.type) {
    case TT_INT32_SPECIFIER:
      ret = types.Int32();
      break;
    case TT_FLT_SPECIFIER:
      ret = types.Float32();
      break;
    case TT_VOID_SPECIFIER:
      ret = types.Void();
      break;
    case TT_BOOL_SPECIFIER:
      ret = types.Bool();
      break;
    default:
      errs::ErrorOutln(errors, "Invalid type:", stmt.return_type.lexeme);
  }
  std::vector<types::Type*> params;
  for (auto& arg : stmt.args) {
    switch (arg.type.type) {
      case TT_INT32_SPECIFIER:
        params.push_back(types.Int32());
        break;
      case TT_FLT_SPECIFIER:
        params.push_back(types.Float32());
        break;
      case TT_VOID_SPECIFIER:
        params.push_back(types.Void());
        break;
      case TT_BOOL_SPECIFIER:
        params.push_back(types.Bool());
        break;
      default:
        errs::ErrorOutln(errors, "Invalid type:", stmt.return_type.lexeme);
    }
  }
  scope->Declare(stmt.name.lexeme, types.Function(ret, params));
  return nullptr;
}

llvm::Value* SemanticAnalyzer::Visit(FunctionStmt& stmt) {
  FunctionProto* proto = dynamic_cast<FunctionProto*>(stmt.proto.get());

  std::shared_ptr<Scope> function_scope = std::make_shared<Scope>(this->scope);
  std::shared_ptr<Scope> previous_scope = this->scope;
  this->scope = function_scope;

  for (auto& arg : proto->args) {
    types::Type* arg_type = ResolveArgType(arg.type);
    scope->Declare(arg.identifier.lexeme, arg_type);
  }

  this->scope = previous_scope;
  return nullptr;
}

llvm::Value* SemanticAnalyzer::Visit(ForStmt& stmt) {
  return nullptr;
}

llvm::Value* SemanticAnalyzer::Visit(WhileStmt& stmt) {
  return nullptr;
}

llvm::Value* SemanticAnalyzer::Visit(IfStmt& stmt) {
  return nullptr;
}

llvm::Value* SemanticAnalyzer::Visit(ExpressionStmt& stmt) {
  return nullptr;
}

llvm::Value* SemanticAnalyzer::Visit(ReturnStmt& stmt) {
  return nullptr;
}

llvm::Value* SemanticAnalyzer::Visit(VarDeclarationStmt& stmt) {
  return nullptr;
}

llvm::Value* SemanticAnalyzer::Visit(Literal& expr) {
  if (std::holds_alternative<int>(expr.value)) {
    expr.type = types.Int32();
  } else if (std::holds_alternative<float>(expr.value)) {
    expr.type = types.Float32();
  } else {
    UNREACHABLE(VisitLiteral, "Unknown value type");
  }
  return nullptr;
}

/// TODO: Create a bool type
llvm::Value* SemanticAnalyzer::Visit(BoolLiteral& expr) {
  expr.type = types.Int32();
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
  expr.left->Accept(*this);
  expr.right->Accept(*this);

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
      expr.type = types.Bool();
      break;

    default:
      UNREACHABLE(VisitBinary, "Unknown operation: " + expr.op.lexeme);
  }
  return nullptr;
}

llvm::Value* SemanticAnalyzer::Visit(Assign& expr) {
  expr.value->Accept(*this);

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

llvm::Value* SemanticAnalyzer::Visit(CallExpr& expr) {
  for (auto& arg : expr.args) {
    arg->Accept(*this);
  }

  /// TODO: Figure out a type for this
  expr.type = types.Int32();
  return nullptr;
}

types::Type* SemanticAnalyzer::ResolveArgType(Token type) {
  switch (type.type) {
    case TT_INT32_SPECIFIER:
      return types.Int32();
      break;
    case TT_FLT_SPECIFIER:
      return types.Float32();
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
      return types.Int32();
    case TT_FLT_SPECIFIER:
      return types.Float32();
    case TT_VOID_SPECIFIER:
      return types.Void();
    case TT_INT64_SPECIFIER:
    default:
      errs::ErrorOutln(errors, "Invalid type:", type.lexeme, "at line",
                       type.line_num);
  }
  return nullptr;
}