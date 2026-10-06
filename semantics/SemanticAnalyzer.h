#ifndef CALC_SEMANTICS_H
#define CALC_SEMANTICS_H

#include <set>
#include <string>
#include <vector>

#include "../ast/AST.h"

namespace calc {

// Validates a parsed program:
//   - every variable used in the expression is declared in the `with` list
//     (duplicate declarations are rejected by the parser).
// Throws CalcError (with the position of the offending use) on failure.
class SemanticAnalyzer {
public:
  explicit SemanticAnalyzer(const std::vector<std::string> &declared);

  void analyze(const Expr *root) const;

private:
  std::set<std::string> declared_;
  void visit(const Expr *e) const;
};

} // namespace calc

#endif // CALC_SEMANTICS_H
