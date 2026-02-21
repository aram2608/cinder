#ifndef TYPE_CONTEXT_H_
#define TYPE_CONTEXT_H_

#include <string>
#include <unordered_map>

#include "cinder/ast/types.hpp"

/**
 * @brief Owns canonical type instances used during semantic analysis.
 *
 * Primitive types are singletons stored directly in this context. Function
 * types are allocated into an internal pool and live for the lifetime of the
 * context.
 */
class TypeContext {
 public:
  /** @brief Returns canonical `int32` type. */
  cinder::types::IntType* Int32();
  /** @brief Returns canonical `int64` type. */
  cinder::types::IntType* Int64();
  /** @brief Returns canonical `float32` type. */
  cinder::types::FloatType* Float32();
  /** @brief Returns canonical `float64` type. */
  cinder::types::FloatType* Float64();
  /** @brief Returns canonical `bool` type. */
  cinder::types::BoolType* Bool();
  /** @brief Returns canonical `void` type. */
  cinder::types::Type* Void();
  /** @brief Returns canonical string type. */
  cinder::types::StringType* String();

  /**
   * @brief Creates or interns a function type in the context pool.
   * @param ret Return type.
   * @param params Ordered fixed parameter types.
   * @param variadic Whether additional varargs are accepted.
   * @return Pointer to pooled function type.
   */
  cinder::types::FunctionType* Function(
      cinder::types::Type* ret, std::vector<cinder::types::Type*> params,
      bool variadic);

  /** @brief Creates or replaces a struct type by name. */
  cinder::types::StructType* Struct(std::string name,
                                    std::vector<std::string> field_names,
                                    std::vector<cinder::types::Type*> fields);

  /** @brief Looks up a struct type by name. */
  cinder::types::StructType* LookupStruct(const std::string& name);

 private:
  cinder::types::IntType int32_{32, true};
  cinder::types::IntType int64_{64, true};
  cinder::types::FloatType flt32_{32};
  cinder::types::FloatType flt64_{64};
  cinder::types::BoolType bool_{1};
  cinder::types::Type void_{cinder::types::TypeKind::Void};
  cinder::types::StringType str_{};

  std::vector<std::unique_ptr<cinder::types::FunctionType>>
      function_pool_; /**< Owning pool for dynamically created function types.
                       */
  std::unordered_map<std::string, std::unique_ptr<cinder::types::StructType>>
      struct_types_;
};

#endif
