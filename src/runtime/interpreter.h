#ifndef TICK_INTERPRETER_H
#define TICK_INTERPRETER_H

#include "bytecode.h"
#include "runtime.h"
#include "string_pool.h"
#include "../core/dynamic_array.h"

namespace Tick {

class Interpreter {
public:
    Interpreter(Runtime* runtime, StringPool* string_pool);
    
    Value execute(DynamicArray<Instruction>* code, DynamicArray<Value>* constants);

private:
    Runtime* _runtime;
    StringPool* _string_pool;
    DynamicArray<Value> _stack;
    DynamicArray<Value> _locals;
    DynamicArray<Value>* _constants;
    
    void push(Value value);
    Value pop();
    Value peek();
};

}

#endif
