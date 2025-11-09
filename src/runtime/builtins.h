#ifndef TICK_BUILTINS_H
#define TICK_BUILTINS_H

#include "runtime.h"
#include "../core/dynamic_array.h"

namespace Tick {

class Builtins {
public:
    static void register_all(Runtime* runtime);
    static void set_runtime(Runtime* runtime);
    
private:
    static Runtime* _runtime;
    static Value builtin_print(DynamicArray<Value>& args);
    static Value builtin_println(DynamicArray<Value>& args);
    static Value builtin_input(DynamicArray<Value>& args);
    static Value builtin_format(DynamicArray<Value>& args);
};

}

#endif
