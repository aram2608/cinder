#ifndef CODEGEN_OPTS_H_
#define CODEGEN_OPTS_H_

#include <string>
#include <vector>

/**
 * @struct CompilerOptions
 * @brief A simple data container for the compiler options
 * Populated during CLI argument parse time and used during the final stages of
 * compilation.
 */
struct CodegenOpts {
  enum class Opt {
    EMIT_LLVM,
    RUN,
    COMPILE,
  };
  std::string out_path; /**< The desired outpath */
  Opt mode;             /** The compiler mode, can emit-llvm, run, or compile */
  std::vector<std::string>
      linker_flags; /**< Flags to pass to the clang linker */
  bool debug_info; /**< TODO: Implement debug information, its a bit of a ritual
                    */
  CodegenOpts(std::string out_path, Opt mode, bool debug_info,
              std::vector<std::string> linker_flags_list);
};

#endif