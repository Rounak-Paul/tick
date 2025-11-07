#pragma once

#include <vector>
#include <memory>
#include "process.h"
#include "clock.h"

class Scheduler {
public:
  Scheduler();
  
  void addProcess(ProcessPtr process);
  void addClock(std::shared_ptr<Clock> clock);
  void run();
  void stop();
  
private:
  std::vector<ProcessPtr> processes;
  std::vector<std::shared_ptr<Clock>> clocks;
  bool running;
};
