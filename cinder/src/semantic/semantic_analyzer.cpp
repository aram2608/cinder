#include "cinder/semantic/semantic_analyzer.hpp"

#include <unordered_set>

#include "cinder/ast/stmt/stmt.hpp"
#include "cinder/support/utils.hpp"

using namespace cinder;

/// TODO: Add a control flow analysis check to make sure that there is a return
/// for every path possible in non-void functions

SemanticAnalyzer::SemanticAnalyzer(TypeContext& types)
    : types_(types), current_return(nullptr) {}

void SemanticAnalyzer::Visit(ModuleStmt& stmt) {
  BeginScope();
  for (auto& s : stmt.stmts) {
    Resolve(*s);
  }
  EndScope();
}

void SemanticAnalyzer::Visit(ImportStmt& stmt) {
  return;
}

void SemanticAnalyzer::Visit(StructStmt& stmt) {
  std::vector<std::string> field_names;
  std::vector<types::Type*> field_types;
  std::unordered_set<std::string> seen;

  field_names.reserve(stmt.fields.size());
  field_types.reserve(stmt.fields.size());

  for (auto& field : stmt.fields) {
    if (seen.contains(field.identifier.lexeme)) {
      diagnose_.Error({field.identifier.location.line},
                      "Duplicate struct field: " + field.identifier.lexeme);
      return;
    }
    seen.insert(field.identifier.lexeme);

    types::Type* ty = ResolveType(field.type_token);
    if (!ty || ty->Void() || ty->Function()) {
      diagnose_.Error({field.identifier.location.line},
                      "Invalid struct field type: " + field.type_token.lexeme);
      return;
    }

    field.resolved_type = ty;
    field_names.push_back(field.identifier.lexeme);
    field_types.push_back(ty);
  }

  std::string qualified_name =
      current_mod_.empty() ? stmt.name.lexeme
                           : QualifiedName(current_mod_, stmt.name.lexeme);
  types::StructType* struct_ty =
      types_.Struct(qualified_name, field_names, field_types);

  std::optional<SymbolId> id =
      Declare(qualified_name, struct_ty, false, {stmt.name.location.line});
  if (!id.has_value()) {
    diagnose_.Error({stmt.name.location.line},
                    "Struct could not be declared: " + stmt.name.lexeme);
    return;
  }
  stmt.id = id.value();
}

void SemanticAnalyzer::Visit(FunctionProto& stmt) {
  types::Type* ret = ResolveType(stmt.return_type);

  std::vector<types::Type*> params;
  for (auto& arg : stmt.args) {
    types::Type* arg_type = ResolveArgType(arg.type_token);
    arg.resolved_type = arg_type;
    params.push_back(arg_type);
  }

  std::string declared_name = stmt.name.lexeme;
  if (!stmt.is_extern && !current_mod_.empty()) {
    declared_name = QualifiedName(current_mod_, stmt.name.lexeme);
  }

  std::optional<SymbolId> id =
      Declare(declared_name, types_.Function(ret, params, stmt.is_variadic),
              true, {stmt.name.location.line});
  if (id.has_value()) {
    stmt.id = id.value();
  } else {
    std::string err = "Function could not be declared: " + stmt.name.lexeme;
    diagnose_.Error({stmt.name.location.line}, err);
  }
}

void SemanticAnalyzer::Visit(FunctionStmt& stmt) {
  std::error_code ec;
  FunctionProto* proto = stmt.proto->CastTo<FunctionProto>(ec);

  if (ec) {
    diagnose_.Error({0}, ec.message());
    return;
  }

  current_return = ResolveType(proto->return_type);

  BeginScope();

  for (auto& arg : proto->args) {
    types::Type* arg_type = ResolveArgType(arg.type_token);
    Declare(arg.identifier.lexeme, arg_type, false,
            {arg.identifier.location.line});
  }

  for (auto& s : stmt.body) {
    Resolve(*s);
  }

  EndScope();
}

void SemanticAnalyzer::Visit(ForStmt& stmt) {
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
}

void SemanticAnalyzer::Visit(WhileStmt& stmt) {
  Resolve(*stmt.condition);
  for (auto& s : stmt.body) {
    Resolve(*s);
  }
}

void SemanticAnalyzer::Visit(IfStmt& stmt) {
  Resolve(*stmt.cond);
  Resolve(*stmt.then);
  if (stmt.otherwise) {
    Resolve(*stmt.otherwise);
  }
}

void SemanticAnalyzer::Visit(ExpressionStmt& stmt) {
  Resolve(*stmt.expr);
}

void SemanticAnalyzer::Visit(ReturnStmt& stmt) {
  if (!current_return) {
    diagnose_.Error({stmt.ret_token.location.line},
                    "Return statement outside function body");
    return;
  }

  if (!stmt.value) {
    if (!current_return->IsThisType(types::TypeKind::Void)) {
      diagnose_.Error({stmt.ret_token.location.line},
                      "Return value does not match current return type");
    }
    return;
  }

  Resolve(*stmt.value);
  if (!stmt.value->type) {
    return;
  }
  if (!stmt.value->type->IsThisType(current_return)) {
    diagnose_.Error({stmt.ret_token.location.line},
                    "Return value does not match current return type");
    return;
  }
}

void SemanticAnalyzer::Visit(VarDeclarationStmt& stmt) {
  if (env_.IsDeclaredInCurrentScope(stmt.name.lexeme)) {
    std::string error = "Variable already declared: " + stmt.name.lexeme;
    diagnose_.Error({stmt.name.location.line}, error);
    return;
  }

  types::Type* declared_type = ResolveType(stmt.type);
  Resolve(*stmt.value);

  if (!declared_type || !stmt.value->type) {
    return;
  }

  if (!stmt.value->type->IsThisType(declared_type)) {
    std::string error =
        "Type mismatch in variable declaration: " + stmt.name.lexeme;
    diagnose_.Error({stmt.name.location.line}, error);
    return;
  }

  stmt.value->type = declared_type;
  std::optional<SymbolId> id = Declare(stmt.name.lexeme, declared_type, false,
                                       {stmt.name.location.line});
  if (id.has_value()) {
    stmt.id = id.value();
  }
}

void SemanticAnalyzer::Visit(Variable& expr) {
  auto* sym = LookupSymbol(expr.name.lexeme);
  if (!sym) {
    sym = LookupInCurrentModule(expr.name.lexeme);
  }
  if (!sym) {
    std::string err = "Undeclared variable: " + expr.name.lexeme;
    diagnose_.Error({expr.name.location.line}, err);
    return;
  }
  expr.type = sym->type;
  expr.id = sym->id;
}

void SemanticAnalyzer::Visit(MemberAccess& expr) {
  auto* base = dynamic_cast<Variable*>(expr.object.get());
  if (!base) {
    diagnose_.Error({expr.member.location.line},
                    "Unsupported member access base expression");
    return;
  }

  SymbolInfo* base_sym = LookupSymbol(base->name.lexeme);
  if (!base_sym) {
    base_sym = LookupInCurrentModule(base->name.lexeme);
  }

  if (base_sym && base_sym->type && base_sym->type->Struct()) {
    base->id = base_sym->id;
    base->type = base_sym->type;
    auto struct_type = base_sym->type->CastTo<types::StructType>();
    if (std::error_code ec = struct_type.getError()) {
      diagnose_.Error({expr.member.location.line},
                      "Invalid struct member access base");
      return;
    }

    int idx = struct_type.get()->FieldIndex(expr.member.lexeme);
    if (idx < 0) {
      diagnose_.Error({expr.member.location.line},
                      "Unknown field: " + expr.member.lexeme);
      return;
    }

    expr.field_index = static_cast<size_t>(idx);
    expr.type = struct_type.get()->fields[idx];
    return;
  }

  SymbolInfo* symbol =
      LookupSymbol(QualifiedName(base->name.lexeme, expr.member.lexeme));
  if (!symbol) {
    diagnose_.Error(
        {expr.member.location.line},
        "Undefined member: " + base->name.lexeme + "." + expr.member.lexeme);
    return;
  }

  expr.id = symbol->id;
  expr.type = symbol->type;
}

void SemanticAnalyzer::Visit(Binary& expr) {
  Resolve(*expr.left);
  Resolve(*expr.right);

  if (!expr.left->type || !expr.right->type) {
    return;
  }

  if (!expr.left->type->IsThisType(expr.right->type)) {
    std::string err = "Type mismatch: " + expr.op.lexeme;
    diagnose_.Error({expr.op.location.line}, err);
    return;
  }

  switch (expr.op.kind) {
    case Token::Type::Plus:
    case Token::Type::Minus:
    case Token::Type::STAR:
    case Token::Type::SLASH:
      expr.type = expr.left->type;
      break;
    case Token::Type::EQEQ:
    case Token::Type::BANGEQ:
      expr.type = types_.Bool();
      break;
    default:
      UNREACHABLE(VisitBinary, "Unknown operation: " + expr.op.lexeme);
  }
}

void SemanticAnalyzer::Visit(Assign& expr) {
  Resolve(*expr.value);
  if (!expr.value->type) {
    return;
  }
  auto* sym = LookupSymbol(expr.name.lexeme);
  if (!sym) {
    std::string err = "Assignment to undelcared variable: " + expr.name.lexeme;
    diagnose_.Error({expr.name.location.line}, err);
    return;
  }

  if (!sym->type || !sym->type->IsThisType(expr.value->type)) {
    std::string err = "Type mismatch in assignment: " + expr.name.lexeme;
    diagnose_.Error({expr.name.location.line}, err);
    return;
  }

  expr.type = sym->type;
  expr.id = sym->id;
}

void SemanticAnalyzer::Visit(MemberAssign& expr) {
  Resolve(*expr.target);
  Resolve(*expr.value);

  if (!expr.target->type) {
    diagnose_.Error({expr.target->member.location.line},
                    "Invalid member assignment target");
    return;
  }

  if (!expr.target->type->IsThisType(expr.value->type)) {
    diagnose_.Error({expr.target->member.location.line},
                    "Type mismatch in member assignment");
    return;
  }

  auto* base = dynamic_cast<Variable*>(expr.target->object.get());
  if (!base || !base->HasID()) {
    diagnose_.Error({expr.target->member.location.line},
                    "Member assignment requires variable base");
    return;
  }

  if (!expr.target->field_index.has_value()) {
    diagnose_.Error({expr.target->member.location.line},
                    "Member assignment target is not a struct field");
    return;
  }

  expr.base_id = base->GetID();
  expr.id = base->id;
  expr.type = expr.target->type;
}

void SemanticAnalyzer::Visit(PreFixOp& expr) {
  auto* sym = LookupSymbol(expr.name.lexeme);
  if (!sym) {
    std::string err = "Variable is not defined: " + expr.name.lexeme;
    diagnose_.Error({expr.op.location.line}, err);
    return;
  }

  if (sym->type->kind != types::TypeKind::Int &&
      sym->type->kind != types::TypeKind::Float) {
    std::string err =
        "Prefix operator requires numeric operand: " + expr.name.lexeme;
    return;
  }

  expr.type = sym->type;
  expr.id = sym->id;
}

void SemanticAnalyzer::Visit(Conditional& expr) {
  Resolve(*expr.left);
  Resolve(*expr.right);
  if (expr.left->type->kind != expr.right->type->kind) {
    std::string err = "Type mismatch: " + expr.op.lexeme;
    diagnose_.Error({expr.op.location.line}, err);
    return;
  }
  expr.type = types_.Bool();
}

void SemanticAnalyzer::Visit(Grouping& expr) {
  Resolve(*expr.expr);
}

void SemanticAnalyzer::Visit(CallExpr& expr) {
  SymbolInfo* symbol = nullptr;
  SourceLoc call_loc{0};
  std::string call_name;
  std::error_code ec;

  if (auto* callee = dynamic_cast<Variable*>(expr.callee.get())) {
    call_loc = {callee->name.location.line};
    call_name = callee->name.lexeme;
    symbol = LookupInCurrentModule(callee->name.lexeme);
    if (!symbol) {
      symbol = LookupSymbol(callee->name.lexeme);
    }
    if (symbol) {
      callee->id = symbol->id;
      callee->type = symbol->type;
    }
  } else if (auto* member = dynamic_cast<MemberAccess*>(expr.callee.get())) {
    auto* base = dynamic_cast<Variable*>(member->object.get());
    if (!base) {
      diagnose_.Error({member->member.location.line},
                      "Unsupported callee expression");
      return;
    }

    call_loc = {member->member.location.line};
    call_name = base->name.lexeme + "." + member->member.lexeme;
    symbol =
        LookupSymbol(QualifiedName(base->name.lexeme, member->member.lexeme));
    if (symbol) {
      member->id = symbol->id;
      member->type = symbol->type;
    }
  } else {
    diagnose_.Error({0}, "Unsupported callee expression");
    return;
  }

  if (!symbol) {
    diagnose_.Error(call_loc, "Undefined function: " + call_name);
    return;
  }

  if (!symbol->type) {
    diagnose_.Error(call_loc,
                    "Callable symbol has unresolved type: " + call_name);
    return;
  }

  if (!symbol->is_function && symbol->type->Struct()) {
    auto* struct_type = symbol->type->CastTo<types::StructType>(ec);
    if (ec) {
      diagnose_.Error(call_loc, "Invalid struct constructor: " + call_name);
      return;
    }

    if (expr.args.size() != struct_type->fields.size()) {
      diagnose_.Error(
          call_loc,
          "Argument count mismatch for struct constructor: " + call_name);
      return;
    }

    for (size_t i = 0; i < expr.args.size(); ++i) {
      Resolve(*expr.args[i]);
      if (!expr.args[i]->type) {
        return;
      }
      if (!expr.args[i]->type->IsThisType(struct_type->fields[i])) {
        diagnose_.Error(call_loc,
                        "Type mismatch in struct constructor argument");
        return;
      }
    }

    expr.id = symbol->id;
    expr.type = struct_type;
    return;
  }

  if (!symbol->is_function) {
    std::string err = "Symbol is not callable: " + call_name;
    diagnose_.Error(call_loc, err);
    return;
  }

  auto* func_type = symbol->type->CastTo<types::FunctionType>(ec);
  if (ec) {
    std::string err = "Symbol is not callable: " + call_name;
    diagnose_.Error(call_loc, err);
    return;
  }

  size_t num_params = func_type->params.size();
  size_t num_args = expr.args.size();

  if (func_type->IsVariadic()) {
    if (num_args < num_params) {
      std::string err = "Too few arguments for variadic function: " + call_name;
      diagnose_.Error(call_loc, err);
      return;
    }
  } else if (num_args != num_params) {
    std::string err = "Argument count mismatch for: " + call_name;
    diagnose_.Error(call_loc, err);
    return;
  }

  for (size_t i = 0; i < num_args; i++) {
    Resolve(*expr.args[i]);
    if (!expr.args[i]->type) {
      return;
    }
    if (i < num_params) {
      if (expr.args[i]->type->kind != func_type->params[i]->kind) {
        diagnose_.Error(call_loc, "Type mismatch in fixed argument");
        return;
      }
    } else {
      // This should come in handy later when types get extended
      VariadicPromotion(expr.args[i].get());
    }
  }

  expr.type = func_type->return_type;
}

/// TODO: Extend literal types
void SemanticAnalyzer::Visit(Literal& expr) {
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
}

types::Type* SemanticAnalyzer::ResolveArgType(Token type) {
  if (type.kind == Token::Type::IDENTIFER) {
    types::StructType* struct_type = types_.LookupStruct(
        current_mod_.empty() ? type.lexeme
                             : QualifiedName(current_mod_, type.lexeme));
    if (!struct_type) {
      struct_type = types_.LookupStruct(type.lexeme);
    }
    if (struct_type) {
      return struct_type;
    }
  }

  switch (type.kind) {
    case Token::Type::INT32_SPECIFIER:
      return types_.Int32();
    case Token::Type::FLT32_SPECIFIER:
      return types_.Float32();
    case Token::Type::FLT64_SPECIFIER:
      return types_.Float64();
    case Token::Type::BOOL_SPECIFIER:
      return types_.Bool();
    case Token::Type::STR_SPECIFIER:
      return types_.String();
    case Token::Type::INT64_SPECIFIER:
    // Not valid in args
    case Token::Type::VOID_SPECIFIER:
    default:
      diagnose_.Error({type.location.line}, "Invalid type");
  }
  return nullptr;
}

types::Type* SemanticAnalyzer::ResolveType(Token type) {
  if (type.kind == Token::Type::IDENTIFER) {
    types::StructType* struct_type = types_.LookupStruct(
        current_mod_.empty() ? type.lexeme
                             : QualifiedName(current_mod_, type.lexeme));
    if (!struct_type) {
      struct_type = types_.LookupStruct(type.lexeme);
    }
    if (struct_type) {
      return struct_type;
    }
  }

  switch (type.kind) {
    case Token::Type::INT32_SPECIFIER:
      return types_.Int32();
    case Token::Type::FLT32_SPECIFIER:
      return types_.Float32();
    case Token::Type::FLT64_SPECIFIER:
      return types_.Float64();
    case Token::Type::VOID_SPECIFIER:
      return types_.Void();
    case Token::Type::STR_SPECIFIER:
      return types_.String();
    case Token::Type::BOOL_SPECIFIER:
      return types_.Bool();
    case Token::Type::INT64_SPECIFIER:
    default:
      diagnose_.Error({type.location.line}, "Invalid type: " + type.lexeme);
  }
  return nullptr;
}

void SemanticAnalyzer::Resolve(Stmt& stmt) {
  stmt.Accept(*this);
}

void SemanticAnalyzer::Resolve(Expr& expr) {
  expr.Accept(*this);
}

SymbolInfo* SemanticAnalyzer::LookupSymbol(const std::string& name) {
  SymbolId* id = env_.Lookup(name);
  if (!id) {
    return nullptr;
  }

  return symbols_.GetSymbolInfo(*id);
}

SymbolInfo* SemanticAnalyzer::LookupInCurrentModule(const std::string& name) {
  if (current_mod_.empty()) {
    return nullptr;
  }
  return LookupSymbol(QualifiedName(current_mod_, name));
}

std::string SemanticAnalyzer::QualifiedName(const std::string& qualifier,
                                            const std::string& name) const {
  return qualifier + "." + name;
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
  AnalyzeProgram({&mod});
}

void SemanticAnalyzer::AnalyzeProgram(const std::vector<ModuleStmt*>& modules) {
  BeginScope();

  for (ModuleStmt* mod : modules) {
    current_mod_ = mod->name.lexeme;
    for (auto& stmt : mod->stmts) {
      if (auto* strct = dynamic_cast<StructStmt*>(stmt.get())) {
        Resolve(*strct);
      }
    }
  }

  for (ModuleStmt* mod : modules) {
    current_mod_ = mod->name.lexeme;
    for (auto& stmt : mod->stmts) {
      if (auto* fn = dynamic_cast<FunctionStmt*>(stmt.get())) {
        Resolve(*fn->proto);
      } else if (auto* proto = dynamic_cast<FunctionProto*>(stmt.get())) {
        Resolve(*proto);
      }
    }
  }

  for (ModuleStmt* mod : modules) {
    current_mod_ = mod->name.lexeme;
    for (auto& stmt : mod->stmts) {
      if (stmt->IsImport() || stmt->IsFunctionP() || stmt->IsStruct()) {
        continue;
      }
      Resolve(*stmt);
    }
  }

  EndScope();
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
