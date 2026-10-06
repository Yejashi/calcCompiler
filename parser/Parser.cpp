#include "Parser.h"

#include <stdexcept>
#include <string>

namespace calc {

namespace {

const char *tokenName(Token::TokenKind K) {
  switch (K) {
  case Token::eoi:     return "end-of-input";
  case Token::unknown: return "unknown";
  case Token::ident:   return "identifier";
  case Token::number:  return "number";
  case Token::comma:   return "','";
  case Token::colon:   return "':'";
  case Token::plus:    return "'+'";
  case Token::minus:   return "'-'";
  case Token::star:    return "'*'";
  case Token::slash:   return "'/'";
  case Token::l_paren: return "'('";
  case Token::r_paren: return "')'";
  case Token::KW_with: return "'with'";
  }
  return "<?>";
}

} // namespace

Parser::Parser(Lexer &lexer, std::vector<std::string> &vars)
    : lexer_(lexer), vars_(vars) {
  advance();
}

std::unique_ptr<Expr> Parser::parse() {
  auto result = parseCalc();
  expect(Token::eoi);
  return result;
}

void Parser::expect(Token::TokenKind kind) {
  if (current_.getKind() != kind) {
    throw CalcError(current_.getPosition(),
                    std::string("expected ") + tokenName(kind) +
                        " but found " + tokenName(current_.getKind()));
  }
  advance();
}

void Parser::advance() {
  lexer_.next(current_);
}

std::unique_ptr<Expr> Parser::parseCalc() {
  if (!current_.is(Token::KW_with))
    return parseExpr();

  advance(); // consume 'with'
  parseVarList();
  expect(Token::colon);
  return parseExpr();
}

void Parser::parseVarList() {
  pushVar();
  while (current_.is(Token::comma)) {
    advance(); // consume ','
    pushVar();
  }
}

void Parser::pushVar() {
  if (!current_.is(Token::ident))
    throw CalcError(current_.getPosition(),
                    std::string("expected identifier but found ") +
                        tokenName(current_.getKind()));
  const SourcePosition pos = current_.getPosition();
  std::string name = current_.getText().str();
  for (const std::string &declared : vars_) {
    if (declared == name)
      throw CalcError(pos, "duplicate variable declaration '" + name + "'");
  }
  vars_.push_back(std::move(name));
  advance();
}

std::unique_ptr<Expr> Parser::parseExpr() {
  auto lhs = parseTerm();

  while (current_.isOneOf(Token::plus, Token::minus)) {
    const SourcePosition opPos = current_.getPosition();
    BinaryOp op =
        (current_.getKind() == Token::plus) ? BinaryOp::plus : BinaryOp::minus;
    advance();
    auto rhs = parseTerm();
    lhs = std::make_unique<BinaryExpr>(op, std::move(lhs), std::move(rhs), opPos);
  }

  return lhs;
}

std::unique_ptr<Expr> Parser::parseTerm() {
  auto lhs = parseFactor();

  while (current_.isOneOf(Token::star, Token::slash)) {
    const SourcePosition opPos = current_.getPosition();
    BinaryOp op = (current_.getKind() == Token::star) ? BinaryOp::multiply
                                                      : BinaryOp::divide;
    advance();
    auto rhs = parseFactor();
    lhs = std::make_unique<BinaryExpr>(op, std::move(lhs), std::move(rhs), opPos);
  }

  return lhs;
}

std::unique_ptr<Expr> Parser::parseFactor() {
  const SourcePosition pos = current_.getPosition();

  if (current_.is(Token::number)) {
    double value;
    try {
      value = std::stod(current_.getText().str());
    } catch (const std::exception &) {
      throw CalcError(pos, "number literal out of range: '" +
                               current_.getText().str() + "'");
    }
    advance();
    return std::make_unique<NumberExpr>(value, pos);
  }

  if (current_.is(Token::ident)) {
    std::string name = current_.getText().str();
    advance();
    return std::make_unique<VariableExpr>(std::move(name), pos);
  }

  if (current_.is(Token::l_paren)) {
    advance(); // consume '('
    auto expr = parseExpr();
    expect(Token::r_paren);
    return expr;
  }

  if (current_.is(Token::minus)) {
    advance(); // consume '-'
    auto operand = parseFactor();
    return std::make_unique<UnaryExpr>(UnaryOp::minus, std::move(operand), pos);
  }

  throw CalcError(pos, std::string("expected a number, identifier, or '(' "
                                   "but found ") +
                           tokenName(current_.getKind()));
}

} // namespace calc
