#include "../include/lexer.hpp"
#include "../include/parser.hpp"

#include <stdio.h>

int main(void) {
  const char* source =
      "! + - * % = == != [ ] { } ( ) ; : \" def end if then else for int float"
      "str 1234";

  Lexer lexer{source};
  lexer.ScanTokens();
  Parser parser{lexer.tokens};
  return 0;
}
