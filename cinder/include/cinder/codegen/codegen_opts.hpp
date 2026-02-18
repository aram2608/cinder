#ifndef CODEGEN_OPTS_H_
#define CODEGEN_OPTS_H_

#include <string>
#include <vector>

/**
 * @brief Immutable options used to control backend/codegen behavior.
 *
 * Populated by CLI parsing and consumed by `Codegen`.
 */
struct CodegenOpts {
  /** @brief Backend execution mode. */
  enum class Opt {
    EMIT_LLVM,
    RUN,
    COMPILE,
  };
  std::string out_path; /**< Destination path for emitted artifact. */
  Opt mode;             /**< Requested backend mode. */
  std::vector<std::string> linker_flags; /**< Additional linker flags. */
  bool debug_info; /**< Enables debug info generation (planned). */

  /**
   * @brief Constructs codegen options.
   * @param out_path Destination path for emitted artifact.
   * @param mode Backend mode.
   * @param debug_info Whether debug information is requested.
   * @param linker_flags_list Additional linker flags.
   */
  CodegenOpts(std::string out_path, Opt mode, bool debug_info,
              std::vector<std::string> linker_flags_list);
};

#endif
