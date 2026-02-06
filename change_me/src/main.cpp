#include <fstream>

#include "../include/compiler.hpp"
#include "../include/lexer.hpp"
#include "../include/parser.hpp"
#include "../vendor/cxxopts.hpp"

std::string ReadEntireFile(std::string file_path) {
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

bool ParseCLI(int argc, char** argv) {
  using namespace cxxopts;
  Options options{"name-me", "Compiler for an unammed lang as of now"};
  options.positional_help("[optional args]").show_positional_help();

  options.set_width(70).set_tab_expansion();
  options.allow_unrecognised_options();

  /// TODO: Add debug information
  // options.add_options()("g", "Debug information");
  options.add_options()("h,help", "Print this help message");
  options.add_options()("compile", "Compiles the program to an executable");
  options.add_options()("run", "Compiles the program to llvm");
  options.add_options()("emit-tokens", "Emits the lexers tokens");
  options.add_options()("emit-ast", "Emits the scanners ast");
  options.add_options()("emit-llvm", "Emits llvm output");
  options.add_options()("l", "Linker option");
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
      Parser parser{lexer.tokens};
      std::unique_ptr<Stmt> mod = parser.Parse();
      parser.EmitAST(std::move(mod));
    }
    return true;
  }

  // if (result.contains("emit-llvm")) {
  //   std::vector<std::string> file_paths =
  //       result["src"].as<std::vector<std::string>>();
  //   std::string out_path = "main";
  //   if (result.contains("o")) {
  //     out_path = result["o"].as<std::string>();
  //   }
  //   for (auto it = file_paths.begin(); it != file_paths.end(); ++it) {
  //     std::string source = ReadEntireFile(*it);
  //     Lexer lexer{source};
  //     lexer.ScanTokens();
  //     Parser parser{lexer.tokens};
  //     std::unique_ptr<Stmt> mod = parser.Parse();
  //     Compiler compiler{
  //         std::move(mod),
  //         CompilerOptions{out_path, CompilerMode::EMIT_LLVM, false, {}}};
  //     return compiler.Compile();
  //   }
  // }

  if (result.contains("compile")) {
    bool debug_info = false;
    std::vector<std::string> linker_flags;
    std::vector<std::string> file_paths =
        result["src"].as<std::vector<std::string>>();
    std::string out_path = "main";
    if (result.contains("o")) {
      out_path = result["o"].as<std::string>();
    }
    if (result.contains("l")) {
      linker_flags.push_back("-l");
      linker_flags.push_back(result["l"].as<std::string>());
    }
    if (result.contains("g")) {
      debug_info = true;
      linker_flags.push_back("-g");
    }
    for (auto it = file_paths.begin(); it != file_paths.end(); ++it) {
      std::string source = ReadEntireFile(*it);
      Lexer lexer{source};
      lexer.ScanTokens();
      Parser parser{lexer.tokens};
      std::unique_ptr<Stmt> mod = parser.Parse();
      CompilerOptions opts{out_path, CompilerMode::COMPILE, debug_info,
                           linker_flags};
      Compiler compiler{std::move(mod), opts};
      return compiler.Compile();
    }
  }

  // if (result.contains("run")) {
  //   std::vector<std::string> file_paths =
  //       result["src"].as<std::vector<std::string>>();
  //   std::string out_path = "main";
  //   if (result.contains("o")) {
  //     out_path = result["o"].as<std::string>();
  //   }
  //   for (auto it = file_paths.begin(); it != file_paths.end(); ++it) {
  //     std::string source = ReadEntireFile(*it);
  //     Lexer lexer{source};
  //     lexer.ScanTokens();
  //     Parser parser{lexer.tokens};
  //     std::unique_ptr<Stmt> mod = parser.Parse();
  //     Compiler compiler{
  //         std::move(mod),
  //         CompilerOptions{out_path, CompilerMode::RUN, false, {}}};
  //     return compiler.Compile();
  //   }
  // }
  std::cout << "Unknown arguments provided\n" << std::endl;
  std::cout << options.help() << std::endl;
  return false;
}

int main(int argc, char** argv) {
  return ParseCLI(argc, argv) ? 0 : 1;
}
