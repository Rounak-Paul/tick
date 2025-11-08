#pragma once

#include <memory>
#include <string>
#include <map>
#include <vector>
#include <stdexcept>
#include "ast.h"

// Strong typing system for TDL
class TypeChecker {
public:
  TypeChecker() = default;

  // Main entry point: check a program for type errors
  void checkProgram(const std::shared_ptr<Program>& program);

  // Get collected errors
  const std::vector<std::string>& getErrors() const { return errors; }
  bool hasErrors() const { return !errors.empty(); }

private:
  struct TypeInfo {
    std::string type;  // "int", "float", "double", "bool", "string", "void"
    bool isValid;
    
    TypeInfo() : type("unknown"), isValid(false) {}
    explicit TypeInfo(const std::string& t) : type(t), isValid(true) {}
  };

  struct VariableInfo {
    std::string type;
    bool isInitialized;
    
    VariableInfo() : type(""), isInitialized(false) {}
    VariableInfo(const std::string& t, bool init = true) : type(t), isInitialized(init) {}
  };

  struct FunctionInfo {
    std::string returnType;
    std::vector<std::pair<std::string, std::string>> parameters; // (name, type)
  };

  // Symbol tables
  std::map<std::string, VariableInfo> globalVariables;
  std::map<std::string, VariableInfo> currentScope;
  std::map<std::string, FunctionInfo> functions;
  std::vector<std::string> errors;

  // Check methods
  void checkDeclaration(const std::shared_ptr<Declaration>& decl);
  void checkFunctionDecl(const std::shared_ptr<FunctionDecl>& funcDecl);
  void checkStatement(const std::shared_ptr<Statement>& stmt);
  void checkBlock(const std::vector<std::shared_ptr<Statement>>& statements);
  void checkExpression(const std::shared_ptr<Expression>& expr, TypeInfo& outType);
  void checkBinaryOp(const std::shared_ptr<BinaryOp>& op, TypeInfo& outType);
  void checkUnaryOp(const std::shared_ptr<UnaryOp>& op, TypeInfo& outType);
  void checkFunctionCall(const std::shared_ptr<FunctionCall>& call, TypeInfo& outType);
  void checkVariableDecl(const std::shared_ptr<VariableDecl>& varDecl);

  // Helper methods
  bool isValidType(const std::string& type);
  bool isNumericType(const std::string& type);
  bool areTypesCompatible(const std::string& expected, const std::string& actual);
  void addError(const std::string& message);
  void pushScope();
  void popScope();
};
