#include "runtime/compiler.h"
#include <cstdio>
#include <cstring>

int main(int argc, char** argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: tick <source.tick> [-o output] [--keep-c]\n");
        return 1;
    }
    
    const char* source_file = argv[1];
    const char* output_file = "a.out";
    bool keep_c = false;
    
    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
            output_file = argv[i + 1];
            i++;
        } else if (strcmp(argv[i], "--keep-c") == 0) {
            keep_c = true;
        }
    }
    
    printf("Compiling %s -> %s\n", source_file, output_file);
    
    if (!Compiler::compile_to_native(source_file, output_file, keep_c)) {
        fprintf(stderr, "Compilation failed\n");
        return 1;
    }
    
    printf("Success\n");
    return 0;
}
