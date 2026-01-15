#ifndef EXPR_H_
#define EXPR_H_

#include "common.hpp"
#include "tokens.hpp"

/// @brief Base class for the AST nodes for expressions
struct Expr {
  virtual ~Expr();
  virtual std::string ToString() = 0;
};

/// @brief Numeric Node, stores floats and ints as of now
struct Numeric : Expr {
  ValueType value_type;
  Value value;

  Numeric(ValueType value_type, Value value);
  std::string ToString() override;
};

struct Boolean : Expr {
  bool boolean;

  Boolean(bool boolean);
  std::string ToString() override;
};

struct Binary : Expr {
  std::unique_ptr<Expr> left;
  std::unique_ptr<Expr> right;
  Token op;

  Binary(std::unique_ptr<Expr> left, std::unique_ptr<Expr> right, Token op);
  std::string ToString() override;
};

struct Variable : Expr {
  std::string name;
  Variable(const std::string name);
  std::string ToString() override;
};

#endif