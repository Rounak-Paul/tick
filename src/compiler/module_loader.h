#ifndef TICK_MODULE_LOADER_H
#define TICK_MODULE_LOADER_H

#include "ast.h"
#include "../core/string.h"
#include "../core/hash_map.h"
#include "../core/dynamic_array.h"

namespace Tick {

class ModuleLoader {
public:
    ModuleLoader();
    ~ModuleLoader();
    
    Program* load_module(const char* module_name, const char* current_file_path);
    bool is_module_loaded(const char* module_name);
    Program* get_loaded_module(const char* module_name);
    
private:
    HashMap<const char*, Program*> _loaded_modules;
    
    String resolve_module_path(const char* module_name, const char* current_file_path);
    char* read_file(const char* path);
    Program* parse_module(const char* path);
};

}

#endif
