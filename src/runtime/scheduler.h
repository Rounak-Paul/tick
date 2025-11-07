#pragma once

#include <memory>
#include "../compiler/ast.h"

class Scheduler {
public:
  // Main entry point: execute a complete TDL program
  int executeProgram(const std::shared_ptr<Program>& program);
};
