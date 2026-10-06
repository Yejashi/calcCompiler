#include "lexer.h"

namespace charinfo {
LLVM_READNONE inline bool isWhitespace(char c) {
  return c == ' ' || c == '\t' || c == '\f' || c == '\v' || c == '\r' || c == '\n';
}
LLVM_READNONE inline bool isDigit(char c) {
  return c >= '0' && c <= '9';
}
LLVM_READNONE inline bool isLetter(char c) {
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}
}

void Lexer::advanceTo(const char *End) {
  for (const char *p = BufferPtr; p != End; ++p) {
    if (*p == '\n') {
      ++Line;
      Column = 1;
    } else {
      ++Column;
    }
  }
  BufferPtr = End;
}

void Lexer::next(Token &token) {
  while (*BufferPtr && charinfo::isWhitespace(*BufferPtr))
    advanceTo(BufferPtr + 1);

  if (!*BufferPtr) {
    token.Kind = Token::eoi;
    token.Pos  = position();
    token.Text = llvm::StringRef(BufferPtr, 0);
    return;
  }

  const SourcePosition start = position();

  if (charinfo::isLetter(*BufferPtr)) {
    const char *end = BufferPtr + 1;
    while (charinfo::isLetter(*end))
      ++end;
    llvm::StringRef Name(BufferPtr, end - BufferPtr);
    formToken(token, end, (Name == "with") ? Token::KW_with : Token::ident);
    return;
  }

  if (charinfo::isDigit(*BufferPtr)) {
    const char *end = BufferPtr + 1;
    while (charinfo::isDigit(*end))
      ++end;
    if (*end == '.') {
      // A fractional part must have at least one digit after the dot.
      if (!charinfo::isDigit(end[1]))
        throw CalcError(start, "malformed number literal '" +
                                   std::string(BufferPtr, end - BufferPtr + 1) +
                                   "'");
      ++end;
      while (charinfo::isDigit(*end))
        ++end;
    }
    formToken(token, end, Token::number);
    return;
  }

  switch (*BufferPtr) {
#define CASE(ch, tok) case ch: formToken(token, BufferPtr + 1, tok); break
    CASE('+', Token::plus);
    CASE('-', Token::minus);
    CASE('*', Token::star);
    CASE('/', Token::slash);
    CASE('(', Token::l_paren);
    CASE(')', Token::r_paren);
    CASE(':', Token::colon);
    CASE(',', Token::comma);
#undef CASE
    default:
      throw CalcError(start,
                      std::string("unexpected character '") + *BufferPtr + "'");
  }
}

void Lexer::formToken(Token &Tok, const char *TokEnd, Token::TokenKind Kind) {
  Tok.Kind = Kind;
  Tok.Pos  = position();
  Tok.Text = llvm::StringRef(BufferPtr, TokEnd - BufferPtr);
  advanceTo(TokEnd);
}
