#pragma once

#include "../compiler/ast.h"
#include <string>
#include <sstream>
#include <map>
#include <set>
#include <vector>

class ExecutableCodeGenerator {
public:
  std::string generate(const std::shared_ptr<Program>& program);
  
private:
  struct ProcessInfo {
    std::string name;
    std::vector<std::string> paramNames;
    std::vector<std::string> paramTypes;
    std::vector<std::shared_ptr<Statement>> body;
  };
  
  struct ClockInfo {
    std::string name;
    int frequency;
  };
  
  struct FunctionInfo {
    std::string name;
    std::string returnType;
    std::vector<std::string> paramNames;
    std::vector<std::string> paramTypes;
    std::vector<std::shared_ptr<Statement>> body;
  };
  
  std::vector<ClockInfo> clocks;
  std::vector<ProcessInfo> processes;
  std::vector<FunctionInfo> functions;
  std::map<std::string, std::string> variables;
  
  std::string generateIncludes();
  std::string generateRuntimeSetup();
  std::string generateClockSetup();
  std::string generateUserFunctions();
  std::string generateProcesses();
  std::string generateProcessBody(const ProcessInfo& proc);
  std::string generateStatement(const std::shared_ptr<Statement>& stmt);
  std::string generateExpression(const std::shared_ptr<Expression>& expr);
  std::string generateMainFunction();
  
  void collectDeclarations(const std::shared_ptr<Program>& program);
  
  int indent = 0;
  std::string getIndent();
};
