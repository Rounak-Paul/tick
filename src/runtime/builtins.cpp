#include "builtins.h"
#include "string_pool.h"
#include <cstdio>
#include <cstring>

namespace Tick {

Runtime* Builtins::_runtime = nullptr;

void Builtins::set_runtime(Runtime* runtime) {
    _runtime = runtime;
}

void Builtins::register_all(Runtime* runtime) {
    _runtime = runtime;
    runtime->register_function("print", builtin_print);
    runtime->register_function("input", builtin_input);
    runtime->register_function("format", builtin_format);
    runtime->register_function("str_to_int", builtin_str_to_int);
    runtime->register_function("str_to_float", builtin_str_to_float);
    runtime->register_function("str_to_double", builtin_str_to_double);
}

Value Builtins::builtin_print(DynamicArray<Value>& args) {
    StringPool* pool = _runtime ? _runtime->get_string_pool() : nullptr;
    
    for (size_t i = 0; i < args.size(); i++) {
        Value val = args[i];
        
        if (val.type == Value::INT) {
            printf("%d", val.int_val);
        } else if (val.type == Value::BOOL) {
            printf("%s", val.bool_val ? "true" : "false");
        } else if (val.type == Value::STRING && pool) {
            printf("%s", pool->get(val.string_id));
        }
        
        if (i < args.size() - 1) {
            printf(" ");
        }
    }
    
    fflush(stdout);
    return Value(0);
}

Value Builtins::builtin_input(DynamicArray<Value>& args) {
    StringPool* pool = _runtime ? _runtime->get_string_pool() : nullptr;
    if (!pool) return Value(0, true);
    
    if (args.size() > 0 && args[0].type == Value::STRING) {
        const char* prompt = pool->get(args[0].string_id);
        printf("%s", prompt);
        fflush(stdout);
    }
    
    char buffer[1024];
    if (fgets(buffer, sizeof(buffer), stdin)) {
        size_t len = strlen(buffer);
        if (len > 0 && buffer[len - 1] == '\n') {
            buffer[len - 1] = '\0';
        }
        int string_id = pool->add(String(buffer));
        return Value(string_id, true);
    }
    
    return Value(pool->add(String("")), true);
}

Value Builtins::builtin_format(DynamicArray<Value>& args) {
    if (args.size() == 0) return Value(0, true);
    
    StringPool* pool = _runtime ? _runtime->get_string_pool() : nullptr;
    if (!pool) return Value(0, true);
    
    const char* format_str = pool->get(args[0].string_id);
    char* result = (char*)malloc(1024);
    size_t result_pos = 0;
    
    size_t arg_index = 1;
    for (size_t i = 0; format_str[i] != '\0' && result_pos < 1023; i++) {
        if (format_str[i] == '{' && format_str[i+1] == '}' && arg_index < args.size()) {
            Value arg = args[arg_index++];
            char temp[64];
            if (arg.type == Value::INT) {
                snprintf(temp, sizeof(temp), "%d", arg.int_val);
                size_t len = strlen(temp);
                if (result_pos + len < 1023) {
                    memcpy(result + result_pos, temp, len);
                    result_pos += len;
                }
            } else if (arg.type == Value::FLOAT) {
                snprintf(temp, sizeof(temp), "%g", arg.float_val);
                size_t len = strlen(temp);
                if (result_pos + len < 1023) {
                    memcpy(result + result_pos, temp, len);
                    result_pos += len;
                }
            } else if (arg.type == Value::DOUBLE) {
                snprintf(temp, sizeof(temp), "%g", arg.double_val);
                size_t len = strlen(temp);
                if (result_pos + len < 1023) {
                    memcpy(result + result_pos, temp, len);
                    result_pos += len;
                }
            } else if (arg.type == Value::BOOL) {
                const char* bool_str = arg.bool_val ? "true" : "false";
                size_t len = strlen(bool_str);
                if (result_pos + len < 1023) {
                    memcpy(result + result_pos, bool_str, len);
                    result_pos += len;
                }
            } else if (arg.type == Value::STRING) {
                const char* str = pool->get(arg.string_id);
                size_t len = strlen(str);
                if (result_pos + len < 1023) {
                    memcpy(result + result_pos, str, len);
                    result_pos += len;
                }
            }
            i++;
        } else {
            result[result_pos++] = format_str[i];
        }
    }
    result[result_pos] = '\0';
    
    int result_id = pool->add(String(result));
    free(result);
    return Value(result_id, true);
}

Value Builtins::builtin_str_to_int(DynamicArray<Value>& args) {
    if (args.size() == 0 || args[0].type != Value::STRING) {
        return Value(0);
    }
    
    StringPool* pool = _runtime ? _runtime->get_string_pool() : nullptr;
    if (!pool) return Value(0);
    
    const char* str = pool->get(args[0].string_id);
    int value = 0;
    sscanf(str, "%d", &value);
    return Value(value);
}

Value Builtins::builtin_str_to_float(DynamicArray<Value>& args) {
    if (args.size() == 0 || args[0].type != Value::STRING) {
        return Value(0.0f);
    }
    
    StringPool* pool = _runtime ? _runtime->get_string_pool() : nullptr;
    if (!pool) return Value(0.0f);
    
    const char* str = pool->get(args[0].string_id);
    float value = 0.0f;
    sscanf(str, "%f", &value);
    return Value(value);
}

Value Builtins::builtin_str_to_double(DynamicArray<Value>& args) {
    if (args.size() == 0 || args[0].type != Value::STRING) {
        return Value(0.0);
    }
    
    StringPool* pool = _runtime ? _runtime->get_string_pool() : nullptr;
    if (!pool) return Value(0.0);
    
    const char* str = pool->get(args[0].string_id);
    double value = 0.0;
    sscanf(str, "%lf", &value);
    return Value(value);
}

}
