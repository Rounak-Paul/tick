#include "runtime/driver.h"
#include <cstdio>
#include <cstring>

using namespace Tick;

static void usage() {
    fprintf(stderr,
        "Usage: tick <source.tick> [options]\n"
        "  -o <file>     output binary (default: a.out)\n"
        "  --validate    full validation layer (bounds, null, overflow, use-after-move)\n"
        "  --release     strip all checks, maximum performance\n"
        "  --keep-c      keep the generated C file\n"
        "  --emit-c      print generated C to stdout and exit\n");
}

int main(int argc, char** argv) {
    if (argc < 2) { usage(); return 1; }

    DriverOptions opts;
    opts.source_file = argv[1];

    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
            opts.output_file = argv[++i];
        } else if (strcmp(argv[i], "--validate") == 0) {
            opts.mode = BuildMode::VALIDATE;
        } else if (strcmp(argv[i], "--release") == 0) {
            opts.mode = BuildMode::RELEASE;
        } else if (strcmp(argv[i], "--keep-c") == 0) {
            opts.keep_c = true;
        } else if (strcmp(argv[i], "--emit-c") == 0) {
            opts.emit_c_only = true;
        } else {
            fprintf(stderr, "Unknown option: %s\n", argv[i]);
            usage();
            return 1;
        }
    }

    return Driver::run(opts) ? 0 : 1;
}
