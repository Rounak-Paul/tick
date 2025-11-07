#pragma once

#include <string>
#include <variant>

enum class TokenType {
  // Keywords
  CLOCK, PROC, CHAN, ON, IF, LET, STATIC, WHILE, FOR, RETURN, PRINTLN,
  PAR, IS, SOME, NONE, TRUE, FALSE, IN, OUT, FUNC,
  
  // Operators
  PLUS, MINUS, MUL, DIV, MOD, ASSIGN, EQ, NE, LT, LE, GT, GE, AND, OR, NOT,
  
  // Punctuation
  LPAREN, RPAREN, LBRACE, RBRACE, LBRACKET, RBRACKET,
  SEMICOLON, COMMA, DOT, COLON, ARROW,
  
  // Literals
  INT, FLOAT, STRING, IDENTIFIER,
  
  // Special
  HZ,
  TRY_RECV, SEND, BIND,
  
  // End of file
  EOF_TOKEN,
  
  // Error
  ERROR
};

struct Token {
  TokenType type;
  std::string lexeme;
  std::variant<int, double, std::string> literal;
  int line;
  int column;
  
  Token() : type(TokenType::ERROR), line(0), column(0) {}
  
  Token(TokenType t, const std::string& lex, int l = 0, int c = 0)
    : type(t), lexeme(lex), line(l), column(c) {}
};
