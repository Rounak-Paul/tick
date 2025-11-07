#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include "compiler/lexer.h"
#include "compiler/parser.h"
#include "codegen/codegen_new.h"

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
    // Read source file
    std::string source = readFile(inputFile);
    std::cout << "Compiling: " << inputFile << "\n";
    
    // Lexical analysis
    std::cout << "  [1/3] Lexical analysis...\n";
    Lexer lexer(source);
    auto tokens = lexer.tokenize();
    std::cout << "    Tokens generated: " << tokens.size() << "\n";
    
    if (tokens.empty()) {
      std::cerr << "Error: No tokens generated from source file\n";
      return 1;
    }
    
    // Syntax analysis (parsing)
    std::cout << "  [2/3] Parsing...\n";
    Parser parser(tokens);
    auto program = parser.parse();
    
    if (!program) {
      std::cerr << "Error: Failed to parse program\n";
      return 1;
    }
    
    std::cout << "    Parsed " << program->declarations.size() << " declarations\n";
    std::cout << "    Parsed " << program->globalStatements.size() << " statements\n";
    
    // Code generation
    std::cout << "  [3/3] Code generation...\n";
    ExecutableCodeGenerator codegen;
    std::string generated = codegen.generate(program);
    
    // Write generated code to file
    std::string cppFileName = inputFile.substr(0, inputFile.rfind(".")) + ".cpp";
    std::ofstream cppFile(cppFileName);
    cppFile << generated;
    cppFile.close();
    std::cout << "Generated: " << cppFileName << "\n";
    
    // Compile the generated C++ code
    std::cout << "  [4/4] Compiling C++...\n";
    std::string compileCmd = "cd " + fs::path(cppFileName).parent_path().string() + " && "
                             "g++ -std=c++17 -O2 -pthread " + fs::path(cppFileName).filename().string() +
                             " -o " + fs::path(cppFileName).stem().string();
    int result = system(compileCmd.c_str());
    
    if (result == 0) {
      std::cout << "Compilation successful!\n";
      
      // Run the compiled program
      std::cout << "\n=== Execution Output ===\n";
      std::string execCmd = (fs::path(cppFileName).parent_path() / fs::path(cppFileName).stem()).string();
      system(execCmd.c_str());
      
      // Keep generated files for debugging
      // std::remove(cppFileName.c_str());
      // std::string binaryName = fs::path(cppFileName).stem().string();
      // std::remove(binaryName.c_str());
    } else {
      std::cerr << "C++ compilation failed!\n";
      return 1;
    }
    
  } catch (const std::exception& e) {
    std::cerr << "Error: " << e.what() << "\n";
    return 1;
  }
  
  return 0;
}
