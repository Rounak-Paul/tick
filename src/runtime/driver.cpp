#include "driver.h"
#include "codegen.h"
#include "../compiler/lexer.h"
#include "../compiler/parser.h"
#include "../compiler/checker.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>

#ifdef __APPLE__
#include <mach-o/dyld.h>
#endif

namespace Tick {

static char* read_file(const char* path) {
    FILE* f = fopen(path, "rb");
    if (!f) { fprintf(stderr, "tick: cannot open %s\n", path); return nullptr; }
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    char* buf = (char*)malloc(size + 1);
    size_t n = fread(buf, 1, size, f);
    buf[n] = '\0';
    fclose(f);
    return buf;
}

static void write_file(const char* path, const char* content) {
    FILE* f = fopen(path, "wb");
    if (f) { fputs(content, f); fclose(f); }
}

/// Directory holding tick_runtime.{c,h}, resolved relative to the tick binary.
static void runtime_dir(char* out, size_t cap) {
    char exe[1024];
#ifdef __APPLE__
    uint32_t sz = sizeof(exe);
    if (_NSGetExecutablePath(exe, &sz) != 0) { snprintf(out, cap, "."); return; }
#else
    ssize_t n = readlink("/proc/self/exe", exe, sizeof(exe) - 1);
    if (n < 0) { snprintf(out, cap, "."); return; }
    exe[n] = '\0';
#endif
    char real[1024];
    if (realpath(exe, real)) strncpy(exe, real, sizeof(exe) - 1);
    char* slash = strrchr(exe, '/');
    if (slash) *slash = '\0';
    // tick_runtime lives next to sources; the build keeps it at <repo>/src/runtime
    // and installs to <prefix>/share/tick/runtime. Probe both.
    snprintf(out, cap, "%s", exe);
}

static bool find_runtime(char* dir_out, size_t cap) {
    char base[1024];
    runtime_dir(base, sizeof(base));
    const char* candidates[] = {
        "%s/../src/runtime",
        "%s/../share/tick/runtime",
        "%s/src/runtime",
        "%s",
    };
    for (const char* pat : candidates) {
        char path[1100];
        snprintf(path, sizeof(path), pat, base);
        char hdr[1200];
        snprintf(hdr, sizeof(hdr), "%s/tick_runtime.h", path);
        if (access(hdr, R_OK) == 0) { snprintf(dir_out, cap, "%s", path); return true; }
    }
    return false;
}

bool Driver::run(const DriverOptions& opts) {
    char* source = read_file(opts.source_file);
    if (!source) return false;

    Lexer lexer(source);
    DynamicArray<Token> tokens = lexer.tokenize();

    Parser parser(tokens);
    Program* program = parser.parse();

    Checker checker(program);
    if (!checker.check()) {
        delete program;
        free(source);
        return false;
    }

    CodeGen codegen(program, &checker, opts.mode);
    String c_code = codegen.generate();

    if (opts.emit_c_only) {
        fwrite(c_code.c_str(), 1, c_code.length(), stdout);
        delete program;
        free(source);
        return true;
    }

    char rt_dir[1200];
    if (!find_runtime(rt_dir, sizeof(rt_dir))) {
        fprintf(stderr, "tick: cannot locate tick_runtime\n");
        delete program;
        free(source);
        return false;
    }

    char c_path[1024];
    if (opts.keep_c) snprintf(c_path, sizeof(c_path), "%s.c", opts.output_file);
    else snprintf(c_path, sizeof(c_path), "/tmp/tick_%d.c", (int)getpid());
    write_file(c_path, c_code.c_str());

    const char* opt_flag = (opts.mode == BuildMode::RELEASE) ? "-O2 -DNDEBUG" : "-O2";
    char link_flags[2048] = {0};
    size_t off = 0;
    for (size_t i = 0; i < program->link_flags.size(); i++) {
        int w = snprintf(link_flags + off, sizeof(link_flags) - off, "%s ",
                         program->link_flags[i].c_str());
        if (w > 0) off += (size_t)w;
    }

    char cmd[4096];
    snprintf(cmd, sizeof(cmd),
             "cc %s -I%s %s %s/tick_runtime.c -o %s -pthread -lm %s",
             opt_flag, rt_dir, c_path, rt_dir, opts.output_file, link_flags);

    int rc = system(cmd);

    if (!opts.keep_c) remove(c_path);
    else printf("tick: generated C kept at %s\n", c_path);

    delete program;
    free(source);

    if (rc != 0) {
        fprintf(stderr, "tick: C compilation failed\n");
        return false;
    }
    return true;
}

}
