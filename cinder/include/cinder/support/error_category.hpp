#ifndef ERROR_CATEGORY_H_
#define ERROR_CATEGORY_H_

#include <string>
#include <system_error>
#include <type_traits>

enum class Errors {
  Success = 0,
  BadCast,
};

// 2. Create the Category class
class ErrorCategory : public std::error_category {
 public:
  const char* name() const noexcept override;

  std::string message(int ev) const override;
};

// 3. Provide a global way to access the category instance
const ErrorCategory& GetErrorCategory();

/// @brief Overload for make_error_code func
std::error_code make_error_code(Errors e);

namespace std {
template <>
struct is_error_code_enum<Errors> : true_type {};
}  // namespace std

#endif
