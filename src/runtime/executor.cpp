#include "executor.h"
#include <iostream>
#include <sstream>

// Scope implementation
Value Scope::get(const std::string& name) const {
  auto it = variables.find(name);
  if (it != variables.end()) {
    return it->second;
  }
  if (parent) {
    return parent->get(name);
  }
  throw std::runtime_error("Undefined variable: " + name);
}

void Scope::set(const std::string& name, const Value& val) {
  auto it = variables.find(name);
  if (it != variables.end()) {
    it->second = val;
  } else if (parent && parent->has(name)) {
    parent->set(name, val);
  } else {
    variables[name] = val;
  }
}

bool Scope::has(const std::string& name) const {
  if (variables.find(name) != variables.end()) {
    return true;
  }
  if (parent) {
    return parent->has(name);
  }
  return false;
}

// Executor implementation
Executor::Executor() 
  : globalScope(std::make_shared<Scope>()), enableParallelization(true), returnTriggered(false) {}

int Executor::executeProgram(const std::shared_ptr<Program>& program) {
  registerFunctions(program->declarations);

  // Execute global statements
  executeBlock(program->globalStatements, globalScope);

  // Auto-invoke main() if it exists
  if (functions.find("main") != functions.end()) {
    try {
      callFunction("main", {}, globalScope);
    } catch (const std::exception& e) {
      std::cerr << "Error calling main: " << e.what() << std::endl;
      return 1;
    }
  }

  return 0;
}

void Executor::registerFunctions(const std::vector<std::shared_ptr<Declaration>>& declarations) {
  for (const auto& decl : declarations) {
    if (auto funcDecl = std::dynamic_pointer_cast<FunctionDecl>(decl)) {
      functions[funcDecl->name] = funcDecl;
    }
  }
}

Value Executor::evaluateExpression(const std::shared_ptr<Expression>& expr, std::shared_ptr<Scope> scope) {
  if (!expr) {
    return Value::Void();
  }

  if (auto lit = std::dynamic_pointer_cast<IntLiteral>(expr)) {
    return Value(lit->value);
  }

  if (auto ident = std::dynamic_pointer_cast<Identifier>(expr)) {
    return scope->get(ident->name);
  }

  if (auto binOp = std::dynamic_pointer_cast<BinaryOp>(expr)) {
    return evaluateBinaryOp(binOp, scope);
  }

  if (auto unOp = std::dynamic_pointer_cast<UnaryOp>(expr)) {
    return evaluateUnaryOp(unOp, scope);
  }

  if (auto funcCall = std::dynamic_pointer_cast<FunctionCall>(expr)) {
    std::vector<Value> args;
    for (const auto& arg : funcCall->arguments) {
      args.push_back(evaluateExpression(arg, scope));
    }
    return callFunction(funcCall->name, args, scope);
  }

  return Value::Void();
}

Value Executor::evaluateBinaryOp(const std::shared_ptr<BinaryOp>& op, std::shared_ptr<Scope> scope) {
  Value left = evaluateExpression(op->left, scope);
  Value right = evaluateExpression(op->right, scope);

  if (op->op == "=") {
    if (auto var = std::dynamic_pointer_cast<Identifier>(op->left)) {
      scope->set(var->name, right);
      return right;
    }
    throw std::runtime_error("Invalid assignment target");
  }

  if (op->op == "+") return Value(left.intValue + right.intValue);
  if (op->op == "-") return Value(left.intValue - right.intValue);
  if (op->op == "*") return Value(left.intValue * right.intValue);
  if (op->op == "/") {
    if (right.intValue == 0) throw std::runtime_error("Division by zero");
    return Value(left.intValue / right.intValue);
  }
  if (op->op == "%") return Value(left.intValue % right.intValue);

  if (op->op == "==") return Value(left.intValue == right.intValue);
  if (op->op == "!=") return Value(left.intValue != right.intValue);
  if (op->op == "<") return Value(left.intValue < right.intValue);
  if (op->op == "<=") return Value(left.intValue <= right.intValue);
  if (op->op == ">") return Value(left.intValue > right.intValue);
  if (op->op == ">=") return Value(left.intValue >= right.intValue);

  if (op->op == "&&") return Value(left.boolValue && right.boolValue);
  if (op->op == "||") return Value(left.boolValue || right.boolValue);

  throw std::runtime_error("Unknown operator: " + op->op);
}

Value Executor::evaluateUnaryOp(const std::shared_ptr<UnaryOp>& op, std::shared_ptr<Scope> scope) {
  Value operand = evaluateExpression(op->operand, scope);

  if (op->op == "-") return Value(-operand.intValue);
  if (op->op == "!") return Value(!operand.boolValue);

  throw std::runtime_error("Unknown unary operator: " + op->op);
}

Value Executor::callFunction(const std::string& name, const std::vector<Value>& args, std::shared_ptr<Scope> callScope) {
  // Check for built-in functions
  if (name == "println") {
    if (!args.empty()) {
      output += std::to_string(args[0].intValue) + "\n";
      std::cout << args[0].intValue << std::endl;
    }
    return Value::Void();
  }

  // Look up user-defined function
  auto it = functions.find(name);
  if (it == functions.end()) {
    throw std::runtime_error("Undefined function: " + name);
  }

  auto funcDecl = it->second;

  // Check argument count
  if (args.size() != funcDecl->parameters.size()) {
    throw std::runtime_error("Function " + name + " expects " + 
                           std::to_string(funcDecl->parameters.size()) + 
                           " arguments, got " + std::to_string(args.size()));
  }

  // Create new scope for function
  auto funcScope = std::make_shared<Scope>(globalScope);

  // Bind parameters
  for (size_t i = 0; i < args.size(); ++i) {
    funcScope->variables[funcDecl->parameters[i].first] = args[i];
  }

  // Execute function body (sequentially within the function to avoid race conditions with return)
  bool prevParallel = enableParallelization;
  enableParallelization = false;  // Disable parallelization within function bodies
  
  returnTriggered = false;
  returnValue = Value::Void();
  
  executeBlock(funcDecl->body, funcScope);

  Value result = returnValue;
  returnTriggered = false;
  enableParallelization = prevParallel;  // Restore parallelization setting
  return result;
}

Value Executor::executeStatement(const std::shared_ptr<Statement>& stmt, std::shared_ptr<Scope> scope) {
  if (!stmt) return Value::Void();

  if (returnTriggered) {
    return Value::Void();
  }

  if (auto exprStmt = std::dynamic_pointer_cast<ExpressionStatement>(stmt)) {
    return evaluateExpression(exprStmt->expr, scope);
  }

  if (auto ifStmt = std::dynamic_pointer_cast<IfStatement>(stmt)) {
    return executeIfStatement(ifStmt, scope);
  }

  if (auto whileStmt = std::dynamic_pointer_cast<WhileStatement>(stmt)) {
    return executeWhileStatement(whileStmt, scope);
  }

  if (auto varDecl = std::dynamic_pointer_cast<VariableDecl>(stmt)) {
    return executeVariableDecl(varDecl, scope);
  }

  if (auto retStmt = std::dynamic_pointer_cast<ReturnStatement>(stmt)) {
    return executeReturnStatement(retStmt, scope);
  }

  if (auto block = std::dynamic_pointer_cast<Block>(stmt)) {
    auto blockScope = std::make_shared<Scope>(scope);
    return executeBlock(block->statements, blockScope);
  }

  return Value::Void();
}

Value Executor::executeBlock(const std::vector<std::shared_ptr<Statement>>& statements, std::shared_ptr<Scope> scope) {
  if (enableParallelization) {
    return executeBlockParallel(statements, scope);
  }

  Value result = Value::Void();
  for (const auto& stmt : statements) {
    result = executeStatement(stmt, scope);
    if (returnTriggered) {
      break;
    }
  }
  return result;
}

Value Executor::executeBlockParallel(const std::vector<std::shared_ptr<Statement>>& statements, std::shared_ptr<Scope> scope) {
  // Analyze dependencies
  DependencyAnalyzer analyzer;
  auto layers = analyzer.analyzeDependencies(statements);

  Value result = Value::Void();

  // Execute each layer
  for (const auto& layer : layers) {
    if (returnTriggered) {
      break;
    }

    // If layer has only one statement, execute sequentially
    if (layer.statementIndices.size() == 1) {
      size_t idx = layer.statementIndices[0];
      result = executeStatement(statements[idx], scope);
    } else {
      // Execute multiple statements in parallel using threads
      std::vector<std::thread> threads;
      std::vector<Value> results(layer.statementIndices.size(), Value::Void());

      for (size_t i = 0; i < layer.statementIndices.size(); ++i) {
        size_t idx = layer.statementIndices[i];
        threads.emplace_back([this, idx, &statements, &scope, &results, i]() {
          results[i] = executeStatement(statements[idx], scope);
        });
      }

      // Wait for all threads to complete
      for (auto& thread : threads) {
        thread.join();
      }
    }
  }

  return result;
}

Value Executor::executeIfStatement(const std::shared_ptr<IfStatement>& stmt, std::shared_ptr<Scope> scope) {
  Value cond = evaluateExpression(stmt->condition, scope);
  
  if (cond.boolValue) {
    auto thenScope = std::make_shared<Scope>(scope);
    return executeBlock(stmt->thenBody, thenScope);
  } else if (!stmt->elseBody.empty()) {
    auto elseScope = std::make_shared<Scope>(scope);
    return executeBlock(stmt->elseBody, elseScope);
  }
  
  return Value::Void();
}

Value Executor::executeWhileStatement(const std::shared_ptr<WhileStatement>& stmt, std::shared_ptr<Scope> scope) {
  Value result = Value::Void();
  while (evaluateExpression(stmt->condition, scope).boolValue) {
    auto loopScope = std::make_shared<Scope>(scope);
    result = executeBlock(stmt->body, loopScope);
    if (returnTriggered) {
      break;
    }
  }
  return result;
}

Value Executor::executeVariableDecl(const std::shared_ptr<VariableDecl>& stmt, std::shared_ptr<Scope> scope) {
  Value initialValue = Value::Void();
  if (stmt->initializer) {
    initialValue = evaluateExpression(stmt->initializer, scope);
  }
  scope->variables[stmt->name] = initialValue;
  return Value::Void();
}

Value Executor::executeReturnStatement(const std::shared_ptr<ReturnStatement>& stmt, std::shared_ptr<Scope> scope) {
  returnValue = evaluateExpression(stmt->value, scope);
  returnTriggered = true;
  return returnValue;
}
