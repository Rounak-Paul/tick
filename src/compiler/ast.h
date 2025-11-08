#pragma once

#include <vector>
#include <memory>
#include <string>

// Forward declarations for AST nodes
struct ASTNode;
struct Expression;
struct Statement;
struct Declaration;
struct ChannelDecl;

// Base AST Node
struct ASTNode {
  virtual ~ASTNode() = default;
};

// Expressions
struct Expression : public ASTNode {
  virtual ~Expression() = default;
};

struct IntLiteral : public Expression {
  int value;
  explicit IntLiteral(int v) : value(v) {}
};

struct FloatLiteral : public Expression {
  double value;
  explicit FloatLiteral(double v) : value(v) {}
};

struct StringLiteral : public Expression {
  std::string value;
  explicit StringLiteral(const std::string& v) : value(v) {}
};

struct Identifier : public Expression {
  std::string name;
  explicit Identifier(const std::string& n) : name(n) {}
};

struct BinaryOp : public Expression {
  std::shared_ptr<Expression> left;
  std::string op;
  std::shared_ptr<Expression> right;
  
  BinaryOp(std::shared_ptr<Expression> l, const std::string& o, std::shared_ptr<Expression> r)
    : left(l), op(o), right(r) {}
};

struct UnaryOp : public Expression {
  std::string op;
  std::shared_ptr<Expression> operand;
  
  UnaryOp(const std::string& o, std::shared_ptr<Expression> expr)
    : op(o), operand(expr) {}
};

struct FunctionCall : public Expression {
  std::string name;
  std::vector<std::shared_ptr<Expression>> arguments;
  
  FunctionCall(const std::string& n) : name(n) {}
};

struct ChannelOp : public Expression {
  std::string channel;
  std::string operation; // "send" or "try_recv"
  std::shared_ptr<Expression> value; // for send operation
  std::vector<std::shared_ptr<Expression>> arguments; // for generic channel operations
  
  ChannelOp(const std::string& ch, const std::string& op, std::shared_ptr<Expression> v = nullptr)
    : channel(ch), operation(op), value(v) {}
};

// Statements
struct Statement : public ASTNode {
  virtual ~Statement() = default;
};

struct ExpressionStatement : public Statement {
  std::shared_ptr<Expression> expr;
  explicit ExpressionStatement(std::shared_ptr<Expression> e) : expr(e) {}
};

struct IfStatement : public Statement {
  std::shared_ptr<Expression> condition;
  std::vector<std::shared_ptr<Statement>> thenBody;
  std::vector<std::shared_ptr<Statement>> elseBody;
};

struct WhileStatement : public Statement {
  std::shared_ptr<Expression> condition;
  std::vector<std::shared_ptr<Statement>> body;
};

struct Block : public Statement {
  std::vector<std::shared_ptr<Statement>> statements;
};

struct VariableDecl : public Statement {
  std::string name;
  std::string type;
  std::shared_ptr<Expression> initializer;
  bool isStatic;
  
  VariableDecl(const std::string& n, const std::string& t, bool s = false)
    : name(n), type(t), isStatic(s) {}
};

struct ReturnStatement : public Statement {
  std::shared_ptr<Expression> value;
};

struct ParallelBlock : public Statement {
  std::vector<std::shared_ptr<Statement>> processes;
};

// Declarations
struct Declaration : public ASTNode {
  virtual ~Declaration() = default;
};

struct ChannelDecl : public Declaration {
  std::string name;
  std::string elementType;
  int bound; // capacity of the channel
  
  ChannelDecl(const std::string& n, const std::string& type, int b)
    : name(n), elementType(type), bound(b) {}
};

struct FunctionDecl : public Declaration {
  std::string name;
  std::string returnType;
  std::vector<std::pair<std::string, std::string>> parameters; // (name, type)
  std::vector<std::shared_ptr<Statement>> body;
  
  FunctionDecl(const std::string& n, const std::string& rt) 
    : name(n), returnType(rt) {}
};

struct Program : public ASTNode {
  std::vector<std::shared_ptr<Declaration>> declarations;
  std::vector<std::shared_ptr<Statement>> globalStatements;
};
