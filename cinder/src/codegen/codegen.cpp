#include "cinder/codegen/codegen.hpp"

#include <cstdlib>
#include <memory>

#include "cinder/ast/expr.hpp"
#include "cinder/ast/stmt.hpp"
#include "cinder/ast/types.hpp"
#include "cinder/codegen/codegen_bindings.hpp"
#include "cinder/frontend/tokens.hpp"
#include "cinder/support/utils.hpp"
#include "llvm/ADT/APFloat.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Value.h"
#include "llvm/Support/CodeGen.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Target/TargetOptions.h"
#include "llvm/TargetParser/Host.h"
#include "llvm/Transforms/Scalar.h"

using namespace llvm;
static ExitOnError ExitOnErr;

static ostream::RawOutStream errors{2};

void Codegen::AddPrintf() {
  /// HACK: This should let us use printf for now
  /// I need to find a way to support variadics, we check for arity at call time
  /// and variadics obviously can take an arbitrary number
  /// For now we expect only 2 arguments
  Type* type = Type::getInt32Ty(ctx_->GetContext());
  PointerType* char_ptr_type = PointerType::getUnqual(ctx_->GetContext());
  FunctionType* printf_type = FunctionType::get(type, char_ptr_type, true);
  Function* printfFunc = Function::Create(
      printf_type, GlobalValue::ExternalLinkage, "printf", ctx_->GetModule());
  verifyFunction(*printfFunc);
  // symbol_table->Declare("printf", symb);
}

Codegen::Codegen(std::unique_ptr<Stmt> mod, CompilerOptions opts)
    : mod(std::move(mod)), opts(opts), pass_(types_) {}

bool Codegen::Generate() {
  InitializeAllTargetInfos();
  InitializeAllTargets();
  InitializeAllTargetMCs();
  InitializeAllAsmParsers();
  InitializeAllAsmPrinters();

  GenerateIR();

  auto TargetTriple = sys::getDefaultTargetTriple();
  ctx_->GetModule().setTargetTriple(Triple(TargetTriple));

  std::string Error;
  auto Target =
      TargetRegistry::lookupTarget(ctx_->GetModule().getTargetTriple(), Error);

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
      ctx_->GetModule().setDataLayout(TheTargetMachine->createDataLayout());
      CompileBinary(TheTargetMachine);
      return true;
    case CompilerMode::EMIT_LLVM:
      ctx_->GetModule().setDataLayout(TheTargetMachine->createDataLayout());
      EmitLLVM();
      return true;
    case CompilerMode::RUN:
      CompileRun();
      return false;
    default:
      UNREACHABLE(COMPILER_MODE, "Unknown compile type");
  }
}

void Codegen::GenerateIR() {
  mod->Accept(*this);
}

void Codegen::EmitLLVM() {
  std::error_code EC;
  StringRef name{opts.out_path};
  raw_fd_ostream OS(name, EC, sys::fs::OF_None);
  ctx_->GetModule().print(OS, nullptr);
}

void Codegen::CompileRun() {
  IMPLEMENT(CompileRun);
}

void Codegen::CompileBinary(TargetMachine* target_machine) {
  std::error_code EC;
  std::string temp = "." + opts.out_path + ".o";
  StringRef name{temp};
  raw_fd_ostream object_file(name, EC, sys::fs::OF_None);

  legacy::PassManager pass;
  auto FileType = CodeGenFileType::ObjectFile;

  if (target_machine->addPassesToEmitFile(pass, object_file, nullptr,
                                          FileType)) {
    ostream::ErrorOutln(errors,
                        "TheTargetMachine can't emit a file of this type");
    return;
  }

  pass.run(ctx_->GetModule());
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

void Codegen::SemanticPass(ModuleStmt& mod) {
  pass_.Analyze(mod);
}

Value* Codegen::Visit(ModuleStmt& stmt) {
  SemanticPass(stmt);
  pass_.DebugSymbols();
  pass_.DumpErrors();
  exit(0);

  for (auto& stmt : stmt.stmts) {
    stmt->Accept(*this);
  }

  return nullptr;
}

Value* Codegen::Visit(ExpressionStmt& stmt) {
  stmt.expr->Accept(*this);
  return nullptr;
}

Value* Codegen::Visit(WhileStmt& stmt) {
  Function* func = ctx_->GetBuilder().GetInsertBlock()->getParent();
  BasicBlock* cond_block =
      BasicBlock::Create(ctx_->GetContext(), "loop.cond", func);
  BasicBlock* loop_block =
      BasicBlock::Create(ctx_->GetContext(), "loop.body", func);
  BasicBlock* after_block =
      BasicBlock::Create(ctx_->GetContext(), "loop.body", func);

  ctx_->GetBuilder().CreateBr(cond_block);
  ctx_->GetBuilder().SetInsertPoint(cond_block);
  Value* condition = stmt.condition->Accept(*this);

  ctx_->GetBuilder().CreateCondBr(condition, loop_block, after_block);

  ctx_->GetBuilder().SetInsertPoint(loop_block);
  for (auto& stmt : stmt.body) {
    stmt->Accept(*this);
  }

  ctx_->GetBuilder().CreateBr(cond_block);
  ctx_->GetBuilder().SetInsertPoint(after_block);
  return nullptr;
}

Value* Codegen::Visit(ForStmt& stmt) {
  // std::shared_ptr<SymbolTable> for_scope =
  //     std::make_shared<SymbolTable>(this->symbol_table);
  // std::shared_ptr<SymbolTable> previous = this->symbol_table;
  // this->symbol_table = for_scope;

  // if (stmt.initializer) {
  //   stmt.initializer->Accept(*this);
  // }

  // Function* func = ctx_->GetBuilder().GetInsertBlock()->getParent();
  // BasicBlock* cond_block = BasicBlock::Create(ctx_->GetContext(),
  // "loop.cond", func); BasicBlock* loop_block =
  // BasicBlock::Create(ctx_->GetContext(), "loop.body", func); BasicBlock*
  // step_block = BasicBlock::Create(ctx_->GetContext(), "loop.step", func);
  // BasicBlock* after_block = BasicBlock::Create(ctx_->GetContext(),
  // "loop.end", func);

  // ctx_->GetBuilder().CreateBr(cond_block);

  // ctx_->GetBuilder().SetInsertPoint(cond_block);
  // Value* condition = stmt.condition->Accept(*this);

  // ctx_->GetBuilder().CreateCondBr(condition, loop_block, after_block);

  // ctx_->GetBuilder().SetInsertPoint(loop_block);
  // for (auto& stmt : stmt.body) {
  //   stmt->Accept(*this);
  // }

  // ctx_->GetBuilder().CreateBr(step_block);

  // ctx_->GetBuilder().SetInsertPoint(step_block);
  // if (stmt.step) {
  //   stmt.step->Accept(*this);
  // }
  // ctx_->GetBuilder().CreateBr(cond_block);

  // ctx_->GetBuilder().SetInsertPoint(after_block);
  // this->symbol_table = previous;
  return nullptr;
}

Value* Codegen::Visit(IfStmt& stmt) {
  Value* condition = stmt.cond->Accept(*this);

  Function* Func = ctx_->GetBuilder().GetInsertBlock()->getParent();

  BasicBlock* then_block =
      BasicBlock::Create(ctx_->GetContext(), "if.then", Func);
  BasicBlock* merge = BasicBlock::Create(ctx_->GetContext(), "if.cont");
  BasicBlock* else_block = merge;

  if (stmt.otherwise) {
    else_block = BasicBlock::Create(ctx_->GetContext(), "if.else");
  }

  ctx_->GetBuilder().CreateCondBr(condition, then_block, else_block);

  ctx_->GetBuilder().SetInsertPoint(then_block);
  /// TODO: Add multiple statements in the body of the then and else branches
  /// We do not have block statements so passing a bunch of stuff at once
  /// probably needs a vector for now
  stmt.then->Accept(*this);

  if (!ctx_->GetBuilder().GetInsertBlock()->getTerminator()) {
    ctx_->GetBuilder().CreateBr(merge);
  }
  then_block = ctx_->GetBuilder().GetInsertBlock();

  if (stmt.otherwise) {
    Func->insert(Func->end(), else_block);
    ctx_->GetBuilder().SetInsertPoint(else_block);
    stmt.otherwise->Accept(*this);

    if (!ctx_->GetBuilder().GetInsertBlock()->getTerminator()) {
      ctx_->GetBuilder().CreateBr(merge);
    }
    else_block = ctx_->GetBuilder().GetInsertBlock();
  }

  Func->insert(Func->end(), merge);
  ctx_->GetBuilder().SetInsertPoint(merge);
  return nullptr;
}

Value* Codegen::Visit(FunctionStmt& stmt) {
  // Function* Func = dyn_cast<Function>(stmt.proto->Accept(*this));
  // std::shared_ptr<SymbolTable> function_scope =
  //     std::make_shared<SymbolTable>(this->symbol_table);
  // std::shared_ptr<SymbolTable> previous = this->symbol_table;
  // this->symbol_table = function_scope;

  // BasicBlock* BB = BasicBlock::Create(ctx_->GetContext(), "entry", Func);
  // ctx_->GetBuilder().SetInsertPoint(BB);

  // for (auto& body_stmt : stmt.body) {
  //   body_stmt->Accept(*this);
  // }

  // verifyFunction(*Func);
  // TheFPM->run(*Func, *TheFAM);
  // this->symbol_table = previous;
  // return Func;
}

Value* Codegen::Visit(FunctionProto& stmt) {
  // std::string func_name = stmt.name.lexeme;
  // Type* return_type = ResolveType(stmt.resolved_type);

  // std::vector<Type*> arg_types;
  // for (auto& arg : stmt.args) {
  //   arg_types.push_back(ResolveArgType(arg.resolved_type));
  // }

  // FunctionType* FT =
  //     FunctionType::get(return_type, arg_types, stmt.is_variadic);
  // Function* Func =
  //     Function::Create(FT, Function::ExternalLinkage, func_name,
  //     ctx_->GetModule());

  // unsigned Idx = 0;
  // for (auto& arg : Func->args()) {
  //   arg.setName(stmt.args[Idx].identifier.lexeme);
  //   InternalSymbol symb{nullptr, &arg};
  //   std::string name = stmt.args[Idx++].identifier.lexeme;
  //   symbol_table->Declare(name, symb);
  // }

  // InternalSymbol symb{nullptr, Func};
  // symbol_table->Declare(func_name, symb);
  // return Func;
}

Value* Codegen::Visit(ReturnStmt& stmt) {
  if (!stmt.value) {
    return ctx_->GetBuilder().CreateRetVoid();
  }

  Value* ret = stmt.value->Accept(*this);
  switch (stmt.value->type->kind) {
    case types::TypeKind::Void:
      return ctx_->GetBuilder().CreateRetVoid();
    case types::TypeKind::Bool:
    case types::TypeKind::String:
    case types::TypeKind::Int:
    case types::TypeKind::Float:
    default:
      return ctx_->GetBuilder().CreateRet(ret);
  }
  return nullptr;
}

Value* Codegen::Visit(VarDeclarationStmt& stmt) {
  // std::string name = stmt.name.lexeme;

  // Type* type = ResolveType(stmt.value->type);

  // Value* value = stmt.value->Accept(*this);

  // AllocaInst* value_ptr = ctx_->GetBuilder().CreateAlloca(type, nullptr,
  // name);

  // ctx_->GetBuilder().CreateStore(value, value_ptr);

  // InternalSymbol symb{value_ptr, value};
  // symbol_table->Declare(name, symb);
  return nullptr;
}

Value* Codegen::Visit(Conditional& expr) {
  Value* left = expr.left->Accept(*this);
  Value* right = expr.right->Accept(*this);

  switch (expr.type->kind) {
    case types::TypeKind::Int:
      switch (expr.op.type) {
        case TT_BANGEQ:
          return ctx_->GetBuilder().CreateCmp(CmpInst::ICMP_NE, left, right,
                                              "cmptmp");
        case TT_EQEQ:
          return ctx_->GetBuilder().CreateCmp(CmpInst::ICMP_EQ, left, right,
                                              "cmptmp");
        case TT_LESSER:
          return ctx_->GetBuilder().CreateCmp(CmpInst::ICMP_SLT, left, right,
                                              "cmptmp");
        case TT_LESSER_EQ:
          return ctx_->GetBuilder().CreateCmp(CmpInst::ICMP_SLE, left, right,
                                              "cmptmp");
        case TT_GREATER:
          return ctx_->GetBuilder().CreateCmp(CmpInst::ICMP_SGT, left, right,
                                              "cmptmp");
        case TT_GREATER_EQ:
          return ctx_->GetBuilder().CreateCmp(CmpInst::ICMP_SGE, left, right,
                                              "cmptmp");
        default:
          break;
      }
    case types::TypeKind::Float:
      switch (expr.op.type) {
        case TT_BANGEQ:
          return ctx_->GetBuilder().CreateFCmp(CmpInst::ICMP_NE, left, right,
                                               "cmptmp");
        case TT_EQEQ:
          return ctx_->GetBuilder().CreateFCmp(CmpInst::ICMP_EQ, left, right,
                                               "cmptmp");
        case TT_LESSER:
          return ctx_->GetBuilder().CreateFCmp(CmpInst::ICMP_SLT, left, right,
                                               "cmptmp");
        case TT_LESSER_EQ:
          return ctx_->GetBuilder().CreateFCmp(CmpInst::ICMP_SLE, left, right,
                                               "cmptmp");
        case TT_GREATER:
          return ctx_->GetBuilder().CreateFCmp(CmpInst::ICMP_SGT, left, right,
                                               "cmptmp");
        case TT_GREATER_EQ:
          return ctx_->GetBuilder().CreateFCmp(CmpInst::ICMP_SGE, left, right,
                                               "cmptmp");
        default:
          break;
      }
    default:
      break;
  }
  return nullptr;
}

Value* Codegen::Visit(Binary& expr) {
  Value* left = expr.left->Accept(*this);
  Value* right = expr.right->Accept(*this);

  switch (expr.type->kind) {
    case types::TypeKind::Int:
      switch (expr.op.type) {
        case TT_PLUS:
          return ctx_->GetBuilder().CreateAdd(left, right, "addtmp");
        case TT_MINUS:
          return ctx_->GetBuilder().CreateSub(left, right, "subtmp");
        case TT_SLASH:
          return ctx_->GetBuilder().CreateSDiv(left, right, "divtmp");
        case TT_STAR:
          return ctx_->GetBuilder().CreateMul(left, right, "multmp");
        default:
          break;
      }
    case types::TypeKind::Float:
      switch (expr.op.type) {
        case TT_PLUS:
          return ctx_->GetBuilder().CreateFAdd(left, right, "addtmp");
        case TT_MINUS:
          return ctx_->GetBuilder().CreateFSub(left, right, "subtmp");
        case TT_STAR:
          return ctx_->GetBuilder().CreateFMul(left, right, "multmp");
        case TT_SLASH:
          return ctx_->GetBuilder().CreateFDiv(left, right, "divtmp");
        default:
          break;
      }
    default:
      break;
  }
  return nullptr;
}

Value* Codegen::Visit(PreFixOp& expr) {
  // InternalSymbol* symb = symbol_table->LookUp(expr.name.lexeme);
  // if (!symb) {
  //   errs::ErrorOutln(errors, "Undefined variable:", expr.name.lexeme);
  //   return nullptr;
  // }

  // if (!symb->alloca_ptr) {
  //   errs::ErrorOutln(
  //       errors,
  //       "Prefix operators are only valid on mutable locals:",
  //       expr.name.lexeme);
  //   return nullptr;
  // }

  // Value* var =
  // ctx_->GetBuilder().CreateLoad(symb->alloca_ptr->getAllocatedType(),
  //                                  symb->alloca_ptr, expr.name.lexeme);

  // Value* inc = nullptr;
  // Value* value = nullptr;

  // /// TODO: maybe refactor this, a but too complex
  // switch (expr.op.type) {
  //   case TT_PLUS_PLUS:
  //     switch (expr.type->kind) {
  //       case types::TypeKind::Int:
  //         inc = ConstantInt::get(ctx_->GetContext(), APInt(32, 1));
  //         value = ctx_->GetBuilder().CreateAdd(var, inc, "addtmp");
  //         break;
  //       case types::TypeKind::Float:
  //         inc = ConstantFP::get(ctx_->GetContext(), APFloat(1.0f));
  //         value = ctx_->GetBuilder().CreateFAdd(var, inc, "addtmp");
  //         break;
  //       default:
  //         break;
  //     }
  //     break;
  //   case TT_MINUS_MINUS:
  //     switch (expr.type->kind) {
  //       case types::TypeKind::Int:
  //         inc = ConstantInt::get(ctx_->GetContext(), APInt(32, 1));
  //         value = ctx_->GetBuilder().CreateSub(var, inc, "subtmp");
  //         break;
  //       case types::TypeKind::Float:
  //         inc = ConstantFP::get(ctx_->GetContext(), APFloat(1.0f));
  //         value = ctx_->GetBuilder().CreateFSub(var, inc, "subtmp");
  //         break;
  //       default:
  //         break;
  //     }
  //     break;
  //   default:
  //     break;
  // }

  // if (!value) {
  //   errs::ErrorOutln(
  //       errors, "Invalid prefix operation for variable:", expr.name.lexeme);
  //   return nullptr;
  // }

  // ctx_->GetBuilder().CreateStore(value, symb->alloca_ptr);
  // return value;
}

Value* Codegen::Visit(Assign& expr) {
  // InternalSymbol* symb = symbol_table->LookUp(expr.name.lexeme);
  // if (!symb || !symb->alloca_ptr) {
  //   errs::ErrorOutln(errors, "Assignment to undefined or immutable
  //   variable:",
  //                    expr.name.lexeme);
  //   return nullptr;
  // }

  // Value* value = expr.value->Accept(*this);
  // ctx_->GetBuilder().CreateStore(value, symb->alloca_ptr);
  // return value;
}

Value* Codegen::Visit(CallExpr& expr) {
  Function* callee = dyn_cast<Function>(expr.callee->Accept(*this));

  std::vector<Value*> call_args;
  for (auto& arg : expr.args) {
    call_args.push_back(arg->Accept(*this));
  }

  if (expr.type->kind == types::TypeKind::Void) {
    return ctx_->GetBuilder().CreateCall(callee, call_args);
  }

  return ctx_->GetBuilder().CreateCall(callee, call_args, callee->getName());
}

Value* Codegen::Visit(Grouping& expr) {
  return expr.expr->Accept(*this);
}

Value* Codegen::Visit(Variable& expr) {
  auto var = ir_bindings_.find(expr.id.value());
  return var->second.value;
}

Value* Codegen::Visit(Literal& expr) {
  switch (expr.type->kind) {
    case types::TypeKind::Bool:
      return ConstantInt::getBool(ctx_->GetContext(),
                                  std::get<bool>(expr.value));
    case types::TypeKind::Float:
      return ConstantFP::get(ctx_->GetContext(),
                             APFloat(std::get<float>(expr.value)));
    case types::TypeKind::Int:
      return EmitInteger(expr);
    case types::TypeKind::String:
      return ctx_->GetBuilder().CreateGlobalString(
          std::get<std::string>(expr.value), "", true);
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

llvm::Value* Codegen::EmitInteger(Literal& expr) {
  types::IntType* int_type = dynamic_cast<types::IntType*>(expr.type);
  int value = std::get<int>(expr.value);
  return ConstantInt::get(ctx_->GetContext(), APInt(int_type->bits, value));
}

llvm::Type* Codegen::ResolveArgType(types::Type* type) {
  switch (type->kind) {
    case types::TypeKind::Bool:
      return Type::getInt1Ty(ctx_->GetContext());
    case types::TypeKind::Float:
      return Type::getFloatTy(ctx_->GetContext());
    case types::TypeKind::Int:
      return Type::getInt32Ty(ctx_->GetContext());
    case types::TypeKind::String:
      return Type::getInt8Ty(ctx_->GetContext());
    case types::TypeKind::Void:
    case types::TypeKind::Struct:
    default:
      break;
  }
  return nullptr;
}

llvm::Type* Codegen::ResolveType(types::Type* type) {
  switch (type->kind) {
    case types::TypeKind::Bool:
      return Type::getInt1Ty(ctx_->GetContext());
    case types::TypeKind::Float:
      return Type::getFloatTy(ctx_->GetContext());
    case types::TypeKind::Int:
      return Type::getInt32Ty(ctx_->GetContext());
    case types::TypeKind::String:
      return Type::getInt8Ty(ctx_->GetContext());
    case types::TypeKind::Void:
      return Type::getVoidTy(ctx_->GetContext());
    case types::TypeKind::Struct:
    default:
      IMPLEMENT(ResolveType);
  }
}
