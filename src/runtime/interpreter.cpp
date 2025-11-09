#include "interpreter.h"
#include <cstring>
#include <cstdio>

namespace Tick {

Interpreter::Interpreter(Runtime* runtime, StringPool* string_pool) 
    : _runtime(runtime), _string_pool(string_pool), _constants(nullptr) {}

Value Interpreter::execute(DynamicArray<Instruction>* code, DynamicArray<Value>* constants) {
    _constants = constants;
    _locals.clear();
    _stack.clear();
    
    for (int i = 0; i < 256; i++) {
        _locals.push(Value(0));
    }
    
    size_t pc = 0;
    
    while (pc < code->size()) {
        Instruction inst = (*code)[pc];
        
        switch (inst.opcode) {
            case OpCode::LOAD_CONST:
                push((*_constants)[inst.operand]);
                break;
                
            case OpCode::LOAD_VAR:
                push(_locals[inst.operand]);
                break;
                
            case OpCode::STORE_VAR:
                _locals[inst.operand] = pop();
                break;
                
            case OpCode::LOAD_GLOBAL: {
                const char* name = _string_pool->get(inst.operand);
                Value val = _runtime->get_global(name);
                push(val);
                break;
            }
            
            case OpCode::STORE_GLOBAL: {
                const char* name = _string_pool->get(inst.operand);
                _runtime->set_global(name, pop());
                break;
            }
                
            case OpCode::ADD: {
                Value b = pop();
                Value a = pop();
                if (a.type == Value::STRING && b.type == Value::STRING) {
                    const char* str_a = _string_pool->get(a.string_id);
                    const char* str_b = _string_pool->get(b.string_id);
                    size_t len_a = strlen(str_a);
                    size_t len_b = strlen(str_b);
                    char* result = (char*)malloc(len_a + len_b + 1);
                    memcpy(result, str_a, len_a);
                    memcpy(result + len_a, str_b, len_b);
                    result[len_a + len_b] = '\0';
                    int result_id = _string_pool->add(String(result));
                    free(result);
                    push(Value(result_id, true));
                } else if (a.type == Value::DOUBLE || b.type == Value::DOUBLE) {
                    double val_a = (a.type == Value::DOUBLE) ? a.double_val : 
                                   (a.type == Value::FLOAT) ? a.float_val : a.int_val;
                    double val_b = (b.type == Value::DOUBLE) ? b.double_val : 
                                   (b.type == Value::FLOAT) ? b.float_val : b.int_val;
                    push(Value(val_a + val_b));
                } else if (a.type == Value::FLOAT || b.type == Value::FLOAT) {
                    float val_a = (a.type == Value::FLOAT) ? a.float_val : a.int_val;
                    float val_b = (b.type == Value::FLOAT) ? b.float_val : b.int_val;
                    push(Value(val_a + val_b));
                } else {
                    push(Value(a.int_val + b.int_val));
                }
                break;
            }
            
            case OpCode::SUB: {
                Value b = pop();
                Value a = pop();
                if (a.type == Value::DOUBLE || b.type == Value::DOUBLE) {
                    double val_a = (a.type == Value::DOUBLE) ? a.double_val : 
                                   (a.type == Value::FLOAT) ? a.float_val : a.int_val;
                    double val_b = (b.type == Value::DOUBLE) ? b.double_val : 
                                   (b.type == Value::FLOAT) ? b.float_val : b.int_val;
                    push(Value(val_a - val_b));
                } else if (a.type == Value::FLOAT || b.type == Value::FLOAT) {
                    float val_a = (a.type == Value::FLOAT) ? a.float_val : a.int_val;
                    float val_b = (b.type == Value::FLOAT) ? b.float_val : b.int_val;
                    push(Value(val_a - val_b));
                } else {
                    push(Value(a.int_val - b.int_val));
                }
                break;
            }
            
            case OpCode::MUL: {
                Value b = pop();
                Value a = pop();
                if (a.type == Value::DOUBLE || b.type == Value::DOUBLE) {
                    double val_a = (a.type == Value::DOUBLE) ? a.double_val : 
                                   (a.type == Value::FLOAT) ? a.float_val : a.int_val;
                    double val_b = (b.type == Value::DOUBLE) ? b.double_val : 
                                   (b.type == Value::FLOAT) ? b.float_val : b.int_val;
                    push(Value(val_a * val_b));
                } else if (a.type == Value::FLOAT || b.type == Value::FLOAT) {
                    float val_a = (a.type == Value::FLOAT) ? a.float_val : a.int_val;
                    float val_b = (b.type == Value::FLOAT) ? b.float_val : b.int_val;
                    push(Value(val_a * val_b));
                } else {
                    push(Value(a.int_val * b.int_val));
                }
                break;
            }
            
            case OpCode::DIV: {
                Value b = pop();
                Value a = pop();
                if (a.type == Value::DOUBLE || b.type == Value::DOUBLE) {
                    double val_a = (a.type == Value::DOUBLE) ? a.double_val : 
                                   (a.type == Value::FLOAT) ? a.float_val : a.int_val;
                    double val_b = (b.type == Value::DOUBLE) ? b.double_val : 
                                   (b.type == Value::FLOAT) ? b.float_val : b.int_val;
                    push(Value(val_a / val_b));
                } else if (a.type == Value::FLOAT || b.type == Value::FLOAT) {
                    float val_a = (a.type == Value::FLOAT) ? a.float_val : a.int_val;
                    float val_b = (b.type == Value::FLOAT) ? b.float_val : b.int_val;
                    push(Value(val_a / val_b));
                } else {
                    push(Value(a.int_val / b.int_val));
                }
                break;
            }
            
            case OpCode::MOD: {
                Value b = pop();
                Value a = pop();
                push(Value(a.int_val % b.int_val));
                break;
            }
            
            case OpCode::CONCAT: {
                Value b = pop();
                Value a = pop();
                const char* str_a = _string_pool->get(a.string_id);
                const char* str_b = _string_pool->get(b.string_id);
                size_t len_a = strlen(str_a);
                size_t len_b = strlen(str_b);
                char* result = (char*)malloc(len_a + len_b + 1);
                memcpy(result, str_a, len_a);
                memcpy(result + len_a, str_b, len_b);
                result[len_a + len_b] = '\0';
                int result_id = _string_pool->add(String(result));
                free(result);
                push(Value(result_id, true));
                break;
            }
            
            case OpCode::EQ: {
                Value b = pop();
                Value a = pop();
                if (a.type == Value::DOUBLE || b.type == Value::DOUBLE) {
                    double val_a = (a.type == Value::DOUBLE) ? a.double_val : 
                                   (a.type == Value::FLOAT) ? a.float_val : a.int_val;
                    double val_b = (b.type == Value::DOUBLE) ? b.double_val : 
                                   (b.type == Value::FLOAT) ? b.float_val : b.int_val;
                    push(Value(val_a == val_b));
                } else if (a.type == Value::FLOAT || b.type == Value::FLOAT) {
                    float val_a = (a.type == Value::FLOAT) ? a.float_val : a.int_val;
                    float val_b = (b.type == Value::FLOAT) ? b.float_val : b.int_val;
                    push(Value(val_a == val_b));
                } else {
                    push(Value(a.int_val == b.int_val));
                }
                break;
            }
            
            case OpCode::NEQ: {
                Value b = pop();
                Value a = pop();
                if (a.type == Value::DOUBLE || b.type == Value::DOUBLE) {
                    double val_a = (a.type == Value::DOUBLE) ? a.double_val : 
                                   (a.type == Value::FLOAT) ? a.float_val : a.int_val;
                    double val_b = (b.type == Value::DOUBLE) ? b.double_val : 
                                   (b.type == Value::FLOAT) ? b.float_val : b.int_val;
                    push(Value(val_a != val_b));
                } else if (a.type == Value::FLOAT || b.type == Value::FLOAT) {
                    float val_a = (a.type == Value::FLOAT) ? a.float_val : a.int_val;
                    float val_b = (b.type == Value::FLOAT) ? b.float_val : b.int_val;
                    push(Value(val_a != val_b));
                } else {
                    push(Value(a.int_val != b.int_val));
                }
                break;
            }
            
            case OpCode::LT: {
                Value b = pop();
                Value a = pop();
                if (a.type == Value::DOUBLE || b.type == Value::DOUBLE) {
                    double val_a = (a.type == Value::DOUBLE) ? a.double_val : 
                                   (a.type == Value::FLOAT) ? a.float_val : a.int_val;
                    double val_b = (b.type == Value::DOUBLE) ? b.double_val : 
                                   (b.type == Value::FLOAT) ? b.float_val : b.int_val;
                    push(Value(val_a < val_b));
                } else if (a.type == Value::FLOAT || b.type == Value::FLOAT) {
                    float val_a = (a.type == Value::FLOAT) ? a.float_val : a.int_val;
                    float val_b = (b.type == Value::FLOAT) ? b.float_val : b.int_val;
                    push(Value(val_a < val_b));
                } else {
                    push(Value(a.int_val < b.int_val));
                }
                break;
            }
            
            case OpCode::GT: {
                Value b = pop();
                Value a = pop();
                if (a.type == Value::DOUBLE || b.type == Value::DOUBLE) {
                    double val_a = (a.type == Value::DOUBLE) ? a.double_val : 
                                   (a.type == Value::FLOAT) ? a.float_val : a.int_val;
                    double val_b = (b.type == Value::DOUBLE) ? b.double_val : 
                                   (b.type == Value::FLOAT) ? b.float_val : b.int_val;
                    push(Value(val_a > val_b));
                } else if (a.type == Value::FLOAT || b.type == Value::FLOAT) {
                    float val_a = (a.type == Value::FLOAT) ? a.float_val : a.int_val;
                    float val_b = (b.type == Value::FLOAT) ? b.float_val : b.int_val;
                    push(Value(val_a > val_b));
                } else {
                    push(Value(a.int_val > b.int_val));
                }
                break;
            }
            
            case OpCode::LTE: {
                Value b = pop();
                Value a = pop();
                if (a.type == Value::DOUBLE || b.type == Value::DOUBLE) {
                    double val_a = (a.type == Value::DOUBLE) ? a.double_val : 
                                   (a.type == Value::FLOAT) ? a.float_val : a.int_val;
                    double val_b = (b.type == Value::DOUBLE) ? b.double_val : 
                                   (b.type == Value::FLOAT) ? b.float_val : b.int_val;
                    push(Value(val_a <= val_b));
                } else if (a.type == Value::FLOAT || b.type == Value::FLOAT) {
                    float val_a = (a.type == Value::FLOAT) ? a.float_val : a.int_val;
                    float val_b = (b.type == Value::FLOAT) ? b.float_val : b.int_val;
                    push(Value(val_a <= val_b));
                } else {
                    push(Value(a.int_val <= b.int_val));
                }
                break;
            }
            
            case OpCode::GTE: {
                Value b = pop();
                Value a = pop();
                if (a.type == Value::DOUBLE || b.type == Value::DOUBLE) {
                    double val_a = (a.type == Value::DOUBLE) ? a.double_val : 
                                   (a.type == Value::FLOAT) ? a.float_val : a.int_val;
                    double val_b = (b.type == Value::DOUBLE) ? b.double_val : 
                                   (b.type == Value::FLOAT) ? b.float_val : b.int_val;
                    push(Value(val_a >= val_b));
                } else if (a.type == Value::FLOAT || b.type == Value::FLOAT) {
                    float val_a = (a.type == Value::FLOAT) ? a.float_val : a.int_val;
                    float val_b = (b.type == Value::FLOAT) ? b.float_val : b.int_val;
                    push(Value(val_a >= val_b));
                } else {
                    push(Value(a.int_val >= b.int_val));
                }
                break;
            }
            
            case OpCode::AND: {
                Value b = pop();
                Value a = pop();
                push(Value(a.bool_val && b.bool_val));
                break;
            }
            
            case OpCode::OR: {
                Value b = pop();
                Value a = pop();
                push(Value(a.bool_val || b.bool_val));
                break;
            }
            
            case OpCode::NOT: {
                Value a = pop();
                push(Value(!a.bool_val));
                break;
            }
            
            case OpCode::NEG: {
                Value a = pop();
                if (a.type == Value::DOUBLE) {
                    push(Value(-a.double_val));
                } else if (a.type == Value::FLOAT) {
                    push(Value(-a.float_val));
                } else {
                    push(Value(-a.int_val));
                }
                break;
            }
            
            case OpCode::JUMP:
                pc = inst.operand;
                continue;
                
            case OpCode::JUMP_IF_FALSE: {
                Value cond = pop();
                if (!cond.bool_val && cond.type == Value::BOOL) {
                    pc = inst.operand;
                    continue;
                }
                if (cond.int_val == 0 && cond.type == Value::INT) {
                    pc = inst.operand;
                    continue;
                }
                break;
            }
            
            case OpCode::JUMP_IF_TRUE: {
                Value cond = pop();
                if (cond.bool_val && cond.type == Value::BOOL) {
                    pc = inst.operand;
                    continue;
                }
                if (cond.int_val != 0 && cond.type == Value::INT) {
                    pc = inst.operand;
                    continue;
                }
                break;
            }
            
            case OpCode::CALL: {
                Value arg_count_val = pop();
                int arg_count = arg_count_val.int_val;
                
                DynamicArray<Value> args;
                for (int i = 0; i < arg_count; i++) {
                    args.push(Value());
                }
                for (int i = arg_count - 1; i >= 0; i--) {
                    args[i] = pop();
                }
                
                const char* func_name = _string_pool->get(inst.operand);
                Value result = _runtime->call_function(func_name, args);
                push(result);
                break;
            }
            
            case OpCode::RETURN:
                return pop();
                
            case OpCode::SIGNAL_EMIT: {
                const char* signal_name = _string_pool->get(inst.operand);
                SignalQueue* queue = _runtime->get_signal(signal_name);
                if (queue) {
                    Value val = pop();
                    queue->emit(val);
                }
                break;
            }
            
            case OpCode::SIGNAL_RECV: {
                const char* signal_name = _string_pool->get(inst.operand);
                SignalQueue* queue = _runtime->get_signal(signal_name);
                if (queue) {
                    Value val = queue->recv();
                    push(val);
                } else {
                    push(Value(0));
                }
                break;
            }
            
            case OpCode::EVENT_EXECUTE: {
                const char* event_name = _string_pool->get(inst.operand);
                _runtime->execute_event(event_name);
                _runtime->wait_event_completion();
                break;
            }
                
            case OpCode::POP:
                pop();
                break;
                
            case OpCode::DUP:
                push(peek());
                break;
                
            case OpCode::BUILD_ARRAY: {
                int size = inst.operand;
                DynamicArray<Value>* arr = new DynamicArray<Value>();
                for (int i = 0; i < size; i++) {
                    arr->push(pop());
                }
                for (int i = 0; i < size / 2; i++) {
                    Value temp = (*arr)[i];
                    (*arr)[i] = (*arr)[size - 1 - i];
                    (*arr)[size - 1 - i] = temp;
                }
                push(Value((void*)arr, false));
                break;
            }
            
            case OpCode::ARRAY_INDEX: {
                Value index = pop();
                Value array = pop();
                DynamicArray<Value>* arr = (DynamicArray<Value>*)array.array_ptr;
                push((*arr)[index.int_val]);
                break;
            }
            
            case OpCode::ARRAY_STORE: {
                Value value = pop();
                Value index = pop();
                Value array = pop();
                DynamicArray<Value>* arr = (DynamicArray<Value>*)array.array_ptr;
                (*arr)[index.int_val] = value;
                break;
            }
                
            case OpCode::HALT:
                return Value(0);
        }
        
        pc++;
    }
    
    return Value(0);
}

Value Interpreter::execute_function(DynamicArray<Instruction>* code, DynamicArray<Value>* constants, DynamicArray<Value>& args) {
    _constants = constants;
    _locals.clear();
    _stack.clear();
    
    for (size_t i = 0; i < args.size() && i < 256; i++) {
        _locals.push(args[i]);
    }
    for (size_t i = args.size(); i < 256; i++) {
        _locals.push(Value(0));
    }
    
    size_t pc = 0;
    
    while (pc < code->size()) {
        Instruction inst = (*code)[pc];
        
        switch (inst.opcode) {
            case OpCode::LOAD_CONST:
                push((*_constants)[inst.operand]);
                break;
                
            case OpCode::LOAD_VAR:
                push(_locals[inst.operand]);
                break;
                
            case OpCode::STORE_VAR:
                _locals[inst.operand] = pop();
                break;
                
            case OpCode::LOAD_GLOBAL: {
                const char* name = _string_pool->get(inst.operand);
                push(_runtime->get_global(name));
                break;
            }
                
            case OpCode::STORE_GLOBAL: {
                const char* name = _string_pool->get(inst.operand);
                _runtime->set_global(name, pop());
                break;
            }
                
            case OpCode::ADD: {
                Value b = pop();
                Value a = pop();
                if (a.type == Value::STRING && b.type == Value::STRING) {
                    const char* str_a = _string_pool->get(a.string_id);
                    const char* str_b = _string_pool->get(b.string_id);
                    size_t len_a = strlen(str_a);
                    size_t len_b = strlen(str_b);
                    char* result = (char*)malloc(len_a + len_b + 1);
                    memcpy(result, str_a, len_a);
                    memcpy(result + len_a, str_b, len_b);
                    result[len_a + len_b] = '\0';
                    int result_id = _string_pool->add(String(result));
                    free(result);
                    push(Value(result_id, true));
                } else if (a.type == Value::DOUBLE || b.type == Value::DOUBLE) {
                    double val_a = (a.type == Value::DOUBLE) ? a.double_val : 
                                   (a.type == Value::FLOAT) ? a.float_val : a.int_val;
                    double val_b = (b.type == Value::DOUBLE) ? b.double_val : 
                                   (b.type == Value::FLOAT) ? b.float_val : b.int_val;
                    push(Value(val_a + val_b));
                } else if (a.type == Value::FLOAT || b.type == Value::FLOAT) {
                    float val_a = (a.type == Value::FLOAT) ? a.float_val : a.int_val;
                    float val_b = (b.type == Value::FLOAT) ? b.float_val : b.int_val;
                    push(Value(val_a + val_b));
                } else {
                    push(Value(a.int_val + b.int_val));
                }
                break;
            }
                
            case OpCode::SUB: {
                Value b = pop();
                Value a = pop();
                if (a.type == Value::DOUBLE || b.type == Value::DOUBLE) {
                    double val_a = (a.type == Value::DOUBLE) ? a.double_val : 
                                   (a.type == Value::FLOAT) ? a.float_val : a.int_val;
                    double val_b = (b.type == Value::DOUBLE) ? b.double_val : 
                                   (b.type == Value::FLOAT) ? b.float_val : b.int_val;
                    push(Value(val_a - val_b));
                } else if (a.type == Value::FLOAT || b.type == Value::FLOAT) {
                    float val_a = (a.type == Value::FLOAT) ? a.float_val : a.int_val;
                    float val_b = (b.type == Value::FLOAT) ? b.float_val : b.int_val;
                    push(Value(val_a - val_b));
                } else {
                    push(Value(a.int_val - b.int_val));
                }
                break;
            }
                
            case OpCode::MUL: {
                Value b = pop();
                Value a = pop();
                if (a.type == Value::DOUBLE || b.type == Value::DOUBLE) {
                    double val_a = (a.type == Value::DOUBLE) ? a.double_val : 
                                   (a.type == Value::FLOAT) ? a.float_val : a.int_val;
                    double val_b = (b.type == Value::DOUBLE) ? b.double_val : 
                                   (b.type == Value::FLOAT) ? b.float_val : b.int_val;
                    push(Value(val_a * val_b));
                } else if (a.type == Value::FLOAT || b.type == Value::FLOAT) {
                    float val_a = (a.type == Value::FLOAT) ? a.float_val : a.int_val;
                    float val_b = (b.type == Value::FLOAT) ? b.float_val : b.int_val;
                    push(Value(val_a * val_b));
                } else {
                    push(Value(a.int_val * b.int_val));
                }
                break;
            }
                
            case OpCode::DIV: {
                Value b = pop();
                Value a = pop();
                if (a.type == Value::DOUBLE || b.type == Value::DOUBLE) {
                    double val_a = (a.type == Value::DOUBLE) ? a.double_val : 
                                   (a.type == Value::FLOAT) ? a.float_val : a.int_val;
                    double val_b = (b.type == Value::DOUBLE) ? b.double_val : 
                                   (b.type == Value::FLOAT) ? b.float_val : b.int_val;
                    push(Value(val_a / val_b));
                } else if (a.type == Value::FLOAT || b.type == Value::FLOAT) {
                    float val_a = (a.type == Value::FLOAT) ? a.float_val : a.int_val;
                    float val_b = (b.type == Value::FLOAT) ? b.float_val : b.int_val;
                    push(Value(val_a / val_b));
                } else {
                    push(Value(a.int_val / b.int_val));
                }
                break;
            }
                
            case OpCode::MOD: {
                Value b = pop();
                Value a = pop();
                push(Value(a.int_val % b.int_val));
                break;
            }
                
            case OpCode::EQ: {
                Value b = pop();
                Value a = pop();
                if (a.type == Value::DOUBLE || b.type == Value::DOUBLE) {
                    double val_a = (a.type == Value::DOUBLE) ? a.double_val : 
                                   (a.type == Value::FLOAT) ? a.float_val : a.int_val;
                    double val_b = (b.type == Value::DOUBLE) ? b.double_val : 
                                   (b.type == Value::FLOAT) ? b.float_val : b.int_val;
                    push(Value(val_a == val_b));
                } else if (a.type == Value::FLOAT || b.type == Value::FLOAT) {
                    float val_a = (a.type == Value::FLOAT) ? a.float_val : a.int_val;
                    float val_b = (b.type == Value::FLOAT) ? b.float_val : b.int_val;
                    push(Value(val_a == val_b));
                } else {
                    push(Value(a.int_val == b.int_val));
                }
                break;
            }
                
            case OpCode::NEQ: {
                Value b = pop();
                Value a = pop();
                if (a.type == Value::DOUBLE || b.type == Value::DOUBLE) {
                    double val_a = (a.type == Value::DOUBLE) ? a.double_val : 
                                   (a.type == Value::FLOAT) ? a.float_val : a.int_val;
                    double val_b = (b.type == Value::DOUBLE) ? b.double_val : 
                                   (b.type == Value::FLOAT) ? b.float_val : b.int_val;
                    push(Value(val_a != val_b));
                } else if (a.type == Value::FLOAT || b.type == Value::FLOAT) {
                    float val_a = (a.type == Value::FLOAT) ? a.float_val : a.int_val;
                    float val_b = (b.type == Value::FLOAT) ? b.float_val : b.int_val;
                    push(Value(val_a != val_b));
                } else {
                    push(Value(a.int_val != b.int_val));
                }
                break;
            }
                
            case OpCode::LT: {
                Value b = pop();
                Value a = pop();
                if (a.type == Value::DOUBLE || b.type == Value::DOUBLE) {
                    double val_a = (a.type == Value::DOUBLE) ? a.double_val : 
                                   (a.type == Value::FLOAT) ? a.float_val : a.int_val;
                    double val_b = (b.type == Value::DOUBLE) ? b.double_val : 
                                   (b.type == Value::FLOAT) ? b.float_val : b.int_val;
                    push(Value(val_a < val_b));
                } else if (a.type == Value::FLOAT || b.type == Value::FLOAT) {
                    float val_a = (a.type == Value::FLOAT) ? a.float_val : a.int_val;
                    float val_b = (b.type == Value::FLOAT) ? b.float_val : b.int_val;
                    push(Value(val_a < val_b));
                } else {
                    push(Value(a.int_val < b.int_val));
                }
                break;
            }
                
            case OpCode::GT: {
                Value b = pop();
                Value a = pop();
                if (a.type == Value::DOUBLE || b.type == Value::DOUBLE) {
                    double val_a = (a.type == Value::DOUBLE) ? a.double_val : 
                                   (a.type == Value::FLOAT) ? a.float_val : a.int_val;
                    double val_b = (b.type == Value::DOUBLE) ? b.double_val : 
                                   (b.type == Value::FLOAT) ? b.float_val : b.int_val;
                    push(Value(val_a > val_b));
                } else if (a.type == Value::FLOAT || b.type == Value::FLOAT) {
                    float val_a = (a.type == Value::FLOAT) ? a.float_val : a.int_val;
                    float val_b = (b.type == Value::FLOAT) ? b.float_val : b.int_val;
                    push(Value(val_a > val_b));
                } else {
                    push(Value(a.int_val > b.int_val));
                }
                break;
            }
                
            case OpCode::LTE: {
                Value b = pop();
                Value a = pop();
                if (a.type == Value::DOUBLE || b.type == Value::DOUBLE) {
                    double val_a = (a.type == Value::DOUBLE) ? a.double_val : 
                                   (a.type == Value::FLOAT) ? a.float_val : a.int_val;
                    double val_b = (b.type == Value::DOUBLE) ? b.double_val : 
                                   (b.type == Value::FLOAT) ? b.float_val : b.int_val;
                    push(Value(val_a <= val_b));
                } else if (a.type == Value::FLOAT || b.type == Value::FLOAT) {
                    float val_a = (a.type == Value::FLOAT) ? a.float_val : a.int_val;
                    float val_b = (b.type == Value::FLOAT) ? b.float_val : b.int_val;
                    push(Value(val_a <= val_b));
                } else {
                    push(Value(a.int_val <= b.int_val));
                }
                break;
            }
                
            case OpCode::GTE: {
                Value b = pop();
                Value a = pop();
                if (a.type == Value::DOUBLE || b.type == Value::DOUBLE) {
                    double val_a = (a.type == Value::DOUBLE) ? a.double_val : 
                                   (a.type == Value::FLOAT) ? a.float_val : a.int_val;
                    double val_b = (b.type == Value::DOUBLE) ? b.double_val : 
                                   (b.type == Value::FLOAT) ? b.float_val : b.int_val;
                    push(Value(val_a >= val_b));
                } else if (a.type == Value::FLOAT || b.type == Value::FLOAT) {
                    float val_a = (a.type == Value::FLOAT) ? a.float_val : a.int_val;
                    float val_b = (b.type == Value::FLOAT) ? b.float_val : b.int_val;
                    push(Value(val_a >= val_b));
                } else {
                    push(Value(a.int_val >= b.int_val));
                }
                break;
            }
                
            case OpCode::AND: {
                Value b = pop();
                Value a = pop();
                push(Value(a.bool_val && b.bool_val));
                break;
            }
                
            case OpCode::OR: {
                Value b = pop();
                Value a = pop();
                push(Value(a.bool_val || b.bool_val));
                break;
            }
                
            case OpCode::NOT: {
                Value a = pop();
                push(Value(!a.bool_val));
                break;
            }
                
            case OpCode::NEG: {
                Value a = pop();
                if (a.type == Value::DOUBLE) {
                    push(Value(-a.double_val));
                } else if (a.type == Value::FLOAT) {
                    push(Value(-a.float_val));
                } else {
                    push(Value(-a.int_val));
                }
                break;
            }
                
            case OpCode::JUMP:
                pc = inst.operand;
                continue;
                
            case OpCode::JUMP_IF_FALSE: {
                Value cond = pop();
                if (!cond.bool_val) {
                    pc = inst.operand;
                    continue;
                }
                break;
            }
                
            case OpCode::JUMP_IF_TRUE: {
                Value cond = pop();
                if (cond.bool_val) {
                    pc = inst.operand;
                    continue;
                }
                break;
            }
                
            case OpCode::CALL: {
                Value arg_count_val = pop();
                int arg_count = arg_count_val.int_val;
                
                DynamicArray<Value> args;
                for (int i = 0; i < arg_count; i++) {
                    args.push(Value());
                }
                for (int i = arg_count - 1; i >= 0; i--) {
                    args[i] = pop();
                }
                
                const char* func_name = _string_pool->get(inst.operand);
                Value result = _runtime->call_function(func_name, args);
                push(result);
                break;
            }
            
            case OpCode::RETURN:
                return pop();
                
            case OpCode::SIGNAL_EMIT: {
                const char* signal_name = _string_pool->get(inst.operand);
                SignalQueue* queue = _runtime->get_signal(signal_name);
                if (queue) {
                    Value val = pop();
                    queue->emit(val);
                }
                break;
            }
            
            case OpCode::SIGNAL_RECV: {
                const char* signal_name = _string_pool->get(inst.operand);
                SignalQueue* queue = _runtime->get_signal(signal_name);
                if (queue) {
                    Value val = queue->recv();
                    push(val);
                } else {
                    push(Value(0));
                }
                break;
            }
            
            case OpCode::EVENT_EXECUTE: {
                const char* event_name = _string_pool->get(inst.operand);
                _runtime->execute_event(event_name);
                _runtime->wait_event_completion();
                break;
            }
                
            case OpCode::POP:
                pop();
                break;
                
            case OpCode::DUP:
                push(peek());
                break;
                
            case OpCode::BUILD_ARRAY: {
                int size = inst.operand;
                DynamicArray<Value>* arr = new DynamicArray<Value>();
                for (int i = 0; i < size; i++) {
                    arr->push(pop());
                }
                for (int i = 0; i < size / 2; i++) {
                    Value temp = (*arr)[i];
                    (*arr)[i] = (*arr)[size - 1 - i];
                    (*arr)[size - 1 - i] = temp;
                }
                push(Value((void*)arr, false));
                break;
            }
            
            case OpCode::ARRAY_INDEX: {
                Value index = pop();
                Value array = pop();
                DynamicArray<Value>* arr = (DynamicArray<Value>*)array.array_ptr;
                push((*arr)[index.int_val]);
                break;
            }
            
            case OpCode::ARRAY_STORE: {
                Value value = pop();
                Value index = pop();
                Value array = pop();
                DynamicArray<Value>* arr = (DynamicArray<Value>*)array.array_ptr;
                (*arr)[index.int_val] = value;
                break;
            }
                
            case OpCode::HALT:
                return Value(0);
        }
        
        pc++;
    }
    
    return Value(0);
}

void Interpreter::push(Value value) {
    _stack.push(value);
}

Value Interpreter::pop() {
    if (_stack.size() == 0) {
        return Value(0);
    }
    Value val = _stack[_stack.size() - 1];
    _stack.pop_back();
    return val;
}

Value Interpreter::peek() {
    if (_stack.size() == 0) {
        return Value(0);
    }
    return _stack[_stack.size() - 1];
}

}
