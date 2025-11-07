#include "scheduler.h"
#include "executor.h"

int Scheduler::executeProgram(const std::shared_ptr<Program>& program) {
  if (!program) return 1;
  
  // Create executor for auto-parallelized execution
  auto executor = std::make_shared<Executor>();
  executor->executeProgram(program);
  
  return 0;
}
