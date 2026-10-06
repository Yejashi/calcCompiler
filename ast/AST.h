#ifndef CALC_AST_H
#define CALC_AST_H

#include <memory>
#include <string>

#include "llvm/ADT/StringRef.h"

#include "../lexer/lexer.h"

namespace calc {

enum class BinaryOp { plus, minus, multiply, divide };
enum class UnaryOp { minus };

class Expr {
public:
  virtual ~Expr() = default;

  // Render the subtree as a source-like string (for diagnostics/tests).
  // Canonical form: the root expression prints without outer parentheses;
  // every nested binary expression is parenthesized; a unary prints as
  // "-x" (its operand parenthesized when it is a binary or another unary).
  virtual std::string toString(bool isRoot = true) const = 0;

  // Source position of the node (the operator token for binary/unary).
  SourcePosition getPosition() const { return Pos; }

protected:
  explicit Expr(SourcePosition Pos) : Pos(Pos) {}

private:
  SourcePosition Pos;
};

class NumberExpr : public Expr {
  double Value;

public:
  NumberExpr(double Value, SourcePosition Pos) : Expr(Pos), Value(Value) {}
  double getValue() const { return Value; }
  std::string toString(bool isRoot = true) const override;
};

class VariableExpr : public Expr {
  std::string Name;

public:
  VariableExpr(std::string Name, SourcePosition Pos)
      : Expr(Pos), Name(std::move(Name)) {}
  llvm::StringRef getName() const { return Name; }
  std::string toString(bool isRoot = true) const override { return Name; }
};

class BinaryExpr : public Expr {
  BinaryOp Op;
  std::unique_ptr<Expr> LHS;
  std::unique_ptr<Expr> RHS;

public:
  BinaryExpr(BinaryOp Op, std::unique_ptr<Expr> LHS, std::unique_ptr<Expr> RHS,
             SourcePosition Pos)
      : Expr(Pos), Op(Op), LHS(std::move(LHS)), RHS(std::move(RHS)) {}

  BinaryOp getOp() const { return Op; }
  Expr *getLHS() const { return LHS.get(); }
  Expr *getRHS() const { return RHS.get(); }
  std::unique_ptr<Expr> takeLHS() { return std::move(LHS); }
  std::unique_ptr<Expr> takeRHS() { return std::move(RHS); }
  std::string toString(bool isRoot = true) const override;
};

class UnaryExpr : public Expr {
  UnaryOp Op;
  std::unique_ptr<Expr> Operand;

public:
  UnaryExpr(UnaryOp Op, std::unique_ptr<Expr> Operand, SourcePosition Pos)
      : Expr(Pos), Op(Op), Operand(std::move(Operand)) {}

  UnaryOp getOp() const { return Op; }
  Expr *getOperand() const { return Operand.get(); }
  std::unique_ptr<Expr> takeOperand() { return std::move(Operand); }
  std::string toString(bool isRoot = true) const override;
};

} // namespace calc

#endif // CALC_AST_H
