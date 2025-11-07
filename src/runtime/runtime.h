#pragma once

#include <memory>
#include "scheduler.h"
#include "clock.h"

class Runtime {
public:
  static Runtime& getInstance();
  
  std::shared_ptr<Clock> createClock(const std::string& name, int frequencyHz);
  std::shared_ptr<Scheduler> getScheduler() { return scheduler; }
  
private:
  Runtime();
  std::shared_ptr<Scheduler> scheduler;
};
