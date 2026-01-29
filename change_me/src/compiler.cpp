#include "../include/compiler.hpp"
#include "../include/JIT.hpp"

using namespace llvm;
using namespace llvm::orc;

static std::unique_ptr<LLVMContext> TheContext;
static std::unique_ptr<Module> TheModule;
static std::unique_ptr<IRBuilder<>> Builder;
static std::unique_ptr<JIT> TheJIT;
static std::unique_ptr<FunctionPassManager>
    TheFPM; /**< Function pass optimizer */
static std::unique_ptr<LoopAnalysisManager> TheLAM; /** Loop optimizer */
static std::unique_ptr<FunctionAnalysisManager> TheFAM;
static std::unique_ptr<CGSCCAnalysisManager> TheCGAM;
static std::unique_ptr<ModuleAnalysisManager> TheMAM;
static std::unique_ptr<PassInstrumentationCallbacks> ThePIC;
static std::unique_ptr<StandardInstrumentations> TheSI;
static std::unordered_map<std::string, Value*> NamedValues;
static ExitOnError ExitOnErr;

#define UNREACHABLE(x, y)                                     \
  do {                                                        \
    std::cout << "UNREACHABLE " << #x << " " << #y << "\n";   \
    std::cout << "at" << __FILE__ << " " << __LINE__ << "\n"; \
    exit(1);                                                  \
  } while (0)

/// TODO: Declare the printf method early on to make it visible to all mods
/// This will allow us to see output prior to getting a stdlib set up

/// TODO: Set up global string constants so they can be used in printf as well
/// as variables/function arguments

Compiler::Compiler(std::unique_ptr<Stmt> mod, CompilerOptions opts)
    : mod(std::move(mod)),
      symbol_table{},
      opts(opts),
      /**
       * TODO: Find a way to have the context stored as a member var
       * For now the module outlives the context leading a seg fault during
       * cleanup At least thats what I think is happening since I don't have
       * debug info for LLVM
       */
      context(std::make_unique<LLVMContext>()) {}

bool Compiler::Compile() {
  InitializeAllTargetInfos();
  InitializeAllTargets();
  InitializeAllTargetMCs();
  InitializeAllAsmParsers();
  InitializeAllAsmPrinters();

  GenerateIR();
  auto TargetTriple = sys::getDefaultTargetTriple();
  TheModule->setTargetTriple(Triple(TargetTriple));

  std::string Error;
  auto Target =
      TargetRegistry::lookupTarget(TheModule->getTargetTriple(), Error);

  if (!Target) {
    std::cout << "Failed to create target";
    return true;
  }

  auto CPU = "generic";
  auto Features = "";
  TargetOptions opt;
  auto TheTargetMachine = Target->createTargetMachine(
      Triple(TargetTriple), CPU, Features, opt, Reloc::PIC_);

  switch (opts.mode) {
    case CompilerMode::COMPILE:
      TheModule->setDataLayout(TheTargetMachine->createDataLayout());

      CompileBinary(TheTargetMachine);
      return true;
    case CompilerMode::EMIT_LLVM:
      TheModule->setDataLayout(TheTargetMachine->createDataLayout());
      EmitLLVM();
      return true;
    case CompilerMode::RUN:
      TheModule->setDataLayout(TheJIT->GetDataLayout());
      CompileRun();
      return true;
    default:
      UNREACHABLE(COMPILER_MODE, "Unknown compile type");
  }
}

void Compiler::GenerateIR() {
  TheContext = std::make_unique<LLVMContext>();
  TheJIT = ExitOnErr(JIT::Create());
  mod->Accept(*this);
}

void Compiler::EmitLLVM() {
  std::error_code EC;
  StringRef name{opts.out_path};
  raw_fd_ostream OS(name, EC, sys::fs::OF_None);
  TheModule->print(OS, nullptr);
}

void Compiler::CompileRun() {
  auto RT = TheJIT->GetMainJITDylib().createResourceTracker();
  auto TSM = ThreadSafeModule(std::move(TheModule), std::move(TheContext));
  ExitOnErr(TheJIT->AddModule(std::move(TSM), RT));
  auto ExprSymbol = ExitOnErr(TheJIT->Lookup("main"));
  int (*FP)() = ExprSymbol.toPtr<int (*)()>();
  fprintf(stderr, "Exit code %d\n", FP());
}

void Compiler::CompileBinary(TargetMachine* target_machine) {
  std::error_code EC;
  std::string temp = "." + opts.out_path + ".o";
  StringRef name{temp};
  raw_fd_ostream object_file(name, EC, sys::fs::OF_None);

  legacy::PassManager pass;
  auto FileType = CodeGenFileType::ObjectFile;

  if (target_machine->addPassesToEmitFile(pass, object_file, nullptr,
                                          FileType)) {
    errs() << "TheTargetMachine can't emit a file of this type";
    return;
  }

  pass.run(*TheModule);
  object_file.flush();

  std::string linker_flags{};
  for (auto opt : opts.linker_flags) {
    linker_flags += opt;
  }

  std::string CMD =
      "clang " + temp + " " + linker_flags + " -o " + opts.out_path;
  std::string CLEANUP = "rm " + temp;
  system(CMD.c_str());
  system(CLEANUP.c_str());
}

Value* Compiler::VisitModuleStmt(ModuleStmt& stmt) {
  TheModule = CreateModule(stmt, *TheContext);
  Builder = std::make_unique<IRBuilder<>>(*TheContext);
  TheFPM = std::make_unique<FunctionPassManager>();
  TheLAM = std::make_unique<LoopAnalysisManager>();
  TheFAM = std::make_unique<FunctionAnalysisManager>();
  TheCGAM = std::make_unique<CGSCCAnalysisManager>();
  TheMAM = std::make_unique<ModuleAnalysisManager>();
  ThePIC = std::make_unique<PassInstrumentationCallbacks>();

  TheSI = std::make_unique<StandardInstrumentations>(*TheContext, true);
  TheSI->registerCallbacks(*ThePIC, TheMAM.get());

  TheFPM->addPass(InstCombinePass());
  TheFPM->addPass(ReassociatePass());
  TheFPM->addPass(GVNPass());
  TheFPM->addPass(SimplifyCFGPass());

  PassBuilder PB;
  PB.registerModuleAnalyses(*TheMAM);
  PB.registerFunctionAnalyses(*TheFAM);
  PB.crossRegisterProxies(*TheLAM, *TheFAM, *TheCGAM, *TheMAM);

  for (auto& stmt : stmt.stmts) {
    stmt->Accept(*this);
  }
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

  for (auto& body_stmt : stmt.body) {
    body_stmt->Accept(*this);
  }

  verifyFunction(*Func);
  TheFPM->run(*Func, *TheFAM);
  symbol_table.clear();
  return Func;
}

Value* Compiler::VisitFunctionProto(FunctionProto& stmt) {
  std::string func_name = stmt.name.lexeme;
  auto func = func_table.find(func_name);
  if (func != func_table.end()) {
    std::cout << "functions can not be redefined\n";
    exit(1);
  }

  Type* return_type;
  switch (stmt.return_type.token_type) {
    case TT_INT32_SPECIFIER:
      return_type = Type::getInt32Ty(*TheContext);
      break;
    case TT_INT64_SPECIFIER:
      return_type = Type::getInt64Ty(*TheContext);
      break;
    case TT_FLT_SPECIFIER:
      return_type = Type::getDoubleTy(*TheContext);
      break;
    case TT_VOID_SPECIFIER:
      return_type = Type::getVoidTy(*TheContext);
      break;
    default:
      UNREACHABLE(FunctionProto, "Unknown return type");
  }

  std::vector<Type*> arg_types;
  for (auto& arg : stmt.args) {
    switch (arg.type) {
      case VT_INT32:
        arg_types.push_back(Type::getInt32Ty(*TheContext));
        break;
      case VT_INT64:
        arg_types.push_back(Type::getInt64Ty(*TheContext));
        break;
      case VT_FLT:
        arg_types.push_back(Type::getFloatTy(*TheContext));
        break;
      case VT_STR:
        std::cout
            << "Implement me compiler.cpp VisitFUnctionProto string arg type\n";
        exit(1);
      case VT_VOID:
        arg_types.push_back(Type::getVoidTy(*TheContext));
        break;
      default:
        UNREACHABLE(FunctionProto, "Unknown argument type");
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

  func_table[func_name] = stmt.args.size();
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
    case TT_INT32_SPECIFIER:
      type = Type::getInt32Ty(*TheContext);
      break;
    case TT_INT64_SPECIFIER:
      type = Type::getInt64Ty(*TheContext);
      break;
    case TT_FLT_SPECIFIER:
      type = Type::getFloatTy(*TheContext);
      break;
    case TT_BOOL_SPECIFIER:
      type = Type::getInt1Ty(*TheContext);
      break;
    case TT_STR_SPECIFIER:
      std::cout << "IMPLEMENT STRING TYPES\n";
      exit(1);
      break;
    default:
      UNREACHABLE(VarDeclarationStmt, "Unknown variable type");
  }
  Value* value = stmt.value->Accept(*this);
  AllocaInst* value_ptr = Builder->CreateAlloca(type, nullptr, name);
  Builder->CreateStore(value, value_ptr);
  symbol_table[name] = value_ptr;
  return nullptr;
}

Value* Compiler::VisitConditional(Conditional& expr) {
  Value* left = expr.left->Accept(*this);
  Value* right = expr.right->Accept(*this);

  Type* l_type = left->getType();
  Type* r_type = right->getType();

  if (l_type->isIntegerTy() && r_type->isIntegerTy()) {
    switch (expr.op.token_type) {
      case TT_BANGEQ:
        return Builder->CreateCmp(CmpInst::ICMP_NE, left, right, "cmptmp");
      case TT_EQEQ:
        return Builder->CreateCmp(CmpInst::ICMP_EQ, left, right, "cmptmp");
      case TT_LESSER:
        return Builder->CreateCmp(CmpInst::ICMP_SLT, left, right, "cmptmp");
      case TT_LESSER_EQ:
        return Builder->CreateCmp(CmpInst::ICMP_SLE, left, right, "cmptmp");
      case TT_GREATER:
        return Builder->CreateCmp(CmpInst::ICMP_SGT, left, right, "cmptmp");
      case TT_GREATER_EQ:
        return Builder->CreateCmp(CmpInst::ICMP_SGE, left, right, "cmptmp");
      default:
        UNREACHABLE(Conditional, "Unknown integer operation");
    }
  } else if (l_type->isFloatTy() && r_type->isFloatTy()) {
    switch (expr.op.token_type) {
      case TT_BANGEQ:
        return Builder->CreateFCmp(CmpInst::ICMP_NE, left, right, "cmptmp");
      case TT_EQEQ:
        return Builder->CreateFCmp(CmpInst::ICMP_EQ, left, right, "cmptmp");
      case TT_LESSER:
        return Builder->CreateFCmp(CmpInst::ICMP_SLT, left, right, "cmptmp");
      case TT_LESSER_EQ:
        return Builder->CreateFCmp(CmpInst::ICMP_SLE, left, right, "cmptmp");
      case TT_GREATER:
        return Builder->CreateFCmp(CmpInst::ICMP_SGT, left, right, "cmptmp");
      case TT_GREATER_EQ:
        return Builder->CreateFCmp(CmpInst::ICMP_SGE, left, right, "cmptmp");
      default:
        UNREACHABLE(Conditional, "Unknown floating point operation");
    }
  } else {
    std::cout << "incompatible types in '" << expr.op.lexeme
              << "' expression.\n";
    std::cout << left->getName().str();
    l_type->print(errs(), true);
    std::cout << "\n";
    r_type->print(errs(), true);
    std::cout << "\n";
    exit(1);
  }
  return nullptr;
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
        UNREACHABLE(Binary, "Unknown integer operation");
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
        UNREACHABLE(Binary, "Unknown floating point operation");
    }
  } else {
    std::cout << "incompatible types in '" << expr.op.lexeme
              << "' expression.\n";
    l_type->print(errs(), true);
    std::cout << "\n";
    r_type->print(errs(), true);
    std::cout << "\n";
    exit(1);
  }
  return nullptr;
}

Value* Compiler::VisitPreIncrement(PreFixOp& expr) {
  LoadInst* var = dyn_cast<LoadInst>(expr.var->Accept(*this));
  AllocaInst* var_ptr = dyn_cast<AllocaInst>(var->getPointerOperand());

  Type* type = var_ptr->getAllocatedType();

  Value* inc;
  Value* value;

  switch (expr.op.token_type) {
    case TT_PLUS_PLUS:
      if (type->isIntegerTy()) {
        inc = ConstantInt::get(*TheContext, APInt(32, 1));
        value = Builder->CreateAdd(var, inc, "addtmp");
      } else if (type->isFloatTy()) {
        inc = ConstantFP::get(*TheContext, APFloat(1.0f));
        value = Builder->CreateFAdd(var, inc, "addtmp");
      } else {
        UNREACHABLE(PreInc, "Unknown value type");
      }
      break;
    case TT_MINUS_MINUS:
      if (type->isIntegerTy()) {
        inc = ConstantInt::get(*TheContext, APInt(32, 1));
        value = Builder->CreateSub(var, inc, "subtmp");
      } else if (type->isFloatTy()) {
        inc = ConstantFP::get(*TheContext, APFloat(1.0f));
        value = Builder->CreateFSub(var, inc, "subtmp");
      } else {
        UNREACHABLE(PreInc, "Unknown value type");
      }
      break;
    default:
      UNREACHABLE(PreInc, "Unknown value type");
  }
  Builder->CreateStore(value, var_ptr);
  return nullptr;
}

Value* Compiler::VisitAssignment(Assign& expr) {
  Value* value = expr.value->Accept(*this);
  LoadInst* var = dyn_cast<LoadInst>(expr.name->Accept(*this));
  AllocaInst* var_ptr = dyn_cast<AllocaInst>(var->getPointerOperand());
  Builder->CreateStore(value, var_ptr);
  return nullptr;
}

Value* Compiler::VisitCall(CallExpr& expr) {
  Function* callee = dyn_cast<Function>(expr.callee->Accept(*this));
  if (!callee) {
    std::cout << "callee is not a function\n";
    exit(1);
  }

  std::string name = callee->getName().str();
  auto func = func_table.find(name);
  if (func->second != expr.args.size()) {
    std::cout << "callee arguments do not match function signature\n";
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

Value* Compiler::VisitGrouping(Grouping& expr) {
  return expr.expr->Accept(*this);
}

Value* Compiler::VisitVariable(Variable& expr) {
  std::string lex = expr.name.lexeme;
  auto func = func_table.find(lex);
  if (func != func_table.end()) {
    return TheModule->getFunction(lex);
  }

  auto arg = argument_table.find(lex);
  if (arg != argument_table.end()) {
    return arg->second;
  }

  auto temp = symbol_table.find(lex);
  if (temp == symbol_table.end()) {
    std::cout << "Variables is undefined " << lex;
    exit(1);
  }

  AllocaInst* var_ptr = temp->second;
  Type* type = var_ptr->getAllocatedType();
  Value* value = Builder->CreateLoad(type, var_ptr, lex);
  return value;
}

Value* Compiler::VisitBoolean(BoolLiteral& expr) {
  if (expr.boolean) {
    return ConstantInt::getTrue(*TheContext);
  }
  return ConstantInt::getFalse(*TheContext);
}

Value* Compiler::VisitLiteral(Literal& expr) {
  switch (expr.value_type) {
    case VT_INT32:
      return ConstantInt::get(*TheContext,
                              APInt(32, std::get<int>(expr.value)));
    case VT_INT64:
      return ConstantInt::get(*TheContext,
                              APInt(64, std::get<int>(expr.value)));
    case VT_FLT:
      return ConstantFP::get(*TheContext, APFloat(std::get<float>(expr.value)));
    case VT_STR:
      /// TODO: No clue if this will work
      return Builder->CreateGlobalString(std::get<std::string>(expr.value), "",
                                         true);
    case VT_VOID:
      return nullptr;  // This is a horrible hack for void return types
                       // I need to think of a better solution
    default:
      UNREACHABLE(Literal, "Unknown literal value");
  }
}

CompilerOptions::CompilerOptions(std::string out_path, CompilerMode mode,
                                 bool debug_info,
                                 std::vector<std::string> linker_flags_list)
    : out_path(out_path), mode(mode), debug_info(debug_info) {
  for (auto it = linker_flags_list.begin(); it != linker_flags_list.end();
       ++it) {
    linker_flags += *it;
  }
}

std::unique_ptr<llvm::Module> Compiler::CreateModule(ModuleStmt& stmt,
                                                     llvm::LLVMContext& ctx) {
  return std::make_unique<Module>(stmt.name.lexeme, *TheContext);
}
