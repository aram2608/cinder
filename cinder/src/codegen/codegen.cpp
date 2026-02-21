#include "cinder/codegen/codegen.hpp"

#include <cstdlib>
#include <memory>
#include <optional>
#include <system_error>
#include <unordered_map>

#include "cinder/ast/types.hpp"
#include "cinder/codegen/codegen_bindings.hpp"
#include "cinder/support/utils.hpp"
#include "llvm/ADT/APFloat.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
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
#include "llvm/Support/Program.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Target/TargetOptions.h"
#include "llvm/TargetParser/Host.h"
#include "llvm/TargetParser/Triple.h"
#include "llvm/Transforms/Scalar.h"

/// TODO: Refactor some of the functions in this file, a bit redundant and
/// sloppy

using namespace llvm;
using namespace cinder;
static ExitOnError ExitOnErr;

static ostream::RawOutStream errors{2};

namespace {

std::optional<cinder::SourceLocation> ExprLocation(const Expr* expr) {
  if (!expr) {
    return std::nullopt;
  }

  if (const auto* v = dynamic_cast<const Variable*>(expr)) {
    return v->name.location;
  }
  if (const auto* m = dynamic_cast<const MemberAccess*>(expr)) {
    return m->member.location;
  }
  if (const auto* p = dynamic_cast<const PreFixOp*>(expr)) {
    return p->op.location;
  }
  if (const auto* b = dynamic_cast<const Binary*>(expr)) {
    return b->op.location;
  }
  if (const auto* c = dynamic_cast<const Conditional*>(expr)) {
    return c->op.location;
  }
  if (const auto* a = dynamic_cast<const Assign*>(expr)) {
    return a->name.location;
  }
  if (const auto* ma = dynamic_cast<const MemberAssign*>(expr)) {
    return ma->target ? std::optional<cinder::SourceLocation>(
                            ma->target->member.location)
                      : std::nullopt;
  }
  if (const auto* g = dynamic_cast<const Grouping*>(expr)) {
    return ExprLocation(g->expr.get());
  }
  if (const auto* call = dynamic_cast<const CallExpr*>(expr)) {
    return ExprLocation(call->callee.get());
  }

  return std::nullopt;
}

}  // namespace

#ifdef __APPLE__
static bool GenerateDsymBundle(const std::string& binary_path) {
  llvm::ErrorOr<std::string> dsymutil =
      llvm::sys::findProgramByName("dsymutil");
  if (!dsymutil) {
    return false;
  }

  llvm::SmallVector<llvm::StringRef, 4> args;
  args.push_back(*dsymutil);
  args.push_back(binary_path);
  args.push_back("-o");
  args.push_back(binary_path + ".dSYM");

  int rc = llvm::sys::ExecuteAndWait(*dsymutil, args);
  return rc == 0;
}
#endif

Codegen::Codegen(std::vector<ModuleStmt*> modules, CodegenOpts opts)
    : modules_(std::move(modules)),
      opts(opts),
      ctx_(std::make_unique<CodegenContext>(this->opts.out_path)),
      pass_(types_) {}

bool Codegen::Generate() {
  InitAllTargets();

  if (!SemanticPass(modules_)) {
    return false;
  }

  InitDebugInfo();
  GenerateIR();
  FinalizeDebugInfo();

  std::string target_trip = sys::getDefaultTargetTriple();
  ctx_->SetTargetTriple(Triple(target_trip));

  std::string Error;
  auto target = ctx_->LookupTarget();

  if (!target) {
    std::cout << "Failed to create target";
    return false;
  }

  TargetOptions gen_opt;
  auto target_machine = ctx_->CreateTargetMachine(target, target_trip);

  ctx_->SetModDataLayout(target_machine);

  switch (opts.mode) {
    case CodegenOpts::Opt::COMPILE:
      CompileBinary(target_machine);
      return true;
    case CodegenOpts::Opt::EMIT_LLVM:
      EmitLLVM();
      return true;
    case CodegenOpts::Opt::RUN:
      CompileRun();
      return false;
    default:
      UNREACHABLE(COMPILER_MODE, "Unknown compile type");
  }
}

void Codegen::InitAllTargets() {
  InitializeAllTargetInfos();
  InitializeAllTargets();
  InitializeAllTargetMCs();
  InitializeAllAsmParsers();
  InitializeAllAsmPrinters();
}

void Codegen::InitDebugInfo() {
  if (!opts.debug_info) {
    return;
  }

  auto& module = ctx_->GetModule();
  module.addModuleFlag(Module::Warning, "Debug Info Version",
                       DEBUG_METADATA_VERSION);
#ifdef __APPLE__
  module.addModuleFlag(Module::Warning, "Dwarf Version", 2);
#else
  module.addModuleFlag(Module::Warning, "Dwarf Version", 4);
#endif

  di_builder_ = std::make_unique<DIBuilder>(module);

  std::string file_name = "input.ci";
  if (!modules_.empty() && modules_[0] && !modules_[0]->name.lexeme.empty()) {
    file_name = modules_[0]->name.lexeme + ".ci";
  }

  di_file_ = di_builder_->createFile(file_name, ".");
  di_compile_unit_ = di_builder_->createCompileUnit(dwarf::DW_LANG_C, di_file_,
                                                    "cinder", false, "", 0);
  di_scope_ = di_compile_unit_;
}

void Codegen::FinalizeDebugInfo() {
  if (!di_builder_) {
    return;
  }
  di_builder_->finalize();
}

void Codegen::SetDebugLocation(const cinder::SourceLocation& loc) {
  if (!opts.debug_info || !di_scope_) {
    return;
  }

  unsigned line = static_cast<unsigned>(loc.line == 0 ? 1 : loc.line);
  unsigned col = static_cast<unsigned>(loc.column == 0 ? 1 : loc.column);
  auto* debug_loc = DILocation::get(ctx_->GetContext(), line, col, di_scope_);
  ctx_->GetBuilder().SetCurrentDebugLocation(debug_loc);
}

void Codegen::EmitDbgValue(llvm::Value* value, llvm::DILocalVariable* variable,
                           const cinder::SourceLocation& loc) {
  if (!opts.debug_info || !di_builder_ || !di_scope_ || !value || !variable) {
    return;
  }

  unsigned line = static_cast<unsigned>(loc.line == 0 ? 1 : loc.line);
  unsigned col = static_cast<unsigned>(loc.column == 0 ? 1 : loc.column);
  auto* dl = DILocation::get(ctx_->GetContext(), line, col, di_scope_);
  di_builder_->insertDbgValueIntrinsic(value, variable,
                                       di_builder_->createExpression(), dl,
                                       ctx_->GetBuilder().GetInsertPoint());
}

DIType* Codegen::ResolveDebugType(cinder::types::Type* type) {
  if (!di_builder_) {
    return nullptr;
  }

  if (!type) {
    return di_builder_->createUnspecifiedType("auto");
  }

  switch (type->kind) {
    case types::TypeKind::Bool:
      return di_builder_->createBasicType("bool", 1, dwarf::DW_ATE_boolean);
    case types::TypeKind::Int: {
      auto* i = dynamic_cast<types::IntType*>(type);
      uint64_t bits = i ? i->bits : 32;
      unsigned encoding =
          (i && !i->is_signed) ? dwarf::DW_ATE_unsigned : dwarf::DW_ATE_signed;
      return di_builder_->createBasicType("int", bits, encoding);
    }
    case types::TypeKind::Float: {
      auto* f = dynamic_cast<types::FloatType*>(type);
      uint64_t bits = f ? f->bits : 32;
      return di_builder_->createBasicType("float", bits, dwarf::DW_ATE_float);
    }
    case types::TypeKind::String: {
      DIType* char_ty =
          di_builder_->createBasicType("char", 8, dwarf::DW_ATE_signed_char);
      return di_builder_->createPointerType(char_ty, 64);
    }
    case types::TypeKind::Struct: {
      auto* s = dynamic_cast<types::StructType*>(type);
      return di_builder_->createUnspecifiedType(s ? s->name : "struct");
    }
    case types::TypeKind::Void:
    case types::TypeKind::Function:
    default:
      return di_builder_->createUnspecifiedType("void");
  }
}

void Codegen::GenerateIR() {
  for (ModuleStmt* mod : modules_) {
    if (!mod) {
      continue;
    }
    mod->Accept(*this);
  }
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

  bool keep_temp_object = false;
  bool ok = ClangDriver::LinkObject(temp, opts.out_path, opts.linker_flags);
  if (!ok) {
    ostream::ErrorOutln(errors, "clang driver link step failed");
    keep_temp_object = true;
  } else if (opts.debug_info) {
#ifdef __APPLE__
    if (!GenerateDsymBundle(opts.out_path)) {
      ostream::ErrorOutln(errors,
                          "dsymutil not available or failed; keeping object "
                          "file for debug map");
      keep_temp_object = true;
    }
#endif
  }

  if (!keep_temp_object) {
    auto err = sys::fs::remove(temp);
    if (err) {
      diagnose_.Error({0}, err.message());
    }
  }
}

bool Codegen::SemanticPass(const std::vector<ModuleStmt*>& modules) {
  pass_.AnalyzeProgram(modules);
  if (pass_.HadError()) {
    pass_.DumpErrors();
    return false;
  }
  return true;
}

Value* Codegen::Visit(ModuleStmt& stmt) {
  for (auto& module_stmt : stmt.stmts) {
    if (module_stmt->IsImport()) {
      continue;
    }
    module_stmt->Accept(*this);
  }

  return nullptr;
}

Value* Codegen::Visit(ImportStmt& stmt) {
  return nullptr;
}

Value* Codegen::Visit(StructStmt& stmt) {
  return nullptr;
}

Value* Codegen::Visit(ExpressionStmt& stmt) {
  if (auto loc = ExprLocation(stmt.expr.get())) {
    SetDebugLocation(*loc);
  }
  stmt.expr->Accept(*this);
  return nullptr;
}

Value* Codegen::Visit(WhileStmt& stmt) {
  if (auto loc = ExprLocation(stmt.condition.get())) {
    SetDebugLocation(*loc);
  }
  Function* func = ctx_->GetInsertBlockParent();

  BasicBlock* cond_block = ctx_->CreateBasicBlock("loop.cond", func);
  BasicBlock* loop_block = ctx_->CreateBasicBlock("loop.body", func);
  BasicBlock* after_block = ctx_->CreateBasicBlock("loop.body", func);

  ctx_->CreateBr(cond_block);
  ctx_->SetInsertPoint(cond_block);

  Value* condition = stmt.condition->Accept(*this);

  ctx_->CreateBasicCondBr(condition, loop_block, after_block);
  ctx_->SetInsertPoint(loop_block);
  DIScope* previous_scope = di_scope_;
  if (opts.debug_info && di_builder_ && di_scope_) {
    unsigned line = 1;
    unsigned col = 1;
    if (auto loc = ExprLocation(stmt.condition.get())) {
      line = static_cast<unsigned>(loc->line == 0 ? 1 : loc->line);
      col = static_cast<unsigned>(loc->column == 0 ? 1 : loc->column);
    }
    di_scope_ = di_builder_->createLexicalBlock(di_scope_, di_file_, line, col);
  }
  for (auto& body_stmt : stmt.body) {
    body_stmt->Accept(*this);
  }
  di_scope_ = previous_scope;

  ctx_->CreateBr(cond_block);
  ctx_->SetInsertPoint(after_block);
  return nullptr;
}

Value* Codegen::Visit(ForStmt& stmt) {
  if (stmt.initializer) {
    stmt.initializer->Accept(*this);
  }

  if (stmt.condition) {
    if (auto loc = ExprLocation(stmt.condition.get())) {
      SetDebugLocation(*loc);
    }
  }

  Function* func = ctx_->GetBuilder().GetInsertBlock()->getParent();
  BasicBlock* cond_block = ctx_->CreateBasicBlock("loop.cond", func);
  BasicBlock* loop_block = ctx_->CreateBasicBlock("loop.body", func);
  BasicBlock* step_block = ctx_->CreateBasicBlock("loop.step", func);
  BasicBlock* after_block = ctx_->CreateBasicBlock("loop.end", func);

  ctx_->CreateBr(cond_block);
  ctx_->SetInsertPoint(cond_block);

  Value* condition = stmt.condition->Accept(*this);

  ctx_->CreateBasicCondBr(condition, loop_block, after_block);
  ctx_->SetInsertPoint(loop_block);

  DIScope* previous_scope = di_scope_;
  if (opts.debug_info && di_builder_ && di_scope_) {
    unsigned line = 1;
    unsigned col = 1;
    if (auto loc = ExprLocation(stmt.condition.get())) {
      line = static_cast<unsigned>(loc->line == 0 ? 1 : loc->line);
      col = static_cast<unsigned>(loc->column == 0 ? 1 : loc->column);
    }
    di_scope_ = di_builder_->createLexicalBlock(di_scope_, di_file_, line, col);
  }
  for (auto& body_stmt : stmt.body) {
    body_stmt->Accept(*this);
  }
  di_scope_ = previous_scope;

  ctx_->CreateBr(step_block);
  ctx_->SetInsertPoint(step_block);

  if (stmt.step) {
    if (auto loc = ExprLocation(stmt.step.get())) {
      SetDebugLocation(*loc);
    }
    stmt.step->Accept(*this);
  }

  ctx_->CreateBr(cond_block);
  ctx_->SetInsertPoint(after_block);
  return nullptr;
}

Value* Codegen::Visit(IfStmt& stmt) {
  if (auto loc = ExprLocation(stmt.cond.get())) {
    SetDebugLocation(*loc);
  }
  Value* condition = stmt.cond->Accept(*this);

  Function* Func = ctx_->GetBuilder().GetInsertBlock()->getParent();

  BasicBlock* then_block = ctx_->CreateBasicBlock("if.then", Func);
  BasicBlock* merge = ctx_->CreateBasicBlock("if.cont");
  BasicBlock* else_block = merge;

  if (stmt.otherwise) {
    else_block = BasicBlock::Create(ctx_->GetContext(), "if.else");
  }

  ctx_->CreateBasicCondBr(condition, then_block, else_block);
  ctx_->SetInsertPoint(then_block);

  DIScope* previous_scope = di_scope_;
  if (opts.debug_info && di_builder_ && di_scope_) {
    unsigned line = 1;
    unsigned col = 1;
    if (auto loc = ExprLocation(stmt.cond.get())) {
      line = static_cast<unsigned>(loc->line == 0 ? 1 : loc->line);
      col = static_cast<unsigned>(loc->column == 0 ? 1 : loc->column);
    }
    di_scope_ = di_builder_->createLexicalBlock(di_scope_, di_file_, line, col);
  }

  /// TODO: Add multiple statements in the body of the then and else branches
  /// We do not have block statements so passing a bunch of stuff at once
  /// probably needs a vector for now
  stmt.then->Accept(*this);

  if (!ctx_->GetInsertBlockTerminator()) {
    ctx_->CreateBr(merge);
  }

  then_block = ctx_->GetInsertBlock();
  di_scope_ = previous_scope;

  if (stmt.otherwise) {
    Func->insert(Func->end(), else_block);
    ctx_->SetInsertPoint(else_block);

    if (opts.debug_info && di_builder_ && previous_scope) {
      unsigned line = 1;
      unsigned col = 1;
      if (auto loc = ExprLocation(stmt.cond.get())) {
        line = static_cast<unsigned>(loc->line == 0 ? 1 : loc->line);
        col = static_cast<unsigned>(loc->column == 0 ? 1 : loc->column);
      }
      di_scope_ =
          di_builder_->createLexicalBlock(previous_scope, di_file_, line, col);
    }

    stmt.otherwise->Accept(*this);

    if (!ctx_->GetInsertBlockTerminator()) {
      ctx_->CreateBr(merge);
    }
    else_block = ctx_->GetInsertBlock();
    di_scope_ = previous_scope;
  }

  Func->insert(Func->end(), merge);
  ctx_->SetInsertPoint(merge);
  return nullptr;
}

Value* Codegen::Visit(FunctionStmt& stmt) {
  auto* proto_stmt = dynamic_cast<FunctionProto*>(stmt.proto.get());
  Function* func = dyn_cast<Function>(stmt.proto->Accept(*this));
  BasicBlock* entry = ctx_->CreateBasicBlock("entry", func);
  ctx_->SetInsertPoint(entry);
  di_locals_.clear();

  DIScope* previous_scope = di_scope_;
  if (opts.debug_info && di_builder_ && di_file_) {
    auto debug_type_from_token = [&](const Token& tok) -> DIType* {
      switch (tok.kind) {
        case Token::Type::BOOL_SPECIFIER:
          return di_builder_->createBasicType("bool", 1, dwarf::DW_ATE_boolean);
        case Token::Type::INT32_SPECIFIER:
          return di_builder_->createBasicType("int", 32, dwarf::DW_ATE_signed);
        case Token::Type::INT64_SPECIFIER:
          return di_builder_->createBasicType("int", 64, dwarf::DW_ATE_signed);
        case Token::Type::FLT32_SPECIFIER:
          return di_builder_->createBasicType("float", 32, dwarf::DW_ATE_float);
        case Token::Type::FLT64_SPECIFIER:
          return di_builder_->createBasicType("double", 64,
                                              dwarf::DW_ATE_float);
        case Token::Type::STR_SPECIFIER: {
          auto* char_ty = di_builder_->createBasicType(
              "char", 8, dwarf::DW_ATE_signed_char);
          return di_builder_->createPointerType(char_ty, 64);
        }
        case Token::Type::VOID_SPECIFIER:
        default:
          return di_builder_->createUnspecifiedType("void");
      }
    };

    SmallVector<Metadata*, 8> param_types;
    if (proto_stmt) {
      param_types.push_back(debug_type_from_token(proto_stmt->return_type));
      for (const auto& arg : proto_stmt->args) {
        param_types.push_back(ResolveDebugType(arg.resolved_type));
      }
    } else {
      param_types.push_back(di_builder_->createUnspecifiedType("void"));
    }

    auto* subroutine = di_builder_->createSubroutineType(
        di_builder_->getOrCreateTypeArray(ArrayRef<Metadata*>(param_types)));
    unsigned line = 1;
    if (proto_stmt) {
      line = static_cast<unsigned>(proto_stmt->name.location.line == 0
                                       ? 1
                                       : proto_stmt->name.location.line);
    }

    auto* subprogram = di_builder_->createFunction(
        di_file_, func->getName(), func->getName(), di_file_, line, subroutine,
        line, DINode::FlagPrototyped, DISubprogram::SPFlagDefinition);
    func->setSubprogram(subprogram);
    di_scope_ = subprogram;
    if (proto_stmt) {
      SetDebugLocation(proto_stmt->name.location);

      size_t arg_index = 0;
      for (auto& arg : func->args()) {
        if (arg_index >= proto_stmt->args.size()) {
          break;
        }

        const FuncArg& arg_meta = proto_stmt->args[arg_index];
        unsigned line =
            static_cast<unsigned>(arg_meta.identifier.location.line == 0
                                      ? 1
                                      : arg_meta.identifier.location.line);
        unsigned col =
            static_cast<unsigned>(arg_meta.identifier.location.column == 0
                                      ? 1
                                      : arg_meta.identifier.location.column);

        auto* dbg_param = di_builder_->createParameterVariable(
            di_scope_, arg_meta.identifier.lexeme,
            static_cast<unsigned>(arg_index + 1), di_file_, line,
            ResolveDebugType(arg_meta.resolved_type), true);
        auto* dl = DILocation::get(ctx_->GetContext(), line, col, di_scope_);
        di_builder_->insertDbgValueIntrinsic(
            &arg, dbg_param, di_builder_->createExpression(), dl, entry);
        ++arg_index;
      }
    }
  }

  for (auto& body_stmt : stmt.body) {
    body_stmt->Accept(*this);
  }

  /// TODO: Catch malformed non void fuctions
  /// Right now it seg faults when a return is not provided
  if (!ctx_->GetInsertBlockTerminator()) {
    if (func->getReturnType()->isVoidTy()) {
      ctx_->CreateVoidReturn();
    }
  }

  verifyFunction(*func);
  di_scope_ = previous_scope;
  di_locals_.clear();
  return func;
}

Value* Codegen::Visit(FunctionProto& stmt) {
  SetDebugLocation(stmt.name.location);
  Type* ret_type = ctx_->CreateTypeFromToken(stmt.return_type);

  std::vector<Type*> arg_types;
  arg_types.reserve(stmt.args.size());
  for (auto& arg : stmt.args) {
    arg_types.push_back(ResolveArgType(arg.resolved_type));
  }

  FunctionType* func_type =
      ctx_->GetFuncType(ret_type, arg_types, stmt.is_variadic);

  Function* func = ctx_->CreatePublicFunc(func_type, stmt.name.lexeme);

  size_t idx = 0;
  for (auto& arg : func->args()) {
    arg.setName(stmt.args[idx].identifier.lexeme);
    ++idx;
  }

  if (stmt.id.has_value()) {
    std::unique_ptr<Binding>& b = ir_bindings_[stmt.GetID()];
    if (!b || !b->IsFunction()) {
      b = std::make_unique<FuncBinding>();
    }
    auto f = b->CastTo<FuncBinding>();
    if (std::error_code ec = f.getError()) {
      return nullptr;
    }
    f.get()->function = func;
  }
  return func;
}

Value* Codegen::Visit(ReturnStmt& stmt) {
  SetDebugLocation(stmt.ret_token.location);
  if (stmt.value->type->Void()) {
    return ctx_->CreateVoidReturn();
  }
  Value* ret = stmt.value->Accept(*this);
  return ctx_->CreateReturn(ret);
  return nullptr;
}

Value* Codegen::Visit(VarDeclarationStmt& stmt) {
  SetDebugLocation(stmt.name.location);
  Value* init = stmt.value->Accept(*this);
  Type* ty = ResolveType(stmt.value->type);
  AllocaInst* slot = ctx_->CreateAlloca(ty, nullptr, stmt.name.lexeme);
  ctx_->CreateStore(init, slot);

  if (opts.debug_info && di_builder_ && di_scope_) {
    unsigned line = static_cast<unsigned>(
        stmt.name.location.line == 0 ? 1 : stmt.name.location.line);
    unsigned col = static_cast<unsigned>(
        stmt.name.location.column == 0 ? 1 : stmt.name.location.column);
    auto* variable = di_builder_->createAutoVariable(
        di_scope_, stmt.name.lexeme, di_file_, line,
        ResolveDebugType(stmt.value->type));
    di_builder_->insertDeclare(
        slot, variable, di_builder_->createExpression(),
        DILocation::get(ctx_->GetContext(), line, col, di_scope_),
        ctx_->GetBuilder().GetInsertBlock());

    if (stmt.HasID()) {
      di_locals_[stmt.GetID()] = variable;
    }
    EmitDbgValue(init, variable, stmt.name.location);
  }

  if (stmt.HasID()) {
    std::unique_ptr<Binding>& b = ir_bindings_[stmt.GetID()];
    if (!b || !b->IsVariable()) {
      b = std::make_unique<VarBinding>();
    }
    ErrorOr<VarBinding*> var = b->CastTo<VarBinding>();
    if (var.getError()) {
      return nullptr;
    }
    var.get()->SetAlloca(slot);
  }
  return init;
}

Value* Codegen::Visit(Conditional& expr) {
  SetDebugLocation(expr.op.location);
  Value* left = expr.left->Accept(*this);
  Value* right = expr.right->Accept(*this);

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
  SetDebugLocation(expr.op.location);
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
  SetDebugLocation(expr.op.location);
  auto symbol = ir_bindings_.find(expr.GetID());
  if (symbol == ir_bindings_.end() || !symbol->second ||
      !symbol->second->IsVariable()) {
    return nullptr;
  }

  ErrorOr<VarBinding*> bind = symbol->second->CastTo<VarBinding>();
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
  Value* result = ctx_->CreatePreOp(expr.type, expr.op.kind, var, a);
  if (expr.HasID()) {
    auto it = di_locals_.find(expr.GetID());
    if (it != di_locals_.end()) {
      EmitDbgValue(result, it->second, expr.name.location);
    }
  }

  return result;
}

Value* Codegen::Visit(Assign& expr) {
  SetDebugLocation(expr.name.location);
  auto symbol = ir_bindings_.find(expr.GetID());
  if (symbol == ir_bindings_.end() || !symbol->second->IsVariable()) {
    return nullptr;
  }

  ErrorOr<VarBinding*> var = symbol->second->CastTo<VarBinding>();
  if (std::error_code ec = var.getError()) {
    return nullptr;
  }

  if (!var.get()->GetAlloca()) {
    return nullptr;
  }

  Value* value = expr.value->Accept(*this);
  ctx_->CreateStore(value, var.get()->GetAlloca());
  if (expr.HasID()) {
    auto it = di_locals_.find(expr.GetID());
    if (it != di_locals_.end()) {
      EmitDbgValue(value, it->second, expr.name.location);
    }
  }
  return value;
}

Value* Codegen::Visit(MemberAssign& expr) {
  SetDebugLocation(expr.target->member.location);
  if (!expr.base_id.has_value() || !expr.target->field_index.has_value()) {
    return nullptr;
  }

  auto symbol = ir_bindings_.find(expr.base_id.value());
  if (symbol == ir_bindings_.end() || !symbol->second ||
      !symbol->second->IsVariable()) {
    return nullptr;
  }

  ErrorOr<VarBinding*> var = symbol->second->CastTo<VarBinding>();
  if (std::error_code ec = var.getError()) {
    return nullptr;
  }

  AllocaInst* slot = var.get()->GetAlloca();
  if (!slot) {
    return nullptr;
  }

  Value* rhs = expr.value->Accept(*this);
  if (!rhs) {
    return nullptr;
  }

  Value* current =
      ctx_->CreateLoad(slot->getAllocatedType(), slot, "struct.assign.current");
  Value* updated = ctx_->GetBuilder().CreateInsertValue(
      current, rhs, {static_cast<unsigned>(expr.target->field_index.value())},
      "struct.assign.updated");
  ctx_->CreateStore(updated, slot);
  return rhs;
}

Value* Codegen::Visit(CallExpr& expr) {
  if (auto loc = ExprLocation(expr.callee.get())) {
    SetDebugLocation(*loc);
  }
  if (expr.type->kind == types::TypeKind::Struct) {
    std::error_code ec;
    expr.type->CastTo<types::StructType>(ec);
    if (ec) {
      return nullptr;
    }

    Type* llvm_struct_ty = ResolveType(expr.type);
    if (!llvm_struct_ty || !llvm::isa<llvm::StructType>(llvm_struct_ty)) {
      return nullptr;
    }

    Value* aggregate = UndefValue::get(llvm_struct_ty);
    for (size_t i = 0; i < expr.args.size(); ++i) {
      Value* arg_value = expr.args[i]->Accept(*this);
      aggregate = ctx_->GetBuilder().CreateInsertValue(
          aggregate, arg_value, {static_cast<unsigned>(i)});
    }
    return aggregate;
  }

  Function* callee = dyn_cast<Function>(expr.callee->Accept(*this));
  if (!callee) {
    return nullptr;
  }

  std::vector<Value*> call_args;
  for (auto& arg : expr.args) {
    call_args.push_back(arg->Accept(*this));
  }

  if (expr.type->kind == types::TypeKind::Void) {
    return ctx_->CreateVoidCall(callee, call_args);
  }

  return ctx_->CreateCall(callee, call_args, callee->getName());
}

Value* Codegen::Visit(MemberAccess& expr) {
  SetDebugLocation(expr.member.location);
  if (expr.field_index.has_value()) {
    Value* object = expr.object->Accept(*this);
    if (!object) {
      return nullptr;
    }

    std::error_code ec;
    auto* struct_ty = expr.object->type->CastTo<types::StructType>(ec);
    if (ec) {
      return nullptr;
    }

    Type* llvm_struct_ty = ResolveType(struct_ty);
    if (!llvm_struct_ty || !llvm::isa<llvm::StructType>(llvm_struct_ty)) {
      return nullptr;
    }

    return ctx_->GetBuilder().CreateExtractValue(
        object, {static_cast<unsigned>(expr.field_index.value())},
        struct_ty->field_names[expr.field_index.value()]);
  }

  if (!expr.HasID()) {
    return nullptr;
  }

  auto it = ir_bindings_.find(expr.GetID());
  if (it == ir_bindings_.end() || !it->second || !it->second->IsFunction()) {
    return nullptr;
  }

  ErrorOr<FuncBinding*> f = it->second->CastTo<FuncBinding>();
  if (std::error_code ec = f.getError()) {
    return nullptr;
  }

  return f.get()->function;
}

Value* Codegen::Visit(Grouping& expr) {
  if (auto loc = ExprLocation(expr.expr.get())) {
    SetDebugLocation(*loc);
  }
  return expr.expr->Accept(*this);
}

Value* Codegen::Visit(Variable& expr) {
  SetDebugLocation(expr.name.location);
  if (expr.HasID()) {
    auto it = ir_bindings_.find(expr.id.value());
    if (it != ir_bindings_.end()) {
      Binding* b = it->second.get();
      if (!b) {
        return nullptr;
      }
      if (b->IsFunction()) {
        ErrorOr<FuncBinding*> f = b->CastTo<FuncBinding>();
        return f.get()->function;
      }
      if (b->IsVariable()) {
        ErrorOr<VarBinding*> v = b->CastTo<VarBinding>();
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
  if (ctx_->GetInsertBlock()) {
    Function* func = ctx_->GetInsertBlockParent();
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

Value* Codegen::EmitInteger(Literal& expr) {
  types::IntType* int_type = dynamic_cast<types::IntType*>(expr.type);
  int value = std::get<int>(expr.value);
  return ConstantInt::get(ctx_->GetContext(), APInt(int_type->bits, value));
}

static Type* ResolveStructType() {
  return nullptr;
}

Type* Codegen::ResolveType(types::Type* type, bool allow_void) {
  auto& ctx = ctx_->GetContext();

  switch (type->kind) {
    case types::TypeKind::Bool:
      return Type::getInt1Ty(ctx);
    case types::TypeKind::Int:
      return Type::getInt32Ty(ctx);
    case types::TypeKind::Float:
      return Type::getFloatTy(ctx);
    case types::TypeKind::String:
      return PointerType::getInt8Ty(ctx);
    case types::TypeKind::Void:
      return allow_void ? Type::getVoidTy(ctx) : nullptr;
    case types::TypeKind::Struct:
      break;
    default:
      UNREACHABLE(CodeGen, ResolveType);
      return nullptr;
  }

  auto* s = dynamic_cast<types::StructType*>(type);
  if (!s) {
    return ResolveStructType();
  }

  auto it = struct_types_.find(s->name);
  llvm::StructType* llvm_struct = nullptr;
  if (it != struct_types_.end()) {
    llvm_struct = it->second;
  } else {
    llvm_struct = llvm::StructType::create(ctx_->GetContext(), s->name);
    struct_types_[s->name] = llvm_struct;
  }

  if (!llvm_struct->isOpaque()) {
    return llvm_struct;
  }

  std::vector<llvm::Type*> field_types;
  field_types.reserve(s->fields.size());
  for (auto* f : s->fields) {
    llvm::Type* ft = ResolveType(f, false);
    if (!ft) {
      return nullptr;
    }
    field_types.push_back(ft);
  }
  llvm_struct->setBody(field_types, false);
  return llvm_struct;
}

Type* Codegen::ResolveArgType(types::Type* type) {
  return ResolveType(type, false);
}

Type* Codegen::ResolveType(types::Type* type) {
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
