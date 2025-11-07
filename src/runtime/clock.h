#pragma once

#include <string>
#include <chrono>
#include <vector>
#include <functional>
#include <atomic>
#include <memory>

class Clock {
public:
  using TickCallback = std::function<void()>;
  
  Clock(const std::string& name, int frequencyHz);
  
  std::string getName() const { return name; }
  int getFrequency() const { return frequencyHz; }
  long long getCurrentTick() const { return currentTick; }
  double getPeriodMs() const { return 1000.0 / frequencyHz; }
  
  void registerCallback(TickCallback callback);
  void tick();
  void start(int numTicks);
  void stop();
  bool isRunning() const { return running; }
  
  // Statistics
  long long getTicksExecuted() const { return ticksExecuted; }
  double getAverageSlack() const;
  
private:
  std::string name;
  int frequencyHz;
  long long currentTick;
  long long ticksExecuted;
  std::vector<TickCallback> callbacks;
  std::atomic<bool> running;
  std::vector<double> tickSlacks; // Track slack for each tick
};
