// compiler_driver — end-to-end calc compiler.
//
// Pipeline per expression:
//   lex -> parse -> semantic analysis -> LLVM IR codegen -> ORC JIT -> run
//
// Interactive REPL by default. Options:
//   -q, --quiet   suppress prompts; print only results/errors (for scripts)
//   --ir          print the generated LLVM IR to stderr before JIT
//
// Results are printed as "Result = <value>"; front-end diagnostics are
// printed as "error: <message> (line L, column C)" and the REPL continues.

#include <cmath>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "llvm/ExecutionEngine/Orc/Core.h"
#include "llvm/ExecutionEngine/Orc/LLJIT.h"
#include "llvm/ExecutionEngine/Orc/ThreadSafeModule.h"
#include "llvm/Support/Error.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/raw_ostream.h"

#include "../../lexer/lexer.h"
#include "../../parser/Parser.h"
#include "../../semantics/SemanticAnalyzer.h"
#include "../../codegen/CodeGen.h"

namespace {

bool quiet = false;
bool printIR = false;

std::string trim(const std::string &s) {
  const size_t b = s.find_first_not_of(" \t\r\n");
  if (b == std::string::npos)
    return "";
  const size_t e = s.find_last_not_of(" \t\r\n");
  return s.substr(b, e - b + 1);
}

// Whole numbers print as integers; everything else with up to 10
// significant digits (trailing zeros trimmed by %g).
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
  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg == "-q" || arg == "--quiet")
      quiet = true;
    else if (arg == "--ir")
      printIR = true;
    else {
      std::cerr << "Unknown option: " << arg << "\n"
                << "Usage: compiler_driver [-q|--quiet] [--ir]\n";
      return 1;
    }
  }

  // Initialise LLVM targets / MC / AsmParsers so ORC JIT can create a
  // target machine for the host architecture.
  llvm::InitializeAllTargets();
  llvm::InitializeAllTargetMCs();
  llvm::InitializeAllAsmPrinters();
  llvm::InitializeAllAsmParsers();

  auto JITOrErr = llvm::orc::LLJITBuilder().create();
  if (!JITOrErr) {
    llvm::errs() << "Failed to create ORC JIT: "
                 << llvm::toString(JITOrErr.takeError()) << "\n";
    return 1;
  }
  std::unique_ptr<llvm::orc::LLJIT> JIT = std::move(*JITOrErr);

  unsigned expressionCount = 0;

  while (true) {
    if (!quiet) {
      std::cout << "calc> " << std::flush;
    }

    std::string line;
    if (!std::getline(std::cin, line))
      break; // EOF
    if (std::cin.bad())
      break;

    line = trim(line);
    if (line.empty())
      continue; // skip blank / whitespace-only lines

    std::vector<std::string> vars;
    std::unique_ptr<calc::Expr> ast;

    // ---- 1-2. Lex & parse ----------------------------------------------
    try {
      Lexer lexer(line);
      calc::Parser parser(lexer, vars);
      ast = parser.parse();
    } catch (const CalcError &e) {
      std::cerr << "error: " << e.what() << "\n";
      continue;
    } catch (const std::exception &e) {
      std::cerr << "error: " << e.what() << "\n";
      continue;
    }

    // ---- 3. Semantic analysis -------------------------------------------
    try {
      calc::SemanticAnalyzer analyzer(vars);
      analyzer.analyze(ast.get());
    } catch (const CalcError &e) {
      std::cerr << "error: " << e.what() << "\n";
      continue;
    }

    // ---- 4. Codegen ------------------------------------------------------
    // A unique symbol suffix per expression: all expressions share the
    // JIT's default dylib, and ORC rejects redefinition of a symbol
    // within a single dylib.
    const std::string suffix = "_" + std::to_string(expressionCount++);
    auto ctx = std::make_unique<llvm::LLVMContext>();
    calc::CodeGen gen(*ctx, "calc_module" + suffix, vars, suffix);
    try {
      gen.codegen(ast.get());
    } catch (const CalcError &e) {
      std::cerr << "error: " << e.what() << "\n";
      continue;
    }

    if (printIR)
      gen.module()->print(llvm::errs(), nullptr);

    // ---- 5. JIT compile ---------------------------------------------------
    auto TSM = llvm::orc::ThreadSafeModule(gen.takeModule(), std::move(ctx));
    if (auto Err = JIT->addIRModule(std::move(TSM))) {
      std::cerr << "error: JIT compilation failed: "
                << llvm::toString(std::move(Err)) << "\n";
      continue;
    }

    auto sym = JIT->lookup("evaluate_wrapper" + suffix);
    if (!sym) {
      std::cerr << "error: JIT lookup failed: "
                << llvm::toString(sym.takeError()) << "\n";
      continue;
    }

    // Signature: double evaluate_wrapper(double* args, int count)
    using WrapperFn = double(double *, int);
    auto wrapperFn = (*sym).toPtr<WrapperFn>();

    // ---- 6. Prompt for variable values ------------------------------------
    std::vector<double> values(vars.size(), 0.0);
    for (size_t i = 0; i < vars.size(); ++i) {
      if (!quiet)
        std::cout << "Enter value for " << vars[i] << ": " << std::flush;
      double v;
      if (!(std::cin >> v)) {
        std::cerr << "Invalid input for " << vars[i] << ". Using 0.\n";
        std::cin.clear();
        std::string dummy;
        std::getline(std::cin, dummy);
      } else {
        values[i] = v;
      }
      if (std::cin.bad())
        break;
    }
    if (std::cin.bad())
      break;

    // ---- 7-8. Evaluate & print ---------------------------------------------
    const double result =
        wrapperFn(values.data(), static_cast<int>(values.size()));
    std::cout << "Result = " << formatResult(result) << "\n";
  }

  return 0;
}
