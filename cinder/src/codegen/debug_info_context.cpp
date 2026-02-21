#include "cinder/codegen/debug_info_context.hpp"

using namespace llvm;
using namespace cinder;

DebugInfoContext::DebugInfoContext(llvm::LLVMContext& llvm_ctx,
                                   llvm::IRBuilder<>& builder)
    : llvm_ctx_(llvm_ctx), builder_(builder) {}

void DebugInfoContext::Init(bool debug, llvm::Module& module,
                            std::vector<ModuleStmt*>& modules) {
  if (!debug) {
    return;
  }

  module.addModuleFlag(Module::Warning, "Debug Info Version",
                       DEBUG_METADATA_VERSION);
#ifdef __APPLE__
  module.addModuleFlag(Module::Warning, "Dwarf Version", 2);
#else
  module.addModuleFlag(Module::Warning, "Dwarf Version", 4);
#endif

  di_builder_ = std::make_unique<DIBuilder>(module);

  std::string file_name = "input.ci";
  if (!modules.empty() && modules[0] && !modules[0]->name.lexeme.empty()) {
    file_name = modules[0]->name.lexeme + ".ci";
  }

  di_file_ = di_builder_->createFile(file_name, ".");
  di_compile_unit_ = di_builder_->createCompileUnit(dwarf::DW_LANG_C, di_file_,
                                                    "cinder", false, "", 0);
  di_scope_ = di_compile_unit_;
}

void DebugInfoContext::Finalize() {
  if (!di_builder_) {
    return;
  }
  di_builder_->finalize();
}

llvm::DIBuilder* DebugInfoContext::GetBuilder() {
  return di_builder_.get();
}

llvm::DIFile* DebugInfoContext::GetFile() {
  return di_file_;
}

llvm::DIScope* DebugInfoContext::GetScope() {
  return di_scope_;
}

void DebugInfoContext::SetScope(llvm::DIScope* scope) {
  di_scope_ = scope;
}

llvm::DIType* DebugInfoContext::ResolveType(cinder::types::Type* type) {
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

void DebugInfoContext::SetLocation(const cinder::SourceLocation& loc) {
  if (!di_scope_) {
    return;
  }

  unsigned line = static_cast<unsigned>(loc.line == 0 ? 1 : loc.line);
  unsigned col = static_cast<unsigned>(loc.column == 0 ? 1 : loc.column);
  auto* debug_loc = DILocation::get(llvm_ctx_, line, col, di_scope_);
  builder_.SetCurrentDebugLocation(debug_loc);
}

void DebugInfoContext::EmitValue(llvm::Value* value,
                                 llvm::DILocalVariable* variable,
                                 const cinder::SourceLocation& loc) {
  if (!di_builder_ || !di_scope_ || !value || !variable) {
    return;
  }

  unsigned line = static_cast<unsigned>(loc.line == 0 ? 1 : loc.line);
  unsigned col = static_cast<unsigned>(loc.column == 0 ? 1 : loc.column);
  auto* dl = DILocation::get(llvm_ctx_, line, col, di_scope_);
  di_builder_->insertDbgValueIntrinsic(value, variable,
                                       di_builder_->createExpression(), dl,
                                       builder_.GetInsertPoint());
}

void DebugInfoContext::CreateLexicalBlock(
    std::optional<cinder::SourceLocation> loc) {
  if (!di_builder_ || !di_scope_) {
    return;
  }

  unsigned line = 1;
  unsigned col = 1;
  if (loc) {
    line = static_cast<unsigned>(loc->line == 0 ? 1 : loc->line);
    col = static_cast<unsigned>(loc->column == 0 ? 1 : loc->column);
  }

  di_scope_ = di_builder_->createLexicalBlock(di_scope_, di_file_, line, col);
}
