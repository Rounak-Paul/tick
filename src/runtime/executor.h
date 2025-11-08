#pragma once

#include <memory>
#include <string>
#include <map>
#include <vector>
#include <thread>
#include "../compiler/ast.h"
#include "dependency_analyzer.h"

// Value type system for TDL with strong typing support
struct Value {
  enum Type {
    INT,
    FLOAT,
    DOUBLE,
    BOOL,
    STRING,
    VOID,
    UNKNOWN
  };

  Type type;
  int intValue;
  float floatValue;
  double doubleValue;
  bool boolValue;
  std::string stringValue;

  Value() : type(VOID), intValue(0), floatValue(0.0f), doubleValue(0.0), boolValue(false), stringValue("") {}
  explicit Value(int i) : type(INT), intValue(i), floatValue(0.0f), doubleValue(0.0), boolValue(false), stringValue("") {}
  explicit Value(float f) : type(FLOAT), intValue(0), floatValue(f), doubleValue(0.0), boolValue(false), stringValue("") {}
  explicit Value(double d) : type(DOUBLE), intValue(0), floatValue(0.0f), doubleValue(d), boolValue(false), stringValue("") {}
  explicit Value(bool b) : type(BOOL), intValue(0), floatValue(0.0f), doubleValue(0.0), boolValue(b), stringValue("") {}
  explicit Value(const std::string& s) : type(STRING), intValue(0), floatValue(0.0f), doubleValue(0.0), boolValue(false), stringValue(s) {}

  static Value Void() { return Value(); }
  bool isInt() const { return type == INT; }
  bool isFloat() const { return type == FLOAT; }
  bool isDouble() const { return type == DOUBLE; }
  bool isBool() const { return type == BOOL; }
  bool isString() const { return type == STRING; }
  bool isVoid() const { return type == VOID; }
  bool isNumeric() const { return type == INT || type == FLOAT || type == DOUBLE; }
  
  std::string typeString() const {
    switch (type) {
      case INT: return "int";
      case FLOAT: return "float";
      case DOUBLE: return "double";
      case BOOL: return "bool";
      case STRING: return "string";
      case VOID: return "void";
      default: return "unknown";
    }
  }
};

// Scope for variable storage
struct Scope {
  std::map<std::string, Value> variables;
  std::shared_ptr<Scope> parent;

  Scope(std::shared_ptr<Scope> p = nullptr) : parent(p) {}

  Value get(const std::string& name) const;
  void set(const std::string& name, const Value& val);
  bool has(const std::string& name) const;
};

// Executor: Interprets AST and executes it
class Executor {
public:
  Executor();
  
  // Execute a program
  int executeProgram(const std::shared_ptr<Program>& program);

  // Register function declarations so they can be called
  void registerFunctions(const std::vector<std::shared_ptr<Declaration>>& declarations);

  // Get output from executed program
  std::string getOutput() const { return output; }

private:
  std::map<std::string, std::shared_ptr<FunctionDecl>> functions;
  std::shared_ptr<Scope> globalScope;
  std::string output;
  bool enableParallelization;  // Enable auto-parallelization

  // Expression evaluation
  Value evaluateExpression(const std::shared_ptr<Expression>& expr, std::shared_ptr<Scope> scope);
  Value evaluateBinaryOp(const std::shared_ptr<BinaryOp>& op, std::shared_ptr<Scope> scope);
  Value evaluateUnaryOp(const std::shared_ptr<UnaryOp>& op, std::shared_ptr<Scope> scope);
  Value callFunction(const std::string& name, const std::vector<Value>& args, std::shared_ptr<Scope> scope);

  // Statement execution
  Value executeStatement(const std::shared_ptr<Statement>& stmt, std::shared_ptr<Scope> scope);
  Value executeBlock(const std::vector<std::shared_ptr<Statement>>& statements, std::shared_ptr<Scope> scope);
  Value executeBlockParallel(const std::vector<std::shared_ptr<Statement>>& statements, std::shared_ptr<Scope> scope);
  Value executeIfStatement(const std::shared_ptr<IfStatement>& stmt, std::shared_ptr<Scope> scope);
  Value executeWhileStatement(const std::shared_ptr<WhileStatement>& stmt, std::shared_ptr<Scope> scope);
  Value executeVariableDecl(const std::shared_ptr<VariableDecl>& stmt, std::shared_ptr<Scope> scope);
  Value executeReturnStatement(const std::shared_ptr<ReturnStatement>& stmt, std::shared_ptr<Scope> scope);

  // Control flow
  bool returnTriggered;
  Value returnValue;
};
