#include "clock.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <numeric>

Clock::Clock(const std::string& name, int frequencyHz)
  : name(name), frequencyHz(frequencyHz), currentTick(0), ticksExecuted(0), running(false) {}

void Clock::registerCallback(TickCallback callback) {
  callbacks.push_back(callback);
}

void Clock::tick() {
  auto tickStart = std::chrono::high_resolution_clock::now();
  
  // Execute all registered callbacks
  for (auto& callback : callbacks) {
    callback();
  }
  
  currentTick++;
  ticksExecuted++;
  
  // Calculate slack for statistics
  auto tickEnd = std::chrono::high_resolution_clock::now();
  auto tickDuration = std::chrono::duration<double, std::milli>(tickEnd - tickStart).count();
  double expectedDuration = getPeriodMs();
  double slack = expectedDuration - tickDuration;
  tickSlacks.push_back(slack);
}

void Clock::start(int numTicks) {
  running = true;
  auto periodMs = getPeriodMs();
  
  for (int i = 0; i < numTicks && running; i++) {
    auto tickStart = std::chrono::high_resolution_clock::now();
    
    tick();
    
    auto tickEnd = std::chrono::high_resolution_clock::now();
    auto elapsed = std::chrono::duration<double, std::milli>(tickEnd - tickStart).count();
    auto sleepTime = periodMs - elapsed;
    
    if (sleepTime > 0) {
      std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<long long>(sleepTime)));
    }
  }
  
  running = false;
}

void Clock::stop() {
  running = false;
}

double Clock::getAverageSlack() const {
  if (tickSlacks.empty()) return 0.0;
  double sum = std::accumulate(tickSlacks.begin(), tickSlacks.end(), 0.0);
  return sum / tickSlacks.size();
}
