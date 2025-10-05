#include <iostream>
#include <memory>
#include "lexer.h"
#include "llvm/Support/ErrorOr.h"
#include "llvm/Support/raw_ostream.h"

static const char* tokenKindToString(Token::TokenKind K) {
  switch (K) {
  case Token::eoi:      return "eoi";
  case Token::unknown:  return "unknown";
  case Token::ident:    return "ident";
  case Token::number:   return "number";
  case Token::comma:    return "comma";
  case Token::colon:    return "colon";
  case Token::plus:     return "plus";
  case Token::minus:    return "minus";
  case Token::star:     return "star";
  case Token::slash:    return "slash";
  case Token::l_paren:  return "l_paren";
  case Token::r_paren:  return "r_paren";
  case Token::KW_with:  return "KW_with";
  }
  return "<?>"; // fallback
}

int main(int argc, char **argv) {
  std::unique_ptr<llvm::MemoryBuffer> MB;

  if (argc > 1) {
    auto BufOrErr = llvm::MemoryBuffer::getFileOrSTDIN(argv[1]);
    if (!BufOrErr) {
      llvm::errs() << "Error: could not open " << argv[1] << "\n";
      return 1;
    }
    MB = std::move(*BufOrErr);
  } else {
    MB = llvm::MemoryBuffer::getMemBufferCopy("with x: (a + b) * 42, y\n");
  }

  Lexer L(MB->getBuffer());
  Token T;
  do {
    L.next(T);
    std::cout << tokenKindToString(T.getKind());
    if (T.getText().size() > 0)
      std::cout << " [" << T.getText().str() << "]";
    std::cout << "\n";
  } while (!T.is(Token::eoi));

  return 0;
}
