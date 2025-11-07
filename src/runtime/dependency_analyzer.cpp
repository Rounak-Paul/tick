#include "dependency_analyzer.h"
#include <algorithm>

std::vector<ExecutionLayer> DependencyAnalyzer::analyzeDependencies(
    const std::vector<std::shared_ptr<Statement>>& statements) {
  
  dependencies.clear();

  // Analyze each statement
  for (size_t i = 0; i < statements.size(); ++i) {
    dependencies.push_back(analyzeSingleStatement(i, statements[i]));
  }

  // Build execution layers
  return buildExecutionLayers();
}

void DependencyAnalyzer::extractReadVariables(const std::shared_ptr<Expression>& expr, std::set<std::string>& vars) {
  if (!expr) return;

  if (auto ident = std::dynamic_pointer_cast<Identifier>(expr)) {
    vars.insert(ident->name);
  } else if (auto binOp = std::dynamic_pointer_cast<BinaryOp>(expr)) {
    extractReadVariables(binOp->left, vars);
    extractReadVariables(binOp->right, vars);
  } else if (auto unOp = std::dynamic_pointer_cast<UnaryOp>(expr)) {
    extractReadVariables(unOp->operand, vars);
  } else if (auto funcCall = std::dynamic_pointer_cast<FunctionCall>(expr)) {
    for (const auto& arg : funcCall->arguments) {
      extractReadVariables(arg, vars);
    }
  }
}

void DependencyAnalyzer::extractWriteVariables(const std::shared_ptr<Statement>& stmt, std::set<std::string>& vars) {
  if (!stmt) return;

  if (auto varDecl = std::dynamic_pointer_cast<VariableDecl>(stmt)) {
    vars.insert(varDecl->name);
  } else if (auto exprStmt = std::dynamic_pointer_cast<ExpressionStatement>(stmt)) {
    // Check if it's an assignment (binary op with = operator)
    if (auto binOp = std::dynamic_pointer_cast<BinaryOp>(exprStmt->expr)) {
      if (binOp->op == "=" && std::dynamic_pointer_cast<Identifier>(binOp->left)) {
        auto var = std::dynamic_pointer_cast<Identifier>(binOp->left);
        vars.insert(var->name);
      }
    }
  }
}

StatementDependency DependencyAnalyzer::analyzeSingleStatement(size_t index, const std::shared_ptr<Statement>& stmt) {
  StatementDependency dep;
  dep.statementIndex = index;

  if (!stmt) {
    return dep;
  }

  // Extract variables written by this statement
  extractWriteVariables(stmt, dep.writesVariables);

  // Extract variables read by this statement
  if (auto exprStmt = std::dynamic_pointer_cast<ExpressionStatement>(stmt)) {
    extractReadVariables(exprStmt->expr, dep.readsVariables);
  } else if (auto varDecl = std::dynamic_pointer_cast<VariableDecl>(stmt)) {
    if (varDecl->initializer) {
      extractReadVariables(varDecl->initializer, dep.readsVariables);
    }
  } else if (auto ifStmt = std::dynamic_pointer_cast<IfStatement>(stmt)) {
    extractReadVariables(ifStmt->condition, dep.readsVariables);
  } else if (auto whileStmt = std::dynamic_pointer_cast<WhileStatement>(stmt)) {
    extractReadVariables(whileStmt->condition, dep.readsVariables);
  }

  // Remove written variables from read set (don't depend on own writes)
  for (const auto& var : dep.writesVariables) {
    dep.readsVariables.erase(var);
  }

  // Find dependencies: which previous statements write variables this reads
  for (size_t i = 0; i < index; ++i) {
    const auto& prevDep = dependencies[i];
    
    // Check if any variable this statement reads is written by a previous statement
    for (const auto& var : dep.readsVariables) {
      if (prevDep.writesVariables.count(var) > 0) {
        dep.dependsOn.insert(i);
        break;  // One dependency per previous statement is enough
      }
    }
  }

  return dep;
}

std::vector<ExecutionLayer> DependencyAnalyzer::buildExecutionLayers() {
  std::vector<ExecutionLayer> layers;
  std::set<size_t> executed;

  while (executed.size() < dependencies.size()) {
    ExecutionLayer currentLayer;

    // Find all statements ready to execute (all dependencies have been executed)
    for (size_t i = 0; i < dependencies.size(); ++i) {
      if (executed.count(i) > 0) {
        continue;  // Already executed
      }

      // Check if all dependencies are satisfied
      bool allDependenciesSatisfied = true;
      for (size_t dep : dependencies[i].dependsOn) {
        if (executed.count(dep) == 0) {
          allDependenciesSatisfied = false;
          break;
        }
      }

      if (allDependenciesSatisfied) {
        currentLayer.statementIndices.push_back(i);
        executed.insert(i);
      }
    }

    if (currentLayer.statementIndices.empty()) {
      break;  // Prevent infinite loops in case of circular dependencies
    }

    layers.push_back(currentLayer);
  }

  return layers;
}
