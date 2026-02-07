#include "cinder/semantic/semantic_analyzer.hpp"

#include <optional>
#include <string>
#include <variant>
#include <vector>

#include "cinder/ast/types.hpp"
#include "cinder/frontend/tokens.hpp"
#include "cinder/semantic/symbol.hpp"
#include "cinder/semantic/type_context.hpp"
#include "cinder/support/diagnostic.hpp"
#include "cinder/support/utils.hpp"

SemanticAnalyzer::SemanticAnalyzer(TypeContext& types)
    : types_(types), current_return(nullptr) {}

llvm::Value* SemanticAnalyzer::Visit(ModuleStmt& stmt) {
  BeginScope();
  for (auto& s : stmt.stmts) {
    Resolve(*s);
  }
  EndScope();
  return nullptr;
}

llvm::Value* SemanticAnalyzer::Visit(FunctionProto& stmt) {
  types::Type* ret = ResolveType(stmt.return_type);

  std::vector<types::Type*> params;
  for (auto& arg : stmt.args) {
    types::Type* arg_type = ResolveArgType(arg.type_token);
    arg.resolved_type = arg_type;
    params.push_back(arg_type);
  }

  std::optional<SymbolId> id =
      Declare(stmt.name.lexeme, types_.Function(ret, params, stmt.is_variadic),
              true, {stmt.name.line_num});
  if (id.has_value()) {
    stmt.id = id.value();
  } else {
    std::string err = "Function could not be declared: " + stmt.name.lexeme;
    diagnose_.Error({stmt.name.line_num}, err);
  }
  return nullptr;
}

llvm::Value* SemanticAnalyzer::Visit(FunctionStmt& stmt) {
  Resolve(*stmt.proto);
  FunctionProto* proto = dynamic_cast<FunctionProto*>(stmt.proto.get());
  current_return = ResolveType(proto->return_type);

  BeginScope();

  for (auto& arg : proto->args) {
    types::Type* arg_type = ResolveArgType(arg.type_token);
    Declare(arg.identifier.lexeme, arg_type, false, {arg.identifier.line_num});
  }

  for (auto& s : stmt.body) {
    Resolve(*s);
  }

  EndScope();
  return nullptr;
}

llvm::Value* SemanticAnalyzer::Visit(ForStmt& stmt) {
  BeginScope();

  Resolve(*stmt.initializer);
  Resolve(*stmt.condition);
  if (stmt.step) {
    Resolve(*stmt.step);
  }

  for (auto& s : stmt.body) {
    Resolve(*s);
  }

  EndScope();
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
    diagnose_.Error({stmt.ret_token.line_num},
                    "Return statement outside function body");
    return nullptr;
  }

  if (!stmt.value) {
    if (current_return->kind != types::TypeKind::Void) {
      diagnose_.Error({stmt.ret_token.line_num},
                      "Return value does not match current return type");
    }
    return nullptr;
  }

  Resolve(*stmt.value);
  if (stmt.value->type->kind != current_return->kind) {
    diagnose_.Error({stmt.ret_token.line_num},
                    "Return value does not match current return type");
  }
  return nullptr;
}

llvm::Value* SemanticAnalyzer::Visit(VarDeclarationStmt& stmt) {
  if (env_.IsDeclaredInCurrentScope(stmt.name.lexeme)) {
    std::string error = "Variable already declared: " + stmt.name.lexeme;
    diagnose_.Error({stmt.name.line_num}, error);
    return nullptr;
  }

  types::Type* declared_type = ResolveType(stmt.type);
  Resolve(*stmt.value);

  if (stmt.value->type->kind != declared_type->kind) {
    std::string error =
        "Type mismatch in variable declaration: " + stmt.name.lexeme;
    diagnose_.Error({stmt.name.line_num}, error);
    return nullptr;
  }

  stmt.value->type = declared_type;
  std::optional<SymbolId> id =
      Declare(stmt.name.lexeme, declared_type, false, {stmt.name.line_num});
  if (id.has_value()) {
    stmt.id = id.value();
  }
  return nullptr;
}

llvm::Value* SemanticAnalyzer::Visit(Variable& expr) {
  auto* sym = LookupSymbol(expr.name.lexeme);
  if (!sym) {
    std::string err = "Undeclared variable: " + expr.name.lexeme;
    diagnose_.Error({expr.name.line_num}, err);
    return nullptr;
  }
  expr.type = sym->type;
  expr.id = sym->id;
  return nullptr;
}

llvm::Value* SemanticAnalyzer::Visit(Binary& expr) {
  Resolve(*expr.left);
  Resolve(*expr.right);

  if (expr.left->type->kind != expr.right->type->kind) {
    std::string err = "Type mismatch: " + expr.op.lexeme;
    diagnose_.Error({expr.op.line_num}, err);
    return nullptr;
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
      expr.type = types_.Bool();
      break;
    default:
      UNREACHABLE(VisitBinary, "Unknown operation: " + expr.op.lexeme);
  }

  return nullptr;
}

llvm::Value* SemanticAnalyzer::Visit(Assign& expr) {
  Resolve(*expr.value);
  auto* sym = LookupSymbol(expr.name.lexeme);
  if (!sym) {
    std::string err = "Assignment to undelcared variable: " + expr.name.lexeme;
    diagnose_.Error({expr.name.line_num}, err);
    return nullptr;
  }

  if (sym->type->kind != expr.value->type->kind) {
    std::string err = "Type mismatch in assignment: " + expr.name.lexeme;
    diagnose_.Error({expr.name.line_num}, err);
    return nullptr;
  }

  expr.type = sym->type;
  expr.id = sym->id;
  return nullptr;
}

llvm::Value* SemanticAnalyzer::Visit(PreFixOp& expr) {
  auto* sym = LookupSymbol(expr.name.lexeme);
  if (!sym) {
    std::string err = "Variable is not defined: " + expr.name.lexeme;
    diagnose_.Error({expr.op.line_num}, err);
    return nullptr;
  }

  if (sym->type->kind != types::TypeKind::Int &&
      sym->type->kind != types::TypeKind::Float) {
    std::string err =
        "Prefix operator requires numeric operand: " + expr.name.lexeme;
    return nullptr;
  }

  expr.type = sym->type;
  expr.id = sym->id;
  return nullptr;
}

llvm::Value* SemanticAnalyzer::Visit(Conditional& expr) {
  Resolve(*expr.left);
  Resolve(*expr.right);
  if (expr.left->type->kind != expr.right->type->kind) {
    std::string err = "Type mismatch: " + expr.op.lexeme;
    diagnose_.Error({expr.op.line_num}, err);
    return nullptr;
  }
  expr.type = types_.Bool();
  return nullptr;
}

llvm::Value* SemanticAnalyzer::Visit(Grouping& expr) {
  Resolve(*expr.expr);
  return nullptr;
}

llvm::Value* SemanticAnalyzer::Visit(CallExpr& expr) {
  Variable* callee = dynamic_cast<Variable*>(expr.callee.get());
  if (!callee) {
    diagnose_.Error({}, "Object is not a callable");
    return nullptr;
  }

  SymbolInfo* symbol = LookupSymbol(callee->name.lexeme);

  if (!symbol) {
    std::string err = "Undefined functions: " + callee->name.lexeme;
    diagnose_.Error({callee->name.line_num}, err);
    return nullptr;
  }

  callee->id = symbol->id;
  callee->type = symbol->type;

  if (!symbol->is_function) {
    std::string err = "Symbol is not callable: " + callee->name.lexeme;
    diagnose_.Error({callee->name.line_num}, err);
    return nullptr;
  }

  auto* func_type = dynamic_cast<types::FunctionType*>(symbol->type);
  if (!func_type) {
    std::string err = "Symbol is not callable: " + callee->name.lexeme;
    diagnose_.Error({callee->name.line_num}, err);
    return nullptr;
  }

  size_t num_params = func_type->params.size();
  size_t num_args = expr.args.size();

  if (func_type->is_variadic) {
    if (num_args < num_params) {
      std::string err =
          "Too few arguments for variadic function: " + callee->name.lexeme;
      diagnose_.Error({callee->name.line_num}, err);
      return nullptr;
    }
  } else if (num_args != num_params) {
    std::string err = "Argument count mismatch for: " + callee->name.lexeme;
    diagnose_.Error({callee->name.line_num}, err);
    return nullptr;
  }

  /// TODO: make sure this works, im not entirely sure if it will or not
  for (size_t i = 0; i < num_args; i++) {
    Resolve(*expr.args[i]);
    if (i < num_params) {
      if (expr.args[i]->type->kind != func_type->params[i]->kind) {
        diagnose_.Error({callee->name.line_num},
                        "Type mismatch in fixed argument");
        return nullptr;
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
    expr.type = types_.Int32();
  } else if (std::holds_alternative<float>(expr.value)) {
    expr.type = types_.Float32();
  } else if (std::holds_alternative<std::string>(expr.value)) {
    expr.type = types_.String();
  } else if (std::holds_alternative<bool>(expr.value)) {
    expr.type = types_.Bool();
  } else {
    UNREACHABLE(VisitLiteral, "Unknown value type");
  }
  return nullptr;
}

types::Type* SemanticAnalyzer::ResolveArgType(Token type) {
  switch (type.type) {
    case TT_INT32_SPECIFIER:
      return types_.Int32();
    case TT_FLT_SPECIFIER:
      return types_.Float32();
    case TT_BOOL_SPECIFIER:
      return types_.Bool();
    case TT_STR_SPECIFIER:
      return types_.String();
    case TT_INT64_SPECIFIER:
    // Not valid in args
    case TT_VOID_SPECIFIER:
    default:
      diagnose_.Error({type.line_num}, "Invalid type");
  }
  return nullptr;
}

types::Type* SemanticAnalyzer::ResolveType(Token type) {
  switch (type.type) {
    case TT_INT32_SPECIFIER:
      return types_.Int32();
    case TT_FLT_SPECIFIER:
      return types_.Float32();
    case TT_VOID_SPECIFIER:
      return types_.Void();
    case TT_STR_SPECIFIER:
      return types_.String();
    case TT_BOOL_SPECIFIER:
      return types_.Bool();
    case TT_INT64_SPECIFIER:
    default:
      diagnose_.Error({type.line_num}, "Invalid type: " + type.lexeme);
  }
  return nullptr;
}

llvm::Value* SemanticAnalyzer::Resolve(Stmt& stmt) {
  return stmt.Accept(*this);
}

llvm::Value* SemanticAnalyzer::Resolve(Expr& expr) {
  return expr.Accept(*this);
}

SymbolInfo* SemanticAnalyzer::LookupSymbol(const std::string& name) {
  SymbolId* id = env_.Lookup(name);
  if (!id) {
    return nullptr;
  }

  return symbols_.GetSymbolInfo(*id);
}

void SemanticAnalyzer::BeginScope() {
  env_.PushScope();
}

void SemanticAnalyzer::EndScope() {
  env_.PopScope();
}

std::optional<SymbolId> SemanticAnalyzer::Declare(std::string name,
                                                  types::Type* type,
                                                  bool is_function,
                                                  SourceLoc loc) {
  if (env_.IsDeclaredInCurrentScope(name)) {
    diagnose_.Error(loc, "Redefinition of symbol: " + name);
    return std::nullopt;
  }

  SymbolId id = symbols_.Declare(name, type, is_function);
  env_.DeclareLocal(name, id);
  return id;
}

void SemanticAnalyzer::Analyze(ModuleStmt& mod) {
  Resolve(mod);
}

void SemanticAnalyzer::VariadicPromotion(Expr* expr) {
  switch (expr->type->kind) {
    case types::TypeKind::Float:
      expr->type = types_.Float32();
      break;
    case types::TypeKind::Int:
      expr->type = types_.Int32();
      break;
    case types::TypeKind::Bool:
      expr->type = types_.Int32();
      break;
    case types::TypeKind::Function:
    case types::TypeKind::String:
    case types::TypeKind::Struct:
    case types::TypeKind::Void:
    default:
      break;
  }
}

bool SemanticAnalyzer::HadError() {
  return diagnose_.HasErrors();
}

void SemanticAnalyzer::DumpErrors() {
  if (HadError()) {
    diagnose_.DumpErrors();
  }
}

static std::string SymbolTypeToString(types::Type& type) {
  switch (type.kind) {
    case types::TypeKind::Bool:
      return "Bool";
    case types::TypeKind::Float:
      return "Float";
    case types::TypeKind::Int:
      return "Int";
    case types::TypeKind::String:
      return "String";
    case types::TypeKind::Struct:
      return "Struct";
    case types::TypeKind::Void:
      return "Void";
    case types::TypeKind::Function:
      return "Function";
    default:
      return "Not matched to type";
  }
}

void SemanticAnalyzer::DebugSymbols() {
  std::vector<SymbolInfo> symbol_table = symbols_.GetSymbolTable();
  SourceLoc loc{0};
  for (auto& s : symbol_table) {
    std::string symbol = "Resolved: " + s.name;
    symbol += "\nType: " + SymbolTypeToString(*s.type);
    symbol += "\nSymbol: " + std::to_string(s.id) + " ";
    symbol += "\nIs Func: ";
    symbol += (s.is_function ? "true" : "false");
    diagnose_.Debug(loc, symbol);
  }
}
