#ifndef TICK_DRIVER_H
#define TICK_DRIVER_H

#include "build_mode.h"

namespace Tick {

struct DriverOptions {
    const char* source_file = nullptr;
    const char* output_file = "a.out";
    BuildMode mode = BuildMode::DEV;
    bool keep_c = false;
    bool emit_c_only = false;
};

/// Orchestrates the full pipeline: lex -> parse -> check -> codegen -> cc.
class Driver {
public:
    static bool run(const DriverOptions& opts);
};

}

#endif
