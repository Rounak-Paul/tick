#include "../src/compiler/lexer.h"
#include "../src/compiler/parser.h"
#include "../src/compiler/semantic_analyzer.h"
#include <cstdio>
#include <cstring>
#include <sys/time.h>

using namespace Tick;

int tests_passed = 0;
int tests_failed = 0;

void assert_true(bool condition, const char* test_name) {
    if (condition) {
        tests_passed++;
        printf("✓ %s\n", test_name);
    } else {
        tests_failed++;
        printf("✗ %s\n", test_name);
    }
}

void test_lexer_keywords() {
    printf("\n=== Lexer Keyword Tests ===\n");
    
    const char* source = "event signal process int while for if else return break true false";
    Lexer lexer(source);
    DynamicArray<Token> tokens = lexer.tokenize();
    
    assert_true(tokens.size() == 13, "Token count for keywords");
    assert_true(tokens[0].type == TokenType::EVENT, "EVENT token");
    assert_true(tokens[1].type == TokenType::SIGNAL, "SIGNAL token");
    assert_true(tokens[2].type == TokenType::PROCESS, "PROCESS token");
    assert_true(tokens[3].type == TokenType::INT, "INT token");
    assert_true(tokens[4].type == TokenType::WHILE, "WHILE token");
    assert_true(tokens[5].type == TokenType::FOR, "FOR token");
    assert_true(tokens[6].type == TokenType::IF, "IF token");
    assert_true(tokens[7].type == TokenType::ELSE, "ELSE token");
    assert_true(tokens[8].type == TokenType::RETURN, "RETURN token");
    assert_true(tokens[9].type == TokenType::BREAK, "BREAK token");
    assert_true(tokens[10].type == TokenType::TRUE, "TRUE token");
    assert_true(tokens[11].type == TokenType::FALSE, "FALSE token");
}

void test_lexer_operators() {
    printf("\n=== Lexer Operator Tests ===\n");
    
    const char* source = "+ - * / % == != < > <= >= && || !";
    Lexer lexer(source);
    DynamicArray<Token> tokens = lexer.tokenize();
    
    assert_true(tokens[0].type == TokenType::PLUS, "PLUS token");
    assert_true(tokens[1].type == TokenType::MINUS, "MINUS token");
    assert_true(tokens[2].type == TokenType::STAR, "STAR token");
    assert_true(tokens[3].type == TokenType::SLASH, "SLASH token");
    assert_true(tokens[4].type == TokenType::PERCENT, "PERCENT token");
    assert_true(tokens[5].type == TokenType::EQ, "EQ token");
    assert_true(tokens[6].type == TokenType::NEQ, "NEQ token");
    assert_true(tokens[7].type == TokenType::LANGLE, "LANGLE token");
    assert_true(tokens[8].type == TokenType::RANGLE, "RANGLE token");
    assert_true(tokens[9].type == TokenType::LTE, "LTE token");
    assert_true(tokens[10].type == TokenType::GTE, "GTE token");
    assert_true(tokens[11].type == TokenType::AND, "AND token");
    assert_true(tokens[12].type == TokenType::OR, "OR token");
    assert_true(tokens[13].type == TokenType::NOT, "NOT token");
}

void test_lexer_identifiers_and_numbers() {
    printf("\n=== Lexer Identifiers and Numbers ===\n");
    
    const char* source = "x y123 _test 42 0 999";
    Lexer lexer(source);
    DynamicArray<Token> tokens = lexer.tokenize();
    
    assert_true(tokens[0].type == TokenType::IDENTIFIER, "Identifier x");
    assert_true(tokens[1].type == TokenType::IDENTIFIER, "Identifier y123");
    assert_true(tokens[2].type == TokenType::IDENTIFIER, "Identifier _test");
    assert_true(tokens[3].type == TokenType::INTEGER, "Integer 42");
    assert_true(tokens[4].type == TokenType::INTEGER, "Integer 0");
    assert_true(tokens[5].type == TokenType::INTEGER, "Integer 999");
}

void test_parser_simple_function() {
    printf("\n=== Parser Simple Function ===\n");
    
    const char* source = "int main() { return 0; }";
    Lexer lexer(source);
    DynamicArray<Token> tokens = lexer.tokenize();
    Parser parser(tokens);
    Program* program = parser.parse();
    
    assert_true(program != nullptr, "Parse result not null");
    assert_true(program->functions.size() == 1, "One function parsed");
    assert_true(program->functions[0]->name == "main", "Function name is main");
    assert_true(program->functions[0]->return_type == "int", "Return type is int");
    
    delete program;
}

void test_parser_event_and_signal() {
    printf("\n=== Parser Event and Signal ===\n");
    
    const char* source = "event e1; signal<int> sig1;";
    Lexer lexer(source);
    DynamicArray<Token> tokens = lexer.tokenize();
    Parser parser(tokens);
    Program* program = parser.parse();
    
    assert_true(program->events.size() == 1, "One event parsed");
    assert_true(program->events[0]->name == "e1", "Event name is e1");
    assert_true(program->signals.size() == 1, "One signal parsed");
    assert_true(program->signals[0]->name == "sig1", "Signal name is sig1");
    assert_true(program->signals[0]->type_param == "int", "Signal type is int");
    
    delete program;
}

void test_parser_process() {
    printf("\n=== Parser Process ===\n");
    
    const char* source = "@e1 process p1 { int x = 10; }";
    Lexer lexer(source);
    DynamicArray<Token> tokens = lexer.tokenize();
    Parser parser(tokens);
    Program* program = parser.parse();
    
    assert_true(program->processes.size() == 1, "One process parsed");
    assert_true(program->processes[0]->name == "p1", "Process name is p1");
    assert_true(program->processes[0]->event_name == "e1", "Event name is e1");
    
    delete program;
}

void test_parser_expressions() {
    printf("\n=== Parser Expressions ===\n");
    
    const char* source = "int main() { int x = 1 + 2 * 3; return x; }";
    Lexer lexer(source);
    DynamicArray<Token> tokens = lexer.tokenize();
    Parser parser(tokens);
    Program* program = parser.parse();
    
    assert_true(program->functions.size() == 1, "Function parsed");
    BlockStmt* body = program->functions[0]->body;
    assert_true(body->statements.size() == 2, "Two statements in body");
    
    delete program;
}

void test_semantic_analyzer() {
    printf("\n=== Semantic Analyzer Tests ===\n");
    
    const char* source = "event e1; int main() { return 0; }";
    Lexer lexer(source);
    DynamicArray<Token> tokens = lexer.tokenize();
    Parser parser(tokens);
    Program* program = parser.parse();
    
    SemanticAnalyzer analyzer;
    bool result = analyzer.analyze(program);
    assert_true(result == true, "Valid program passes semantic analysis");
    
    delete program;
}

void benchmark_lexer() {
    printf("\n=== Lexer Performance ===\n");
    
    const char* large_source = "int x = 10 + 20; int y = 30 * 40; int z = x + y;";
    
    struct timeval start, end;
    gettimeofday(&start, nullptr);
    
    Lexer lexer(large_source);
    DynamicArray<Token> tokens = lexer.tokenize();
    
    gettimeofday(&end, nullptr);
    
    long seconds = end.tv_sec - start.tv_sec;
    long micros = end.tv_usec - start.tv_usec;
    double elapsed = seconds + micros / 1000000.0;
    
    printf("  Tokenized %zu tokens in %.3f ms (%.0f tokens/sec)\n",
           tokens.size(), elapsed * 1000, tokens.size() / elapsed);
}

void benchmark_parser() {
    printf("\n=== Parser Performance ===\n");
    
    const char* source = "int main() { int x1 = 10; int x2 = 20; int x3 = 30; return 0; }";
    
    Lexer lexer(source);
    DynamicArray<Token> tokens = lexer.tokenize();
    
    struct timeval start, end;
    gettimeofday(&start, nullptr);
    
    Parser parser(tokens);
    Program* program = parser.parse();
    
    gettimeofday(&end, nullptr);
    
    long seconds = end.tv_sec - start.tv_sec;
    long micros = end.tv_usec - start.tv_usec;
    double elapsed = seconds + micros / 1000000.0;
    
    printf("  Parsed program in %.3f ms\n", elapsed * 1000);
    printf("  Functions: %zu\n", program->functions.size());
    
    delete program;
}

int main() {
    printf("╔═══════════════════════════════════════════════╗\n");
    printf("║   Tick Compiler Test Suite                   ║\n");
    printf("╚═══════════════════════════════════════════════╝\n");
    
    test_lexer_keywords();
    test_lexer_operators();
    test_lexer_identifiers_and_numbers();
    test_parser_simple_function();
    test_parser_event_and_signal();
    test_parser_process();
    test_parser_expressions();
    test_semantic_analyzer();
    
    benchmark_lexer();
    benchmark_parser();
    
    printf("\n╔═══════════════════════════════════════════════╗\n");
    printf("║   Test Results                                ║\n");
    printf("╠═══════════════════════════════════════════════╣\n");
    printf("║   Passed: %-3d                                 ║\n", tests_passed);
    printf("║   Failed: %-3d                                 ║\n", tests_failed);
    printf("║   Total:  %-3d                                 ║\n", tests_passed + tests_failed);
    printf("╚═══════════════════════════════════════════════╝\n");
    
    return tests_failed > 0 ? 1 : 0;
}
