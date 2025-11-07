#include "process.h"

Process::Process(const std::string& name, ProcessFn fn)
  : name(name), processFunction(fn) {}

void Process::execute() {
  if (processFunction) {
    processFunction();
  }
}
