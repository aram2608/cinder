#include "cinder/ast/types.hpp"

using namespace cinder;

bool types::Type::Void() {
  return kind == types::TypeKind::Void;
}

bool types::Type::Int() {
  return kind == types::TypeKind::Int;
}

bool types::Type::Float() {
  return kind == types::TypeKind::Float;
}

bool types::Type::Bool() {
  return kind == types::TypeKind::Bool;
}

bool types::Type::String() {
  return kind == types::TypeKind::String;
}

bool types::Type::Function() {
  return kind == types::TypeKind::Function;
}

bool types::Type::Struct() {
  return kind == types::TypeKind::Struct;
}

bool types::Type::IsThisType(types::Type* type) {
  return kind == type->kind;
}

bool types::Type::IsThisType(types::Type& type) {
  return kind == type.kind;
}

bool types::Type::IsThisType(types::TypeKind type) {
  return kind == type;
}

bool types::FunctionType::IsVariadic() {
  return is_variadic;
}