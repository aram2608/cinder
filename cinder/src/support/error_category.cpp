#include "cinder/support/error_category.hpp"

const char* ErrorCategory::name() const noexcept {
  return "MyCustomApp";
}

std::string ErrorCategory::message(int ev) const {
  switch (static_cast<Errors>(ev)) {
    case Errors::Success:
      return "Everything is fine.";
    case Errors::BadCast:
      return "Bad cast made.";
    default:
      return "Unknown error.";
  }
}

const ErrorCategory& GetErrorCategory() {
  static ErrorCategory instance;
  return instance;
}

std::error_code make_error_code(Errors e) {
  return {static_cast<int>(e), GetErrorCategory()};
}