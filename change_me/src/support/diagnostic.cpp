#include "no_name_lang/support/diagnostic.hpp"

void DiagnosticEngine::Error(SourceLoc loc, std::string msg) {
  diags_.push_back({Diagnostic::Severity::Error, msg, loc});
  error_count_++;
}

void DiagnosticEngine::Warning(SourceLoc loc, std::string msg) {
  diags_.push_back({Diagnostic::Severity::Warning, msg, loc});
}

bool DiagnosticEngine::HasErrors() const {
  return error_count_ > 0;
}

const std::vector<Diagnostic>& DiagnosticEngine::All() const {
  return diags_;
}

void DiagnosticEngine::DumpErrors() {
  for (auto& err : diags_) {
    switch (err.severity) {
      case Diagnostic::Severity::Error:
        raw_error_stream_ << "ERROR: " << err.message << "at line ";
        raw_error_stream_ << err.loc.line << "\n";
        break;
      case Diagnostic::Severity::Warning:
        raw_error_stream_ << "WARNING: " << err.message << "at line ";
        raw_error_stream_ << err.loc.line << "\n";
        break;
      default:
        break;
    }
  }
}