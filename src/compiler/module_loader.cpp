#include "module_loader.h"
#include "lexer.h"
#include "parser.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sys/stat.h>

namespace Tick {

ModuleLoader::ModuleLoader() {}

ModuleLoader::~ModuleLoader() {
    _loaded_modules.for_each([](const char* name, Program* program) {
        delete program;
    });
}

String ModuleLoader::resolve_module_path(const char* module_name, const char* current_file_path) {
    const char* last_slash = nullptr;
    const char* ptr = current_file_path;
    while (*ptr) {
        if (*ptr == '/') last_slash = ptr;
        ptr++;
    }
    
    String dir;
    if (last_slash) {
        size_t dir_len = last_slash - current_file_path;
        char* dir_buf = new char[dir_len + 1];
        memcpy(dir_buf, current_file_path, dir_len);
        dir_buf[dir_len] = '\0';
        dir = String(dir_buf);
        delete[] dir_buf;
    } else {
        dir = String(".");
    }
    
    char path_buf[1024];
    snprintf(path_buf, sizeof(path_buf), "%s/%s.tick", dir.c_str(), module_name);
    
    struct stat st;
    if (stat(path_buf, &st) == 0) {
        return String(path_buf);
    }
    
    snprintf(path_buf, sizeof(path_buf), "./%s.tick", module_name);
    if (stat(path_buf, &st) == 0) {
        return String(path_buf);
    }
    
    const char* tick_path = getenv("TICK_PATH");
    if (tick_path) {
        snprintf(path_buf, sizeof(path_buf), "%s/%s.tick", tick_path, module_name);
        if (stat(path_buf, &st) == 0) {
            return String(path_buf);
        }
    }
    
    return String("");
}

char* ModuleLoader::read_file(const char* path) {
    FILE* file = fopen(path, "rb");
    if (!file) {
        return nullptr;
    }
    
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    char* buffer = (char*)malloc(file_size + 1);
    fread(buffer, 1, file_size, file);
    buffer[file_size] = '\0';
    
    fclose(file);
    return buffer;
}

Program* ModuleLoader::parse_module(const char* path) {
    char* source = read_file(path);
    if (!source) {
        fprintf(stderr, "Error: Could not read module file '%s'\n", path);
        return nullptr;
    }
    
    Lexer lexer(source);
    DynamicArray<Token> tokens = lexer.tokenize();
    
    Parser parser(tokens);
    Program* program = parser.parse();
    
    free(source);
    return program;
}

Program* ModuleLoader::load_module(const char* module_name, const char* current_file_path) {
    if (is_module_loaded(module_name)) {
        return get_loaded_module(module_name);
    }
    
    String module_path = resolve_module_path(module_name, current_file_path);
    if (module_path.length() == 0) {
        fprintf(stderr, "Error: Could not find module '%s'\n", module_name);
        return nullptr;
    }
    
    Program* program = parse_module(module_path.c_str());
    if (!program) {
        return nullptr;
    }
    
    char* name_copy = (char*)malloc(strlen(module_name) + 1);
    strcpy(name_copy, module_name);
    _loaded_modules.insert(name_copy, program);
    
    return program;
}

bool ModuleLoader::is_module_loaded(const char* module_name) {
    return _loaded_modules.contains(module_name);
}

Program* ModuleLoader::get_loaded_module(const char* module_name) {
    Program** program_ptr = _loaded_modules.find(module_name);
    if (program_ptr) {
        return *program_ptr;
    }
    return nullptr;
}

}
