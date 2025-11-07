#include "codegen.h"
#include <sstream>

std::string CodeGenerator::generate(const std::shared_ptr<Program>& program) {
  std::stringstream output;
  
  output << "#include <iostream>\n";
  output << "#include <queue>\n";
  output << "#include <memory>\n";
  output << "#include <optional>\n\n";
  
  // Generate declarations
  for (const auto& decl : program->declarations) {
    output << generateDeclaration(decl);
  }
  
  // Generate main function
  output << "\nint main() {\n";
  for (const auto& stmt : program->globalStatements) {
    output << generateStatement(stmt);
  }
  output << "  return 0;\n";
  output << "}\n";
  
  return output.str();
}

std::string CodeGenerator::generateDeclaration(const std::shared_ptr<Declaration>& decl) {
  if (auto clockDecl = std::dynamic_pointer_cast<ClockDecl>(decl)) {
    std::stringstream ss;
    ss << "// Clock: " << clockDecl->name << " at " << clockDecl->frequency << " Hz\n";
    return ss.str();
  }
  
  if (auto procDecl = std::dynamic_pointer_cast<ProcDecl>(decl)) {
    std::stringstream ss;
    ss << "void " << procDecl->name << "() {\n";
    ss << "  // Process: " << procDecl->name << "\n";
    ss << "}\n\n";
    return ss.str();
  }
  
  return "";
}

std::string CodeGenerator::generateStatement(const std::shared_ptr<Statement>& stmt) {
  if (auto exprStmt = std::dynamic_pointer_cast<ExpressionStatement>(stmt)) {
    return generateExpression(exprStmt->expr) + ";\n";
  }
  
  if (auto ifStmt = std::dynamic_pointer_cast<IfStatement>(stmt)) {
    std::stringstream ss;
    ss << "if (" << generateExpression(ifStmt->condition) << ") {\n";
    for (const auto& s : ifStmt->thenBody) {
      ss << generateStatement(s);
    }
    ss << "}\n";
    return ss.str();
  }
  
  if (auto varDecl = std::dynamic_pointer_cast<VariableDecl>(stmt)) {
    std::stringstream ss;
    ss << "  " << varDecl->type << " " << varDecl->name;
    if (varDecl->initializer) {
      ss << " = " << generateExpression(varDecl->initializer);
    }
    ss << ";\n";
    return ss.str();
  }
  
  return "";
}

std::string CodeGenerator::generateExpression(const std::shared_ptr<Expression>& expr) {
  if (auto intLit = std::dynamic_pointer_cast<IntLiteral>(expr)) {
    return std::to_string(intLit->value);
  }
  
  if (auto floatLit = std::dynamic_pointer_cast<FloatLiteral>(expr)) {
    return std::to_string(floatLit->value);
  }
  
  if (auto strLit = std::dynamic_pointer_cast<StringLiteral>(expr)) {
    return "\"" + strLit->value + "\"";
  }
  
  if (auto ident = std::dynamic_pointer_cast<Identifier>(expr)) {
    return ident->name;
  }
  
  if (auto binOp = std::dynamic_pointer_cast<BinaryOp>(expr)) {
    return generateExpression(binOp->left) + " " + binOp->op + " " + generateExpression(binOp->right);
  }
  
  if (auto unaryOp = std::dynamic_pointer_cast<UnaryOp>(expr)) {
    return unaryOp->op + generateExpression(unaryOp->operand);
  }
  
  if (auto call = std::dynamic_pointer_cast<FunctionCall>(expr)) {
    std::stringstream ss;
    ss << call->name << "(";
    for (size_t i = 0; i < call->arguments.size(); ++i) {
      if (i > 0) ss << ", ";
      ss << generateExpression(call->arguments[i]);
    }
    ss << ")";
    return ss.str();
  }
  
  return "";
}
