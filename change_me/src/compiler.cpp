#include "../include/compiler.hpp"
#include "compiler.hpp"

using namespace llvm;
static std::unique_ptr<LLVMContext> TheContext;
static std::unique_ptr<Module> TheModule;
static std::unique_ptr<IRBuilder<>> Builder;
static std::unordered_map<std::string, Value*> NamedValues;
#define UNREACHABLE               \
  do {                            \
    std::cout << "UNREACHABLE\n"; \
    exit(1);                      \
  } while (0)

Compiler::Compiler(std::unique_ptr<Stmt> mod)
    : mod(std::move(mod)), symbol_table{} {}

void Compiler::Compile() {
  TheContext = std::make_unique<LLVMContext>();
  mod->Accept(*this);
}

Value* Compiler::VisitModuleStmt(ModuleStmt& stmt) {
  TheModule = std::make_unique<Module>(stmt.name.lexeme, *TheContext);
  Builder = std::make_unique<IRBuilder<>>(*TheContext);

  for (auto& stmt : stmt.stmts) {
    stmt->Accept(*this);
  }

  std::error_code EC;
  StringRef name = TheModule->getModuleIdentifier() + ".ll";
  raw_fd_ostream OS(name, EC, sys::fs::OF_None);
  TheModule->print(OS, nullptr);
  return nullptr;
}

Value* Compiler::VisitExpressionStmt(ExpressionStmt& stmt) {
  stmt.expr->Accept(*this);
  return nullptr;
}

Value* Compiler::VisitFunctionStmt(FunctionStmt& stmt) {
  Function* Func = dyn_cast<Function>(stmt.proto->Accept(*this));

  BasicBlock* BB = BasicBlock::Create(*TheContext, "entry", Func);
  Builder->SetInsertPoint(BB);

  for (auto& stmt : stmt.body) {
    stmt->Accept(*this);
  }

  verifyFunction(*Func);
  symbol_table.clear();
  return Func;
}

Value* Compiler::VisitFunctionProto(FunctionProto& stmt) {
  std::string func_name = stmt.name.lexeme;
  auto it = std::find(func_table.begin(), func_table.end(), func_name);
  if (it != func_table.end()) {
    std::cout << "functions can not be redefined\n";
    exit(1);
  }

  Type* return_type;
  if (stmt.return_type.token_type == TT_INT_SPECIFIER) {
    return_type = Type::getInt32Ty(*TheContext);
  } else if (stmt.return_type.token_type == TT_FLT_SPECIFIER) {
    return_type = Type::getDoubleTy(*TheContext);
  } else if (stmt.return_type.token_type == TT_VOID_SPECIFIER) {
    return_type = Type::getVoidTy(*TheContext);
  } else {
    UNREACHABLE;
  }

  std::vector<Type*> arg_types;
  for (auto& arg : stmt.args) {
    if (arg.type == VT_INT) {
      arg_types.push_back(Type::getInt32Ty(*TheContext));
    } else if (arg.type == VT_FLT) {
      arg_types.push_back(Type::getFloatTy(*TheContext));
    } else if (arg.type == VT_STR) {
      std::cout << "Implement me compiler.cpp line 47\n";
      exit(1);
    } else {
      UNREACHABLE;
    }
  }

  FunctionType* FT = FunctionType::get(return_type, arg_types, false);
  Function* Func =
      Function::Create(FT, Function::ExternalLinkage, func_name, *TheModule);

  argument_table.clear();
  unsigned Idx = 0;
  for (auto& arg : Func->args()) {
    arg.setName(stmt.args[Idx].identifier.lexeme);
    argument_table[stmt.args[Idx++].identifier.lexeme] = &arg;
  }

  func_table.push_back(func_name);
  return Func;
}

Value* Compiler::VisitReturnStmt(ReturnStmt& stmt) {
  Value* ret = stmt.value->Accept(*this);
  if (!ret) {
    return Builder->CreateRetVoid();
  }
  return Builder->CreateRet(ret);
}

Value* Compiler::VisitVarDeclarationStmt(VarDeclarationStmt& stmt) {
  std::string name = stmt.name.lexeme;
  if (symbol_table.find(name) != symbol_table.end()) {
    std::cout << "redeclaration of variable" << name << "\n";
    exit(1);
  }
  Type* type;
  switch (stmt.type.token_type) {
    case TT_INT_SPECIFIER:
      type = Type::getInt32Ty(*TheContext);
      break;
    case TT_FLT_SPECIFIER:
      type = Type::getFloatTy(*TheContext);
      break;
    case TT_STR_SPECIFIER:
      printf("NOT SUPPORTED YET, compiler.cpp line 86\n");
      exit(1);
      break;
    default:
      UNREACHABLE;
  }
  Value* value = stmt.value->Accept(*this);
  AllocaInst* value_ptr = Builder->CreateAlloca(type, nullptr, name);
  Builder->CreateStore(value, value_ptr);
  symbol_table[name] = value_ptr;
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
    case VT_VOID:
      return nullptr;  // This is a horrible hack for void return types
                       // I need to think of a better solution
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
  std::string lex = expr.name.lexeme;
  auto it = std::find(func_table.begin(), func_table.end(), lex);
  if (it != func_table.end()) {
    return TheModule->getFunction(lex);
  }

  auto arg = argument_table.find(lex);
  if (arg != argument_table.end()) {
    return arg->second;
  }

  auto temp = symbol_table.find(lex);
  if (temp == symbol_table.end()) {
    printf("Variables is undefined %s\n", lex.c_str());
    exit(1);
  }

  AllocaInst* var_ptr = temp->second;
  Type* type = var_ptr->getAllocatedType();
  Value* value = Builder->CreateLoad(type, var_ptr, lex);
  return value;
}

Value* Compiler::VisitGrouping(Grouping& expr) {
  return expr.expr->Accept(*this);
}

Value* Compiler::VisitBinary(Binary& expr) {
  Value* left = expr.left->Accept(*this);
  Value* right = expr.right->Accept(*this);

  Type* l_type = left->getType();
  Type* r_type = right->getType();

  if (l_type->isIntegerTy() && r_type->isIntegerTy()) {
    switch (expr.op.token_type) {
      case TT_PLUS:
        return Builder->CreateAdd(left, right, "addtmp");
      case TT_MINUS:
        return Builder->CreateSub(left, right, "subtmp");
      case TT_STAR:
        return Builder->CreateMul(left, right, "multmp");
      case TT_SLASH:
        return Builder->CreateSDiv(left, right, "divtmp");
      default:
        printf("UNREACHABLE\n");
        exit(1);
    }
  } else if (l_type->isFloatTy() && r_type->isFloatTy()) {
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
  } else {
    l_type->print(errs(), true);
    r_type->print(errs(), true);
    printf("incompatible types\n");
    exit(1);
  }
  return nullptr;
}

Value* Compiler::VisitCall(CallExpr& expr) {
  Function* callee = dyn_cast<Function>(expr.callee->Accept(*this));
  if (!callee) {
    printf("callee is not a function\n");
    exit(1);
  }

  std::vector<Value*> call_args;
  for (auto& arg : expr.args) {
    call_args.push_back(arg->Accept(*this));
  }

  Type* ret_type = callee->getReturnType();
  if (ret_type->isVoidTy()) {
    return Builder->CreateCall(callee, call_args);
  }

  return Builder->CreateCall(callee, call_args, callee->getName());
}