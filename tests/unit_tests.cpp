// Unit tests for the calc front end: lexer, parser, semantic analysis,
// and the tree-walking evaluator. No LLVM JIT required.

#include <cmath>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "../lexer/lexer.h"
#include "../parser/Parser.h"
#include "../semantics/SemanticAnalyzer.h"
#include "../evaluator/Evaluator.h"

// ---------------------------------------------------------------------------
// Minimal test framework
// ---------------------------------------------------------------------------
static int g_checks = 0;
static int g_failures = 0;

static void reportFailure(const char *file, int line, const std::string &what) {
  ++g_failures;
  std::cerr << "FAIL " << file << ":" << line << ": " << what << "\n";
}

#define CHECK(cond)                                                    \
  do {                                                                 \
    ++g_checks;                                                        \
    if (!(cond))                                                       \
      reportFailure(__FILE__, __LINE__, "CHECK failed: " #cond);       \
  } while (0)

#define CHECK_EQ(actual, expected)                                             \
  do {                                                                         \
    ++g_checks;                                                                \
    if (!((actual) == (expected))) {                                           \
      std::ostringstream oss;                                                  \
      oss << "CHECK_EQ failed: " << #actual << " == " << #expected             \
          << " (actual: " << (actual) << ", expected: " << (expected) << ")";  \
      reportFailure(__FILE__, __LINE__, oss.str());                            \
    }                                                                          \
  } while (0)

#define CHECK_NEAR(actual, expected, eps)                                          \
  do {                                                                             \
    ++g_checks;                                                                    \
    const double a_ = (actual), e_ = (expected);                                   \
    if (!(std::fabs(a_ - e_) <= (eps))) {                                          \
      std::ostringstream oss;                                                      \
      oss << "CHECK_NEAR failed: |" << a_ << " - " << e_ << "| > " << (eps);       \
      reportFailure(__FILE__, __LINE__, oss.str());                                \
    }                                                                              \
  } while (0)

// CHECK_THROWS_CALC(expr, substring): expr must throw CalcError whose
// message contains `substring`.
#define CHECK_THROWS_CALC(expr, substring)                                          \
  do {                                                                              \
    ++g_checks;                                                                     \
    bool threw_ = false;                                                            \
    std::string msg_;                                                               \
    try {                                                                           \
      (void)(expr);                                                                 \
    } catch (const CalcError &e_) {                                                 \
      threw_ = true;                                                                \
      msg_ = e_.what();                                                             \
    } catch (const std::exception &e_) {                                            \
      reportFailure(__FILE__, __LINE__,                                             \
                    std::string("expected CalcError, got std::exception: ") +       \
                        e_.what());                                                 \
      break;                                                                        \
    }                                                                               \
    if (!threw_)                                                                    \
      reportFailure(__FILE__, __LINE__, "expected CalcError, none thrown");         \
    else if (msg_.find(substring) == std::string::npos)                             \
      reportFailure(__FILE__, __LINE__,                                             \
                    std::string("CalcError message '") + msg_ +                     \
                    "' does not contain '" + (substring) + "'");                    \
  } while (0)

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------
struct Tok {
  Token::TokenKind kind;
  std::string text;
  SourcePosition pos;
};

static std::vector<Tok> lexAll(const std::string &src) {
  std::vector<Tok> out;
  Lexer L(src);
  Token T;
  do {
    L.next(T);
    out.push_back({T.getKind(), T.getText().str(), T.getPosition()});
  } while (!T.is(Token::eoi));
  return out;
}

static bool throwsCalcError(const std::string &src) {
  try {
    lexAll(src);
    return false;
  } catch (const CalcError &) {
    return true;
  }
}

static std::unique_ptr<calc::Expr> parseAst(
    const std::string &src, std::vector<std::string> *vars = nullptr) {
  Lexer L(src);
  std::vector<std::string> v;
  calc::Parser P(L, v);
  auto ast = P.parse();
  if (vars)
    *vars = v;
  return ast;
}

static std::string parseToString(const std::string &src) {
  return parseAst(src)->toString();
}

static double evalExpr(const std::string &src,
                       const std::map<std::string, double> &env = {}) {
  std::vector<std::string> vars;
  auto ast = parseAst(src, &vars);
  calc::SemanticAnalyzer A(vars);
  A.analyze(ast.get());
  calc::Evaluator E(env);
  return E.evaluate(ast.get());
}

// ---------------------------------------------------------------------------
// Lexer tests
// ---------------------------------------------------------------------------
static void testLexer() {
  // Basic token stream (README example).
  auto toks = lexAll("with a, b: a * (4 + b)");
  CHECK_EQ(toks.size(), (size_t)13);
  CHECK(toks[0].kind == Token::KW_with && toks[0].text == "with");
  CHECK(toks[1].kind == Token::ident && toks[1].text == "a");
  CHECK(toks[2].kind == Token::comma);
  CHECK(toks[3].kind == Token::ident && toks[3].text == "b");
  CHECK(toks[4].kind == Token::colon);
  CHECK(toks[5].kind == Token::ident);
  CHECK(toks[6].kind == Token::star);
  CHECK(toks[7].kind == Token::l_paren);
  CHECK(toks[8].kind == Token::number && toks[8].text == "4");
  CHECK(toks[9].kind == Token::plus);
  CHECK(toks[10].kind == Token::ident);
  CHECK(toks[11].kind == Token::r_paren);
  CHECK(toks[12].kind == Token::eoi);

  // Whitespace is skipped and does not change the stream.
  auto sparse = lexAll("  with\t a ,\n b : a*(4+b)  ");
  CHECK_EQ(sparse.size(), toks.size());
  for (size_t i = 0; i < toks.size(); ++i)
    CHECK(sparse[i].kind == toks[i].kind && sparse[i].text == toks[i].text);

  // Numbers.
  CHECK(lexAll("42")[0].kind == Token::number);
  CHECK(lexAll("3.14")[0].text == "3.14");
  CHECK(lexAll("0.5")[0].text == "0.5");
  CHECK(lexAll("0")[0].text == "0");

  // Keywords vs identifiers (case-sensitive).
  CHECK(lexAll("with")[0].kind == Token::KW_with);
  CHECK(lexAll("withx")[0].kind == Token::ident);
  CHECK(lexAll("WITH")[0].kind == Token::ident);

  // Positions: 1-based line and column.
  auto pos = lexAll("1 +\n2");
  CHECK(pos[0].pos.Line == 1 && pos[0].pos.Column == 1);
  CHECK(pos[1].pos.Line == 1 && pos[1].pos.Column == 3);
  CHECK(pos[2].pos.Line == 2 && pos[2].pos.Column == 1);

  // Empty input.
  auto empty = lexAll("");
  CHECK_EQ(empty.size(), (size_t)1);
  CHECK(empty[0].kind == Token::eoi);

  // Malformed input throws CalcError.
  CHECK(throwsCalcError("@"));
  CHECK(throwsCalcError("1.2.3"));
  CHECK(throwsCalcError(".5"));
  CHECK(throwsCalcError("3."));
  CHECK(throwsCalcError("1 + ; 2"));
}

// ---------------------------------------------------------------------------
// Parser tests
// ---------------------------------------------------------------------------
static void testParser() {
  std::vector<std::string> vars;

  auto ast = parseAst("with a, b: a * (4 + b)", &vars);
  CHECK_EQ(vars.size(), (size_t)2);
  CHECK(vars[0] == "a" && vars[1] == "b");
  CHECK_EQ(ast->toString(), std::string("a * (4 + b)"));

  // Precedence and associativity (canonical form: no outer parens on the
  // root, parens on every nested binary, unary as "-x").
  CHECK_EQ(parseToString("1 + 2 * 3"), std::string("1 + (2 * 3)"));
  CHECK_EQ(parseToString("2 * 3 + 1"), std::string("(2 * 3) + 1"));
  CHECK_EQ(parseToString("10 - 3 - 2"), std::string("(10 - 3) - 2"));
  CHECK_EQ(parseToString("100 / 10 / 5"), std::string("(100 / 10) / 5"));
  CHECK_EQ(parseToString("(1 + 2) * 3"), std::string("(1 + 2) * 3"));

  // Unary minus binds tighter than * and /.
  CHECK_EQ(parseToString("-2 * 3"), std::string("(-2) * 3"));
  CHECK_EQ(parseToString("2 * -3"), std::string("2 * -3"));
  CHECK_EQ(parseToString("-(2 * 3)"), std::string("-(2 * 3)"));
  CHECK_EQ(parseToString("- -5"), std::string("-(-5)"));

  // No 'with' clause.
  vars.clear();
  ast = parseAst("1 + 2", &vars);
  CHECK(vars.empty());
  CHECK_EQ(ast->toString(), std::string("1 + 2"));

  // Error cases.
  CHECK_THROWS_CALC(parseAst("1 +"), "expected a number, identifier");
  CHECK_THROWS_CALC(parseAst("(1"), "expected ')'");
  CHECK_THROWS_CALC(parseAst("with a: 1 2"), "expected end-of-input");
  CHECK_THROWS_CALC(parseAst("with a,, b: a"), "expected identifier");
  CHECK_THROWS_CALC(parseAst("with : 1"), "expected identifier");
  CHECK_THROWS_CALC(parseAst("with a b: a"), "expected ':'");
  CHECK_THROWS_CALC(parseAst("with a, a: a"), "duplicate variable declaration");
  CHECK_THROWS_CALC(parseAst(""), "expected a number, identifier");
  CHECK_THROWS_CALC(parseAst("with a: + 1"), "expected a number, identifier");
  CHECK_THROWS_CALC(parseAst("with a: )"), "expected a number, identifier");
}

// ---------------------------------------------------------------------------
// Semantic analysis tests
// ---------------------------------------------------------------------------
static void testSemantics() {
  // Undeclared variable.
  {
    std::vector<std::string> vars;
    auto ast = parseAst("with a: b + 1", &vars);
    calc::SemanticAnalyzer A(vars);
    CHECK_THROWS_CALC(A.analyze(ast.get()), "undeclared variable 'b'");
  }
  // Variable without a 'with' clause.
  {
    std::vector<std::string> vars;
    auto ast = parseAst("a + 1", &vars);
    calc::SemanticAnalyzer A(vars);
    CHECK_THROWS_CALC(A.analyze(ast.get()), "undeclared variable 'a'");
  }
  // All declared: ok.
  {
    std::vector<std::string> vars;
    auto ast = parseAst("with a, b: a * (4 + b)", &vars);
    calc::SemanticAnalyzer A(vars);
    A.analyze(ast.get()); // must not throw
  }
  // Unused declared variable: ok.
  {
    std::vector<std::string> vars;
    auto ast = parseAst("with a, b: a", &vars);
    calc::SemanticAnalyzer A(vars);
    A.analyze(ast.get()); // must not throw
  }
}

// ---------------------------------------------------------------------------
// Evaluator tests
// ---------------------------------------------------------------------------
static void testEvaluator() {
  CHECK_EQ(evalExpr("1 + 2"), 3.0);
  CHECK_EQ(evalExpr("2 * 3"), 6.0);
  CHECK_EQ(evalExpr("7 - 10"), -3.0);
  CHECK_EQ(evalExpr("10 / 4"), 2.5);
  CHECK_EQ(evalExpr("2 + 3 * 4"), 14.0);
  CHECK_EQ(evalExpr("10 - 3 - 2"), 5.0);
  CHECK_EQ(evalExpr("100 / 10 / 5"), 2.0);
  CHECK_EQ(evalExpr("-5"), -5.0);
  CHECK_EQ(evalExpr("-2 * 3"), -6.0);
  CHECK_EQ(evalExpr("-(2 * 3)"), -6.0);
  CHECK_NEAR(evalExpr("0.1 + 0.2"), 0.3, 1e-9);

  // Division by zero follows IEEE 754.
  CHECK(std::isinf(evalExpr("1 / 0")) && evalExpr("1 / 0") > 0);
  CHECK(std::isinf(evalExpr("-1 / 0")) && evalExpr("-1 / 0") < 0);
  CHECK(std::isnan(evalExpr("0 / 0")));

  // Variables.
  std::map<std::string, double> env;
  env = {{"a", 3.0}, {"b", 2.0}};
  CHECK_EQ(evalExpr("with a, b: a * (4 + b)", env), 18.0);
  env = {{"m", 80.0}, {"v", 5.0}};
  CHECK_EQ(evalExpr("with m, v: 0.5 * m * v * v", env), 1000.0);
}

// ---------------------------------------------------------------------------

int main() {
  testLexer();
  testParser();
  testSemantics();
  testEvaluator();

  std::cout << g_checks - g_failures << " / " << g_checks << " checks passed";
  if (g_failures) {
    std::cout << " — " << g_failures << " FAILED\n";
    return 1;
  }
  std::cout << " — all OK\n";
  return 0;
}
