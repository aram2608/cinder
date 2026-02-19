#include "cinder/codegen/codegen_bindings.hpp"

bool Binding::IsFunction() const {
  return type_ == BindType::Func;
}

bool Binding::IsVariable() const {
  return type_ == BindType::Var;
}

llvm::AllocaInst* VarBinding::GetAlloca() {
  return alloca_ptr;
}

void VarBinding::SetAlloca(llvm::AllocaInst* alloca) {
  alloca_ptr = alloca;
}