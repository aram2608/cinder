#ifndef ERRORS_H_
#define ERRORS_H_

#include <unistd.h>

#include <algorithm>
#include <charconv>
#include <cstring>
#include <iomanip>
#include <string_view>
#include <system_error>

namespace errs {

/**
 * @class RawOutStream
 * @brief A raw output stream similar to LLVM's
 * This implementation is much smaller and simpler
 * It is essentially an overglorified char* buffer that manages its contents
 * internally
 */
class RawOutStream {
  static constexpr size_t BUF_SIZE = 4096;
  char buffer[BUF_SIZE];
  size_t pos = 0;

  void FlushBuffer();

 public:
  /**
   * @brief RawOutstream destructor
   * This method is virtual to ensure that the appropriate destructor is
   * invoked for any derived class
   */
  virtual ~RawOutStream();

  /**
   * @brief The main work horse of the buffer, writes data to console
   * @param data The data to be printed
   * @param size The size of the data to be written
   * @return A pointer to this, lets us chain functions
   */
  RawOutStream& Write(const char* data, size_t size);

  /**
   * @brief Insertion overload for a string view
   * @return A pointer to this so we can chain insertions
   */
  RawOutStream& operator<<(std::string_view s) {
    return Write(s.data(), s.size());
  }

  /**
   * @brief Insertion overload for C string
   * @param c The C string to be printed
   * @return A pointer to this so we can chain insertions
   */
  RawOutStream& operator<<(const char* s) {
    return Write(s, std::strlen(s));
  }

  /**
   * @brief Insertion overload for C string
   * @param s The string to be inserted
   * @return A pointer to this so we can chain insertions
   */
  RawOutStream& operator<<(const std::string s) {
    return Write(s.data(), s.size());
  }

  /**
   * @brief Insertion overload for an integer
   * The int is converted to a char* using the std::to_chars method for
   * minimal overhead compared to fprintf or sprintf.
   *
   * std::to_chars returns a range where t
   * @param n The integer to be printed
   * @return A pointer to this so insertions can be chained
   */
  RawOutStream& operator<<(int n) {
    char num_buf[12];
    auto [ptr, ec] = std::to_chars(num_buf, num_buf + 12, n);
    return Write(num_buf, ptr - num_buf);
  }

  /**
   * @brief Insertion overload for a size_t
   * Converted to a char* using the std::to_chars method for minimal overhead
   *
   * @param n The size_t to be printed
   * @return A pointer to this so we can chain commands
   */
  RawOutStream& operator<<(size_t n) {
    char num_buf[21];
    auto [ptr, ec] = std::to_chars(num_buf, num_buf + 30, n);
    return Write(num_buf, ptr - num_buf);
  }

  /**
   * @brief Insertion overload for a float
   * Converted to char* using the std::to_chars method for minimal overhead
   * Floats are tricky to convert since they do not necessarily fit into a neat
   * consistent buffer. Different buffer sizes may be needed depending on size
   * and precision. Here chars_format::general is used to try and minimize the
   * buffer size. Use this method with caution
   *
   * @param n Float to be printed
   * @return A pointer to this so we can chain insertions
   */
  RawOutStream& operator<<(float n) {
    char num_buf[64];
    auto [ptr, ec] =
        std::to_chars(num_buf, num_buf + 64, n, std::chars_format::general);
    if (ec != std::errc{}) {
      std::string s = std::make_error_code(ec).message();
      return Write(s.data(), s.size());
    } else {
      return Write(num_buf, ptr - num_buf);
    }
  }
};

/**
 * @brief Function to print errors messages and exit out
 * A variadic template method to take any abritray number of arguments
 * and print the raw out stream. This assumes the operator<< has been
 * overloaded for the passed in type.
 *
 * A space is explicitly printed between each argument.
 */
template <typename... Args>
inline void ErrorOutln(RawOutStream& stream, Args const&... args) {
  size_t n = 0;
  ((stream << (n++ > 0 ? " " : "") << args), ...);
  stream << "\n";
  exit(1);
}

}  // namespace errs

#endif