#include "compiler/lexer.h"
#include "compiler/parser.h"
#include "compiler/semantic_analyzer.h"
#include "runtime/runtime.h"
#include "runtime/codegen.h"
#include "runtime/interpreter.h"
#include "runtime/builtins.h"
#include "runtime/cache.h"
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
    
    Runtime runtime;
    Builtins::register_all(&runtime);
    
    DynamicArray<Instruction>* main_code = nullptr;
    HashMap<String, DynamicArray<Instruction>*> function_codes;
    HashMap<String, DynamicArray<Instruction>*> process_codes;
    DynamicArray<String> events;
    DynamicArray<String> signals;
    DynamicArray<String> classes;
    DynamicArray<String> cached_strings;
    DynamicArray<Value> constants;
    StringPool string_pool;
    CodeGenerator* codegen = nullptr;
    
    bool used_cache = BytecodeCache::read_cache(argv[1], &main_code, function_codes, 
                                                 process_codes, events, signals, classes,
                                                 cached_strings, constants);
    
    Program* program = nullptr;
    
    if (!used_cache) {
        char* source = read_file(argv[1]);
        if (!source) {
            return 1;
        }
        
        Lexer lexer(source);
        DynamicArray<Token> tokens = lexer.tokenize();
        
        Parser parser(tokens);
        program = parser.parse();
        
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
        
        for (size_t i = 0; i < program->events.size(); i++) {
            runtime.register_event(program->events[i]->name.c_str());
            events.push(String(program->events[i]->name.c_str()));
        }
        
        for (size_t i = 0; i < program->signals.size(); i++) {
            runtime.register_signal(program->signals[i]->name.c_str());
            signals.push(String(program->signals[i]->name.c_str()));
        }
        
        for (size_t i = 0; i < program->classes.size(); i++) {
            classes.push(String(program->classes[i]->name.c_str()));
        }
        
        codegen = new CodeGenerator();
        codegen->generate(program);
        
        StringPool* gen_pool = codegen->get_string_pool();
        const DynamicArray<String>& pool_strings = gen_pool->get_strings();
        for (size_t i = 0; i < pool_strings.size(); i++) {
            string_pool.add(pool_strings[i]);
        }
        
        DynamicArray<Value>* gen_constants = codegen->get_constants();
        for (size_t i = 0; i < gen_constants->size(); i++) {
            constants.push((*gen_constants)[i]);
        }
        
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
        
        main_code = codegen->get_function_code("main");
        
        for (size_t i = 0; i < program->functions.size(); i++) {
            FunctionDecl* func = program->functions[i];
            if (strcmp(func->name.c_str(), "main") != 0) {
                DynamicArray<Instruction>* code = codegen->get_function_code(func->name.c_str());
                if (code) {
                    function_codes.insert(String(func->name.c_str()), code);
                }
            }
        }
        
        for (size_t i = 0; i < program->classes.size(); i++) {
            ClassDecl* cls = program->classes[i];
            for (size_t j = 0; j < cls->methods.size(); j++) {
                FunctionDecl* method = cls->methods[j];
                size_t name_len = cls->name.length() + method->name.length() + 2;
                char* method_name = (char*)malloc(name_len);
                snprintf(method_name, name_len, "%s.%s", cls->name.c_str(), method->name.c_str());
                
                DynamicArray<Instruction>* code = codegen->get_function_code(method_name);
                if (code) {
                    function_codes.insert(String(method_name), code);
                }
                free(method_name);
            }
        }
        
        for (size_t i = 0; i < program->processes.size(); i++) {
            ProcessDecl* proc = program->processes[i];
            DynamicArray<Instruction>* code = codegen->get_process_code(proc->name.c_str());
            if (code) {
                process_codes.insert(String(proc->name.c_str()), code);
            }
        }
        
        BytecodeCache::write_cache(argv[1], main_code, function_codes, process_codes, 
                                   events, signals, classes, string_pool.get_strings(), constants);
        
        delete program;
        free(source);
    } else {
        string_pool.load_from_array(cached_strings);
        
        for (size_t i = 0; i < events.size(); i++) {
            runtime.register_event(events[i].c_str());
        }
        
        for (size_t i = 0; i < signals.size(); i++) {
            runtime.register_signal(signals[i].c_str());
        }
    }
    
    runtime.set_string_pool(&string_pool);
    runtime.set_constants(&constants);
    
    function_codes.for_each([&](const String& name, DynamicArray<Instruction>* code) {
        runtime.register_user_function(name.c_str(), code);
    });
    
    process_codes.for_each([&](const String& name, DynamicArray<Instruction>* code) {
        ProcessContext* ctx = new ProcessContext();
        ctx->bytecode = code;
        ctx->runtime = &runtime;
        ctx->string_pool = &string_pool;
        ctx->constants = &constants;
        
        for (size_t j = 0; j < events.size(); j++) {
            runtime.register_process(events[j].c_str(), ctx);
        }
    });
    
    Interpreter interpreter(&runtime, &string_pool);
    Value result;
    result.type = Value::INT;
    result.int_val = 0;
    
    if (main_code) {
        result = interpreter.execute(main_code, &constants);
    }
    
    if (codegen) {
        delete codegen;
    }
    
    if (result.type == Value::INT) {
        return result.int_val;
    }
    
    return 0;
}
