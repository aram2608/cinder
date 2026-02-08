#include "cinder/driver/clang_driver.hpp"

#include <cstdlib>
#include <memory>
#include <optional>

#include "clang/Basic/Diagnostic.h"
#include "clang/Basic/DiagnosticIDs.h"
#include "clang/Driver/ToolChain.h"
#include "clang/Frontend/CompilerInvocation.h"
#include "clang/Frontend/TextDiagnosticPrinter.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/TargetParser/Host.h"

/**
 * @brief Function to resolve the sysroot for apple
 * The linker complains if it can't resolve the sysroot so we check some
 * hard coded paths.
 * @return Returns a string if the match is made, otherwise a nullopt
 */
static std::optional<std::string> ResolveDarwinSysroot() {
  if (const char* sdkroot = std::getenv("SDKROOT"); sdkroot && *sdkroot) {
    return std::string(sdkroot);
  }

  static constexpr const char* kCandidates[] = {
      "/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk",
      "/Applications/Xcode.app/Contents/Developer/Platforms/"
      "MacOSX.platform/Developer/SDKs/MacOSX.sdk",
  };

  for (const char* candidate : kCandidates) {
    if (llvm::sys::fs::exists(candidate)) {
      return std::string(candidate);
    }
  }
  return std::nullopt;
}

bool ClangDriver::LinkObject(const std::string& object_path,
                             const std::string& output_path,
                             const std::vector<std::string>& user_link_flags,
                             const std::string& clang_path) {
  llvm::SmallVector<const char*, 32> args;
  std::vector<std::string> owned;
  owned.push_back(clang_path);
  owned.push_back(object_path);
  for (const auto& flag : user_link_flags) {
    owned.push_back(flag);
  }
  owned.push_back("-o");
  owned.push_back(output_path);

#ifdef __APPLE__
  if (auto sysroot = ResolveDarwinSysroot()) {
    owned.push_back("-isysroot");
    owned.push_back(*sysroot);
  }
#endif

  for (const auto& s : owned) {
    args.push_back(s.c_str());
  }

  auto diag_opts = std::make_unique<clang::DiagnosticOptions>();
  auto* diag_client =
      new clang::TextDiagnosticPrinter(llvm::errs(), *diag_opts);
  llvm::IntrusiveRefCntPtr<clang::DiagnosticIDs> diag_ids;
  clang::DiagnosticsEngine diags(diag_ids, *diag_opts, diag_client);

  clang::driver::Driver driver(clang_path, llvm::sys::getDefaultTargetTriple(),
                               diags);
  driver.setTitle("cinder clang linker");

  std::unique_ptr<clang::driver::Compilation> compilation(
      driver.BuildCompilation(args));
  if (!compilation) {
    return false;
  }

  llvm::SmallVector<std::pair<int, const clang::driver::Command*>, 4> failing;
  int exit_code = driver.ExecuteCompilation(*compilation, failing);
  return exit_code == 0;
}
