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
  if (!type || kind != type->kind) {
    return false;
  }

  if (kind == types::TypeKind::Struct) {
    auto lhs = CastTo<types::StructType>();
    auto rhs = type->CastTo<types::StructType>();
    if (lhs.getError() || rhs.getError()) {
      return false;
    }
    return lhs.get()->name == rhs.get()->name;
  }

  return true;
}

bool types::Type::IsThisType(types::Type& type) {
  return IsThisType(&type);
}

bool types::Type::IsThisType(types::TypeKind type) {
  return kind == type;
}

bool types::FunctionType::IsVariadic() {
  return is_variadic;
}

int types::StructType::FieldIndex(const std::string& field) const {
  for (size_t i = 0; i < field_names.size(); ++i) {
    if (field_names[i] == field) {
      return static_cast<int>(i);
    }
  }
  return -1;
}
