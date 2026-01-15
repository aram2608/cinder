#ifndef PARSER_H_
#define PARSER_H_

#include "common.hpp"
#include "tokens.hpp"

struct Parser {
  std::vector<Token> tokens;
  size_t current_tok;
  Parser(std::vector<Token> tokens);

  void Parse();
  bool IsEnd();
};

#endif