#include "../include/lexer.hpp"
#include "../include/parser.hpp"

#include <stdio.h>

int main(void) {
  // const char* source =
  //     "! + - * % = == != [ ] { } ( ) ; : \" def end if then else for int
  //     float" "str 1234";
  const char* source = "1 + 1 / 1 / 1";

  Lexer lexer{source};
  lexer.ScanTokens();
  Parser parser{lexer.tokens};
  parser.Parse();
  return 0;
}
