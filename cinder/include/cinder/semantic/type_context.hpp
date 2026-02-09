#ifndef TYPE_CONTEXT_H_
#define TYPE_CONTEXT_H_

#include "cinder/ast/types.hpp"

class TypeContext {
 public:
  cinder::types::IntType* Int32();
  cinder::types::IntType* Int64();
  cinder::types::FloatType* Float32();
  cinder::types::FloatType* Float64();
  cinder::types::BoolType* Bool();
  cinder::types::Type* Void();
  cinder::types::StringType* String();

  cinder::types::FunctionType* Function(
      cinder::types::Type* ret, std::vector<cinder::types::Type*> params,
      bool variadic);

 private:
  cinder::types::IntType int32_{32, true};
  cinder::types::IntType int64_{64, true};
  cinder::types::FloatType flt32_{32};
  cinder::types::FloatType flt64_{64};
  cinder::types::BoolType bool_{1};
  cinder::types::Type void_{cinder::types::TypeKind::Void};
  cinder::types::StringType str_{};

  std::vector<std::unique_ptr<cinder::types::FunctionType>> function_pool_;
};

#endif
