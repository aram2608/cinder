#include "../include/compiler.hpp"

using namespace llvm;
static std::unique_ptr<LLVMContext> TheContext;
static std::unique_ptr<Module> TheModule;
static std::unique_ptr<IRBuilder<>> Builder;
static std::unordered_map<std::string, Value*> NamedValues;

Compiler::Compiler(std::vector<std::unique_ptr<Stmt>> statements)
    : statements(std::move(statements)), symbol_table{} {}

void Compiler::Compile() {
  TheContext = std::make_unique<LLVMContext>();
  TheModule = std::make_unique<Module>("no-name-lang", *TheContext);
  Builder = std::make_unique<IRBuilder<>>(*TheContext);

  for (auto& stmt : statements) {
    stmt->Accept(*this);
  }

  TheModule->print(outs(), nullptr);
  fprintf(stdin, "\n");
}

Value* Compiler::VisitExpressionStmt(ExpressionStmt& stmt) {
  stmt.expr->Accept(*this);
  return nullptr;
}

Value* Compiler::VisitFunctionStmt(FunctionStmt& stmt) {
  Type* return_type;
  if (stmt.return_type.token_type == TT_INT_SPECIFIER) {
    return_type = Type::getInt32Ty(*TheContext);
  } else if (stmt.return_type.token_type == TT_FLT_SPECIFIER) {
    return_type = Type::getDoubleTy(*TheContext);
  } else {
    return_type = Type::getVoidTy(*TheContext);
  }

  std::vector<Type*> arg_types;
  for (Token& arg : stmt.args) {
    if (arg.value_type == VT_INT) {
      arg_types.push_back(Type::getInt32Ty(*TheContext));
    } else if (arg.value_type == VT_FLT) {
      arg_types.push_back(Type::getDoubleTy(*TheContext));
    } else if (arg.value_type == VT_STR) {
      printf("Implement me\n");
      exit(1);
    } else {
      printf("UNREACHABLE\n");
      exit(1);
    }
  }

  FunctionType* FT = FunctionType::get(return_type, arg_types, false);
  Function* Func = Function::Create(FT, Function::ExternalLinkage,
                                    stmt.name.lexeme, *TheModule);

  BasicBlock* BB = BasicBlock::Create(*TheContext, "entry", Func);
  Builder->SetInsertPoint(BB);

  for (auto& stmt : stmt.body) {
    stmt->Accept(*this);
  }

  Builder->CreateRet(ConstantInt::get(*TheContext, APInt(32, 0)));
  verifyFunction(*Func);
  return Func;
}

Value* Compiler::VisitReturnStmt(ReturnStmt& stmt) {
  Value* ret = stmt.value->Accept(*this);
  return Builder->CreateRet(ret);
}

Value* Compiler::VisitVarDeclarationStmt(VarDeclarationStmt& stmt) {
  Value* value = stmt.value->Accept(*this);
  symbol_table[stmt.name.lexeme] = value;
  return nullptr;
}

Value* Compiler::VisitLiteral(Literal& expr) {
  switch (expr.value_type) {
    case VT_INT:
      return ConstantInt::get(*TheContext,
                              APInt(32, std::get<int>(expr.value)));
    case VT_FLT:
      return ConstantFP::get(*TheContext, APFloat(std::get<float>(expr.value)));
    case VT_STR:
      return ConstantDataArray::getString(*TheContext,
                                          std::get<std::string>(expr.value));
    default:
      printf("UNREACHABLE");
      exit(1);
  }
}

Value* Compiler::VisitBoolean(Boolean& expr) {
  if (expr.boolean) {
    return ConstantInt::getTrue(*TheContext);
  }
  return ConstantInt::getFalse(*TheContext);
}

Value* Compiler::VisitVariable(Variable& expr) {
  Value* value = NamedValues[expr.name];
  if (!value) {
    printf("Variables is undefined %s\n", expr.name.c_str());
    exit(1);
  }
  return value;
}

Value* Compiler::VisitGrouping(Grouping& expr) {
  return expr.expr->Accept(*this);
}

Value* Compiler::VisitBinary(Binary& expr) {
  Value* left = expr.left->Accept(*this);
  Value* right = expr.right->Accept(*this);

  switch (expr.op.token_type) {
    case TT_PLUS:
      return Builder->CreateFAdd(left, right, "addtmp");
    case TT_MINUS:
      return Builder->CreateFSub(left, right, "subtmp");
    case TT_STAR:
      return Builder->CreateFMul(left, right, "multmp");
    case TT_SLASH:
      return Builder->CreateFDiv(left, right, "divtmp");
    default:
      printf("UNREACHABLE\n");
      exit(1);
  }
  return nullptr;
}