#ifndef TYPE_CONTEXT_H_
#define TYPE_CONTEXT_H_

#include "types.hpp"

struct TypeContext {
  types::IntType int32{32};
  types::FloatType float32{32};
  types::Type voidTy{types::TypeKind::Void};
  types::BoolType int1{1};

  /// TODO: Extend types
  /// StructType

  types::Type* Int32();
  types::Type* Float32();
  types::Type* Void();
  types::Type* Bool();
  /// TODO: This is kinda funky, think of a better way to handle it
  types::Type* Function(types::Type* ret, std::vector<types::Type*> params);
};

#endif