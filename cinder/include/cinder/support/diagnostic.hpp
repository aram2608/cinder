#ifndef DIAGNOSTIC_H_
#define DIAGNOSTIC_H_

#include <string>
#include <vector>

#include "cinder/support/raw_outstream.hpp"

/** @brief Minimal source location metadata used by diagnostics. */
struct SourceLoc {
  size_t line = 0;
};

/** @brief A single diagnostic message emitted by compiler passes. */
struct Diagnostic {
  enum class Severity { Error, Warning, Debug } severity;
  std::string message; /**< Human-readable diagnostic text. */
  SourceLoc loc;       /**< Source location associated with the message. */
};

/** @brief Collects, stores, and prints diagnostics across compiler passes. */
class DiagnosticEngine {
 public:
  DiagnosticEngine() : raw_error_stream_(2) {}

  /**
   * @brief Records an error diagnostic.
   * @param loc Source location.
   * @param msg Error message.
   */
  void Error(SourceLoc loc, std::string msg);

  /** @brief Records a warning diagnostic. */
  void Warning(SourceLoc loc, std::string msg);
  /** @brief Records a debug diagnostic. */
  void Debug(SourceLoc loc, std::string msg);

  /** @brief Returns whether at least one error has been recorded. */
  bool HasErrors() const;
  /** @brief Returns all accumulated diagnostics. */
  const std::vector<Diagnostic>& All() const;
  /** @brief Writes all accumulated diagnostics to stderr. */
  void DumpErrors();

 private:
  ostream::RawOutStream raw_error_stream_; /**< Raw stderr stream wrapper. */
  size_t error_count_ = 0;        /**< Count of error-severity records. */
  std::vector<Diagnostic> diags_; /**< Stored diagnostics. */
};

#endif
