#include "../src/runtime/runtime.h"
#include "../src/runtime/interpreter.h"
#include "../src/runtime/codegen.h"
#include "../src/runtime/builtins.h"
#include "../src/compiler/lexer.h"
#include "../src/compiler/parser.h"
#include "../src/runtime/string_pool.h"
#include <cstdio>
#include <sys/time.h>
#include <unistd.h>
#include <cstdlib>

using namespace Tick;

double get_time() {
    struct timeval tv;
    gettimeofday(&tv, nullptr);
    return tv.tv_sec + tv.tv_usec / 1000000.0;
}

double run_python(const char* script) {
    char cmd[1024];
    snprintf(cmd, sizeof(cmd), "python3 -c '%s' 2>/dev/null", script);
    
    double start = get_time();
    int result = system(cmd);
    double end = get_time();
    
    if (result != 0) return -1.0;
    return end - start;
}

Value execute_tick(const char* source) {
    static DynamicArray<HashMap<String, DynamicArray<Instruction>*>*> all_function_codes;
    
    Lexer lexer(source);
    DynamicArray<Token> tokens = lexer.tokenize();
    Parser parser(tokens);
    Program* program = parser.parse();
    if (!program) return Value(0);
    
    Runtime runtime;
    Builtins::register_all(&runtime);
    CodeGenerator codegen;
    codegen.generate(program);
    
    StringPool* sp = codegen.get_string_pool();
    DynamicArray<Value>* constants = codegen.get_constants();
    
    for (size_t i = 0; i < program->events.size(); i++) {
        int idx = sp->add(program->events[i]->name);
        runtime.register_event(sp->get(idx));
    }
    
    for (size_t i = 0; i < program->signals.size(); i++) {
        int idx = sp->add(program->signals[i]->name);
        runtime.register_signal(sp->get(idx));
    }
    
    runtime.set_string_pool(sp);
    runtime.set_constants(constants);
    
    HashMap<String, DynamicArray<Instruction>*>* function_codes = new HashMap<String, DynamicArray<Instruction>*>();
    all_function_codes.push(function_codes);
    
    for (size_t i = 0; i < program->functions.size(); i++) {
        if (strcmp(program->functions[i]->name.c_str(), "main") != 0) {
            DynamicArray<Instruction>* code = codegen.get_function_code(program->functions[i]->name.c_str());
            if (code) {
                function_codes->insert(String(program->functions[i]->name.c_str()), code);
            }
        }
    }
    
    function_codes->for_each([&](const String& name, DynamicArray<Instruction>* code) {
        runtime.register_user_function(name.c_str(), code);
    });
    
    for (size_t i = 0; i < program->processes.size(); i++) {
        ProcessDecl* proc = program->processes[i];
        DynamicArray<Instruction>* code = codegen.get_process_code(proc->name.c_str());
        if (code) {
            ProcessContext* ctx = new ProcessContext();
            ctx->bytecode = code;
            ctx->runtime = &runtime;
            ctx->string_pool = sp;
            ctx->constants = constants;
            runtime.register_process(proc->event_name.c_str(), ctx);
        }
    }
    
    Interpreter interpreter(&runtime, sp);
    DynamicArray<Instruction>* main_code = codegen.get_function_code("main");
    Value result = Value(0);
    if (main_code) result = interpreter.execute(main_code, constants);
    delete program;
    return result;
}

int c_fib(int n) {
    if (n <= 1) return n;
    return c_fib(n - 1) + c_fib(n - 2);
}

int c_factorial(int n) {
    if (n <= 1) return 1;
    return n * c_factorial(n - 1);
}

int c_sum_range(int start, int end) {
    int sum = 0;
    for (int i = start; i <= end; i++) sum += i;
    return sum;
}

void bench_fib() {
    printf("\n=== Fibonacci(35) - Heavy Recursion ===\n");
    const char* src = "func fib(n: int): int { if (n <= 1) { return n; } return fib(n - 1) + fib(n - 2); } func main(): int { return fib(35); }";
    
    double ts = get_time();
    Value tr = execute_tick(src);
    double te = get_time();
    
    double cs = get_time();
    int cr = c_fib(35);
    double ce = get_time();
    
    printf("  C:      %.6f s (result: %d)\n", ce - cs, cr);
    printf("  Tick:   %.6f s (result: %d)\n", te - ts, tr.int_val);
    
    const char* py_script = "def fib(n):\\n    if n <= 1: return n\\n    return fib(n-1) + fib(n-2)\\nfib(35)";
    double py_time = run_python(py_script);
    if (py_time > 0) {
        printf("  Python: %.6f s\n", py_time);
        printf("  Tick vs C: %.2fx slower\n", (te - ts) / (ce - cs));
        printf("  Tick vs Python: %.2fx faster\n", py_time / (te - ts));
    } else {
        printf("  Overhead: %.2fx\n", (te - ts) / (ce - cs));
    }
}

void bench_factorial() {
    printf("\n=== Factorial(25) - Deep Recursion ===\n");
    const char* src = "func factorial(n: int): int { if (n <= 1) { return 1; } return n * factorial(n - 1); } func main(): int { return factorial(25); }";
    
    double ts = get_time();
    Value tr = execute_tick(src);
    double te = get_time();
    
    double cs = get_time();
    int cr = c_factorial(25);
    double ce = get_time();
    
    printf("  C:      %.6f s (result: %d)\n", ce - cs, cr);
    printf("  Tick:   %.6f s (result: %d)\n", te - ts, tr.int_val);
    
    const char* py_script = "def factorial(n):\\n    if n <= 1: return 1\\n    return n * factorial(n-1)\\nfactorial(25)";
    double py_time = run_python(py_script);
    if (py_time > 0) {
        printf("  Python: %.6f s\n", py_time);
        if (ce - cs > 0) {
            printf("  Tick vs C: %.2fx slower\n", (te - ts) / (ce - cs));
        }
        printf("  Tick vs Python: %.2fx faster\n", py_time / (te - ts));
    } else if (ce - cs > 0) {
        printf("  Overhead: %.2fx\n", (te - ts) / (ce - cs));
    } else {
        printf("  Overhead: N/A (C too fast)\n");
    }
}

void bench_sum() {
    printf("\n=== Sum 1 to 5000000 - Heavy Loop ===\n");
    const char* src = "func sum_range(start: int, end: int): int { var sum: int = 0; var i: int = start; while (i <= end) { sum = sum + i; i = i + 1; } return sum; } func main(): int { return sum_range(1, 5000000); }";
    
    double ts = get_time();
    Value tr = execute_tick(src);
    double te = get_time();
    
    double cs = get_time();
    int cr = c_sum_range(1, 5000000);
    double ce = get_time();
    
    printf("  C:      %.6f s (result: %d)\n", ce - cs, cr);
    printf("  Tick:   %.6f s (result: %d)\n", te - ts, tr.int_val);
    
    const char* py_script = "sum(range(1, 5000001))";
    double py_time = run_python(py_script);
    if (py_time > 0) {
        printf("  Python: %.6f s\n", py_time);
        if (ce - cs > 0) {
            printf("  Tick vs C: %.2fx slower\n", (te - ts) / (ce - cs));
        }
        printf("  Tick vs Python: %.2fx faster\n", py_time / (te - ts));
    } else if (ce - cs > 0) {
        printf("  Overhead: %.2fx\n", (te - ts) / (ce - cs));
    } else {
        printf("  Overhead: N/A (C too fast)\n");
    }
}

void bench_nested() {
    printf("\n=== Nested Loops (3000x3000) - Heavy Iteration ===\n");
    const char* src = "func nested(): int { var sum: int = 0; var i: int = 0; while (i < 3000) { var j: int = 0; while (j < 3000) { sum = sum + 1; j = j + 1; } i = i + 1; } return sum; } func main(): int { return nested(); }";
    
    double ts = get_time();
    Value tr = execute_tick(src);
    double te = get_time();
    
    double cs = get_time();
    int cr = 0;
    for (int i = 0; i < 3000; i++) {
        for (int j = 0; j < 3000; j++) cr++;
    }
    double ce = get_time();
    
    printf("  C:      %.6f s (result: %d)\n", ce - cs, cr);
    printf("  Tick:   %.6f s (result: %d)\n", te - ts, tr.int_val);
    
    const char* py_script = "sum = 0\\nfor i in range(3000):\\n    for j in range(3000):\\n        sum += 1";
    double py_time = run_python(py_script);
    if (py_time > 0) {
        printf("  Python: %.6f s\n", py_time);
        if (ce - cs > 0) {
            printf("  Tick vs C: %.2fx slower\n", (te - ts) / (ce - cs));
        }
        printf("  Tick vs Python: %.2fx faster\n", py_time / (te - ts));
    } else if (ce - cs > 0) {
        printf("  Overhead: %.2fx\n", (te - ts) / (ce - cs));
    } else {
        printf("  Overhead: N/A (C too fast)\n");
    }
}

void bench_parallel_workers() {
    printf("\n=== Parallel Workers - Signal Communication ===\n");
    const char* src = R"(
        event compute;
        signal<int> result1;
        signal<int> result2;
        signal<int> result3;
        signal<int> result4;
        
        @compute process worker1 {
            var sum: int = 0;
            var i: int = 0;
            while (i < 25000) {
                sum = sum + i;
                i = i + 1;
            }
            result1.emit(sum);
        }
        
        @compute process worker2 {
            var sum: int = 0;
            var i: int = 25000;
            while (i < 50000) {
                sum = sum + i;
                i = i + 1;
            }
            result2.emit(sum);
        }
        
        @compute process worker3 {
            var sum: int = 0;
            var i: int = 50000;
            while (i < 75000) {
                sum = sum + i;
                i = i + 1;
            }
            result3.emit(sum);
        }
        
        @compute process worker4 {
            var sum: int = 0;
            var i: int = 75000;
            while (i < 100000) {
                sum = sum + i;
                i = i + 1;
            }
            result4.emit(sum);
        }
        
        func main(): int {
            compute.execute();
            var r1: int = result1.recv();
            var r2: int = result2.recv();
            var r3: int = result3.recv();
            var r4: int = result4.recv();
            return r1 + r2 + r3 + r4;
        }
    )";
    
    double ts = get_time();
    Value tr = execute_tick(src);
    double te = get_time();
    
    double cs = get_time();
    int cr = 0;
    for (int i = 0; i < 100000; i++) cr += i;
    double ce = get_time();
    
    printf("  C Sequential:    %.6f s (result: %d)\n", ce - cs, cr);
    printf("  Tick Parallel:   %.6f s (result: %d)\n", te - ts, tr.int_val);
    
    const char* py_script = "sum(range(100000))";
    double py_time = run_python(py_script);
    if (py_time > 0) {
        printf("  Python Sequential: %.6f s\n", py_time);
        printf("  Tick Parallel vs Python: %.2fx faster\n", py_time / (te - ts));
    }
}

void bench_parallel_pipeline() {
    printf("\n=== Parallel Pipeline - Event Chaining ===\n");
    const char* src = R"(
        event stage1;
        event stage2;
        event stage3;
        signal<int> s1;
        signal<int> s2;
        signal<int> s3;
        
        @stage1 process gen {
            var i: int = 0;
            while (i < 5000) {
                s1.emit(i);
                i = i + 1;
            }
        }
        
        @stage2 process process_data {
            var sum: int = 0;
            var i: int = 0;
            while (i < 5000) {
                var val: int = s1.recv();
                sum = sum + val;
                i = i + 1;
            }
            s2.emit(sum);
        }
        
        @stage3 process aggregate {
            var result: int = s2.recv();
            s3.emit(result);
        }
        
        func main(): int {
            stage1.execute();
            stage2.execute();
            stage3.execute();
            return s3.recv();
        }
    )";
    
    double ts = get_time();
    Value tr = execute_tick(src);
    double te = get_time();
    
    printf("  Tick Pipeline: %.6f s (result: %d)\n", te - ts, tr.int_val);
    printf("  [Native parallel event-driven architecture]\n");
}

int main() {
    printf("╔════════════════════════════════════════════════════════════════╗\n");
    printf("║         Tick vs C Performance Benchmark                        ║\n");
    printf("╚════════════════════════════════════════════════════════════════╝\n");
    printf("\nSystem: %d CPU cores\n", (int)sysconf(_SC_NPROCESSORS_ONLN));
    
    printf("\n--- Sequential Performance Tests ---\n");
    bench_fib();
    bench_factorial();
    bench_sum();
    bench_nested();
    
    printf("\n--- Parallel Processing Tests ---\n");
    bench_parallel_workers();
    bench_parallel_pipeline();
    
    printf("\n╔════════════════════════════════════════════════════════════════╗\n");
    printf("║                      Benchmark Complete                        ║\n");
    printf("╚════════════════════════════════════════════════════════════════╝\n");
    return 0;
}
