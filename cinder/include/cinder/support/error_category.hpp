#ifndef ERROR_CATEGORY_H_
#define ERROR_CATEGORY_H_

#include <string>
#include <system_error>
#include <type_traits>

enum class Errors {
  Success = 0,
  BadCast,
};

/** @brief Custom `std::error_category` for project-local error codes. */
class ErrorCategory : public std::error_category {
 public:
  /** @brief Returns a stable category name. */
  const char* name() const noexcept override;

  /** @brief Converts an error value to human-readable text. */
  std::string message(int ev) const override;
};

/** @brief Returns the singleton error category instance. */
const ErrorCategory& GetErrorCategory();

/** @brief Creates `std::error_code` from `Errors`. */
std::error_code make_error_code(Errors e);

namespace std {
template <>
struct is_error_code_enum<Errors> : true_type {};
}  // namespace std

#endif
