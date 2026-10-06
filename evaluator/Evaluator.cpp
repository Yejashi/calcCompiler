#include "Evaluator.h"

#include <stdexcept>

namespace calc {

double Evaluator::visit(const Expr *e) const {
  if (auto *num = dynamic_cast<const NumberExpr *>(e))
    return num->getValue();

  if (auto *var = dynamic_cast<const VariableExpr *>(e)) {
    const std::string name = var->getName().str();
    auto it = env_.find(name);
    if (it == env_.end())
      throw CalcError(var->getPosition(),
                      "undeclared variable '" + name + "'");
    return it->second;
  }

  if (auto *bin = dynamic_cast<const BinaryExpr *>(e)) {
    const double lhs = visit(bin->getLHS());
    const double rhs = visit(bin->getRHS());
    switch (bin->getOp()) {
    case BinaryOp::plus:     return lhs + rhs;
    case BinaryOp::minus:    return lhs - rhs;
    case BinaryOp::multiply: return lhs * rhs;
    case BinaryOp::divide:   return lhs / rhs;
    }
  }

  if (auto *uni = dynamic_cast<const UnaryExpr *>(e))
    return -visit(uni->getOperand());

  throw std::logic_error("unknown AST node type");
}

} // namespace calc
