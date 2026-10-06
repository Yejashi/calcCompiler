#include "AST.h"

#include <cmath>
#include <cstdio>

namespace calc {

namespace {

// Format a double the way the drivers print results: whole numbers as
// integers, everything else with up to 10 significant digits.
std::string formatDouble(double v) {
  if (v == std::floor(v) && std::fabs(v) < 9.007199254740992e15)
    return std::to_string(static_cast<long long>(v));
  char buf[64];
  std::snprintf(buf, sizeof(buf), "%.10g", v);
  return buf;
}

} // namespace

std::string NumberExpr::toString(bool) const {
  return formatDouble(Value);
}

std::string BinaryExpr::toString(bool isRoot) const {
  const char *op = nullptr;
  switch (Op) {
  case BinaryOp::plus:     op = " + "; break;
  case BinaryOp::minus:    op = " - "; break;
  case BinaryOp::multiply: op = " * "; break;
  case BinaryOp::divide:   op = " / "; break;
  }
  std::string lhs = LHS->toString(false);
  if (dynamic_cast<const UnaryExpr *>(LHS.get()))
    lhs = "(" + lhs + ")"; // keep "-2 * 3" readable as "(-2) * 3"
  std::string s = lhs + op + RHS->toString(false);
  if (!isRoot)
    s = "(" + s + ")";
  return s;
}

std::string UnaryExpr::toString(bool) const {
  std::string s;
  if (auto *bin = dynamic_cast<const BinaryExpr *>(Operand.get()))
    s = bin->toString(false); // already parenthesized
  else if (auto *uni = dynamic_cast<const UnaryExpr *>(Operand.get()))
    s = "(" + uni->toString(false) + ")";
  else
    s = Operand->toString(false);
  return "-" + s;
}

} // namespace calc
