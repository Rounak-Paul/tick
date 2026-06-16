#ifndef TICK_BUILD_MODE_H
#define TICK_BUILD_MODE_H

namespace Tick {

/// Validation layer level, selected on the command line.
/// The single knob that trades safety instrumentation for raw speed.
enum class BuildMode {
    DEV,       // default: bounds + null/none checks
    VALIDATE,  // everything: + overflow, use-after-move, weak-upgrade, leak audit
    RELEASE    // none: only statically proven-safe code; checks compiled out
};

}

#endif
