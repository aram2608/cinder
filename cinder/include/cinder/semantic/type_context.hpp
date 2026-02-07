#ifndef TYPE_CONTEXT_H_
#define TYPE_CONTEXT_H_

#include "cinder/ast/types.hpp"

class TypeContext {
 public:
  types::IntType* Int32();
  types::IntType* Int64();
  types::FloatType* Float32();
  types::BoolType* Bool();
  types::Type* Void();
  types::StringType* String();

  types::FunctionType* Function(types::Type* ret,
                                std::vector<types::Type*> params,
                                bool variadic);

 private:
  types::IntType int32_{32, true};
  types::IntType int64_{64, true};
  types::FloatType flt32_{32};
  types::BoolType bool_{1};
  types::Type void_{types::TypeKind::Void};
  types::StringType str_{};

  std::vector<std::unique_ptr<types::FunctionType>> function_pool_;
};

#endif
