#ifndef DIAGNOSTIC_H_
#define DIAGNOSTIC_H_

#include <string>
#include <vector>

#include "cinder/support/raw_outstream.hpp"

struct SourceLoc {
  size_t line = 0;
};

struct Diagnostic {
  enum class Severity { Error, Warning, Debug } severity;
  std::string message;
  SourceLoc loc;
};

class DiagnosticEngine {
 public:
  DiagnosticEngine() : raw_error_stream_(2) {}
  void Error(SourceLoc loc, std::string msg);

  void Warning(SourceLoc loc, std::string msg);
  void Debug(SourceLoc loc, std::string msg);

  bool HasErrors() const;
  const std::vector<Diagnostic>& All() const;
  void DumpErrors();

 private:
  ostream::RawOutStream raw_error_stream_;
  size_t error_count_ = 0;
  std::vector<Diagnostic> diags_;
};

#endif
