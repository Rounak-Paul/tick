#pragma once

#include <string>
#include <vector>
#include "token.h"

class Lexer {
public:
  explicit Lexer(const std::string& source);
  
  Token nextToken();
  std::vector<Token> tokenize();
  
private:
  std::string source;
  size_t current;
  int line;
  int column;
  
  char peek() const;
  char peekNext() const;
  char advance();
  void skipWhitespace();
  void skipComment();
  
  Token makeToken(TokenType type, const std::string& lexeme);
  Token scanNumber();
  Token scanString();
  Token scanIdentifier();
  
  bool isAtEnd() const;
  bool isDigit(char c) const;
  bool isAlpha(char c) const;
  bool isAlphaNumeric(char c) const;
};
