#include "codegen_new.h"
#include <iostream>
#include <algorithm>

std::string ExecutableCodeGenerator::generate(const std::shared_ptr<Program>& program) {
  collectDeclarations(program);
  
  std::stringstream output;
  output << generateIncludes();
  output << generateRuntimeSetup();
  output << generateUserFunctions();
  output << generateProcesses();
  output << generateMainFunction();
  
  return output.str();
}

std::string ExecutableCodeGenerator::generateIncludes() {
  return R"(#include <iostream>
#include <queue>
#include <map>
#include <memory>
#include <optional>
#include <thread>
#include <mutex>
#include <chrono>
#include <vector>
#include <numeric>

// ============ Channel Implementation ============
class ChannelValue {
public:
  using Value = std::variant<int, double, bool, std::string>;
  
  ChannelValue() = default;
  explicit ChannelValue(int v) : value(v) {}
  explicit ChannelValue(double v) : value(v) {}
  explicit ChannelValue(bool v) : value(v) {}
  explicit ChannelValue(const std::string& v) : value(v) {}
  
  bool isInt() const { return std::holds_alternative<int>(value); }
  bool isDouble() const { return std::holds_alternative<double>(value); }
  bool isBool() const { return std::holds_alternative<bool>(value); }
  bool isString() const { return std::holds_alternative<std::string>(value); }
  
  int getInt() const { return std::get<int>(value); }
  double getDouble() const { return std::get<double>(value); }
  bool getBool() const { return std::get<bool>(value); }
  std::string getString() const { return std::get<std::string>(value); }
  
private:
  Value value;
};

class Channel {
public:
  explicit Channel(int maxSize = 4) : maxSize(maxSize), maxDepth(0), totalMessages(0) {}
  
  void send(const ChannelValue& value) {
    std::lock_guard<std::mutex> lock(mtx);
    if (queue.size() < static_cast<size_t>(maxSize)) {
      queue.push(value);
      totalMessages++;
      int currentDepth = queue.size();
      depthHistory.push_back(currentDepth);
      if (currentDepth > maxDepth) maxDepth = currentDepth;
    }
  }
  
  std::optional<ChannelValue> tryRecv() {
    std::lock_guard<std::mutex> lock(mtx);
    if (queue.empty()) return std::nullopt;
    auto value = queue.front();
    queue.pop();
    return value;
  }
  
  bool isEmpty() const {
    std::lock_guard<std::mutex> lock(mtx);
    return queue.empty();
  }
  
  int getSize() const { return queue.size(); }
  int getMaxDepth() const { return maxDepth; }
  int getTotalMessages() const { return totalMessages; }
  double getAverageDepth() const {
    if (depthHistory.empty()) return 0.0;
    double sum = std::accumulate(depthHistory.begin(), depthHistory.end(), 0);
    return sum / depthHistory.size();
  }
  
private:
  std::queue<ChannelValue> queue;
  int maxSize;
  mutable std::mutex mtx;
  int maxDepth = 0;
  int totalMessages = 0;
  std::vector<int> depthHistory;
};

// ============ Statistics ============
struct Statistics {
  std::string clockName;
  int frequency;
  long long ticksExecuted;
  double averageSlack;
  std::map<std::string, int> channelMaxDepth;
  std::map<std::string, double> channelAvgDepth;
  std::map<std::string, int> channelMessages;
};

)";
}

std::string ExecutableCodeGenerator::generateRuntimeSetup() {
  return R"(std::map<std::string, std::shared_ptr<Channel>> gChannels;
std::vector<Statistics> gStatistics;

// Helper function for println
template<typename T>
void println(const T& value) {
  std::cout << value << std::endl;
}

)";
}

std::string ExecutableCodeGenerator::generateUserFunctions() {
  std::stringstream ss;
  
  for (const auto& func : functions) {
    // Map return type to C++ type
    std::string cppReturnType = func.returnType;
    if (cppReturnType == "int") cppReturnType = "int";
    else if (cppReturnType == "float" || cppReturnType == "double") cppReturnType = "double";
    else if (cppReturnType == "bool") cppReturnType = "bool";
    else if (cppReturnType == "string") cppReturnType = "std::string";
    else cppReturnType = "int"; // default
    
    ss << cppReturnType << " " << func.name << "(";
    
    for (size_t i = 0; i < func.paramNames.size(); ++i) {
      if (i > 0) ss << ", ";
      
      std::string paramType = func.paramTypes[i];
      if (paramType == "int") paramType = "int";
      else if (paramType == "float" || paramType == "double") paramType = "double";
      else if (paramType == "bool") paramType = "bool";
      else if (paramType == "string") paramType = "std::string";
      else paramType = "int"; // default
      
      ss << paramType << " " << func.paramNames[i];
    }
    
    ss << ") {\n";
    indent++;
    
    for (const auto& stmt : func.body) {
      ss << generateStatement(stmt);
    }
    
    indent--;
    ss << "}\n\n";
  }
  
  return ss.str();
}

std::string ExecutableCodeGenerator::generateProcesses() {
  std::stringstream ss;
  
  for (const auto& proc : processes) {
    ss << "void " << proc.name << "(";
    for (size_t i = 0; i < proc.paramNames.size(); ++i) {
      if (i > 0) ss << ", ";
      ss << "std::shared_ptr<Channel> " << proc.paramNames[i];
    }
    ss << ") {\n";
    ss << generateProcessBody(proc);
    ss << "}\n\n";
  }
  
  return ss.str();
}

std::string ExecutableCodeGenerator::generateProcessBody(const ProcessInfo& proc) {
  std::stringstream ss;
  indent++;
  
  for (const auto& stmt : proc.body) {
    ss << generateStatement(stmt);
  }
  
  indent--;
  return ss.str();
}

std::string ExecutableCodeGenerator::generateStatement(const std::shared_ptr<Statement>& stmt) {
  if (!stmt) return "";
  
  if (auto exprStmt = std::dynamic_pointer_cast<ExpressionStatement>(stmt)) {
    return getIndent() + generateExpression(exprStmt->expr) + ";\n";
  }
  
  if (auto varDecl = std::dynamic_pointer_cast<VariableDecl>(stmt)) {
    std::stringstream ss;
    ss << getIndent();
    if (varDecl->isStatic) ss << "static ";
    ss << varDecl->type << " " << varDecl->name;
    if (varDecl->initializer) {
      ss << " = " << generateExpression(varDecl->initializer);
    }
    ss << ";\n";
    return ss.str();
  }
  
  if (auto ifStmt = std::dynamic_pointer_cast<IfStatement>(stmt)) {
    std::stringstream ss;
    ss << getIndent() << "if (" << generateExpression(ifStmt->condition) << ") {\n";
    indent++;
    for (const auto& s : ifStmt->thenBody) {
      ss << generateStatement(s);
    }
    indent--;
    ss << getIndent() << "}\n";
    return ss.str();
  }
  
  if (auto whileStmt = std::dynamic_pointer_cast<WhileStatement>(stmt)) {
    std::stringstream ss;
    ss << getIndent() << "while (" << generateExpression(whileStmt->condition) << ") {\n";
    indent++;
    for (const auto& s : whileStmt->body) {
      ss << generateStatement(s);
    }
    indent--;
    ss << getIndent() << "}\n";
    return ss.str();
  }
  
  if (auto onClockStmt = std::dynamic_pointer_cast<OnClockStatement>(stmt)) {
    // Generate code for on-clock statements
    std::stringstream ss;
    for (const auto& s : onClockStmt->body) {
      ss << generateStatement(s);
    }
    return ss.str();
  }
  
  if (auto returnStmt = std::dynamic_pointer_cast<ReturnStatement>(stmt)) {
    std::stringstream ss;
    ss << getIndent() << "return";
    if (returnStmt->value) {
      ss << " " << generateExpression(returnStmt->value);
    }
    ss << ";\n";
    return ss.str();
  }
  
  return "";
}

std::string ExecutableCodeGenerator::generateExpression(const std::shared_ptr<Expression>& expr) {
  if (!expr) return "0";
  
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
    return "(" + generateExpression(binOp->left) + " " + binOp->op + " " + 
           generateExpression(binOp->right) + ")";
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
  
  if (auto chanOp = std::dynamic_pointer_cast<ChannelOp>(expr)) {
    std::stringstream ss;
    if (chanOp->operation == "send") {
      ss << "gChannels[\"" << chanOp->channel << "\"]->send(ChannelValue(";
      if (!chanOp->arguments.empty()) {
        ss << generateExpression(chanOp->arguments[0]);
      }
      ss << "))";
    } else if (chanOp->operation == "try_recv") {
      ss << "gChannels[\"" << chanOp->channel << "\"]->tryRecv()";
    }
    return ss.str();
  }
  
  return "0";
}

std::string ExecutableCodeGenerator::generateMainFunction() {
  std::stringstream ss;
  
  // Check if user defined a main function
  bool hasUserMain = false;
  for (const auto& func : functions) {
    if (func.name == "main") {
      hasUserMain = true;
      break;
    }
  }
  
  // If user has main and no processes/clocks, just provide wrapper
  if (hasUserMain && processes.empty() && clocks.empty()) {
    ss << "// User-defined main function will be called\n";
    return ss.str();
  }
  
  // Otherwise generate temporal runtime main
  ss << "int main() {\n";
  ss << "  // Initialize channels\n";
  
  // Create channels from declarations
  for (const auto& proc : processes) {
    for (const auto& param : proc.paramNames) {
      ss << "  gChannels[\"" << param << "\"] = std::make_shared<Channel>(4);\n";
    }
  }
  
  ss << "\n  // Run simulation\n";
  ss << "  int numTicks = 10;\n";
  
  for (const auto& clock : clocks) {
    ss << "  // Clock: " << clock.name << " at " << clock.frequency << " Hz\n";
    ss << "  double periodMs = 1000.0 / " << clock.frequency << ";\n";
    ss << "  long long ticksExecuted = 0;\n";
    ss << "  std::vector<double> slacks;\n";
    ss << "\n";
    ss << "  for (int tick = 0; tick < numTicks; tick++) {\n";
    ss << "    auto tickStart = std::chrono::high_resolution_clock::now();\n";
    
    // Execute all processes
    for (const auto& proc : processes) {
      ss << "    " << proc.name << "(";
      for (size_t i = 0; i < proc.paramNames.size(); ++i) {
        if (i > 0) ss << ", ";
        ss << "gChannels[\"" << proc.paramNames[i] << "\"]";
      }
      ss << ");\n";
    }
    
    ss << "    ticksExecuted++;\n";
    ss << "    auto tickEnd = std::chrono::high_resolution_clock::now();\n";
    ss << "    auto elapsed = std::chrono::duration<double, std::milli>(tickEnd - tickStart).count();\n";
    ss << "    double slack = periodMs - elapsed;\n";
    ss << "    slacks.push_back(slack);\n";
    ss << "    // Only sleep if frequency is specified (not max-speed)\n";
    ss << "    if (" << clock.frequency << " < 999000 && slack > 0) {\n";
    ss << "      std::this_thread::sleep_for(std::chrono::milliseconds((long long)slack));\n";
    ss << "    }\n";
    ss << "  }\n";
    
    ss << "\n  // Print statistics\n";
    ss << "  std::cout << \"\\n=== Statistics ===\\n\";\n";
    ss << "  std::cout << \"Clock: " << clock.name << "\\n\";\n";
    if (clock.frequency >= 999000) {
      ss << "  std::cout << \"Mode: MAX SPEED (no sleep delays)\\n\";\n";
    } else {
      ss << "  std::cout << \"Frequency: " << clock.frequency << " Hz\\n\";\n";
      ss << "  std::cout << \"Period: \" << periodMs << \" ms\\n\";\n";
    }
    ss << "  std::cout << \"Ticks executed: \" << ticksExecuted << \"\\n\";\n";
    ss << "  double avgSlack = 0;\n";
    ss << "  double minSlack = slacks[0];\n";
    ss << "  double maxSlack = slacks[0];\n";
    ss << "  for (double s : slacks) {\n";
    ss << "    avgSlack += s;\n";
    ss << "    if (s < minSlack) minSlack = s;\n";
    ss << "    if (s > maxSlack) maxSlack = s;\n";
    ss << "  }\n";
    ss << "  avgSlack /= slacks.size();\n";
    ss << "  std::cout << \"Average slack: \" << avgSlack << \" ms\\n\";\n";
    ss << "  std::cout << \"Min slack: \" << minSlack << \" ms\\n\";\n";
    ss << "  std::cout << \"Max slack: \" << maxSlack << \" ms\\n\";\n";
    ss << "\n";
    ss << "  std::cout << \"\\n=== Channel Statistics ===\\n\";\n";
    ss << "  for (auto& [name, channel] : gChannels) {\n";
    ss << "    if (channel->getTotalMessages() > 0) {\n";
    ss << "      std::cout << \"Channel \" << name << \":\\n\";\n";
    ss << "      std::cout << \"  Messages: \" << channel->getTotalMessages() << \"\\n\";\n";
    ss << "      std::cout << \"  Max depth: \" << channel->getMaxDepth() << \"\\n\";\n";
    ss << "      std::cout << \"  Avg depth: \" << channel->getAverageDepth() << \"\\n\";\n";
    ss << "    }\n";
    ss << "  }\n";
  }
  
  ss << "  return 0;\n";
  ss << "}\n";
  
  return ss.str();
}

std::string ExecutableCodeGenerator::getIndent() {
  return std::string(indent * 2, ' ');
}

void ExecutableCodeGenerator::collectDeclarations(const std::shared_ptr<Program>& program) {
  for (const auto& decl : program->declarations) {
    if (auto clockDecl = std::dynamic_pointer_cast<ClockDecl>(decl)) {
      clocks.push_back({clockDecl->name, clockDecl->frequency});
    } else if (auto procDecl = std::dynamic_pointer_cast<ProcDecl>(decl)) {
      ProcessInfo proc;
      proc.name = procDecl->name;
      
      // Extract parameter names and types
      for (const auto& [paramName, paramType] : procDecl->parameters) {
        proc.paramNames.push_back(paramName);
        proc.paramTypes.push_back(paramType);
      }
      
      proc.body = procDecl->body;
      processes.push_back(proc);
    } else if (auto funcDecl = std::dynamic_pointer_cast<FunctionDecl>(decl)) {
      FunctionInfo func;
      func.name = funcDecl->name;
      func.returnType = funcDecl->returnType;
      
      // Extract parameter names and types
      for (const auto& [paramName, paramType] : funcDecl->parameters) {
        func.paramNames.push_back(paramName);
        func.paramTypes.push_back(paramType);
      }
      
      func.body = funcDecl->body;
      functions.push_back(func);
    }
  }
}
