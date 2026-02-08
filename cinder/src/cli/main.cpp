#include <cstdlib>
#include <fstream>
#include <vector>

#include "../vendor/cxxopts.hpp"
#include "cinder/ast/stmt.hpp"
#include "cinder/codegen/codegen.hpp"
#include "cinder/codegen/codegen_opts.hpp"
#include "cinder/frontend/lexer.hpp"
#include "cinder/frontend/parser.hpp"

#define DEBUG_BUILD

static std::string ReadEntireFile(std::string file_path) {
  std::fstream file{file_path};

  if (!file.is_open()) {
    std::cout << "error opening file < " << file_path << " >\n";
    exit(1);
  }

  std::string content((std::istreambuf_iterator<char>(file)),
                      std::istreambuf_iterator<char>());

  file.close();
  return content;
}

static bool GenerateProgram(cxxopts::ParseResult& result,
                            CodegenOpts::Opt opt) {
  bool debug_info = false;
  std::vector<std::string> linker_flags;
  std::vector<std::string> file_paths =
      result["src"].as<std::vector<std::string>>();
  std::string out_path = "cinder";
  if (result.contains("o")) {
    out_path = result["o"].as<std::string>();
  }
  if (result.contains("l")) {
    linker_flags = result["l"].as<std::vector<std::string>>();
  }
  if (result.contains("g")) {
    debug_info = true;
    linker_flags.push_back("-g");
  }
  for (auto it = file_paths.begin(); it != file_paths.end(); ++it) {
    std::string source = ReadEntireFile(*it);
    Lexer lexer{source};
    lexer.ScanTokens();
    Parser parser{lexer.GetTokens()};
    std::unique_ptr<Stmt> mod = parser.Parse();
    CodegenOpts opts{out_path, opt, debug_info, linker_flags};
    Codegen cg{std::move(mod), opts};
    return cg.Generate();
  }
  return true;
}

static void DumpUnknownArgs(cxxopts::ParseResult& result,
                            cxxopts::Options& options) {
  std::cout << "Unknown arguments provided:\n";
  for (auto arg : result.unmatched()) {
    std::cout << "\t" << arg << "\n";
  }
  std::cout << "\n";
  std::cout << options.help() << std::endl;
}

static bool ParseCLI(int argc, char** argv) {
  using namespace cxxopts;
  Options options{"cinder", "Compiler for the Cinder language"};
  options.positional_help("[optional args]").show_positional_help();

  options.set_width(70).set_tab_expansion();
  options.allow_unrecognised_options();

  options.add_options()("h,help", "Print this help message");
#ifdef DEBUG_BUILD
  options.add_options()("emit-tokens", "Emits the lexers tokens");
  options.add_options()("emit-ast", "Emits the scanners ast");
#endif
  options.add_options()("compile", "Compiles the program to an executable");
  /// TODO: Add compile run option
  // options.add_options()("compile-run", "Compiles the program to llvm");
  options.add_options()("emit-llvm", "Emits llvm output");
  /// TODO: Add debug information
  // options.add_options()("g", "Debug information")

  options.add_options()("l,l-flags", "Linker option",
                        value<std::vector<std::string>>());
  options.add_options()("src", "The input files to be compiled",
                        value<std::vector<std::string>>());
  options.add_options()("o,output", "Desired output file",
                        value<std::string>());

  options.parse_positional({"src"});

  auto result = options.parse(argc, argv);

  if (result.contains("help")) {
    std::cout << options.help() << std::endl;
    return true;
  }

#ifdef DEBUG_BUILD
  if (result.contains("emit-tokens")) {
    std::vector<std::string> file_paths =
        result["src"].as<std::vector<std::string>>();
    for (auto it = file_paths.begin(); it != file_paths.end(); ++it) {
      std::string source = ReadEntireFile(*it);
      Lexer lexer{source};
      lexer.ScanTokens();
      lexer.EmitTokens();
    }
    return true;
  }

  if (result.contains("emit-ast")) {
    std::vector<std::string> file_paths =
        result["src"].as<std::vector<std::string>>();
    for (auto it = file_paths.begin(); it != file_paths.end(); ++it) {
      std::string source = ReadEntireFile(*it);
      Lexer lexer{source};
      lexer.ScanTokens();
      Parser parser{lexer.GetTokens()};
      std::unique_ptr<Stmt> mod = parser.Parse();
      parser.EmitAST(std::move(mod));
    }
    return true;
  }
#endif

  if (result.contains("emit-llvm")) {
    return GenerateProgram(result, CodegenOpts::Opt::EMIT_LLVM);
  }

  if (result.contains("compile")) {
    return GenerateProgram(result, CodegenOpts::Opt::COMPILE);
  }

  DumpUnknownArgs(result, options);
  return false;
}

int main(int argc, char** argv) {
  return ParseCLI(argc, argv) ? 0 : 1;
}
