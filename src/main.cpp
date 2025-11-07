#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include "compiler/lexer.h"
#include "compiler/parser.h"
#include "runtime/scheduler.h"

namespace fs = std::filesystem;

void printUsage(const char* programName) {
  std::cout << "Usage: " << programName << " <script.tdl> [options]\n\n";
  std::cout << "Options:\n";
  std::cout << "  --compile-only    Compile to C++ without running\n";
  std::cout << "  --output FILE     Write compiled output to FILE\n";
  std::cout << "  --help            Show this help message\n";
  std::cout << "  --version         Show version\n";
}

std::string readFile(const std::string& filename) {
  std::ifstream file(filename);
  if (!file.is_open()) {
    std::cerr << "Error: Cannot open file '" << filename << "'\n";
    exit(1);
  }
  std::stringstream buffer;
  buffer << file.rdbuf();
  return buffer.str();
}

int main(int argc, char* argv[]) {
  if (argc < 2) {
    std::cerr << "Error: No input file specified\n";
    printUsage(argv[0]);
    return 1;
  }
  
  std::string inputFile = argv[1];
  std::string outputFile;
  bool compileOnly = false;
  
  // Parse command line arguments
  for (int i = 2; i < argc; ++i) {
    std::string arg = argv[i];
    if (arg == "--help") {
      printUsage(argv[0]);
      return 0;
    } else if (arg == "--version") {
      std::cout << "TDL Compiler v0.1.0\n";
      return 0;
    } else if (arg == "--compile-only") {
      compileOnly = true;
    } else if (arg == "--output" && i + 1 < argc) {
      outputFile = argv[++i];
    }
  }
  
  // Check if file exists
  if (!fs::exists(inputFile)) {
    std::cerr << "Error: File '" << inputFile << "' not found\n";
    return 1;
  }
  
  // Check if file has .tdl extension
  if (inputFile.substr(inputFile.find_last_of(".") + 1) != "tdl") {
    std::cerr << "Warning: Expected .tdl file, got '" << inputFile << "'\n";
  }
  
  try {
    // Resolve absolute path of input file
    fs::path inputPath = fs::absolute(inputFile);
    fs::path sourceDir = inputPath.parent_path();
    
    std::string source = readFile(inputFile);
    
    std::cout << "Compiling: " << inputFile << "\n";
    std::cout << "  [1/2] Lexical analysis...\n";
    
    Lexer lexer(source);
    auto tokens = lexer.tokenize();
    std::cout << "    Tokens: " << tokens.size() << "\n";
    
    if (tokens.empty()) {
      std::cerr << "Error: No tokens generated\n";
      return 1;
    }
    
    std::cout << "  [2/2] Parsing...\n";
    Parser parser(tokens);
    auto program = parser.parse();
    
    if (!program) {
      std::cerr << "Error: Failed to parse\n";
      return 1;
    }
    
    std::cout << "    Declarations: " << program->declarations.size() << "\n";
    std::cout << "    Statements: " << program->globalStatements.size() << "\n";
    
    // Execute via unified deterministic scheduler
    // Handles: pure functions, parallel processes, channels, clocks
    std::cout << "\n=== Execution Output ===\n";
    
    Scheduler scheduler;
    return scheduler.executeProgram(program);
    
  } catch (const std::exception& e) {
    std::cerr << "Error: " << e.what() << "\n";
    return 1;
  }
  
  return 0;
}
