#include "compiler/lexer.h"
#include "compiler/parser.h"
#include "compiler/semantic_analyzer.h"
#include "runtime/runtime.h"
#include "runtime/codegen.h"
#include "runtime/interpreter.h"
#include "runtime/builtins.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>

using namespace Tick;

char* read_file(const char* filename) {
    FILE* file = fopen(filename, "rb");
    if (!file) {
        fprintf(stderr, "Error: Could not open file '%s'\n", filename);
        return nullptr;
    }
    
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    char* buffer = (char*)malloc(file_size + 1);
    fread(buffer, 1, file_size, file);
    buffer[file_size] = '\0';
    
    fclose(file);
    return buffer;
}

int main(int argc, char** argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: tick <filename.tick>\n");
        return 1;
    }
    
    char* source = read_file(argv[1]);
    if (!source) {
        return 1;
    }
    
    Lexer lexer(source);
    DynamicArray<Token> tokens = lexer.tokenize();
    
    Parser parser(tokens);
    Program* program = parser.parse();
    
    if (!program) {
        fprintf(stderr, "Error: Failed to parse program\n");
        free(source);
        return 1;
    }
    
    SemanticAnalyzer analyzer;
    if (!analyzer.analyze(program)) {
        fprintf(stderr, "Error: Semantic analysis failed\n");
        delete program;
        free(source);
        return 1;
    }
    
    Runtime runtime;
    Builtins::register_all(&runtime);
    
    for (size_t i = 0; i < program->events.size(); i++) {
        runtime.register_event(program->events[i]->name.c_str());
    }
    
    for (size_t i = 0; i < program->signals.size(); i++) {
        runtime.register_signal(program->signals[i]->name.c_str());
    }
    
    CodeGenerator codegen;
    codegen.generate(program);
    
    runtime.set_string_pool(codegen.get_string_pool());
    
    FunctionDecl* main_func = nullptr;
    for (size_t i = 0; i < program->functions.size(); i++) {
        if (program->functions[i]->name == "main") {
            main_func = program->functions[i];
            break;
        }
    }
    
    if (!main_func) {
        fprintf(stderr, "Error: No main function found\n");
        delete program;
        free(source);
        return 1;
    }
    
    for (size_t i = 0; i < program->processes.size(); i++) {
        ProcessDecl* proc = program->processes[i];
        ProcessContext* ctx = new ProcessContext();
        DynamicArray<Instruction>* code = codegen.get_process_code(proc->name.c_str());
        if (code) {
            ctx->bytecode = code->data();
            ctx->bytecode_size = code->size();
        }
        runtime.register_process(proc->event_name.c_str(), ctx);
    }
    
    Interpreter interpreter(&runtime, codegen.get_string_pool());
    DynamicArray<Instruction>* main_code = codegen.get_function_code("main");
    DynamicArray<Value>* constants = codegen.get_constants();
    
    if (main_code) {
        Value result = interpreter.execute(main_code, constants);
        
        if (result.type == Value::INT) {
            delete program;
            free(source);
            return result.int_val;
        }
    }
    
    delete program;
    free(source);
    return 0;
}
