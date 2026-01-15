#include "../include/parser.hpp"

Parser::Parser(std::vector<Token> tokens) : current_tok(0), tokens(tokens) {}

void Parser::Parse() {
    while(!IsEnd()) {
        
    }
}

bool Parser::IsEnd() {
  return tokens[current_tok].token_type == TT_EOF;
}