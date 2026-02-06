#include "../include/type_context.hpp"

#include "types.hpp"

types::Type* TypeContext::Int32() {
  return new types::IntType{32};
}

types::Type* TypeContext::Float32() {
  return new types::FloatType{32};
}

types::Type* TypeContext::Void() {
  return new types::Type{types::TypeKind::Void};
}

types::Type* TypeContext::Bool() {
  return new types::IntType{1};
}

types::Type* TypeContext::String() {
  return new types::StringType{};
}

/// Leaks memory, fix somehow, maybe an arena
types::Type* TypeContext::Function(types::Type* ret,
                                   std::vector<types::Type*> params,
                                   bool is_variadic) {
  return new types::FunctionType{ret, params, is_variadic};
}