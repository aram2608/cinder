#include "../include/lexer.hpp"
#include "../include/parser.hpp"
#include "../include/compiler.hpp"

#include <fstream>

std::string ReadEntireFile(char* file_path) {
  std::fstream file{file_path};

  if (!file.is_open()) {
    printf("error opening file\n");
    exit(1);
  }

  std::string content((std::istreambuf_iterator<char>(file)),
                      std::istreambuf_iterator<char>());

  file.close();
  return content;
}

void Help() {
  printf("Usage: ./build/main <PATH>\n");
}

int main(int argc, char** argv) {
  if (argc < 2) {
    printf("missing file\n");
    Help();
    exit(1);
  }

  std::string source = ReadEntireFile(argv[1]);
  Lexer lexer{source};
  lexer.ScanTokens();
  // lexer.EmitTokens();
  Parser parser{lexer.tokens};
  std::vector<std::unique_ptr<Stmt>> statements = parser.Parse();
  // Compiler compiler{std::move(statements)};
  // compiler.Compile();
  return 0;
}
