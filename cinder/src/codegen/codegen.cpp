#include "cinder/codegen/codegen.hpp"

#include <cstdlib>
#include <memory>
#include <system_error>
#include <unordered_map>

#include "cinder/ast/types.hpp"
#include "cinder/codegen/codegen_bindings.hpp"
#include "cinder/support/utils.hpp"
#include "llvm/ADT/APFloat.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Value.h"
#include "llvm/Support/CodeGen.h"
#include "llvm/Support/Error.h"
#include "llvm/Support/ErrorOr.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Target/TargetOptions.h"
#include "llvm/TargetParser/Host.h"
#include "llvm/Transforms/Scalar.h"

/// TODO: Refactor some of the functions in this file, a bit redundant and
/// sloppy

using namespace llvm;
using namespace cinder;
static ExitOnError ExitOnErr;

static ostream::RawOutStream errors{2};

Codegen::Codegen(std::unique_ptr<Stmt> mod, CodegenOpts opts)
    : mod(std::move(mod)),
      opts(opts),
      ctx_(std::make_unique<CodegenContext>(this->opts.out_path)),
      pass_(types_) {}

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
  TargetOptions gen_opt;
  auto TheTargetMachine = Target->createTargetMachine(
      Triple(TargetTriple), CPU, Features, gen_opt, Reloc::PIC_);

  switch (opts.mode) {
    case CodegenOpts::Opt::COMPILE:
      ctx_->GetModule().setDataLayout(TheTargetMachine->createDataLayout());
      CompileBinary(TheTargetMachine);
      return true;
    case CodegenOpts::Opt::EMIT_LLVM:
      ctx_->GetModule().setDataLayout(TheTargetMachine->createDataLayout());
      EmitLLVM();
      return true;
    case CodegenOpts::Opt::RUN:
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

  bool ok = ClangDriver::LinkObject(temp, opts.out_path, opts.linker_flags);
  if (!ok) {
    ostream::ErrorOutln(errors, "clang driver link step failed");
  }

  auto err = llvm::sys::fs::remove(temp);
  if (err) {
    diagnose_.Error({0}, err.message());
  }
}

bool Codegen::SemanticPass(ModuleStmt& mod) {
  pass_.Analyze(mod);
  if (pass_.HadError()) {
    pass_.DumpErrors();
    return false;
  }
  return true;
}

Value* Codegen::Visit(ModuleStmt& stmt) {
  if (!SemanticPass(stmt)) {
    return nullptr;
  }

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
  if (stmt.initializer) {
    stmt.initializer->Accept(*this);
  }

  Function* func = ctx_->GetBuilder().GetInsertBlock()->getParent();
  BasicBlock* cond_block =
      BasicBlock::Create(ctx_->GetContext(), "loop.cond", func);
  BasicBlock* loop_block =
      BasicBlock::Create(ctx_->GetContext(), "loop.body", func);
  BasicBlock* step_block =
      BasicBlock::Create(ctx_->GetContext(), "loop.step", func);
  BasicBlock* after_block =
      BasicBlock::Create(ctx_->GetContext(), "loop.end", func);

  ctx_->GetBuilder().CreateBr(cond_block);

  ctx_->GetBuilder().SetInsertPoint(cond_block);
  Value* condition = stmt.condition->Accept(*this);

  ctx_->GetBuilder().CreateCondBr(condition, loop_block, after_block);

  ctx_->GetBuilder().SetInsertPoint(loop_block);
  for (auto& stmt : stmt.body) {
    stmt->Accept(*this);
  }

  ctx_->GetBuilder().CreateBr(step_block);

  ctx_->GetBuilder().SetInsertPoint(step_block);
  if (stmt.step) {
    stmt.step->Accept(*this);
  }
  ctx_->GetBuilder().CreateBr(cond_block);

  ctx_->GetBuilder().SetInsertPoint(after_block);
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
  Function* func = dyn_cast<Function>(stmt.proto->Accept(*this));
  BasicBlock* entry = BasicBlock::Create(ctx_->GetContext(), "entry", func);
  ctx_->GetBuilder().SetInsertPoint(entry);

  for (auto& body_stmt : stmt.body) {
    body_stmt->Accept(*this);
  }

  /// TODO: Catch malformed non void fuctions
  /// Right now it seg faults when a return is not provided
  if (!ctx_->GetBuilder().GetInsertBlock()->getTerminator()) {
    if (func->getReturnType()->isVoidTy()) {
      ctx_->GetBuilder().CreateRetVoid();
    }
  }

  verifyFunction(*func);
  return func;
}

Value* Codegen::Visit(FunctionProto& stmt) {
  auto match_type = [&](Token t) -> Type* {
    switch (t.kind) {
      case Token::Type::INT32_SPECIFIER:
        return Type::getInt32Ty(ctx_->GetContext());
      case Token::Type::FLT32_SPECIFIER:
        return Type::getFloatTy(ctx_->GetContext());
      case Token::Type::FLT64_SPECIFIER:
        return Type::getDoubleTy(ctx_->GetContext());
      case Token::Type::BOOL_SPECIFIER:
        return Type::getInt1Ty(ctx_->GetContext());
      case Token::Type::STR_SPECIFIER:
        return PointerType::getInt8Ty(ctx_->GetContext());
      case Token::Type::VOID_SPECIFIER:
        return Type::getVoidTy(ctx_->GetContext());
      default:
        return nullptr;
    }
  };

  std::vector<Type*> arg_types;
  arg_types.reserve(stmt.args.size());
  for (auto& arg : stmt.args) {
    arg_types.push_back(ResolveArgType(arg.resolved_type));
  }

  FunctionType* func_type = FunctionType::get(match_type(stmt.return_type),
                                              arg_types, stmt.is_variadic);

  Function* func = Function::Create(func_type, Function::ExternalLinkage,
                                    stmt.name.lexeme, ctx_->GetModule());

  size_t idx = 0;
  for (auto& arg : func->args()) {
    arg.setName(stmt.args[idx].identifier.lexeme);
    ++idx;
  }

  if (stmt.id.has_value()) {
    std::unique_ptr<Binding>& b = ir_bindings_[stmt.id.value()];
    if (!b || !b->IsFunction()) {
      b = std::make_unique<FuncBinding>();
    }
    FuncBinding* f = dynamic_cast<FuncBinding*>(b.get());
    if (!f) {
      return nullptr;
    }
    f->function = func;
  }
  return func;
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
  Value* init = stmt.value->Accept(*this);
  Type* ty = ResolveType(stmt.value->type);
  AllocaInst* slot = ctx_->CreateAlloca(ty, nullptr, stmt.name.lexeme);
  ctx_->CreateStore(init, slot);

  if (stmt.HasID()) {
    std::unique_ptr<Binding>& b = ir_bindings_[stmt.GetID()];
    if (!b || !b->IsVariable()) {
      b = std::make_unique<VarBinding>();
    }
    llvm::ErrorOr<VarBinding*> var = b->CastTo<VarBinding>();
    if (var.getError()) {
      return nullptr;
    }
    var.get()->SetAlloca(slot);
  }
  return init;
}

Value* Codegen::Visit(Conditional& expr) {
  Value* left = expr.left->Accept(*this);
  Value* right = expr.right->Accept(*this);

  if (!left || !right) {
    return nullptr;
  }

  switch (expr.left->type->kind) {
    case types::TypeKind::Int:
      return ctx_->CreateIntCmp(expr.op.kind, left, right);
    case types::TypeKind::Float:
      return ctx_->CreateFltCmp(expr.op.kind, left, right);
    default:
      UNREACHABLE(Codegen, VisitConditional);
  }
  return nullptr;
}

Value* Codegen::Visit(Binary& expr) {
  Value* left = expr.left->Accept(*this);
  Value* right = expr.right->Accept(*this);

  switch (expr.type->kind) {
    case types::TypeKind::Int:
      return ctx_->CreateIntBinop(expr.op.kind, left, right);
    case types::TypeKind::Float:
      return ctx_->CreateFltBinop(expr.op.kind, left, right);
    default:
      UNREACHABLE(Codegen, VisitBinary);
  }
  return nullptr;
}

Value* Codegen::Visit(PreFixOp& expr) {
  if (!expr.HasID()) {
    return nullptr;
  }

  auto symbol = ir_bindings_.find(expr.GetID());
  if (symbol == ir_bindings_.end() || !symbol->second ||
      !symbol->second->IsVariable()) {
    return nullptr;
  }

  llvm::ErrorOr<VarBinding*> bind = symbol->second->CastTo<VarBinding>();
  if (std::error_code ec = bind.getError()) {
    return nullptr;
  }

  if (!bind.get()->GetAlloca()) {
    return nullptr;
  }

  AllocaInst* a = bind.get()->GetAlloca();
  Type* type = a->getAllocatedType();
  std::string name = expr.name.lexeme;

  Value* var = ctx_->CreateLoad(type, a, name);

  return ctx_->CreatePreOp(expr.type, expr.op.kind, var, a);
}

Value* Codegen::Visit(Assign& expr) {
  auto symbol = ir_bindings_.find(expr.GetID());
  if (symbol == ir_bindings_.end() || !symbol->second ||
      !symbol->second->IsVariable()) {
    return nullptr;
  }

  llvm::ErrorOr<VarBinding*> var = symbol->second->CastTo<VarBinding>();
  if (std::error_code ec = var.getError()) {
    return nullptr;
  }

  if (!var.get()->GetAlloca()) {
    return nullptr;
  }

  Value* value = expr.value->Accept(*this);
  ctx_->CreateStore(value, var.get()->GetAlloca());
  return value;
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
  if (expr.id.has_value()) {
    auto it = ir_bindings_.find(expr.id.value());
    if (it != ir_bindings_.end()) {
      Binding* b = it->second.get();
      if (!b) {
        return nullptr;
      }
      if (b->IsFunction()) {
        llvm::ErrorOr<FuncBinding*> f = b->CastTo<FuncBinding>();
        return f.get()->function;
      }
      if (b->IsVariable()) {
        llvm::ErrorOr<VarBinding*> v = b->CastTo<VarBinding>();
        if (!v.get()->GetAlloca()) {
          return nullptr;
        }
        AllocaInst* a = v.get()->GetAlloca();
        return ctx_->GetBuilder().CreateLoad(a->getAllocatedType(), a,
                                             expr.name.lexeme);
      }
    }
  }

  // Instead of tracking a scope, we simply search the parent block and see if
  // the variable is defined there
  if (ctx_->GetBuilder().GetInsertBlock()) {
    Function* func = ctx_->GetBuilder().GetInsertBlock()->getParent();
    if (func) {
      for (auto& arg : func->args()) {
        if (arg.getName() == expr.name.lexeme) {
          return &arg;
        }
      }
    }
  }
  return nullptr;
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

llvm::Value* Codegen::EmitInteger(Literal& expr) {
  types::IntType* int_type = dynamic_cast<types::IntType*>(expr.type);
  int value = std::get<int>(expr.value);
  return ConstantInt::get(ctx_->GetContext(), APInt(int_type->bits, value));
}

static Type* ResolveStructType() {
  StructType* ty = nullptr;
  return ty;
}

llvm::Type* Codegen::ResolveType(types::Type* type, bool allow_void) {
  auto& ctx = ctx_->GetContext();

  switch (type->kind) {
    case types::TypeKind::Bool:
      return llvm::Type::getInt1Ty(ctx);
    case types::TypeKind::Int:
      return llvm::Type::getInt32Ty(ctx);
    case types::TypeKind::Float:
      return llvm::Type::getFloatTy(ctx);
    case types::TypeKind::String:
      return PointerType::getInt8Ty(ctx);
    case types::TypeKind::Void:
      return allow_void ? llvm::Type::getVoidTy(ctx) : nullptr;
    case types::TypeKind::Struct:
      return ResolveStructType();
    default:
      UNREACHABLE(CodeGen, ResolveType);
      return nullptr;
  }
}

llvm::Type* Codegen::ResolveArgType(types::Type* type) {
  return ResolveType(type, false);
}

llvm::Type* Codegen::ResolveType(types::Type* type) {
  return ResolveType(type, true);
}

// auto* s = dynamic_cast<types::StructType*>(type);
//   if (!s) return nullptr;
//   auto it = struct_types_.find(s->name);
//   llvm::StructType* llvm_struct = nullptr;
//   if (it != struct_types_.end()) {
//     llvm_struct = it->second;
//   } else {
//     llvm_struct = llvm::StructType::create(ctx_->GetContext(),
//     s->name); struct_types_[s->name] = llvm_struct;  // cache early
//   }
//   if (!llvm_struct->isOpaque()) return llvm_struct;
//   std::vector<llvm::Type*> field_types;
//   field_types.reserve(s->fields.size());
//   for (auto* f : s->fields) {
//     llvm::Type* ft = ResolveType(f);
//     if (!ft) return nullptr;
//     field_types.push_back(ft);
//   }
//   llvm_struct->setBody(field_types, /*isPacked=*/false);
//   return llvm_struct;
// }
