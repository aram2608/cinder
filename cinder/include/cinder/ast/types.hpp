#ifndef TYPES_H_
#define TYPES_H_

#include <memory>
#include <system_error>
#include <vector>

#include "cinder/support/error_category.hpp"

namespace cinder {

namespace types {

enum class TypeKind {
  Void,
  Int,
  Float,
  Bool,
  String,
  Function,
  Struct,
};

struct IntType;
struct FloatType;
struct BoolType;
struct StructType;
struct FunctionType;
struct StructType;

struct Type {
  TypeKind kind;
  virtual ~Type() = default;
  explicit Type(TypeKind kind) : kind(kind) {}

  bool Void();
  bool Int();
  bool Float();
  bool Bool();
  bool String();
  bool Function();
  bool Struct();
  bool IsThisType(Type* type);
  bool IsThisType(Type& type);
  bool IsThisType(TypeKind type);

  template <typename T>
  T* CastTo(std::error_code& ec) {
    T* p = dynamic_cast<T*>(this);
    if (!p) {
      ec = Errors::BadCast;
      return nullptr;
    }
    return p;
  }
};

struct IntType : Type {
  unsigned int bits;
  bool is_signed;

  IntType(unsigned bits, bool is_signed = true)
      : Type(TypeKind::Int), bits(bits), is_signed(is_signed) {}
};

struct FloatType : Type {
  unsigned int bits;

  explicit FloatType(unsigned bits) : Type(TypeKind::Float), bits(bits) {}
};

struct BoolType : Type {
  unsigned int bits;

  explicit BoolType(unsigned int bits) : Type(TypeKind::Bool), bits(bits) {}
};

struct StringType : Type {
  explicit StringType() : Type(TypeKind::String) {}
};

struct FunctionType : Type {
  Type* return_type;
  std::vector<Type*> params;
  bool is_variadic;

  FunctionType(Type* ret, std::vector<Type*> params, bool is_variadic)
      : Type(TypeKind::Function),
        return_type(ret),
        params(std::move(params)),
        is_variadic(is_variadic) {}

  bool IsVariadic();
};

/// TODO: Find a way to implement this
struct StructType : Type {
  std::string name;
  std::vector<Type*> fields;

  StructType(std::string name, std::vector<Type*> fields)
      : Type(TypeKind::Struct),
        name(std::move(name)),
        fields(std::move(fields)) {}
};

}  // namespace types

}  // namespace cinder
#endif
