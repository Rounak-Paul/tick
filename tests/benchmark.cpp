#include "../src/runtime/runtime.h"
#include "../src/runtime/interpreter.h"
#include "../src/runtime/codegen.h"
#include "../src/runtime/builtins.h"
#include "../src/compiler/lexer.h"
#include "../src/compiler/parser.h"
#include <cstdio>
#include <sys/time.h>
#include <unistd.h>

using namespace Tick;

double get_time() {
    struct timeval tv;
    gettimeofday(&tv, nullptr);
    return tv.tv_sec + tv.tv_usec / 1000000.0;
}

Value execute_program(const char* source) {
    Lexer lexer(source);
    DynamicArray<Token> tokens = lexer.tokenize();
    Parser parser(tokens);
    Program* program = parser.parse();
    
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
    runtime.set_constants(codegen.get_constants());
    
    for (size_t i = 0; i < program->functions.size(); i++) {
        FunctionDecl* func = program->functions[i];
        if (strcmp(func->name.c_str(), "main") != 0) {
            DynamicArray<Instruction>* code = codegen.get_function_code(func->name.c_str());
            if (code) {
                runtime.register_user_function(func->name.c_str(), code);
            }
        }
    }
    
    for (size_t i = 0; i < program->processes.size(); i++) {
        ProcessDecl* proc = program->processes[i];
        ProcessContext* ctx = new ProcessContext();
        DynamicArray<Instruction>* code = codegen.get_process_code(proc->name.c_str());
        ctx->bytecode = code;
        ctx->runtime = &runtime;
        ctx->string_pool = codegen.get_string_pool();
        ctx->constants = codegen.get_constants();
        runtime.register_process(proc->event_name.c_str(), ctx);
    }
    
    Interpreter interpreter(&runtime, codegen.get_string_pool());
    DynamicArray<Instruction>* main_code = codegen.get_function_code("main");
    Value result = Value(0);
    
    if (main_code) {
        result = interpreter.execute(main_code, codegen.get_constants());
    }
    
    delete program;
    return result;
}

void benchmark_arithmetic() {
    printf("\n=== Arithmetic Benchmark ===\n");
    
    // Simple arithmetic without variable reassignment
    const char* source = R"(
        int compute(int n) {
            int a = n + 100;
            int b = a * 2;
            int c = b - 50;
            int d = c / 3;
            int e = d % 7;
            return e;
        }
        
        int main() {
            int r1 = compute(10);
            int r2 = compute(20);
            int r3 = compute(30);
            int r4 = compute(40);
            int r5 = compute(50);
            return r1 + r2 + r3 + r4 + r5;
        }
    )";
    
    double start = get_time();
    Value result = execute_program(source);
    double end = get_time();
    
    printf("  Multiple arithmetic operations: %.6f seconds\n", end - start);
    printf("  Result: %d\n", result.int_val);
}

void benchmark_function_calls() {
    printf("\n=== Function Call Benchmark ===\n");
    
    const char* source = R"(
        int add(int a, int b) {
            return a + b;
        }
        
        int compute_chain(int n) {
            int v1 = add(n, 1);
            int v2 = add(v1, 2);
            int v3 = add(v2, 3);
            int v4 = add(v3, 4);
            int v5 = add(v4, 5);
            return v5;
        }
        
        int main() {
            int r1 = compute_chain(10);
            int r2 = compute_chain(20);
            int r3 = compute_chain(30);
            return r1 + r2 + r3;
        }
    )";
    
    double start = get_time();
    Value result = execute_program(source);
    double end = get_time();
    
    printf("  Function call chain: %.6f seconds\n", end - start);
    printf("  Result: %d\n", result.int_val);
}

void benchmark_recursive_fibonacci() {
    printf("\n=== Recursive Fibonacci Benchmark ===\n");
    
    const char* source = R"(
        int fib(int n) {
            if (n == 0) return 0;
            if (n == 1) return 1;
            return fib(n - 1) + fib(n - 2);
        }
        
        int main() {
            return fib(20);
        }
    )";
    
    double start = get_time();
    Value result = execute_program(source);
    double end = get_time();
    
    printf("  fib(20): %.6f seconds\n", end - start);
    printf("  Result: %d\n", result.int_val);
}

void benchmark_parallel_processing() {
    printf("\n=== Parallel Processing Benchmark ===\n");
    
    const char* source = R"(
        event compute;
        signal<int> r1;
        signal<int> r2;
        signal<int> r3;
        signal<int> r4;
        
        @compute
        process w1 {
            int sum = 100;
            r1.emit(sum);
        }
        
        @compute
        process w2 {
            int sum = 200;
            r2.emit(sum);
        }
        
        @compute
        process w3 {
            int sum = 300;
            r3.emit(sum);
        }
        
        @compute
        process w4 {
            int sum = 400;
            r4.emit(sum);
        }
        
        int main() {
            compute.execute();
            int v1 = r1.recv();
            int v2 = r2.recv();
            int v3 = r3.recv();
            int v4 = r4.recv();
            return v1 + v2 + v3 + v4;
        }
    )";
    
    double start = get_time();
    Value result = execute_program(source);
    double end = get_time();
    
    printf("  4 parallel workers: %.6f seconds\n", end - start);
    printf("  Result: %d\n", result.int_val);
}

void benchmark_string_formatting() {
    printf("\n=== String Formatting Benchmark ===\n");
    
    const char* source = R"(
        int main() {
            println(format("Value 1: {}", 10));
            println(format("Value 2: {}", 20));
            println(format("Value 3: {}", 30));
            println(format("Value 4: {}", 40));
            println(format("Value 5: {}", 50));
            return 5;
        }
    )";
    
    double start = get_time();
    Value result = execute_program(source);
    double end = get_time();
    
    printf("  5 format operations: %.6f seconds\n", end - start);
    printf("  Result: %d\n", result.int_val);
}

void benchmark_compilation() {
    printf("\n=== Compilation Benchmark ===\n");
    
    const char* source = R"(
        event compute;
        signal<int> result1;
        signal<int> result2;
        signal<int> final_result;
        
        @compute
        process worker1 {
            int a = 10;
            int b = 20;
            int sum = a + b;
            result1.emit(sum);
        }
        
        @compute
        process worker2 {
            int x = 5;
            int y = 3;
            int product = x * y;
            result2.emit(product);
        }
        
        @compute
        process combiner {
            int val1 = result1.recv();
            int val2 = result2.recv();
            int combined = val1 + val2;
            final_result.emit(combined);
        }
        
        int calculate(int n) {
            int result = n * 2;
            return result;
        }
        
        int main() {
            compute.execute();
            int final_value = final_result.recv();
            int doubled = calculate(final_value);
            return doubled;
        }
    )";
    
    double start = get_time();
    
    Lexer lexer(source);
    double lex_end = get_time();
    printf("  Lexing: %.6f seconds\n", lex_end - start);
    
    DynamicArray<Token> tokens = lexer.tokenize();
    Parser parser(tokens);
    double parse_end = get_time();
    printf("  Parsing: %.6f seconds\n", parse_end - lex_end);
    
    Program* program = parser.parse();
    CodeGenerator codegen;
    double codegen_start = get_time();
    codegen.generate(program);
    double codegen_end = get_time();
    printf("  Code generation: %.6f seconds\n", codegen_end - codegen_start);
    
    printf("  Total compilation: %.6f seconds\n", codegen_end - start);
    
    delete program;
}

int main() {
    printf("╔════════════════════════════════════════════════════════════════╗\n");
    printf("║            Tick Language - Performance Benchmarks              ║\n");
    printf("╚════════════════════════════════════════════════════════════════╝\n");
    
    int num_cores = sysconf(_SC_NPROCESSORS_ONLN);
    printf("\nSystem: %d CPU cores\n", num_cores);
    
    benchmark_compilation();
    benchmark_arithmetic();
    benchmark_function_calls();
    benchmark_recursive_fibonacci();
    benchmark_parallel_processing();
    benchmark_string_formatting();
    
    printf("\n╔════════════════════════════════════════════════════════════════╗\n");
    printf("║                      Benchmark Complete                        ║\n");
    printf("╚════════════════════════════════════════════════════════════════╝\n");
    
    return 0;
}
