#include "type_checker.h"
#include <sstream>

void TypeChecker::checkProgram(const std::shared_ptr<Program>& program) {
  if (!program) return;

  // First pass: register all function declarations
  for (const auto& decl : program->declarations) {
    if (auto funcDecl = std::dynamic_pointer_cast<FunctionDecl>(decl)) {
      FunctionInfo info;
      info.returnType = funcDecl->returnType;
      info.parameters = funcDecl->parameters;
      functions[funcDecl->name] = info;
    }
  }

  // Second pass: check each declaration
  for (const auto& decl : program->declarations) {
    checkDeclaration(decl);
  }

  // Check global statements
  checkBlock(program->globalStatements);
}

void TypeChecker::checkDeclaration(const std::shared_ptr<Declaration>& decl) {
  if (!decl) return;

  if (auto funcDecl = std::dynamic_pointer_cast<FunctionDecl>(decl)) {
    checkFunctionDecl(funcDecl);
  } else if (auto channelDecl = std::dynamic_pointer_cast<ChannelDecl>(decl)) {
    if (!isValidType(channelDecl->elementType)) {
      addError("Invalid channel element type: " + channelDecl->elementType);
    }
  }
}

void TypeChecker::checkFunctionDecl(const std::shared_ptr<FunctionDecl>& funcDecl) {
  if (!funcDecl) return;

  // Validate return type
  if (!isValidType(funcDecl->returnType) && funcDecl->returnType != "void") {
    addError("Function '" + funcDecl->name + "' has invalid return type: " + funcDecl->returnType);
    return;
  }

  // Push new scope for function
  pushScope();

  // Register parameters
  for (const auto& param : funcDecl->parameters) {
    if (!isValidType(param.second)) {
      addError("Function '" + funcDecl->name + "' has parameter '" + param.first + 
               "' with invalid type: " + param.second);
    } else {
      currentScope[param.first] = VariableInfo(param.second, true);
    }
  }

  // Check function body
  checkBlock(funcDecl->body);

  // Pop scope
  popScope();
}

void TypeChecker::checkStatement(const std::shared_ptr<Statement>& stmt) {
  if (!stmt) return;

  if (auto varDecl = std::dynamic_pointer_cast<VariableDecl>(stmt)) {
    checkVariableDecl(varDecl);
  } else if (auto exprStmt = std::dynamic_pointer_cast<ExpressionStatement>(stmt)) {
    TypeInfo outType;
    checkExpression(exprStmt->expr, outType);
  } else if (auto ifStmt = std::dynamic_pointer_cast<IfStatement>(stmt)) {
    TypeInfo condType;
    checkExpression(ifStmt->condition, condType);
    if (!condType.isValid || condType.type != "bool") {
      addError("If condition must be boolean, got: " + condType.type);
    }
    checkBlock(ifStmt->thenBody);
    checkBlock(ifStmt->elseBody);
  } else if (auto whileStmt = std::dynamic_pointer_cast<WhileStatement>(stmt)) {
    TypeInfo condType;
    checkExpression(whileStmt->condition, condType);
    if (!condType.isValid || condType.type != "bool") {
      addError("While condition must be boolean, got: " + condType.type);
    }
    checkBlock(whileStmt->body);
  } else if (auto block = std::dynamic_pointer_cast<Block>(stmt)) {
    checkBlock(block->statements);
  } else if (auto returnStmt = std::dynamic_pointer_cast<ReturnStatement>(stmt)) {
    if (returnStmt->value) {
      TypeInfo returnType;
      checkExpression(returnStmt->value, returnType);
    }
  } else if (auto parallelBlock = std::dynamic_pointer_cast<ParallelBlock>(stmt)) {
    for (const auto& proc : parallelBlock->processes) {
      checkStatement(proc);
    }
  } else if (auto onClockStmt = std::dynamic_pointer_cast<OnClockStatement>(stmt)) {
    checkBlock(onClockStmt->body);
  }
}

void TypeChecker::checkBlock(const std::vector<std::shared_ptr<Statement>>& statements) {
  for (const auto& stmt : statements) {
    checkStatement(stmt);
  }
}

void TypeChecker::checkExpression(const std::shared_ptr<Expression>& expr, TypeInfo& outType) {
  if (!expr) {
    outType = TypeInfo("void");
    return;
  }

  if (auto intLit = std::dynamic_pointer_cast<IntLiteral>(expr)) {
    outType = TypeInfo("int");
  } else if (auto floatLit = std::dynamic_pointer_cast<FloatLiteral>(expr)) {
    outType = TypeInfo("float");
  } else if (auto strLit = std::dynamic_pointer_cast<StringLiteral>(expr)) {
    outType = TypeInfo("string");
  } else if (auto ident = std::dynamic_pointer_cast<Identifier>(expr)) {
    auto it = currentScope.find(ident->name);
    if (it == currentScope.end()) {
      it = globalVariables.find(ident->name);
    }
    
    if (it != currentScope.end() && it != globalVariables.end()) {
      outType = TypeInfo(it->second.type);
    } else {
      addError("Undefined variable: " + ident->name);
      outType = TypeInfo();
    }
  } else if (auto binOp = std::dynamic_pointer_cast<BinaryOp>(expr)) {
    checkBinaryOp(binOp, outType);
  } else if (auto unOp = std::dynamic_pointer_cast<UnaryOp>(expr)) {
    checkUnaryOp(unOp, outType);
  } else if (auto funcCall = std::dynamic_pointer_cast<FunctionCall>(expr)) {
    checkFunctionCall(funcCall, outType);
  } else {
    outType = TypeInfo("unknown");
  }
}

void TypeChecker::checkBinaryOp(const std::shared_ptr<BinaryOp>& op, TypeInfo& outType) {
  if (!op) {
    outType = TypeInfo();
    return;
  }

  TypeInfo leftType, rightType;
  checkExpression(op->left, leftType);
  checkExpression(op->right, rightType);

  if (!leftType.isValid || !rightType.isValid) {
    outType = TypeInfo();
    return;
  }

  // Arithmetic operators
  if (op->op == "+" || op->op == "-" || op->op == "*" || op->op == "/") {
    if (!isNumericType(leftType.type) || !isNumericType(rightType.type)) {
      addError("Arithmetic operator '" + op->op + "' requires numeric operands, got " + 
               leftType.type + " and " + rightType.type);
      outType = TypeInfo();
      return;
    }
    // Result type is the wider of the two types
    if (leftType.type == "double" || rightType.type == "double") {
      outType = TypeInfo("double");
    } else if (leftType.type == "float" || rightType.type == "float") {
      outType = TypeInfo("float");
    } else {
      outType = TypeInfo("int");
    }
  }
  // Modulo operator
  else if (op->op == "%") {
    if (leftType.type != "int" || rightType.type != "int") {
      addError("Modulo operator '%' requires integer operands, got " + 
               leftType.type + " and " + rightType.type);
      outType = TypeInfo();
      return;
    }
    outType = TypeInfo("int");
  }
  // Comparison operators
  else if (op->op == "==" || op->op == "!=" || op->op == "<" || 
           op->op == "<=" || op->op == ">" || op->op == ">=") {
    if ((isNumericType(leftType.type) && isNumericType(rightType.type)) ||
        (leftType.type == rightType.type)) {
      outType = TypeInfo("bool");
    } else {
      addError("Cannot compare types " + leftType.type + " and " + rightType.type);
      outType = TypeInfo();
    }
  }
  // Logical operators
  else if (op->op == "&&" || op->op == "||") {
    if (leftType.type != "bool" || rightType.type != "bool") {
      addError("Logical operator '" + op->op + "' requires boolean operands, got " + 
               leftType.type + " and " + rightType.type);
      outType = TypeInfo();
      return;
    }
    outType = TypeInfo("bool");
  }
  // Assignment
  else if (op->op == "=") {
    if (!areTypesCompatible(leftType.type, rightType.type)) {
      addError("Cannot assign " + rightType.type + " to " + leftType.type);
      outType = TypeInfo();
      return;
    }
    outType = rightType;
  } else {
    addError("Unknown operator: " + op->op);
    outType = TypeInfo();
  }
}

void TypeChecker::checkUnaryOp(const std::shared_ptr<UnaryOp>& op, TypeInfo& outType) {
  if (!op) {
    outType = TypeInfo();
    return;
  }

  TypeInfo operandType;
  checkExpression(op->operand, operandType);

  if (!operandType.isValid) {
    outType = TypeInfo();
    return;
  }

  if (op->op == "-") {
    if (!isNumericType(operandType.type)) {
      addError("Unary minus requires numeric operand, got: " + operandType.type);
      outType = TypeInfo();
      return;
    }
    outType = operandType;
  } else if (op->op == "!") {
    if (operandType.type != "bool") {
      addError("Logical NOT requires boolean operand, got: " + operandType.type);
      outType = TypeInfo();
      return;
    }
    outType = TypeInfo("bool");
  } else {
    addError("Unknown unary operator: " + op->op);
    outType = TypeInfo();
  }
}

void TypeChecker::checkFunctionCall(const std::shared_ptr<FunctionCall>& call, TypeInfo& outType) {
  if (!call) {
    outType = TypeInfo();
    return;
  }

  // Built-in functions
  if (call->name == "println") {
    // println can take any type
    if (!call->arguments.empty()) {
      TypeInfo argType;
      checkExpression(call->arguments[0], argType);
    }
    outType = TypeInfo("void");
    return;
  }

  // Look up user-defined function
  auto it = functions.find(call->name);
  if (it == functions.end()) {
    addError("Undefined function: " + call->name);
    outType = TypeInfo();
    return;
  }

  const FunctionInfo& funcInfo = it->second;

  // Check argument count
  if (call->arguments.size() != funcInfo.parameters.size()) {
    addError("Function '" + call->name + "' expects " + 
             std::to_string(funcInfo.parameters.size()) + " arguments, got " + 
             std::to_string(call->arguments.size()));
    outType = TypeInfo();
    return;
  }

  // Check argument types
  for (size_t i = 0; i < call->arguments.size(); ++i) {
    TypeInfo argType;
    checkExpression(call->arguments[i], argType);
    
    if (!areTypesCompatible(funcInfo.parameters[i].second, argType.type)) {
      addError("Function '" + call->name + "' parameter " + std::to_string(i + 1) + 
               " expects " + funcInfo.parameters[i].second + ", got " + argType.type);
    }
  }

  outType = TypeInfo(funcInfo.returnType);
}

void TypeChecker::checkVariableDecl(const std::shared_ptr<VariableDecl>& varDecl) {
  if (!varDecl) return;

  // Check that declared type is valid
  if (!isValidType(varDecl->type)) {
    addError("Invalid variable type: " + varDecl->type);
    return;
  }

  // Check initializer if present
  if (varDecl->initializer) {
    TypeInfo initType;
    checkExpression(varDecl->initializer, initType);

    if (!areTypesCompatible(varDecl->type, initType.type)) {
      addError("Cannot initialize " + varDecl->type + " variable '" + varDecl->name + 
               "' with value of type " + initType.type);
    }
  }

  // Add to current scope
  currentScope[varDecl->name] = VariableInfo(varDecl->type, true);
}

bool TypeChecker::isValidType(const std::string& type) {
  return type == "int" || type == "float" || type == "double" || 
         type == "bool" || type == "string" || type == "void";
}

bool TypeChecker::isNumericType(const std::string& type) {
  return type == "int" || type == "float" || type == "double";
}

bool TypeChecker::areTypesCompatible(const std::string& expected, const std::string& actual) {
  if (expected == actual) return true;
  
  // Allow numeric type conversions
  if (isNumericType(expected) && isNumericType(actual)) {
    return true;
  }
  
  return false;
}

void TypeChecker::addError(const std::string& message) {
  errors.push_back(message);
}

void TypeChecker::pushScope() {
  // Save current scope and create new empty one
  // For simplicity, we're not implementing nested scopes in this version
}

void TypeChecker::popScope() {
  // Restore previous scope
  // For simplicity, we're not implementing nested scopes in this version
}
