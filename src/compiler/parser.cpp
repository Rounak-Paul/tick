#include "parser.h"
#include <iostream>

Parser::Parser(const std::vector<Token>& tokens)
  : tokens(tokens), current(0) {}

std::shared_ptr<Program> Parser::parse() {
  auto program = std::make_shared<Program>();
  
  while (!isAtEnd()) {
    try {
      if (check(TokenType::FUNC)) {
        auto decl = functionDeclaration();
        if (decl) {
          program->declarations.push_back(decl);
        }
      } else {
        // Skip unknown tokens to prevent infinite loops
        advance();
      }
    } catch (const std::exception& e) {
      std::cerr << "Parse error: " << e.what() << "\n";
      synchronize();
    }
  }
  
  return program;
}

Token Parser::peek() const {
  if (current >= tokens.size()) {
    static Token eofToken(TokenType::EOF_TOKEN, "");
    return eofToken;
  }
  return tokens[current];
}

Token Parser::peekNext() const {
  if (current + 1 >= tokens.size()) {
    static Token eofToken(TokenType::EOF_TOKEN, "");
    return eofToken;
  }
  return tokens[current + 1];
}

Token Parser::advance() {
  if (!isAtEnd()) current++;
  return tokens[current - 1];
}

bool Parser::check(TokenType type) const {
  if (isAtEnd()) return false;
  return peek().type == type;
}

bool Parser::match(TokenType type) {
  if (check(type)) {
    advance();
    return true;
  }
  return false;
}

bool Parser::match(const std::vector<TokenType>& types) {
  for (TokenType type : types) {
    if (check(type)) {
      advance();
      return true;
    }
  }
  return false;
}

Token Parser::consume(TokenType type, const std::string& message) {
  if (check(type)) return advance();
  error(message);
  return Token();
}

bool Parser::isAtEnd() const {
  return peek().type == TokenType::EOF_TOKEN;
}

std::shared_ptr<FunctionDecl> Parser::functionDeclaration() {
  consume(TokenType::FUNC, "Expected 'func'");
  std::string name = consume(TokenType::IDENTIFIER, "Expected function name").lexeme;
  
  auto func = std::make_shared<FunctionDecl>(name, "void");
  
  consume(TokenType::LPAREN, "Expected '('");
  
  // Parse parameters
  if (!check(TokenType::RPAREN)) {
    do {
      // Parse parameter type (e.g., int, float, string)
      std::string paramType;
      if (check(TokenType::IDENTIFIER)) {
        paramType = advance().lexeme;
      } else {
        error("Expected parameter type");
        advance();
        continue;
      }
      
      if (!check(TokenType::IDENTIFIER)) {
        error("Expected parameter name");
        continue;
      }
      std::string paramName = advance().lexeme;
      func->parameters.push_back({paramName, paramType});
    } while (match(TokenType::COMMA));
  }
  
  consume(TokenType::RPAREN, "Expected ')'");
  
  // Parse return type (-> TYPE)
  if (match(TokenType::ARROW)) {
    if (check(TokenType::IDENTIFIER)) {
      func->returnType = advance().lexeme;
    } else {
      error("Expected return type");
    }
  }
  
  consume(TokenType::LBRACE, "Expected '{'");
  
  // Parse body
  while (!check(TokenType::RBRACE) && !isAtEnd()) {
    auto stmt = statement();
    if (stmt) func->body.push_back(stmt);
  }
  
  consume(TokenType::RBRACE, "Expected '}'");
  
  return func;
}

std::shared_ptr<Statement> Parser::statement() {
  if (match(TokenType::IF)) return ifStatement();
  if (match(TokenType::WHILE)) return whileStatement();
  if (match(TokenType::PAR)) return parallelBlock();
  if (match(TokenType::LBRACE)) {
    auto block = std::make_shared<Block>();
    while (!check(TokenType::RBRACE) && !isAtEnd()) {
      auto stmt = statement();
      if (stmt) block->statements.push_back(stmt);
    }
    if (!consume(TokenType::RBRACE, "Expected '}'").lexeme.empty()) {
      // Successfully consumed closing brace
    }
    return block;
  }
  if (check(TokenType::LET) || check(TokenType::STATIC)) {
    return variableDeclaration();
  }
  if (match(TokenType::RETURN)) return returnStatement();
  
  return expressionStatement();
}

std::shared_ptr<Statement> Parser::blockStatement() {
  auto block = std::make_shared<Block>();
  
  while (!check(TokenType::RBRACE) && !isAtEnd()) {
    auto stmt = statement();
    if (stmt) block->statements.push_back(stmt);
  }
  
  consume(TokenType::RBRACE, "Expected '}'");
  return block;
}

std::shared_ptr<Statement> Parser::ifStatement() {
  consume(TokenType::LPAREN, "Expected '(' after 'if'");
  auto condition = expression();
  consume(TokenType::RPAREN, "Expected ')' after if condition");
  
  auto ifStmt = std::make_shared<IfStatement>();
  ifStmt->condition = condition;
  
  consume(TokenType::LBRACE, "Expected '{' before if body");
  while (!check(TokenType::RBRACE) && !isAtEnd()) {
    auto stmt = statement();
    if (stmt) ifStmt->thenBody.push_back(stmt);
  }
  consume(TokenType::RBRACE, "Expected '}'");
  
  return ifStmt;
}

std::shared_ptr<Statement> Parser::whileStatement() {
  consume(TokenType::LPAREN, "Expected '(' after 'while'");
  auto condition = expression();
  consume(TokenType::RPAREN, "Expected ')' after while condition");
  
  auto whileStmt = std::make_shared<WhileStatement>();
  whileStmt->condition = condition;
  
  consume(TokenType::LBRACE, "Expected '{' before while body");
  while (!check(TokenType::RBRACE) && !isAtEnd()) {
    auto stmt = statement();
    if (stmt) whileStmt->body.push_back(stmt);
  }
  consume(TokenType::RBRACE, "Expected '}'");
  
  return whileStmt;
}

std::shared_ptr<Statement> Parser::parallelBlock() {
  consume(TokenType::LBRACE, "Expected '{' after 'par'");
  
  auto par = std::make_shared<ParallelBlock>();
  
  while (!check(TokenType::RBRACE) && !isAtEnd()) {
    auto stmt = statement();
    if (stmt) par->processes.push_back(stmt);
  }
  
  consume(TokenType::RBRACE, "Expected '}'");
  return par;
}

std::shared_ptr<Statement> Parser::variableDeclaration() {
  bool isStatic = match(TokenType::STATIC);
  match(TokenType::LET);
  
  std::string name = consume(TokenType::IDENTIFIER, "Expected variable name").lexeme;
  consume(TokenType::COLON, "Expected ':'");
  std::string type = consume(TokenType::IDENTIFIER, "Expected type name").lexeme;
  
  auto varDecl = std::make_shared<VariableDecl>(name, type, isStatic);
  
  if (match(TokenType::ASSIGN)) {
    varDecl->initializer = expression();
  }
  
  if (!consume(TokenType::SEMICOLON, "Expected ';'").lexeme.empty()) {
    // Successfully consumed semicolon
  }
  return varDecl;
}

std::shared_ptr<Statement> Parser::returnStatement() {
  auto returnStmt = std::make_shared<ReturnStatement>();
  
  if (!check(TokenType::SEMICOLON)) {
    returnStmt->value = expression();
  }
  
  consume(TokenType::SEMICOLON, "Expected ';' after return statement");
  return returnStmt;
}

std::shared_ptr<Statement> Parser::expressionStatement() {
  auto expr = expression();
  if (!consume(TokenType::SEMICOLON, "Expected ';' after expression").lexeme.empty()) {
    // Successfully consumed semicolon
  }
  return std::make_shared<ExpressionStatement>(expr);
}

std::shared_ptr<Expression> Parser::expression() {
  return assignment();
}

std::shared_ptr<Expression> Parser::assignment() {
  auto expr = logicalOr();
  
  if (match(TokenType::ASSIGN)) {
    auto value = assignment();
    if (auto ident = std::dynamic_pointer_cast<Identifier>(expr)) {
      return std::make_shared<BinaryOp>(expr, "=", value);
    }
    error("Invalid assignment target");
  }
  
  return expr;
}

std::shared_ptr<Expression> Parser::logicalOr() {
  auto expr = logicalAnd();
  
  while (match(TokenType::OR)) {
    auto op = tokens[current - 1].lexeme;
    auto right = logicalAnd();
    expr = std::make_shared<BinaryOp>(expr, op, right);
  }
  
  return expr;
}

std::shared_ptr<Expression> Parser::logicalAnd() {
  auto expr = equality();
  
  while (match(TokenType::AND)) {
    auto op = tokens[current - 1].lexeme;
    auto right = equality();
    expr = std::make_shared<BinaryOp>(expr, op, right);
  }
  
  return expr;
}

std::shared_ptr<Expression> Parser::equality() {
  auto expr = comparison();
  
  while (match({TokenType::EQ, TokenType::NE})) {
    auto op = tokens[current - 1].lexeme;
    auto right = comparison();
    expr = std::make_shared<BinaryOp>(expr, op, right);
  }
  
  return expr;
}

std::shared_ptr<Expression> Parser::comparison() {
  auto expr = addition();
  
  while (match({TokenType::LT, TokenType::LE, TokenType::GT, TokenType::GE})) {
    auto op = tokens[current - 1].lexeme;
    auto right = addition();
    expr = std::make_shared<BinaryOp>(expr, op, right);
  }
  
  return expr;
}

std::shared_ptr<Expression> Parser::addition() {
  auto expr = multiplication();
  
  while (match({TokenType::PLUS, TokenType::MINUS})) {
    auto op = tokens[current - 1].lexeme;
    auto right = multiplication();
    expr = std::make_shared<BinaryOp>(expr, op, right);
  }
  
  return expr;
}

std::shared_ptr<Expression> Parser::multiplication() {
  auto expr = unary();
  
  while (match({TokenType::MUL, TokenType::DIV, TokenType::MOD})) {
    auto op = tokens[current - 1].lexeme;
    auto right = unary();
    expr = std::make_shared<BinaryOp>(expr, op, right);
  }
  
  return expr;
}

std::shared_ptr<Expression> Parser::unary() {
  if (match({TokenType::NOT, TokenType::MINUS})) {
    auto op = tokens[current - 1].lexeme;
    auto expr = unary();
    return std::make_shared<UnaryOp>(op, expr);
  }
  
  return call();
}

std::shared_ptr<Expression> Parser::call() {
  auto expr = primary();
  
  while (true) {
    if (match(TokenType::LPAREN)) {
      if (auto ident = std::dynamic_pointer_cast<Identifier>(expr)) {
        auto call = std::make_shared<FunctionCall>(ident->name);
        
        if (!check(TokenType::RPAREN)) {
          do {
            call->arguments.push_back(expression());
          } while (match(TokenType::COMMA));
        }
        
        consume(TokenType::RPAREN, "Expected ')' after arguments");
        expr = call;
      }
    } else if (match(TokenType::DOT)) {
      std::string method = consume(TokenType::IDENTIFIER, "Expected method name").lexeme;
      
      // Handle channel operations like send() and try_recv()
      if ((method == "send" || method == "try_recv") && match(TokenType::LPAREN)) {
        auto chanOp = std::make_shared<ChannelOp>(
          std::dynamic_pointer_cast<Identifier>(expr)->name,
          method
        );
        
        // Parse arguments for send/try_recv
        if (!check(TokenType::RPAREN)) {
          do {
            chanOp->arguments.push_back(expression());
          } while (match(TokenType::COMMA));
        }
        
        consume(TokenType::RPAREN, "Expected ')' after channel operation");
        expr = chanOp;
      } else if (method == "send" || method == "try_recv") {
        // Handle without parens
        expr = std::make_shared<ChannelOp>(
          std::dynamic_pointer_cast<Identifier>(expr)->name,
          method
        );
      }
    } else {
      break;
    }
  }
  
  return expr;
}

std::shared_ptr<Expression> Parser::primary() {
  if (match(TokenType::TRUE)) {
    return std::make_shared<IntLiteral>(1);
  }
  
  if (match(TokenType::FALSE)) {
    return std::make_shared<IntLiteral>(0);
  }
  
  if (match(TokenType::INT)) {
    try {
      return std::make_shared<IntLiteral>(std::get<int>(tokens[current - 1].literal));
    } catch (...) {
      return std::make_shared<IntLiteral>(0);
    }
  }
  
  if (match(TokenType::FLOAT)) {
    try {
      return std::make_shared<FloatLiteral>(std::get<double>(tokens[current - 1].literal));
    } catch (...) {
      return std::make_shared<FloatLiteral>(0.0);
    }
  }
  
  if (match(TokenType::STRING)) {
    try {
      return std::make_shared<StringLiteral>(std::get<std::string>(tokens[current - 1].literal));
    } catch (...) {
      return std::make_shared<StringLiteral>("");
    }
  }
  
  if (match(TokenType::PRINTLN)) {
    consume(TokenType::LPAREN, "Expected '(' after println");
    auto call = std::make_shared<FunctionCall>("println");
    if (!check(TokenType::RPAREN)) {
      do {
        call->arguments.push_back(expression());
      } while (match(TokenType::COMMA));
    }
    consume(TokenType::RPAREN, "Expected ')' after println arguments");
    return call;
  }
  
  if (match(TokenType::IDENTIFIER)) {
    return std::make_shared<Identifier>(tokens[current - 1].lexeme);
  }
  
  if (match(TokenType::LPAREN)) {
    auto expr = expression();
    consume(TokenType::RPAREN, "Expected ')' after expression");
    return expr;
  }
  
  error("Expected expression");
  return nullptr;
}

void Parser::error(const std::string& message) {
  std::cerr << "Parse error at line " << peek().line << ", column " << peek().column
            << ": " << message << " (got '" << peek().lexeme << "')" << std::endl;
}

void Parser::synchronize() {
  advance();
  
  while (!isAtEnd()) {
    if (tokens[current - 1].type == TokenType::SEMICOLON) return;
    
    switch (peek().type) {
      case TokenType::CHAN:
      case TokenType::IF:
      case TokenType::WHILE:
      case TokenType::RETURN:
      case TokenType::FUNC:
        return;
      default:
        break;
    }
    
    advance();
  }
}
