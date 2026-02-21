#ifndef TYPES_H_
#define TYPES_H_

#include <memory>
#include <string>
#include <system_error>
#include <vector>

#include "cinder/support/error_category.hpp"
#include "llvm/Support/ErrorOr.h"

namespace cinder {

namespace types {

/** @brief Discriminant for all semantic types in the compiler. */
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

/** @brief Base class for all semantic type descriptors. */
struct Type {
  TypeKind kind;
  virtual ~Type() = default;
  explicit Type(TypeKind kind) : kind(kind) {}

  /** @brief Returns whether this is `TypeKind::Void`. */
  bool Void();
  /** @brief Returns whether this is `TypeKind::Int`. */
  bool Int();
  /** @brief Returns whether this is `TypeKind::Float`. */
  bool Float();
  /** @brief Returns whether this is `TypeKind::Bool`. */
  bool Bool();
  /** @brief Returns whether this is `TypeKind::String`. */
  bool String();
  /** @brief Returns whether this is `TypeKind::Function`. */
  bool Function();
  /** @brief Returns whether this is `TypeKind::Struct`. */
  bool Struct();
  /** @brief Returns whether this and `type` have the same `TypeKind`. */
  bool IsThisType(Type* type);
  /** @brief Returns whether this and `type` have the same `TypeKind`. */
  bool IsThisType(Type& type);
  /** @brief Returns whether this has exactly `type` kind. */
  bool IsThisType(TypeKind type);

  template <typename T>
  llvm::ErrorOr<T*> CastTo() {
    T* p = dynamic_cast<T*>(this);
    if (!p) {
      return make_error_code(Errors::BadCast);
    }
    return p;
  }

  /**
   * @brief Dynamically casts this type to `T` with error-code reporting.
   * @tparam T Target type node.
   * @param ec Set to `Errors::BadCast` on failure.
   * @return Pointer to `T` on success; otherwise `nullptr`.
   */
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

/** @brief Integer type descriptor. */
struct IntType : Type {
  unsigned int bits; /**< Bit width (for example 32 or 64). */
  bool is_signed;    /**< Signedness flag. */

  IntType(unsigned bits, bool is_signed = true)
      : Type(TypeKind::Int), bits(bits), is_signed(is_signed) {}
};

/** @brief Floating-point type descriptor. */
struct FloatType : Type {
  unsigned int bits; /**< Bit width (for example 32 or 64). */

  explicit FloatType(unsigned bits) : Type(TypeKind::Float), bits(bits) {}
};

/** @brief Boolean type descriptor. */
struct BoolType : Type {
  unsigned int bits; /**< Storage width in bits. */

  explicit BoolType(unsigned int bits) : Type(TypeKind::Bool), bits(bits) {}
};

/** @brief String type descriptor. */
struct StringType : Type {
  explicit StringType() : Type(TypeKind::String) {}
};

/** @brief Function type descriptor containing signature metadata. */
struct FunctionType : Type {
  Type* return_type;         /**< Function return type. */
  std::vector<Type*> params; /**< Ordered fixed parameter types. */
  bool is_variadic;          /**< Whether additional varargs are accepted. */

  FunctionType(Type* ret, std::vector<Type*> params, bool is_variadic)
      : Type(TypeKind::Function),
        return_type(ret),
        params(std::move(params)),
        is_variadic(is_variadic) {}

  /** @brief Returns whether this function type is variadic. */
  bool IsVariadic();
};

/** @brief Struct type descriptor (currently minimal metadata only). */
struct StructType : Type {
  std::string name;                     /**< Struct name. */
  std::vector<std::string> field_names; /**< Field names in order. */
  std::vector<Type*> fields; /**< Field types in declaration order. */

  StructType(std::string name, std::vector<std::string> field_names,
             std::vector<Type*> fields)
      : Type(TypeKind::Struct),
        name(std::move(name)),
        field_names(std::move(field_names)),
        fields(std::move(fields)) {}

  int FieldIndex(const std::string& field) const;
};

}  // namespace types

}  // namespace cinder
#endif
