#include "../include/lexer.h"

#include <stdio.h>

int main(void) {
  // We free memory in the lexer so i am dynamically allocating this memory
  // TODO: Create a function that can read files, it will likely malloc some
  // memory which is why i am bothering to do this now
  char* source = "! + - * % = == !=";
  size_t size = strlen(source);
  char* source_str = (char*)malloc(size + 1);
  strcpy(source_str, source);
  source_str[size + 1] = '\0';

  Lexer* lexer = NewLexer(source_str);
  ScanTokens(lexer);
  EmitTokens(lexer);

  DestroyLexer(lexer);
  return 0;
}
