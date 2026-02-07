#include "cinder/codegen/codegen_opts.hpp"

CodegenOpts::CodegenOpts(std::string out_path, Opt mode,
                                 bool debug_info,
                                 std::vector<std::string> linker_flags_list)
    : out_path(out_path), mode(mode), debug_info(debug_info) {
  for (auto it = linker_flags_list.begin(); it != linker_flags_list.end();
       ++it) {
    linker_flags += *it;
  }
}