#pragma once

#include <string>
#include <memory>
#include "../compiler/ast.h"

class CodeGenerator {
public:
  std::string generate(const std::shared_ptr<Program>& program);
  
private:
  std::string generateDeclaration(const std::shared_ptr<Declaration>& decl);
  std::string generateStatement(const std::shared_ptr<Statement>& stmt);
  std::string generateExpression(const std::shared_ptr<Expression>& expr);
};
