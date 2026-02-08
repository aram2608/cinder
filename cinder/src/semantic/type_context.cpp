#include "cinder/semantic/type_context.hpp"

#include "cinder/ast/types.hpp"

using namespace cinder;

types::IntType* TypeContext::Int32() {
  return &int32_;
}
types::IntType* TypeContext::Int64() {
  return &int64_;
}
types::FloatType* TypeContext::Float32() {
  return &flt32_;
}
types::BoolType* TypeContext::Bool() {
  return &bool_;
}
types::Type* TypeContext::Void() {
  return &void_;
}
types::StringType* TypeContext::String() {
  return &str_;
}

types::FunctionType* TypeContext::Function(types::Type* ret,
                                           std::vector<types::Type*> params,
                                           bool variadic) {
  function_pool_.push_back(
      std::make_unique<types::FunctionType>(ret, std::move(params), variadic));
  return function_pool_.back().get();
}
