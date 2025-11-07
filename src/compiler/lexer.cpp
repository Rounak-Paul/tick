#include "lexer.h"
#include <cctype>
#include <unordered_map>

static const std::unordered_map<std::string, TokenType> keywords = {
  {"clock", TokenType::CLOCK},
  {"proc", TokenType::PROC},
  {"func", TokenType::FUNC},
  {"chan", TokenType::CHAN},
  {"on", TokenType::ON},
  {"if", TokenType::IF},
  {"let", TokenType::LET},
  {"static", TokenType::STATIC},
  {"while", TokenType::WHILE},
  {"for", TokenType::FOR},
  {"return", TokenType::RETURN},
  {"println", TokenType::PRINTLN},
  {"par", TokenType::PAR},
  {"is", TokenType::IS},
  {"some", TokenType::SOME},
  {"none", TokenType::NONE},
  {"true", TokenType::TRUE},
  {"false", TokenType::FALSE},
  {"hz", TokenType::HZ},
};

Lexer::Lexer(const std::string& source)
  : source(source), current(0), line(1), column(1) {}

Token Lexer::nextToken() {
  skipWhitespace();
  
  if (isAtEnd()) {
    return makeToken(TokenType::EOF_TOKEN, "");
  }
  
  char c = advance();
  
  // Single-character tokens
  switch (c) {
    case '(': return makeToken(TokenType::LPAREN, "(");
    case ')': return makeToken(TokenType::RPAREN, ")");
    case '{': return makeToken(TokenType::LBRACE, "{");
    case '}': return makeToken(TokenType::RBRACE, "}");
    case '[': return makeToken(TokenType::LBRACKET, "[");
    case ']': return makeToken(TokenType::RBRACKET, "]");
    case ';': return makeToken(TokenType::SEMICOLON, ";");
    case ',': return makeToken(TokenType::COMMA, ",");
    case '.': return makeToken(TokenType::DOT, ".");
    case ':': return makeToken(TokenType::COLON, ":");
    case '+': return makeToken(TokenType::PLUS, "+");
    case '-':
      if (peek() == '>') {
        advance();
        return makeToken(TokenType::ARROW, "->");
      }
      return makeToken(TokenType::MINUS, "-");
    case '*': return makeToken(TokenType::MUL, "*");
    case '/':
      if (peek() == '/') {
        skipComment();
        return nextToken();
      }
      return makeToken(TokenType::DIV, "/");
    case '%': return makeToken(TokenType::MOD, "%");
    case '=':
      if (peek() == '=') {
        advance();
        return makeToken(TokenType::EQ, "==");
      }
      return makeToken(TokenType::ASSIGN, "=");
    case '!':
      if (peek() == '=') {
        advance();
        return makeToken(TokenType::NE, "!=");
      }
      return makeToken(TokenType::NOT, "!");
    case '<':
      if (peek() == '=') {
        advance();
        return makeToken(TokenType::LE, "<=");
      }
      return makeToken(TokenType::LT, "<");
    case '>':
      if (peek() == '=') {
        advance();
        return makeToken(TokenType::GE, ">=");
      }
      return makeToken(TokenType::GT, ">");
    case '&':
      if (peek() == '&') {
        advance();
        return makeToken(TokenType::AND, "&&");
      }
      break;
    case '|':
      if (peek() == '|') {
        advance();
        return makeToken(TokenType::OR, "||");
      }
      break;
  }
  
  // Multi-character tokens
  if (c == '"') return scanString();
  if (isDigit(c)) {
    current--;
    return scanNumber();
  }
  if (isAlpha(c)) {
    current--;
    return scanIdentifier();
  }
  
  return makeToken(TokenType::ERROR, std::string(1, c));
}

std::vector<Token> Lexer::tokenize() {
  std::vector<Token> tokens;
  Token token;
  
  do {
    token = nextToken();
    tokens.push_back(token);
  } while (token.type != TokenType::EOF_TOKEN);
  
  return tokens;
}

char Lexer::peek() const {
  if (isAtEnd()) return '\0';
  return source[current];
}

char Lexer::peekNext() const {
  if (current + 1 >= source.size()) return '\0';
  return source[current + 1];
}

char Lexer::advance() {
  if (isAtEnd()) return '\0';
  char c = source[current++];
  column++;
  if (c == '\n') {
    line++;
    column = 1;
  }
  return c;
}

void Lexer::skipWhitespace() {
  while (!isAtEnd()) {
    char c = peek();
    if (c == ' ' || c == '\r' || c == '\t' || c == '\n') {
      advance();
    } else {
      break;
    }
  }
}

void Lexer::skipComment() {
  // Skip '//' comments
  while (!isAtEnd() && peek() != '\n') {
    advance();
  }
}

Token Lexer::makeToken(TokenType type, const std::string& lexeme) {
  return Token(type, lexeme, line, column);
}

Token Lexer::scanNumber() {
  std::string number;
  int startColumn = column;
  
  while (!isAtEnd() && isDigit(peek())) {
    number += advance();
  }
  
  if (peek() == '.' && isDigit(peekNext())) {
    number += advance(); // consume '.'
    while (!isAtEnd() && isDigit(peek())) {
      number += advance();
    }
    Token token = makeToken(TokenType::FLOAT, number);
    token.literal = std::stod(number);
    return token;
  }
  
  Token token = makeToken(TokenType::INT, number);
  token.literal = std::stoi(number);
  return token;
}

Token Lexer::scanString() {
  std::string value;
  int startColumn = column;
  
  while (!isAtEnd() && peek() != '"') {
    if (peek() == '\\') {
      advance();
      switch (peek()) {
        case 'n': value += '\n'; advance(); break;
        case 't': value += '\t'; advance(); break;
        case 'r': value += '\r'; advance(); break;
        case '"': value += '"'; advance(); break;
        case '\\': value += '\\'; advance(); break;
        default: value += advance();
      }
    } else {
      value += advance();
    }
  }
  
  if (isAtEnd()) {
    return makeToken(TokenType::ERROR, "Unterminated string");
  }
  
  advance(); // consume closing '"'
  
  Token token = makeToken(TokenType::STRING, "\"" + value + "\"");
  token.literal = value;
  return token;
}

Token Lexer::scanIdentifier() {
  std::string ident;
  
  while (!isAtEnd() && isAlphaNumeric(peek())) {
    ident += advance();
  }
  
  TokenType type = TokenType::IDENTIFIER;
  auto it = keywords.find(ident);
  if (it != keywords.end()) {
    type = it->second;
  }
  
  Token token = makeToken(type, ident);
  if (type == TokenType::IDENTIFIER) {
    token.literal = ident;
  }
  return token;
}

bool Lexer::isAtEnd() const {
  return current >= source.size();
}

bool Lexer::isDigit(char c) const {
  return c >= '0' && c <= '9';
}

bool Lexer::isAlpha(char c) const {
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

bool Lexer::isAlphaNumeric(char c) const {
  return isAlpha(c) || isDigit(c);
}
