#ifndef DIAGNOSTIC_H_
#define DIAGNOSTIC_H_

#include <string>
#include <vector>

#include "cinder/support/errors.hpp"

struct SourceLoc {
  size_t line = 0;
};

struct Diagnostic {
  enum class Severity { Error, Warning } severity;
  std::string message;
  SourceLoc loc;
};

class DiagnosticEngine {
 public:
  void Error(SourceLoc loc, std::string msg);

  void Warning(SourceLoc loc, std::string msg);

  bool HasErrors() const;
  const std::vector<Diagnostic>& All() const;
  void DumpErrors();

 private:
  errs::RawOutStream raw_error_stream_;
  size_t error_count_ = 0;
  std::vector<Diagnostic> diags_;
};

#endif