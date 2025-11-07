#pragma once

#include <memory>
#include <string>
#include <vector>
#include <set>
#include <map>
#include "../compiler/ast.h"

// Represents data dependencies between statements
struct StatementDependency {
  size_t statementIndex;
  std::set<std::string> readsVariables;  // Variables this statement reads
  std::set<std::string> writesVariables; // Variables this statement writes
  std::set<size_t> dependsOn;             // Indices of statements this depends on
};

// A layer of statements that can run in parallel
struct ExecutionLayer {
  std::vector<size_t> statementIndices;  // Indices of statements in this layer
};

// Analyzes data dependencies in a block of statements
class DependencyAnalyzer {
public:
  // Analyze a block of statements and return execution layers
  std::vector<ExecutionLayer> analyzeDependencies(const std::vector<std::shared_ptr<Statement>>& statements);

  // Get detailed dependency information
  const std::vector<StatementDependency>& getDependencies() const { return dependencies; }

private:
  std::vector<StatementDependency> dependencies;

  // Extract variables read by an expression
  void extractReadVariables(const std::shared_ptr<Expression>& expr, std::set<std::string>& vars);

  // Extract variables written by a statement
  void extractWriteVariables(const std::shared_ptr<Statement>& stmt, std::set<std::string>& vars);

  // Analyze a single statement for dependencies
  StatementDependency analyzeSingleStatement(size_t index, const std::shared_ptr<Statement>& stmt);

  // Build execution layers from dependency information
  std::vector<ExecutionLayer> buildExecutionLayers();
};
