#include "../include/compiler.hpp"

using namespace llvm;
static std::unique_ptr<LLVMContext> TheContext;
static std::unique_ptr<Module> TheModule;
static std::unique_ptr<IRBuilder<>> Builder;
static std::unordered_map<std::string, Value*> NamedValues;

Compiler::Compiler(std::vector<std::unique_ptr<Expr>> expressions)
    : expressions(std::move(expressions)) {}

void Compiler::Compile() {}

void Compiler::CompileExpression() {
  TheContext = std::make_unique<LLVMContext>();
  TheModule = std::make_unique<Module>("no-name-lang", *TheContext);
  Builder = std::make_unique<IRBuilder<>>(*TheContext);

  // Main func
  FunctionType* FT = FunctionType::get(Type::getInt32Ty(*TheContext), false);
  Function* MainFunc =
      Function::Create(FT, Function::ExternalLinkage, "main", *TheModule);

  // Entry point
  BasicBlock* BB = BasicBlock::Create(*TheContext, "entry", MainFunc);
  Builder->SetInsertPoint(BB);

  for (auto& expr : expressions) {
    expr->Accept(*this);
  }

  Builder->CreateRet(ConstantInt::get(*TheContext, APInt(32, 0)));
  verifyFunction(*MainFunc);
  TheModule->print(outs(), nullptr);
  fprintf(stdin, "\n");
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
  return nullptr;
}

Value* Compiler::VisitBoolean(Boolean& expr) {
  printf("Visiting Boolean\n");
  return nullptr;
}

Value* Compiler::VisitVariable(Variable& expr) {
  printf("Visiting Variable\n");
  return nullptr;
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