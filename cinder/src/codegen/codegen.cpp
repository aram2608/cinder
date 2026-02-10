#include "cinder/codegen/codegen.hpp"

#include <cstdlib>
#include <memory>
#include <variant>

#include "cinder/codegen/codegen_bindings.hpp"
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
  AllocaInst* slot =
      ctx_->GetBuilder().CreateAlloca(ty, nullptr, stmt.name.lexeme);
  ctx_->GetBuilder().CreateStore(init, slot);

  if (stmt.id.has_value()) {
    std::unique_ptr<Binding>& b = ir_bindings_[stmt.id.value()];
    if (!b || !b->IsVariable()) {
      b = std::make_unique<VarBinding>();
    }
    std::error_code ec;
    VarBinding* var = b->CastTo<VarBinding>(ec);
    if (ec) {
      return nullptr;
    }
    var->alloca_ptr = slot;
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
      switch (expr.op.kind) {
        case Token::Type::BANGEQ:
          return ctx_->GetBuilder().CreateCmp(CmpInst::ICMP_NE, left, right,
                                              "cmptmp");
        case Token::Type::EQEQ:
          return ctx_->GetBuilder().CreateCmp(CmpInst::ICMP_EQ, left, right,
                                              "cmptmp");
        case Token::Type::LESSER:
          return ctx_->GetBuilder().CreateCmp(CmpInst::ICMP_SLT, left, right,
                                              "cmptmp");
        case Token::Type::LESSER_EQ:
          return ctx_->GetBuilder().CreateCmp(CmpInst::ICMP_SLE, left, right,
                                              "cmptmp");
        case Token::Type::GREATER:
          return ctx_->GetBuilder().CreateCmp(CmpInst::ICMP_SGT, left, right,
                                              "cmptmp");
        case Token::Type::GREATER_EQ:
          return ctx_->GetBuilder().CreateCmp(CmpInst::ICMP_SGE, left, right,
                                              "cmptmp");
        default:
          break;
      }
    case types::TypeKind::Float:
      switch (expr.op.kind) {
        case Token::Type::BANGEQ:
          return ctx_->GetBuilder().CreateFCmp(CmpInst::FCMP_ONE, left, right,
                                               "cmptmp");
        case Token::Type::EQEQ:
          return ctx_->GetBuilder().CreateFCmp(CmpInst::FCMP_OEQ, left, right,
                                               "cmptmp");
        case Token::Type::LESSER:
          return ctx_->GetBuilder().CreateFCmp(CmpInst::FCMP_OLT, left, right,
                                               "cmptmp");
        case Token::Type::LESSER_EQ:
          return ctx_->GetBuilder().CreateFCmp(CmpInst::FCMP_OLE, left, right,
                                               "cmptmp");
        case Token::Type::GREATER:
          return ctx_->GetBuilder().CreateFCmp(CmpInst::FCMP_OGT, left, right,
                                               "cmptmp");
        case Token::Type::GREATER_EQ:
          return ctx_->GetBuilder().CreateFCmp(CmpInst::FCMP_OGE, left, right,
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
      switch (expr.op.kind) {
        case Token::Type::PLUS:
          return ctx_->GetBuilder().CreateAdd(left, right, "addtmp");
        case Token::Type::MINUS:
          return ctx_->GetBuilder().CreateSub(left, right, "subtmp");
        case Token::Type::SLASH:
          return ctx_->GetBuilder().CreateSDiv(left, right, "divtmp");
        case Token::Type::STAR:
          return ctx_->GetBuilder().CreateMul(left, right, "multmp");
        default:
          break;
      }
    case types::TypeKind::Float:
      switch (expr.op.kind) {
        case Token::Type::PLUS:
          return ctx_->GetBuilder().CreateFAdd(left, right, "addtmp");
        case Token::Type::MINUS:
          return ctx_->GetBuilder().CreateFSub(left, right, "subtmp");
        case Token::Type::STAR:
          return ctx_->GetBuilder().CreateFMul(left, right, "multmp");
        case Token::Type::SLASH:
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
  if (!expr.id.has_value()) {
    return nullptr;
  }
  auto symbol = ir_bindings_.find(expr.id.value());
  if (symbol == ir_bindings_.end() || !symbol->second ||
      !symbol->second->IsVariable()) {
    return nullptr;
  }
  VarBinding* bind = dynamic_cast<VarBinding*>(symbol->second.get());
  if (!bind || !bind->alloca_ptr) {
    return nullptr;
  }
  Type* type = bind->alloca_ptr->getAllocatedType();
  std::string name = expr.name.lexeme;

  Value* var = ctx_->GetBuilder().CreateLoad(type, bind->alloca_ptr, name);

  Value* inc = nullptr;
  Value* value = nullptr;

  /// TODO: maybe refactor this or find a better way to match this
  switch (expr.op.kind) {
    case Token::Type::PLUS_PLUS:
      switch (expr.type->kind) {
        case types::TypeKind::Int:
          inc = ConstantInt::get(ctx_->GetContext(), APInt(32, 1));
          value = ctx_->GetBuilder().CreateAdd(var, inc, "addtmp");
          break;
        case types::TypeKind::Float:
          inc = ConstantFP::get(ctx_->GetContext(), APFloat(1.0f));
          value = ctx_->GetBuilder().CreateFAdd(var, inc, "addtmp");
          break;
        default:
          break;
      }
      break;
    case Token::Type::MINUS_MINUS:
      switch (expr.type->kind) {
        case types::TypeKind::Int:
          inc = ConstantInt::get(ctx_->GetContext(), APInt(32, 1));
          value = ctx_->GetBuilder().CreateSub(var, inc, "subtmp");
          break;
        case types::TypeKind::Float:
          inc = ConstantFP::get(ctx_->GetContext(), APFloat(1.0f));
          value = ctx_->GetBuilder().CreateFSub(var, inc, "subtmp");
          break;
        default:
          break;
      }
      break;
    default:
      break;
  }

  ctx_->GetBuilder().CreateStore(value, bind->alloca_ptr);
  return value;
}

Value* Codegen::Visit(Assign& expr) {
  auto symbol = ir_bindings_.find(expr.id.value());
  if (symbol == ir_bindings_.end() || !symbol->second ||
      !symbol->second->IsVariable()) {
    return nullptr;
  }
  VarBinding* var = dynamic_cast<VarBinding*>(symbol->second.get());
  if (!var || !var->alloca_ptr) {
    return nullptr;
  }

  Value* value = expr.value->Accept(*this);
  ctx_->GetBuilder().CreateStore(value, var->alloca_ptr);
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
        FuncBinding* f = dynamic_cast<FuncBinding*>(b);
        return f->function;
      }
      if (b->IsVariable()) {
        VarBinding* v = dynamic_cast<VarBinding*>(b);
        if (!v->alloca_ptr) {
          return nullptr;
        }
        return ctx_->GetBuilder().CreateLoad(v->alloca_ptr->getAllocatedType(),
                                             v->alloca_ptr, expr.name.lexeme);
      }
    }
  }

  // Instead of tracking a scope, we simply search the parent block and see if
  // the variable is define there
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

llvm::Type* Codegen::ResolveArgType(types::Type* type) {
  switch (type->kind) {
    case types::TypeKind::Bool:
      return Type::getInt1Ty(ctx_->GetContext());
    case types::TypeKind::Float:
      return Type::getFloatTy(ctx_->GetContext());
    case types::TypeKind::Int:
      return Type::getInt32Ty(ctx_->GetContext());
    case types::TypeKind::String:
      return PointerType::getInt8Ty(ctx_->GetContext());
    case types::TypeKind::Struct:
    case types::TypeKind::Void:
    default:
      break;
  }
  return nullptr;
}

static Type* ResolveStructType() {
  StructType* ty = nullptr;
  return ty;
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
      return PointerType::getInt8Ty(ctx_->GetContext());
    case types::TypeKind::Void:
      return Type::getVoidTy(ctx_->GetContext());
    case types::TypeKind::Struct:
      ResolveStructType();
    default:
      IMPLEMENT(ResolveType);
  }
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
