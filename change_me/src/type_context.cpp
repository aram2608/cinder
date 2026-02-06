#include "../include/type_context.hpp"

#include "types.hpp"

types::Type* TypeContext::Int32() {
  return &int32;
}

types::Type* TypeContext::Float32() {
  return &float32;
}

types::Type* TypeContext::Void() {
  return &voidTy;
}

types::Type* TypeContext::Bool() {
  return &int1;
}

types::Type* TypeContext::String() {
  return &str;
}

/// Leaks memory, fix somehow, maybe an arena
types::Type* TypeContext::Function(types::Type* ret,
                                   std::vector<types::Type*> params,
                                   bool is_variadic) {
  return new types::FunctionType{ret, params, is_variadic};
}