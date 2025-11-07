#include "scheduler.h"
#include <thread>
#include <chrono>

Scheduler::Scheduler() : running(false) {}

void Scheduler::addProcess(ProcessPtr process) {
  processes.push_back(process);
}

void Scheduler::addClock(std::shared_ptr<Clock> clock) {
  clocks.push_back(clock);
}

void Scheduler::run() {
  running = true;
  
  // Start all clocks with 100 ticks
  for (auto& clock : clocks) {
    clock->start(100);
  }
  
  running = false;
}

void Scheduler::stop() {
  running = false;
}
