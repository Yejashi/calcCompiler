#ifndef CALC_PARSER_H
#define CALC_PARSER_H

#include <memory>
#include <string>
#include <vector>

#include "../ast/AST.h"
#include "../lexer/lexer.h"

namespace calc {

// Recursive-descent parser for calc.
//
//   calc   ::= ("with" ident ("," ident)* ":")? expr
//   expr   ::= term (("+" | "-") term)*
//   term   ::= factor (("*" | "/") factor)*
//   factor ::= "-" factor | ident | number | "(" expr ")"
//
// Throws CalcError (with source position) on malformed input.
class Parser {
public:
  explicit Parser(Lexer &lexer, std::vector<std::string> &vars);

  // Parse a complete program. On success `vars` holds the declared
  // variable names in declaration order.
  std::unique_ptr<Expr> parse();

private:
  Lexer &lexer_;
  std::vector<std::string> &vars_;
  Token current_;

  void expect(Token::TokenKind kind);
  void advance();

  std::unique_ptr<Expr> parseCalc();
  void parseVarList();
  void pushVar();
  std::unique_ptr<Expr> parseExpr();
  std::unique_ptr<Expr> parseTerm();
  std::unique_ptr<Expr> parseFactor();
};

} // namespace calc

#endif // CALC_PARSER_H
