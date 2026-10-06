#include "SemanticAnalyzer.h"

#include <stdexcept>

namespace calc {

SemanticAnalyzer::SemanticAnalyzer(const std::vector<std::string> &declared) {
  for (const std::string &name : declared)
    declared_.insert(name);
}

void SemanticAnalyzer::analyze(const Expr *root) const {
  visit(root);
}

void SemanticAnalyzer::visit(const Expr *e) const {
  if (auto *num = dynamic_cast<const NumberExpr *>(e)) {
    (void)num; // nothing to check
  } else if (auto *var = dynamic_cast<const VariableExpr *>(e)) {
    const std::string name = var->getName().str();
    if (declared_.find(name) == declared_.end())
      throw CalcError(var->getPosition(),
                      "undeclared variable '" + name + "'");
  } else if (auto *bin = dynamic_cast<const BinaryExpr *>(e)) {
    visit(bin->getLHS());
    visit(bin->getRHS());
  } else if (auto *uni = dynamic_cast<const UnaryExpr *>(e)) {
    visit(uni->getOperand());
  } else {
    throw std::logic_error("unknown AST node type");
  }
}

} // namespace calc
