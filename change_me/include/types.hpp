#ifndef TYPES_H_
#define TYPES_H_

#include "common.hpp"

namespace types {

enum class TypeKind {
  Void,
  Int,
  Float,
  Pointer,
  Function,
  Struct,
};

struct Type {
  TypeKind kind;
  virtual ~Type() = default;
  explicit Type(TypeKind kind) : kind(kind) {}
};

struct IntType : Type {
  unsigned int bits;
  bool is_signed;

  IntType(unsigned bits, bool is_signed)
      : Type(TypeKind::Int), bits(bits), is_signed(is_signed) {}
};

struct FloatType : Type {
  unsigned int bits;

  explicit FloatType(unsigned bits) : Type(TypeKind::Float), bits(bits) {}
};

struct PointerType : Type {
  Type* pointee;

  explicit PointerType(Type* pointee)
      : Type(TypeKind::Pointer), pointee(pointee) {}
};

struct FunctionType : Type {
  Type* return_type;
  std::vector<Type*> params;

  FunctionType(Type* ret, std::vector<Type*> params)
      : Type(TypeKind::Function), return_type(ret), params(std::move(params)) {}
};

struct StructType : Type {
  std::string name;
  std::vector<Type*> fields;

  StructType(std::string name, std::vector<Type*> fields)
      : Type(TypeKind::Struct),
        name(std::move(name)),
        fields(std::move(fields)) {}
};

}  // namespace types
#endif