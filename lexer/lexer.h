#ifndef LEXER_H
#define LEXER_H

#include <stdexcept>
#include <string>

#include "llvm/ADT/StringRef.h"
#include "llvm/Support/Compiler.h"
#include "llvm/Support/MemoryBuffer.h"

// 1-based position in the source text.
struct SourcePosition {
  unsigned Line = 1;
  unsigned Column = 1;
};

// Base class for all front-end diagnostics (lexer, parser, semantics).
// what() carries the source position: "<message> (line L, column C)".
class CalcError : public std::runtime_error {
public:
  CalcError(SourcePosition Pos, const std::string &Message)
      : std::runtime_error(formatMessage(Pos, Message)), Position(Pos) {}

  static std::string formatMessage(SourcePosition Pos,
                                   const std::string &Message) {
    return Message + " (line " + std::to_string(Pos.Line) +
           ", column " + std::to_string(Pos.Column) + ")";
  }

  SourcePosition getPosition() const { return Position; }

private:
  SourcePosition Position;
};

class Lexer;

class Token {
  friend class Lexer;

public:
  enum TokenKind : unsigned short {
    eoi, unknown, ident, number, comma, colon, plus,
    minus, star, slash, l_paren, r_paren, KW_with
  };

private:
  TokenKind Kind = unknown;
  llvm::StringRef Text;
  SourcePosition Pos;

public:
  TokenKind getKind() const { return Kind; }
  llvm::StringRef getText() const { return Text; }
  SourcePosition getPosition() const { return Pos; }

  bool is(TokenKind K) const { return Kind == K; }
  bool isOneOf(TokenKind K1, TokenKind K2) const { return is(K1) || is(K2); }
  template <typename... Ts>
  bool isOneOf(TokenKind K1, TokenKind K2, Ts... Ks) const {
    return is(K1) || isOneOf(K2, Ks...);
  }
};

class Lexer {
  const char *BufferStart = nullptr;
  const char *BufferPtr   = nullptr;
  unsigned Line   = 1;
  unsigned Column = 1;

public:
  // The buffer must be NUL-terminated (std::string, MemoryBuffer, ...).
  explicit Lexer(const llvm::StringRef &Buffer)
      : BufferStart(Buffer.begin()), BufferPtr(Buffer.begin()) {}

  // Fetch the next token. Throws CalcError on malformed input
  // (unexpected character, malformed number literal).
  void next(Token &token);

private:
  SourcePosition position() const { return {Line, Column}; }
  void advanceTo(const char *End);
  void formToken(Token &Result, const char *TokEnd, Token::TokenKind Kind);
};

#endif // LEXER_H
