#include "../include/lexer.hpp"
#include "../include/parser.hpp"
#include "../include/compiler.hpp"

#include <stdio.h>

int main(void) {
  // const char* source =
  //     "! + - * % = == != [ ] { } ( ) ; : \" def end if then else for int
  //     float" "str 1234";
  const char* source = "1.0 + 1.0 * 2.0";
  Lexer lexer{source};
  lexer.ScanTokens();
  Parser parser{lexer.tokens};
  std::vector<std::unique_ptr<Expr>> expressions = parser.Parse();
  Compiler compiler{std::move(expressions)};
  compiler.CompileExpression();
  return 0;
}
