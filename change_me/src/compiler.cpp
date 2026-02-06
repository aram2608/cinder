#include "../include/compiler.hpp"

#include <stddef.h>

#include <cstdlib>

#include "../include/utils.hpp"
#include "errors.hpp"
#include "expr.hpp"
#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Target/TargetOptions.h"
#include "llvm/TargetParser/Host.h"
#include "llvm/Transforms/InstCombine/InstCombine.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/Scalar/SimplifyCFG.h"
#include "stmt.hpp"
#include "symbol_table.hpp"
#include "types.hpp"

using namespace llvm;

static std::unique_ptr<LLVMContext> TheContext;
static std::unique_ptr<Module> TheModule;
static std::unique_ptr<IRBuilder<>> Builder;
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

static errs::RawOutStream errors{};

void Compiler::AddPrintf() {
  /// HACK: This should let us use printf for now
  /// I need to find a way to support variadics, we check for arity at call time
  /// and variadics obviously can take an arbitrary number
  /// For now we expect only 2 arguments
  Type* type = Type::getInt32Ty(*TheContext);
  PointerType* char_ptr_type = PointerType::getUnqual(*TheContext);
  FunctionType* printf_type = FunctionType::get(type, char_ptr_type, true);
  Function* printfFunc = Function::Create(
      printf_type, GlobalValue::ExternalLinkage, "printf", *TheModule);
  verifyFunction(*printfFunc);
  InternalSymbol symb{nullptr, printfFunc};
  symbol_table->Declare("printf", symb);
}

Compiler::Compiler(std::unique_ptr<Stmt> mod, CompilerOptions opts)
    : mod(std::move(mod)),
      opts(opts),
      /**
       * TODO: Find a way to have the context stored as a member var
       * For now the module outlives the context leading a seg fault during
       * cleanup At least thats what I think is happening since I don't have
       * debug info for LLVM
       */
      context(std::make_unique<LLVMContext>()),
      pass(&type_context),
      symbol_table(nullptr) {}

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
      CompileRun();
      return false;
    default:
      UNREACHABLE(COMPILER_MODE, "Unknown compile type");
  }
}

void Compiler::GenerateIR() {
  TheContext = std::make_unique<LLVMContext>();
  mod->Accept(*this);
}

void Compiler::EmitLLVM() {
  std::error_code EC;
  StringRef name{opts.out_path};
  raw_fd_ostream OS(name, EC, sys::fs::OF_None);
  TheModule->print(OS, nullptr);
}

void Compiler::CompileRun() {
  IMPLEMENT(CompileRun);
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
    errs::ErrorOutln(errors, "TheTargetMachine can't emit a file of this type");
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

Value* Compiler::Visit(ModuleStmt& stmt) {
  SemanticPass(stmt);
  exit(0);
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

Value* Compiler::Visit(ExpressionStmt& stmt) {
  stmt.expr->Accept(*this);
  return nullptr;
}

Value* Compiler::Visit(WhileStmt& stmt) {
  Function* func = Builder->GetInsertBlock()->getParent();
  BasicBlock* cond_block = BasicBlock::Create(*TheContext, "loop.cond", func);
  BasicBlock* loop_block = BasicBlock::Create(*TheContext, "loop.body", func);
  BasicBlock* after_block = BasicBlock::Create(*TheContext, "loop.body", func);

  Builder->CreateBr(cond_block);
  Builder->SetInsertPoint(cond_block);
  Value* condition = stmt.condition->Accept(*this);

  Builder->CreateCondBr(condition, loop_block, after_block);

  Builder->SetInsertPoint(loop_block);
  for (auto& stmt : stmt.body) {
    stmt->Accept(*this);
  }

  Builder->CreateBr(cond_block);
  Builder->SetInsertPoint(after_block);
  return nullptr;
}

Value* Compiler::Visit(ForStmt& stmt) {
  if (stmt.initializer) {
    stmt.initializer->Accept(*this);
  }

  Function* func = Builder->GetInsertBlock()->getParent();
  BasicBlock* cond_block = BasicBlock::Create(*TheContext, "loop.cond", func);
  BasicBlock* loop_block = BasicBlock::Create(*TheContext, "loop.body", func);
  BasicBlock* step_block = BasicBlock::Create(*TheContext, "loop.step", func);
  BasicBlock* after_block = BasicBlock::Create(*TheContext, "loop.end", func);

  Builder->CreateBr(cond_block);

  Builder->SetInsertPoint(cond_block);
  Value* condition = stmt.condition->Accept(*this);

  Builder->CreateCondBr(condition, loop_block, after_block);

  Builder->SetInsertPoint(loop_block);
  for (auto& stmt : stmt.body) {
    stmt->Accept(*this);
  }

  Builder->CreateBr(step_block);

  Builder->SetInsertPoint(step_block);
  if (stmt.step) {
    stmt.step->Accept(*this);
  }
  Builder->CreateBr(cond_block);

  Builder->SetInsertPoint(after_block);
  return nullptr;
}

Value* Compiler::Visit(IfStmt& stmt) {
  Value* condition = stmt.cond->Accept(*this);

  Function* Func = Builder->GetInsertBlock()->getParent();

  BasicBlock* then_block = BasicBlock::Create(*TheContext, "if.then", Func);
  BasicBlock* else_block = BasicBlock::Create(*TheContext, "if.else");
  BasicBlock* merge = BasicBlock::Create(*TheContext, "if.cont");

  Builder->CreateCondBr(condition, then_block, else_block);

  Builder->SetInsertPoint(then_block);
  /// TODO: Add multiple statements in the body of the then and else branches
  /// We do not have block statements so passing a bunch of stuff at once
  /// probably needs a vector for now
  stmt.then->Accept(*this);

  Builder->CreateBr(merge);
  then_block = Builder->GetInsertBlock();

  Func->insert(Func->end(), else_block);
  Builder->SetInsertPoint(else_block);
  stmt.otherwise->Accept(*this);

  Builder->CreateBr(merge);
  else_block = Builder->GetInsertBlock();

  Func->insert(Func->end(), merge);
  Builder->SetInsertPoint(merge);
  return nullptr;
}

Value* Compiler::Visit(FunctionStmt& stmt) {
  Function* Func = dyn_cast<Function>(stmt.proto->Accept(*this));

  BasicBlock* BB = BasicBlock::Create(*TheContext, "entry", Func);
  Builder->SetInsertPoint(BB);

  for (auto& body_stmt : stmt.body) {
    body_stmt->Accept(*this);
  }

  verifyFunction(*Func);
  TheFPM->run(*Func, *TheFAM);
  // symbol_table.clear();
  return Func;
}

Value* Compiler::Visit(FunctionProto& stmt) {
  std::string func_name = stmt.name.lexeme;
  Type* return_type = ResolveType(stmt.resolved_type);

  std::vector<Type*> arg_types;
  for (auto& arg : stmt.args) {
    UNUSED(arg);
    arg_types.push_back(ResolveArgType(arg.resolved_type));
  }

  FunctionType* FT =
      FunctionType::get(return_type, arg_types, stmt.is_variadic);
  Function* Func =
      Function::Create(FT, Function::ExternalLinkage, func_name, *TheModule);

  argument_table.clear();
  unsigned Idx = 0;
  for (auto& arg : Func->args()) {
    arg.setName(stmt.args[Idx].identifier.lexeme);
    argument_table[stmt.args[Idx++].identifier.lexeme] = &arg;
  }

  InternalSymbol symb{nullptr, Func};
  symbol_table->Declare(func_name, symb);
  return Func;
}

Value* Compiler::Visit(ReturnStmt& stmt) {
  // Value* ret = ResolveType(stmt.value->type);
  // if (!ret) {
    // return Builder->CreateRetVoid();
  // }
  // return Builder->CreateRet(ret);
  return nullptr;
}

Value* Compiler::Visit(VarDeclarationStmt& stmt) {
  std::string name = stmt.name.lexeme;

  Type* type = ResolveType(stmt.value->type);

  Value* value = stmt.value->Accept(*this);

  AllocaInst* value_ptr = Builder->CreateAlloca(type, nullptr, name);

  Builder->CreateStore(value, value_ptr);

  InternalSymbol symb{value_ptr, value};
  symbol_table->Declare(name, symb);
  return nullptr;
}

Value* Compiler::Visit(Conditional& expr) {
  Value* left = expr.left->Accept(*this);
  Value* right = expr.right->Accept(*this);

  Type* l_type = left->getType();
  Type* r_type = right->getType();

  if (l_type->isIntegerTy() && r_type->isIntegerTy()) {
    switch (expr.op.type) {
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
    switch (expr.op.type) {
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
  }
  return nullptr;
}

Value* Compiler::Visit(Binary& expr) {
  Value* left = expr.left->Accept(*this);
  Value* right = expr.right->Accept(*this);

  Type* l_type = left->getType();
  Type* r_type = right->getType();

  if (l_type->isIntegerTy() && r_type->isIntegerTy()) {
    switch (expr.op.type) {
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
    switch (expr.op.type) {
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
  }
  return nullptr;
}

Value* Compiler::Visit(PreFixOp& expr) {
  // // LoadInst* var = dyn_cast<LoadInst>(expr.var->Accept(*this));
  // AllocaInst* var_ptr = dyn_cast<AllocaInst>(var->getPointerOperand());

  // Type* type = var_ptr->getAllocatedType();

  // Value* inc;
  // Value* value;

  // switch (expr.op.type) {
  //   case TT_PLUS_PLUS:
  //     if (type->isIntegerTy()) {
  //       inc = ConstantInt::get(*TheContext, APInt(32, 1));
  //       value = Builder->CreateAdd(var, inc, "addtmp");
  //     } else if (type->isFloatTy()) {
  //       inc = ConstantFP::get(*TheContext, APFloat(1.0f));
  //       value = Builder->CreateFAdd(var, inc, "addtmp");
  //     } else {
  //       UNREACHABLE(PreInc, "Unknown value type");
  //     }
  //     break;
  //   case TT_MINUS_MINUS:
  //     if (type->isIntegerTy()) {
  //       inc = ConstantInt::get(*TheContext, APInt(32, 1));
  //       value = Builder->CreateSub(var, inc, "subtmp");
  //     } else if (type->isFloatTy()) {
  //       inc = ConstantFP::get(*TheContext, APFloat(1.0f));
  //       value = Builder->CreateFSub(var, inc, "subtmp");
  //     } else {
  //       UNREACHABLE(PreInc, "Unknown value type");
  //     }
  //     break;
  //   default:
  //     UNREACHABLE(PreInc, "Unknown value type");
  // }
  // Builder->CreateStore(value, var_ptr);
  return nullptr;
}

Value* Compiler::Visit(Assign& expr) {
  Value* value = expr.value->Accept(*this);
  // LoadInst* var = dyn_cast<LoadInst>(expr.name->Accept(*this));
  // AllocaInst* var_ptr = dyn_cast<AllocaInst>(var->getPointerOperand());
  // Builder->CreateStore(value, var_ptr);
  return nullptr;
}

Value* Compiler::Visit(CallExpr& expr) {
  UNUSED(expr);
  // Function* callee = dyn_cast<Function>(expr.callee->Accept(*this));
  // if (!callee) {
  //   std::cout << "callee is not a function\n";
  //   exit(1);
  // }

  // std::string name = callee->getName().str();
  // auto func = func_table.find(name);
  // if (func->second != expr.args.size()) {
  //   std::cout << "callee arguments do not match function signature\n";
  //   exit(1);
  // }

  // std::vector<Value*> call_args;
  // for (auto& arg : expr.args) {
  //   call_args.push_back(arg->Accept(*this));
  // }

  // Type* ret_type = callee->getReturnType();
  // if (ret_type->isVoidTy()) {
  //   return Builder->CreateCall(callee, call_args);
  // }

  // return Builder->CreateCall(callee, call_args, callee->getName());
}

Value* Compiler::Visit(Grouping& expr) {
  return expr.expr->Accept(*this);
}

Value* Compiler::Visit(Variable& expr) {
  InternalSymbol* symb = symbol_table->LookUp(expr.name.lexeme);
  return symb->value;
  // std::string lex = expr.name.lexeme;
  // auto func = func_table.find(lex);
  // if (func != func_table.end()) {
  //   return TheModule->getFunction(lex);
  // }

  // auto arg = argument_table.find(lex);
  // if (arg != argument_table.end()) {
  //   return arg->second;
  // }

  // auto temp = symbol_table.find(lex);
  // if (temp == symbol_table.end()) {
  //   std::cout << "Variables is undefined: '" << lex << "'\n";
  //   exit(1);
  // }

  // AllocaInst* var_ptr = temp->second;
  // Type* type = var_ptr->getAllocatedType();
  // Value* value = Builder->CreateLoad(type, var_ptr, lex);
  // return value;
}

Value* Compiler::Visit(Literal& expr) {
  switch (expr.type->kind) {
    case types::TypeKind::Bool:
      return ConstantInt::getBool(*TheContext, std::get<bool>(expr.value));
    case types::TypeKind::Float:
      return ConstantFP::get(*TheContext, APFloat(std::get<float>(expr.value)));
    case types::TypeKind::Int:
      return EmitInteger(expr);
    case types::TypeKind::String:
      return Builder->CreateGlobalString(std::get<std::string>(expr.value), "",
                                         true);
    case types::TypeKind::Struct:
    case types::TypeKind::Void:
    default:
      UNREACHABLE(Literal, "Invalid type");
  }
  return nullptr;
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

void Compiler::SemanticPass(ModuleStmt& mod) {
  pass.Analyze(mod);
}

llvm::Value* Compiler::EmitInteger(Literal& expr) {
  types::IntType* int_type = dynamic_cast<types::IntType*>(expr.type);
  int value = std::get<int>(expr.value);
  return ConstantInt::get(*TheContext, APInt(int_type->bits, value));
}

llvm::Type* Compiler::ResolveArgType(types::Type* type) {
  switch (type->kind) {
    case types::TypeKind::Bool:
      return Type::getInt1Ty(*TheContext);
    case types::TypeKind::Float:
      return Type::getFloatTy(*TheContext);
    case types::TypeKind::Int:
      return Type::getInt32Ty(*TheContext);
    case types::TypeKind::String:
      return Type::getInt8Ty(*TheContext);
    case types::TypeKind::Void:
    case types::TypeKind::Struct:
    default:
      break;
  }
}

llvm::Type* Compiler::ResolveType(types::Type* type) {
  switch (type->kind) {
    case types::TypeKind::Bool:
      return Type::getInt1Ty(*TheContext);
    case types::TypeKind::Float:
      return Type::getFloatTy(*TheContext);
    case types::TypeKind::Int:
      return Type::getInt32Ty(*TheContext);
    case types::TypeKind::String:
      return Type::getInt8Ty(*TheContext);
    case types::TypeKind::Void:
      return Type::getVoidTy(*TheContext);
    case types::TypeKind::Struct:
    default:
      IMPLEMENT(ResolveType);
  }
}