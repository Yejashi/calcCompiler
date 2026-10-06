#ifndef CALC_EVALUATOR_H
#define CALC_EVALUATOR_H

#include <map>
#include <string>

#include "../ast/AST.h"

namespace calc {

// Tree-walking evaluator for calc expressions (no LLVM involved).
// Arithmetic is IEEE 754 double: division by zero yields inf/NaN.
// Throws CalcError if a variable is missing from the environment.
class Evaluator {
public:
  explicit Evaluator(std::map<std::string, double> env)
      : env_(std::move(env)) {}

  double evaluate(const Expr *root) const { return visit(root); }

private:
  std::map<std::string, double> env_;
  double visit(const Expr *e) const;
};

} // namespace calc

#endif // CALC_EVALUATOR_H
