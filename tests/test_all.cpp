#include "../src/runtime/runtime.h"
#include "../src/runtime/interpreter.h"
#include "../src/runtime/codegen.h"
#include "../src/runtime/builtins.h"
#include "../src/compiler/lexer.h"
#include "../src/compiler/parser.h"
#include "../src/compiler/semantic_analyzer.h"
#include "../src/runtime/string_pool.h"
#include <cstdio>
#include <cstring>

using namespace Tick;

int tests_passed = 0;
int tests_failed = 0;

#define TEST(name) printf("  • %s: ", name)
#define PASS() do { tests_passed++; printf("✓\n"); } while(0)
#define FAIL(msg) do { tests_failed++; printf("✗ %s\n", msg); } while(0)
#define ASSERT_EQ(expected, actual) do { if ((expected) != (actual)) { printf("✗ expected %d, got %d\n", (expected), (actual)); tests_failed++; return; } } while(0)
#define ASSERT_TRUE(cond) do { if (!(cond)) { FAIL("condition false"); return; } } while(0)

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
    
    StringPool* string_pool = codegen.get_string_pool();
    DynamicArray<Value>* constants = codegen.get_constants();
    
    for (size_t i = 0; i < program->events.size(); i++) {
        int idx = string_pool->add(program->events[i]->name);
        runtime.register_event(string_pool->get(idx));
    }
    
    for (size_t i = 0; i < program->signals.size(); i++) {
        int idx = string_pool->add(program->signals[i]->name);
        runtime.register_signal(string_pool->get(idx));
    }
    
    runtime.set_string_pool(string_pool);
    runtime.set_constants(constants);
    
    HashMap<String, DynamicArray<Instruction>*>* function_codes = new HashMap<String, DynamicArray<Instruction>*>();
    all_function_codes.push(function_codes);
    
    for (size_t i = 0; i < program->functions.size(); i++) {
        FunctionDecl* func = program->functions[i];
        if (strcmp(func->name.c_str(), "main") != 0) {
            DynamicArray<Instruction>* code = codegen.get_function_code(func->name.c_str());
            if (code) {
                function_codes->insert(String(func->name.c_str()), code);
            }
        }
    }
    
    for (size_t i = 0; i < program->classes.size(); i++) {
        ClassDecl* cls = program->classes[i];
        for (size_t j = 0; j < cls->methods.size(); j++) {
            FunctionDecl* method = cls->methods[j];
            char method_name[256];
            snprintf(method_name, sizeof(method_name), "%s.%s", cls->name.c_str(), method->name.c_str());
            
            DynamicArray<Instruction>* code = codegen.get_function_code(method_name);
            if (code) {
                function_codes->insert(String(method_name), code);
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
            ctx->string_pool = string_pool;
            ctx->constants = constants;
            runtime.register_process(proc->event_name.c_str(), ctx);
        }
    }
    
    Interpreter interpreter(&runtime, string_pool);
    DynamicArray<Instruction>* main_code = codegen.get_function_code("main");
    Value result = Value(0);
    
    if (main_code) {
        result = interpreter.execute(main_code, constants);
    }
    
    delete program;
    return result;
}

void test_arithmetic() {
    TEST("addition");
    Value r = execute_tick("func main(): int { return 10 + 5; }");
    ASSERT_EQ(15, r.int_val);
    PASS();
    
    TEST("subtraction");
    r = execute_tick("func main(): int { return 20 - 8; }");
    ASSERT_EQ(12, r.int_val);
    PASS();
    
    TEST("multiplication");
    r = execute_tick("func main(): int { return 7 * 6; }");
    ASSERT_EQ(42, r.int_val);
    PASS();
    
    TEST("division");
    r = execute_tick("func main(): int { return 100 / 4; }");
    ASSERT_EQ(25, r.int_val);
    PASS();
    
    TEST("modulo");
    r = execute_tick("func main(): int { return 17 % 5; }");
    ASSERT_EQ(2, r.int_val);
    PASS();
    
    TEST("complex expression");
    r = execute_tick("func main(): int { return (10 + 5) * 2 - 3; }");
    ASSERT_EQ(27, r.int_val);
    PASS();
}

void test_variables() {
    TEST("variable declaration and assignment");
    Value r = execute_tick("func main(): int { var x: int = 42; return x; }");
    ASSERT_EQ(42, r.int_val);
    PASS();
    
    TEST("variable reassignment");
    r = execute_tick("func main(): int { var x: int = 10; x = 20; return x; }");
    ASSERT_EQ(20, r.int_val);
    PASS();
    
    TEST("multiple variables");
    r = execute_tick("func main(): int { var x: int = 10; var y: int = 20; return x + y; }");
    ASSERT_EQ(30, r.int_val);
    PASS();
}

void test_control_flow() {
    TEST("if statement true");
    Value r = execute_tick("func main(): int { if (1 == 1) { return 42; } return 0; }");
    ASSERT_EQ(42, r.int_val);
    PASS();
    
    TEST("if statement false");
    r = execute_tick("func main(): int { if (1 == 2) { return 42; } return 10; }");
    ASSERT_EQ(10, r.int_val);
    PASS();
    
    TEST("if-else true branch");
    r = execute_tick("func main(): int { if (5 > 3) { return 1; } else { return 2; } }");
    ASSERT_EQ(1, r.int_val);
    PASS();
    
    TEST("if-else false branch");
    r = execute_tick("func main(): int { if (3 > 5) { return 1; } else { return 2; } }");
    ASSERT_EQ(2, r.int_val);
    PASS();
    
    TEST("while loop");
    r = execute_tick(R"(
        func main(): int {
            var sum: int = 0;
            var i: int = 1;
            while (i <= 5) {
                sum = sum + i;
                i = i + 1;
            }
            return sum;
        }
    )");
    ASSERT_EQ(15, r.int_val);
    PASS();
}

void test_comparisons() {
    TEST("less than true");
    Value r = execute_tick("func main(): int { if (3 < 5) { return 1; } return 0; }");
    ASSERT_EQ(1, r.int_val);
    PASS();
    
    TEST("less than false");
    r = execute_tick("func main(): int { if (5 < 3) { return 1; } return 0; }");
    ASSERT_EQ(0, r.int_val);
    PASS();
    
    TEST("greater than");
    r = execute_tick("func main(): int { if (10 > 5) { return 1; } return 0; }");
    ASSERT_EQ(1, r.int_val);
    PASS();
    
    TEST("equal");
    r = execute_tick("func main(): int { if (7 == 7) { return 1; } return 0; }");
    ASSERT_EQ(1, r.int_val);
    PASS();
    
    TEST("not equal");
    r = execute_tick("func main(): int { if (5 != 3) { return 1; } return 0; }");
    ASSERT_EQ(1, r.int_val);
    PASS();
    
    TEST("less than or equal");
    r = execute_tick("func main(): int { if (5 <= 5) { return 1; } return 0; }");
    ASSERT_EQ(1, r.int_val);
    PASS();
    
    TEST("greater than or equal");
    r = execute_tick("func main(): int { if (10 >= 9) { return 1; } return 0; }");
    ASSERT_EQ(1, r.int_val);
    PASS();
}

void test_functions() {
    TEST("simple function");
    Value r = execute_tick(R"(
        func add(a: int, b: int): int {
            return a + b;
        }
        func main(): int {
            return add(10, 20);
        }
    )");
    ASSERT_EQ(30, r.int_val);
    PASS();
    
    TEST("nested function calls");
    r = execute_tick(R"(
        func twice(x: int): int {
            return x * 2;
        }
        func quad(x: int): int {
            return twice(twice(x));
        }
        func main(): int {
            return quad(5);
        }
    )");
    ASSERT_EQ(20, r.int_val);
    PASS();
    
    TEST("recursion factorial");
    r = execute_tick(R"(
        func factorial(n: int): int {
            if (n <= 1) {
                return 1;
            }
            return n * factorial(n - 1);
        }
        func main(): int {
            return factorial(5);
        }
    )");
    ASSERT_EQ(120, r.int_val);
    PASS();
    
    TEST("recursion fibonacci");
    r = execute_tick(R"(
        func fib(n: int): int {
            if (n <= 1) { return n; }
            return fib(n-1) + fib(n-2);
        }
        func main(): int {
            return fib(10);
        }
    )");
    ASSERT_EQ(55, r.int_val);
    PASS();
}

void test_classes() {
    TEST("class field access");
    Value r = execute_tick(R"(
        class Box {
            var value: int;
        }
        func main(): int {
            var b: Box = new Box();
            b.value = 42;
            return b.value;
        }
    )");
    ASSERT_EQ(42, r.int_val);
    PASS();
    
    TEST("class method");
    r = execute_tick(R"(
        class Counter {
            var count: int;
            func get(): int {
                return count;
            }
            func set(val: int): int {
                count = val;
                return count;
            }
        }
        func main(): int {
            var c: Counter = new Counter();
            c.set(100);
            return c.get();
        }
    )");
    ASSERT_EQ(100, r.int_val);
    PASS();
    
    TEST("class method modifying fields");
    r = execute_tick(R"(
        class Adder {
            var total: int;
            func add(x: int): int {
                total = total + x;
                return total;
            }
        }
        func main(): int {
            var a: Adder = new Adder();
            a.total = 0;
            a.add(10);
            a.add(20);
            a.add(30);
            return a.total;
        }
    )");
    ASSERT_EQ(60, r.int_val);
    PASS();
}

void test_signals() {
    TEST("signal emit and recv");
    Value r = execute_tick(R"(
        signal<int> s;
        func main(): int {
            s.emit(99);
            return s.recv();
        }
    )");
    ASSERT_EQ(99, r.int_val);
    PASS();
    
    TEST("signal multiple emit/recv");
    r = execute_tick(R"(
        signal<int> s;
        func main(): int {
            s.emit(10);
            s.emit(20);
            var a: int = s.recv();
            var b: int = s.recv();
            return a + b;
        }
    )");
    ASSERT_EQ(30, r.int_val);
    PASS();
    
    TEST("signal reuse");
    r = execute_tick(R"(
        signal<int> s;
        func main(): int {
            s.emit(5);
            var a: int = s.recv();
            s.emit(10);
            var b: int = s.recv();
            s.emit(15);
            var c: int = s.recv();
            return a + b + c;
        }
    )");
    ASSERT_EQ(30, r.int_val);
    PASS();
}

void test_parallel() {
    TEST("parallel process");
    Value r = execute_tick(R"(
        event task;
        signal<int> result;
        
        @task process worker {
            var sum: int = 0;
            var i: int = 1;
            while (i <= 10) {
                sum = sum + i;
                i = i + 1;
            }
            result.emit(sum);
        }
        
        func main(): int {
            task.execute();
            return result.recv();
        }
    )");
    ASSERT_EQ(55, r.int_val);
    PASS();
    
    TEST("multiple parallel workers");
    r = execute_tick(R"(
        event task;
        signal<int> r1;
        signal<int> r2;
        signal<int> r3;
        
        @task process w1 {
            r1.emit(10);
        }
        
        @task process w2 {
            r2.emit(20);
        }
        
        @task process w3 {
            r3.emit(30);
        }
        
        func main(): int {
            task.execute();
            var a: int = r1.recv();
            var b: int = r2.recv();
            var c: int = r3.recv();
            return a + b + c;
        }
    )");
    ASSERT_EQ(60, r.int_val);
    PASS();
}

int main() {
    printf("╔════════════════════════════════════════════════════════════════╗\n");
    printf("║                   Tick Language Test Suite                     ║\n");
    printf("╚════════════════════════════════════════════════════════════════╝\n\n");
    
    printf("Arithmetic Operations\n");
    test_arithmetic();
    
    printf("\nVariables\n");
    test_variables();
    
    printf("\nControl Flow\n");
    test_control_flow();
    
    printf("\nComparison Operators\n");
    test_comparisons();
    
    printf("\nFunctions\n");
    test_functions();
    
    printf("\nClasses\n");
    test_classes();
    
    printf("\nSignals\n");
    test_signals();
    
    printf("\nParallel Execution\n");
    test_parallel();
    
    printf("\n╔════════════════════════════════════════════════════════════════╗\n");
    printf("║  Results: %d passed, %d failed                                    \n", tests_passed, tests_failed);
    printf("╚════════════════════════════════════════════════════════════════╝\n");
    
    return tests_failed > 0 ? 1 : 0;
}
