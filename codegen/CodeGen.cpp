#include "CodeGen.h"

#include <stdexcept>

#include <llvm/IR/Function.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Type.h>

namespace calc {

CodeGen::CodeGen(llvm::LLVMContext &ctx,
                 const std::string &moduleName,
                 const std::vector<std::string> &varNames,
                 const std::string &nameSuffix)
    : context_(ctx),
      module_(std::make_unique<llvm::Module>(moduleName, context_)),
      builder_(context_), varNames_(varNames), nameSuffix_(nameSuffix) {
  buildFunction();
  buildWrapper();

  // Restore builder insertion point to the evaluate function's entry block
  // (buildWrapper() temporarily moved it to the wrapper's block).
  builder_.SetInsertPoint(entryBlock_);
}

llvm::Module *CodeGen::module() { return module_.get(); }

std::unique_ptr<llvm::Module> CodeGen::takeModule() {
  return std::move(module_);
}

llvm::Function *CodeGen::wrapper() {
  return module_->getFunction("evaluate_wrapper" + nameSuffix_);
}

void CodeGen::codegen(const Expr *e) {
  codegenExpr(e);
  finalize();
}

void CodeGen::buildFunction() {
  // Create the function "evaluate" with double return type
  // and one double parameter per variable
  llvm::Type *doubleType = llvm::Type::getDoubleTy(context_);
  std::vector<llvm::Type *> paramTypes;
  for (size_t i = 0; i < varNames_.size(); ++i) {
    (void)i;
    paramTypes.push_back(doubleType);
  }

  llvm::FunctionType *funcType =
      llvm::FunctionType::get(doubleType, paramTypes, false);
  function_ = llvm::Function::Create(
      funcType, llvm::Function::ExternalLinkage, "evaluate" + nameSuffix_,
      module_.get());

  // Set parameter names to match variable names
  size_t idx = 0;
  for (auto &arg : function_->args()) {
    arg.setName(varNames_[idx++]);
  }

  // Create the entry basic block
  entryBlock_ = llvm::BasicBlock::Create(context_, "entry", function_);
  builder_.SetInsertPoint(entryBlock_);

  // Map each variable name to its corresponding function parameter
  idx = 0;
  for (auto &arg : function_->args()) {
    variables_[varNames_[idx++]] = &arg;
  }
}

llvm::Value *CodeGen::codegenExpr(const Expr *e) {
  llvm::Value *val = nullptr;
  if (auto *num = dynamic_cast<const NumberExpr *>(e)) {
    val = codegenNumber(num);
  } else if (auto *var = dynamic_cast<const VariableExpr *>(e)) {
    val = codegenVariable(var);
  } else if (auto *bin = dynamic_cast<const BinaryExpr *>(e)) {
    val = codegenBinary(bin);
  } else if (auto *uni = dynamic_cast<const UnaryExpr *>(e)) {
    val = codegenUnary(uni);
  } else {
    throw std::logic_error("unknown AST node type");
  }
  lastValue_ = val;
  return val;
}

void CodeGen::finalize() {
  builder_.CreateRet(lastValue_);
}

llvm::Value *CodeGen::codegenNumber(const NumberExpr *e) {
  return llvm::ConstantFP::get(context_, llvm::APFloat(e->getValue()));
}

llvm::Value *CodeGen::codegenVariable(const VariableExpr *e) {
  auto it = variables_.find(e->getName().str());
  if (it == variables_.end()) {
    // Semantic analysis should have caught this; fail loudly instead of
    // emitting invalid IR.
    throw CalcError(e->getPosition(),
                    "undeclared variable '" + e->getName().str() + "'");
  }
  return it->second;
}

llvm::Value *CodeGen::codegenBinary(const BinaryExpr *e) {
  llvm::Value *lhs = codegenExpr(e->getLHS());
  llvm::Value *rhs = codegenExpr(e->getRHS());

  switch (e->getOp()) {
  case BinaryOp::plus:
    return builder_.CreateFAdd(lhs, rhs, "addtmp");
  case BinaryOp::minus:
    return builder_.CreateFSub(lhs, rhs, "subtmp");
  case BinaryOp::multiply:
    return builder_.CreateFMul(lhs, rhs, "multmp");
  case BinaryOp::divide:
    return builder_.CreateFDiv(lhs, rhs, "divtmp");
  }
  return nullptr;
}

llvm::Value *CodeGen::codegenUnary(const UnaryExpr *e) {
  llvm::Value *operand = codegenExpr(e->getOperand());
  return builder_.CreateFNeg(operand, "negtmp");
}

void CodeGen::buildWrapper() {
  // Generate a wrapper: double evaluate_wrapper(double* args, int count)
  // that calls evaluate(args[0], args[1], ..., args[n-1])
  llvm::Type *doubleTy = llvm::Type::getDoubleTy(context_);
  llvm::Type *doublePtrTy = llvm::PointerType::get(context_, 0);
  llvm::Type *intTy = llvm::Type::getInt32Ty(context_);

  std::vector<llvm::Type *> paramTypes = { doublePtrTy, intTy };
  llvm::FunctionType *wrapperType =
      llvm::FunctionType::get(doubleTy, paramTypes, false);

  llvm::Function *wrapper = llvm::Function::Create(
      wrapperType, llvm::Function::ExternalLinkage,
      "evaluate_wrapper" + nameSuffix_, module_.get());

  // Set builder insertion point to the wrapper's entry block
  // (otherwise instructions are appended to the evaluate function's
  //  basic block, leaving evaluate_wrapper bodyless and corrupting IR).
  llvm::BasicBlock *wrapperBB =
      llvm::BasicBlock::Create(context_, "entry", wrapper);
  builder_.SetInsertPoint(wrapperBB);

  // Get the two parameters
  auto it = wrapper->arg_begin();
  llvm::Value *argsPtr = &*it;
  ++it;
  llvm::Value *countVal = &*it;
  (void)countVal;

  // Load each argument from the array and call evaluate
  std::vector<llvm::Value *> callArgs;
  for (int i = 0; i < static_cast<int>(varNames_.size()); ++i) {
    llvm::Value *idx = llvm::ConstantInt::get(intTy, i);
    llvm::Value *gep =
        builder_.CreateInBoundsGEP(doubleTy, argsPtr, {idx}, "arg_load");
    llvm::Value *val = builder_.CreateLoad(doubleTy, gep, "arg");
    callArgs.push_back(val);
  }

  // Create the call to evaluate(...)
  auto *evalCall = builder_.CreateCall(function_, callArgs, "eval_call");

  // Return the result of evaluate()
  builder_.CreateRet(evalCall);
}

} // namespace calc
