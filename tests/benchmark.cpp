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
        func compute(n : int) : int {
            var a : int = n + 100;
            var b : int = a * 2;
            var c : int = b - 50;
            var d : int = c / 3;
            var e : int = d % 7;
            return e;
        }
        
        func main() : int {
            var r1 : int = compute(10);
            var r2 : int = compute(20);
            var r3 : int = compute(30);
            var r4 : int = compute(40);
            var r5 : int = compute(50);
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
        func add(a : int, b : int) : int {
            return a + b;
        }
        
        func compute_chain(n : int) : int {
            var v1 : int = add(n, 1);
            var v2 : int = add(v1, 2);
            var v3 : int = add(v2, 3);
            var v4 : int = add(v3, 4);
            var v5 : int = add(v4, 5);
            return v5;
        }
        
        func main() : int {
            var r1 : int = compute_chain(10);
            var r2 : int = compute_chain(20);
            var r3 : int = compute_chain(30);
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
        func fib(n : int) : int {
            if (n == 0) return 0;
            if (n == 1) return 1;
            return fib(n - 1) + fib(n - 2);
        }
        
        func main() : int {
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
            var sum : int = 100;
            r1.emit(sum);
        }
        
        @compute
        process w2 {
            var sum : int = 200;
            r2.emit(sum);
        }
        
        @compute
        process w3 {
            var sum : int = 300;
            r3.emit(sum);
        }
        
        @compute
        process w4 {
            var sum : int = 400;
            r4.emit(sum);
        }
        
        func main() : int {
            compute.execute();
            var v1 : int = r1.recv();
            var v2 : int = r2.recv();
            var v3 : int = r3.recv();
            var v4 : int = r4.recv();
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
        func main() : int {
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
            var a : int = 10;
            var b : int = 20;
            var sum : int = a + b;
            result1.emit(sum);
        }
        
        @compute
        process worker2 {
            var x : int = 5;
            var y : int = 3;
            var product : int = x * y;
            result2.emit(product);
        }
        
        @compute
        process combiner {
            var val1 : int = result1.recv();
            var val2 : int = result2.recv();
            var combined : int = val1 + val2;
            final_result.emit(combined);
        }
        
        func calculate(n : int) : int {
            var result : int = n * 2;
            return result;
        }
        
        func main() : int {
            compute.execute();
            var final_value : int = final_result.recv();
            var doubled : int = calculate(final_value);
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
