#pragma once

#include <vector>
#include <memory>
#include "token.h"
#include "ast.h"

class Parser {
public:
  explicit Parser(const std::vector<Token>& tokens);
  
  std::shared_ptr<Program> parse();
  
private:
  std::vector<Token> tokens;
  size_t current;
  
  Token peek() const;
  Token peekNext() const;
  Token advance();
  bool check(TokenType type) const;
  bool match(TokenType type);
  bool match(const std::vector<TokenType>& types);
  Token consume(TokenType type, const std::string& message);
  bool isAtEnd() const;
  
  // Parsing methods
  std::shared_ptr<FunctionDecl> functionDeclaration();
  
  std::shared_ptr<Statement> statement();
  std::shared_ptr<Statement> blockStatement();
  std::shared_ptr<Statement> ifStatement();
  std::shared_ptr<Statement> whileStatement();
  std::shared_ptr<Statement> parallelBlock();
  std::shared_ptr<Statement> variableDeclaration();
  std::shared_ptr<Statement> returnStatement();
  std::shared_ptr<Statement> expressionStatement();
  
  std::shared_ptr<Expression> expression();
  std::shared_ptr<Expression> assignment();
  std::shared_ptr<Expression> logicalOr();
  std::shared_ptr<Expression> logicalAnd();
  std::shared_ptr<Expression> equality();
  std::shared_ptr<Expression> comparison();
  std::shared_ptr<Expression> addition();
  std::shared_ptr<Expression> multiplication();
  std::shared_ptr<Expression> unary();
  std::shared_ptr<Expression> call();
  std::shared_ptr<Expression> primary();
  
  void error(const std::string& message);
  void synchronize();
};
