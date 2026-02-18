#ifndef CLANG_DRIVER_H_
#define CLANG_DRIVER_H_

#include "clang/Basic/Diagnostic.h"
#include "clang/Basic/DiagnosticIDs.h"
#include "clang/Basic/DiagnosticOptions.h"
#include "clang/Basic/HeaderInclude.h"
#include "clang/Basic/Stack.h"
#include "clang/Config/config.h"
#include "clang/Driver/Compilation.h"
#include "clang/Driver/Driver.h"
#include "clang/Driver/DriverDiagnostic.h"
#include "clang/Driver/ToolChain.h"
#include "llvm/ADT/IntrusiveRefCntPtr.h"
#include "llvm/Support/LLVMDriver.h"
#include "llvm/Support/VirtualFileSystem.h"

/** @brief Thin wrapper around Clang's driver for final link steps. */
struct ClangDriver {
  /** @brief Constructs a driver wrapper. */
  ClangDriver();

  /**
   * @brief Links an object file into an executable via clang.
   * @param object_path Input object file path.
   * @param output_path Output executable path.
   * @param user_link_flags Additional linker flags.
   * @param clang_path Path to clang binary (defaults to `clang`).
   * @return `true` when linking succeeds; otherwise `false`.
   */
  static bool LinkObject(const std::string& object_path,
                         const std::string& output_path,
                         const std::vector<std::string>& user_link_flags,
                         const std::string& clang_path = "clang");
};

#endif
