#ifndef CALC_CODEGEN_H
#define CALC_CODEGEN_H

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"

#include "../ast/AST.h"

namespace calc {

class CodeGen {
public:
  // nameSuffix is appended to the generated symbol names ("evaluate" and
  // "evaluate_wrapper") so that multiple expressions can coexist in one
  // JIT dylib without redefinition conflicts.
  explicit CodeGen(llvm::LLVMContext &ctx,
                   const std::string &moduleName,
                   const std::vector<std::string> &varNames,
                   const std::string &nameSuffix = "");

   llvm::Module *module();
   std::unique_ptr<llvm::Module> takeModule();

  // Generate a wrapper function that takes a double* array and count,
  // calls evaluate() with unpacked arguments. Returns the wrapper function.
  llvm::Function *wrapper();

  // Generate LLVM IR for the given expression and finalize the function.
  // Must be called after construction and before JIT compilation.
  void codegen(const Expr *e);

private:
  llvm::LLVMContext &context_;
  std::unique_ptr<llvm::Module> module_;
  llvm::IRBuilder<> builder_;

  // Map from variable name to LLVM value (the function parameter)
  std::map<std::string, llvm::Value *> variables_;

  // Last generated expression value (for ret instruction)
  llvm::Value *lastValue_ = nullptr;

  // Variable names for function parameters
  std::vector<std::string> varNames_;

  // Suffix appended to generated symbol names
  std::string nameSuffix_;

  // Function being built
  llvm::Function *function_ = nullptr;

  // Entry block of the evaluate function (saved so we can restore builder
  // insertion point after buildWrapper changes it to the wrapper's block).
  llvm::BasicBlock *entryBlock_ = nullptr;

  // Code generation methods
  llvm::Value *codegenExpr(const Expr *e);
  llvm::Value *codegenNumber(const NumberExpr *e);
  llvm::Value *codegenVariable(const VariableExpr *e);
  llvm::Value *codegenBinary(const BinaryExpr *e);
  llvm::Value *codegenUnary(const UnaryExpr *e);

  void buildFunction();
  void buildWrapper();
  void finalize();
};

} // namespace calc

#endif // CALC_CODEGEN_H
