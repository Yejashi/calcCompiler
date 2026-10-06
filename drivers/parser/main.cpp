#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "../parser/Parser.h"

static std::string readFile(const std::string &path) {
  std::ifstream file(path);
  if (!file.is_open()) {
    throw std::runtime_error("Cannot open file: " + path);
  }
  std::stringstream ss;
  ss << file.rdbuf();
  return ss.str();
}

static std::string exprTypeName(const calc::Expr *e) {
  if (e == nullptr) {
    return "null";
  }
  if (dynamic_cast<const calc::NumberExpr *>(e)) {
    return "NumberExpr";
  }
  if (dynamic_cast<const calc::VariableExpr *>(e)) {
    return "VariableExpr";
  }
  if (dynamic_cast<const calc::BinaryExpr *>(e)) {
    return "BinaryExpr";
  }
  if (dynamic_cast<const calc::UnaryExpr *>(e)) {
    return "UnaryExpr";
  }
  return "UnknownExpr";
}

int main(int argc, char *argv[]) {
  try {
    std::string input;

    if (argc > 1) {
      input = readFile(argv[1]);
    } else {
      input = "with a, b: a * (4 + b)";
    }

    Lexer lexer(input);
    std::vector<std::string> vars;
    calc::Parser parser(lexer, vars);
    auto ast = parser.parse();

    std::cout << "Parsed successfully, root type: "
              << exprTypeName(ast.get()) << "\n";
    std::cout << "AST: " << ast->toString() << "\n";

    if (!vars.empty()) {
      std::cout << "Variables declared: ";
      for (size_t i = 0; i < vars.size(); ++i) {
        if (i > 0)
          std::cout << ", ";
        std::cout << vars[i];
      }
      std::cout << "\n";
    }
  } catch (const CalcError &e) {
    std::cerr << "error: " << e.what() << "\n";
    return 1;
  } catch (const std::exception &e) {
    std::cerr << "error: " << e.what() << "\n";
    return 1;
  }

  return 0;
}
