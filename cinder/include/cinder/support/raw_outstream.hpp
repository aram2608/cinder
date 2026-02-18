#ifndef ERRORS_H_
#define ERRORS_H_

#include <unistd.h>

#include <algorithm>
#include <charconv>
#include <cstdio>
#include <cstring>
#include <iomanip>
#include <string_view>
#include <system_error>

namespace ostream {

using FD = int;

/**
 * @brief Buffered POSIX file-descriptor output stream.
 *
 * This utility is intentionally small and only supports common write patterns
 * needed by the compiler diagnostics path.
 */
class RawOutStream {
  static constexpr size_t BUF_SIZE = 4096;
  char buffer[BUF_SIZE];
  size_t pos = 0;
  FD fd;

  void FlushBuffer();

 public:
  /**
   * @brief Creates a stream bound to a file descriptor.
   * @param fd POSIX file descriptor (for example `1` stdout or `2` stderr).
   */
  RawOutStream(FD fd);

  /**
   * @brief Flushes remaining buffered bytes and releases stream resources.
   */
  virtual ~RawOutStream();

  /**
   * @brief Writes a byte range to the stream buffer.
   * @param data Bytes to write.
   * @param size Number of bytes to write.
   * @return Reference to this stream for chaining.
   */
  RawOutStream& Write(const char* data, size_t size);

  /**
   * @brief Insertion overload for `std::string_view`.
   * @return Reference to this stream for chaining.
   */
  RawOutStream& operator<<(std::string_view s) {
    return Write(s.data(), s.size());
  }

  /**
   * @brief Insertion overload for C strings.
   * @param s Null-terminated character buffer.
   * @return Reference to this stream for chaining.
   */
  RawOutStream& operator<<(const char* s) {
    return Write(s, std::strlen(s));
  }

  /**
   * @brief Insertion overload for `std::string`.
   * @param s String value to write.
   * @return Reference to this stream for chaining.
   */
  RawOutStream& operator<<(const std::string s) {
    return Write(s.data(), s.size());
  }

  /**
   * @brief Insertion overload for `int` using `std::to_chars`.
   * @param n Integer value to write.
   * @return Reference to this stream for chaining.
   */
  RawOutStream& operator<<(int n) {
    char num_buf[12];
    auto [ptr, ec] = std::to_chars(num_buf, num_buf + 12, n);
    return Write(num_buf, ptr - num_buf);
  }

  /**
   * @brief Insertion overload for `size_t` using `std::to_chars`.
   * @param n Size value to write.
   * @return Reference to this stream for chaining.
   */
  RawOutStream& operator<<(size_t n) {
    char num_buf[21];
    auto [ptr, ec] = std::to_chars(num_buf, num_buf + 30, n);
    return Write(num_buf, ptr - num_buf);
  }

  /**
   * @brief Insertion overload for `float` using `std::to_chars`.
   * @param n Floating-point value to write.
   * @return Reference to this stream for chaining.
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
 * @brief Writes a space-separated message, appends newline, then exits.
 * @tparam Args Printable argument types.
 * @param stream Destination stream.
 * @param args Values to print in order.
 */
template <typename... Args>
inline void ErrorOutln(RawOutStream& stream, Args const&... args) {
  size_t n = 0;
  ((stream << (n++ > 0 ? " " : "") << args), ...);
  stream << "\n";
  exit(1);
}

}  // namespace ostream

#endif
