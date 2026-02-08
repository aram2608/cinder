#include "cinder/codegen/codegen_opts.hpp"

CodegenOpts::CodegenOpts(std::string out_path, Opt mode, bool debug_info,
                         std::vector<std::string> linker_flags)
    : out_path(out_path),
      mode(mode),
      linker_flags(linker_flags),
      debug_info(debug_info) {}