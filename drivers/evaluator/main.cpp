// evaluator_driver — runs calc programs with the tree-walking evaluator
// (no LLVM JIT). Reads one program from a file argument or stdin and
// prints "Result = <value>".

#include <cmath>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "../lexer/lexer.h"
#include "../parser/Parser.h"
#include "../semantics/SemanticAnalyzer.h"
#include "../evaluator/Evaluator.h"

namespace {

std::string trim(const std::string &s) {
  const size_t b = s.find_first_not_of(" \t\r\n");
  if (b == std::string::npos)
    return "";
  const size_t e = s.find_last_not_of(" \t\r\n");
  return s.substr(b, e - b + 1);
}

std::string formatResult(double v) {
  if (std::isnan(v))
    return "NaN";
  if (std::isinf(v))
    return v > 0 ? "Infinity" : "-Infinity";
  if (v == std::floor(v) && std::fabs(v) < 9.007199254740992e15)
    return std::to_string(static_cast<long long>(v));
  char buf[64];
  std::snprintf(buf, sizeof(buf), "%.10g", v);
  return buf;
}

} // namespace

int main(int argc, char **argv) {
  std::string program;

  if (argc > 1) {
    std::ifstream file(argv[1]);
    if (!file.is_open()) {
      std::cerr << "error: cannot open file '" << argv[1] << "'\n";
      return 1;
    }
    std::stringstream ss;
    ss << file.rdbuf();
    program = ss.str();
  } else {
    std::cout << "calc> " << std::flush;
    if (!std::getline(std::cin, program))
      return 0;
  }

  program = trim(program);
  if (program.empty())
    return 0;

  std::vector<std::string> vars;
  std::unique_ptr<calc::Expr> ast;
  try {
    Lexer lexer(program);
    calc::Parser parser(lexer, vars);
    ast = parser.parse();
    calc::SemanticAnalyzer analyzer(vars);
    analyzer.analyze(ast.get());
  } catch (const CalcError &e) {
    std::cerr << "error: " << e.what() << "\n";
    return 1;
  } catch (const std::exception &e) {
    std::cerr << "error: " << e.what() << "\n";
    return 1;
  }

  std::map<std::string, double> env;
  for (const std::string &name : vars) {
    double v = 0.0;
    std::cout << "Enter value for " << name << ": " << std::flush;
    if (!(std::cin >> v)) {
      std::cerr << "Invalid input for " << name << ". Using 0.\n";
      std::cin.clear();
      std::string dummy;
      std::getline(std::cin, dummy);
    }
    env[name] = v;
  }

  calc::Evaluator evaluator(env);
  const double result = evaluator.evaluate(ast.get());
  std::cout << "Result = " << formatResult(result) << "\n";
  return 0;
}
