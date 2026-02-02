#include "../include/semantic_analyzer.hpp"

/// TODO: Better error handling

llvm::Value* SemanticAnalyzer::Visit(Literal& expr) {
  if (std::holds_alternative<int>(expr.value))
    expr.type = types.Int32();
  else if (std::holds_alternative<float>(expr.value))
    expr.type = types.Float32();
  else
    UNREACHABLE(VisitLiteral, "Unknown value type");
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
    // Error(expr.name, "Undeclared variable");
    std::cout << "Undeclared variable: " << expr.name.lexeme << "\n";
    exit(1);
  }
  expr.type = sym->type;
  return nullptr;
}

llvm::Value* SemanticAnalyzer::Visit(Binary& expr) {
  expr.left->Accept(*this);
  expr.right->Accept(*this);

  if (expr.left->type != expr.right->type) {
    std::cout << "Type mismatch: " << expr.op.lexeme << "\n";
    exit(1);
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
      expr.type = types.Int32();
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
    std::cout << "Assignment to undeclared variable: " << expr.name.lexeme
              << "\n";
    exit(1);
  }

  if (sym->type != expr.value->type) {
    std::cout << "Type mismatch in assignment: " << expr.name.lexeme << "\n";
    exit(1);
  }

  expr.type = sym->type;
  return nullptr;
}

llvm::Value* SemanticAnalyzer::Visit(CallExpr& expr) {
  for (auto& arg : expr.args) arg->Accept(*this);

  /// TODO: Figures out a type for this
  expr.type = types.Int32();
  return nullptr;
}
