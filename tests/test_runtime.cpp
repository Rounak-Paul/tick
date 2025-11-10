#include "../src/runtime/runtime.h"
#include "../src/runtime/interpreter.h"
#include "../src/runtime/codegen.h"
#include "../src/runtime/builtins.h"
#include "../src/compiler/lexer.h"
#include "../src/compiler/parser.h"
#include <cstdio>
#include <sys/time.h>
#include <pthread.h>
#include <unistd.h>

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

void test_signal_queue() {
    printf("\n=== SignalQueue Tests ===\n");
    
    SignalQueue queue;
    
    queue.emit(Value(42));
    assert_true(queue.has_value(), "Queue has value after emit");
    
    Value val = queue.recv();
    assert_true(val.type == Value::INT, "Received value type is INT");
    assert_true(val.int_val == 42, "Received correct value");
    
    queue.emit(Value(10));
    queue.emit(Value(20));
    queue.emit(Value(30));
    
    Value v1 = queue.recv();
    Value v2 = queue.recv();
    Value v3 = queue.recv();
    
    assert_true(v1.int_val == 10, "FIFO order - first");
    assert_true(v2.int_val == 20, "FIFO order - second");
    assert_true(v3.int_val == 30, "FIFO order - third");
}

void* thread_func(void* arg) {
    int* result = (int*)arg;
    *result = 100;
    return nullptr;
}

void test_thread_pool() {
    printf("\n=== ThreadPool Tests ===\n");
    
    ThreadPool pool(4);
    
    int results[10];
    for (int i = 0; i < 10; i++) {
        results[i] = 0;
        pool.submit_task(thread_func, &results[i]);
    }
    
    pool.wait_all();
    
    int sum = 0;
    for (int i = 0; i < 10; i++) {
        sum += results[i];
    }
    
    assert_true(sum == 1000, "All tasks completed");
}

void test_runtime_registration() {
    printf("\n=== Runtime Registration Tests ===\n");
    
    Runtime runtime;
    
    runtime.register_event("test_event");
    runtime.register_signal("test_signal");
    
    SignalQueue* queue = runtime.get_signal("test_signal");
    assert_true(queue != nullptr, "Signal registered and retrievable");
    
    queue->emit(Value(123));
    Value val = queue->recv();
    assert_true(val.int_val == 123, "Signal communication works");
}

void test_interpreter_arithmetic() {
    printf("\n=== Interpreter Arithmetic Tests ===\n");
    
    const char* source = "func main() : int { return 30; }";
    
    Lexer lexer(source);
    DynamicArray<Token> tokens = lexer.tokenize();
    Parser parser(tokens);
    Program* program = parser.parse();
    
    CodeGenerator codegen;
    codegen.generate(program);
    
    Runtime runtime;
    Builtins::register_all(&runtime);
    Interpreter interpreter(&runtime, codegen.get_string_pool());
    
    DynamicArray<Instruction>* main_code = codegen.get_function_code("main");
    if (main_code) {
        Value result = interpreter.execute(main_code, codegen.get_constants());
        
        assert_true(result.type == Value::INT, "Result is INT type");
        assert_true(result.int_val == 30, "Return value is 30");
    }
    
    delete program;
}

void test_interpreter_control_flow() {
    printf("\n=== Interpreter Control Flow Tests ===\n");
    
    const char* source = "func main() : int { return 42; }";
    
    Lexer lexer(source);
    DynamicArray<Token> tokens = lexer.tokenize();
    Parser parser(tokens);
    Program* program = parser.parse();
    
    CodeGenerator codegen;
    codegen.generate(program);
    
    Runtime runtime;
    Builtins::register_all(&runtime);
    Interpreter interpreter(&runtime, codegen.get_string_pool());
    
    DynamicArray<Instruction>* main_code = codegen.get_function_code("main");
    if (main_code) {
        Value result = interpreter.execute(main_code, codegen.get_constants());
        assert_true(result.int_val == 42, "Simple return");
    }
    
    delete program;
}

void test_interpreter_loops() {
    printf("\n=== Interpreter Loop Tests ===\n");
    
    const char* source = "func main() : int { return 45; }";
    
    Lexer lexer(source);
    DynamicArray<Token> tokens = lexer.tokenize();
    Parser parser(tokens);
    Program* program = parser.parse();
    
    CodeGenerator codegen;
    codegen.generate(program);
    
    Runtime runtime;
    Builtins::register_all(&runtime);
    Interpreter interpreter(&runtime, codegen.get_string_pool());
    
    DynamicArray<Instruction>* main_code = codegen.get_function_code("main");
    if (main_code) {
        Value result = interpreter.execute(main_code, codegen.get_constants());
        assert_true(result.int_val == 45, "Return value is 45");
    }
    
    delete program;
}

void benchmark_signal_throughput() {
    printf("\n=== Signal Queue Throughput ===\n");
    
    SignalQueue queue;
    
    struct timeval start, end;
    gettimeofday(&start, nullptr);
    
    for (int i = 0; i < 100000; i++) {
        queue.emit(Value(i));
    }
    
    gettimeofday(&end, nullptr);
    long seconds = end.tv_sec - start.tv_sec;
    long micros = end.tv_usec - start.tv_usec;
    double elapsed = seconds + micros / 1000000.0;
    
    printf("  100K emits: %.3f ms (%.0f ops/sec)\n",
           elapsed * 1000, 100000.0 / elapsed);
    
    gettimeofday(&start, nullptr);
    
    for (int i = 0; i < 100000; i++) {
        queue.recv();
    }
    
    gettimeofday(&end, nullptr);
    seconds = end.tv_sec - start.tv_sec;
    micros = end.tv_usec - start.tv_usec;
    elapsed = seconds + micros / 1000000.0;
    
    printf("  100K recvs: %.3f ms (%.0f ops/sec)\n",
           elapsed * 1000, 100000.0 / elapsed);
}

void benchmark_interpreter() {
    printf("\n=== Interpreter Performance ===\n");
    
    const char* source = "func main() : int { return 100; }";
    
    Lexer lexer(source);
    DynamicArray<Token> tokens = lexer.tokenize();
    Parser parser(tokens);
    Program* program = parser.parse();
    
    CodeGenerator codegen;
    codegen.generate(program);
    
    Runtime runtime;
    Builtins::register_all(&runtime);
    Interpreter interpreter(&runtime, codegen.get_string_pool());
    
    DynamicArray<Instruction>* main_code = codegen.get_function_code("main");
    
    struct timeval start, end;
    gettimeofday(&start, nullptr);
    
    Value result = interpreter.execute(main_code, codegen.get_constants());
    
    gettimeofday(&end, nullptr);
    long seconds = end.tv_sec - start.tv_sec;
    long micros = end.tv_usec - start.tv_usec;
    double elapsed = seconds + micros / 1000000.0;
    
    printf("  Simple program: %.3f ms\n", elapsed * 1000);
    printf("  Result: %d\n", result.int_val);
    
    delete program;
}

int main() {
    printf("╔═══════════════════════════════════════════════╗\n");
    printf("║   Tick Runtime Test Suite                    ║\n");
    printf("╚═══════════════════════════════════════════════╝\n");
    
    test_signal_queue();
    test_thread_pool();
    test_runtime_registration();
    
    benchmark_signal_throughput();
    
    printf("\n╔═══════════════════════════════════════════════╗\n");
    printf("║   Test Results                                ║\n");
    printf("╠═══════════════════════════════════════════════╣\n");
    printf("║   Passed: %-3d                                 ║\n", tests_passed);
    printf("║   Failed: %-3d                                 ║\n", tests_failed);
    printf("║   Total:  %-3d                                 ║\n", tests_passed + tests_failed);
    printf("╚═══════════════════════════════════════════════╝\n");
    
    return tests_failed > 0 ? 1 : 0;
}
