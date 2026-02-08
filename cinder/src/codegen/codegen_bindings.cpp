#include "cinder/codegen/codegen_bindings.hpp"

bool Binding::IsFunction() const {
  return type_ == BindType::Func;
}

bool Binding::IsVariable() const {
  return type_ == BindType::Var;
}
