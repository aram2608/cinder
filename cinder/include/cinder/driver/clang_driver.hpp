#ifndef CLANG_DRIVER_H_
#define CLANG_DRIVER_H_

#include <string>
#include <vector>

#include "clang/Config/config.h"

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
