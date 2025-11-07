#include "channel.h"
#include <iostream>
#include <numeric>

Channel::Channel(int maxSize) : maxSize(maxSize), maxDepth(0), totalMessages(0) {}

void Channel::send(const ChannelValue& value) {
  std::lock_guard<std::mutex> lock(mtx);
  if (queue.size() < static_cast<size_t>(maxSize)) {
    queue.push(value);
    totalMessages++;
    
    int currentDepth = queue.size();
    depthHistory.push_back(currentDepth);
    if (currentDepth > maxDepth) {
      maxDepth = currentDepth;
    }
  }
}

std::optional<ChannelValue> Channel::tryRecv() {
  std::lock_guard<std::mutex> lock(mtx);
  if (queue.empty()) {
    return std::nullopt;
  }
  
  auto value = queue.front();
  queue.pop();
  return value;
}

bool Channel::isFull() const {
  std::lock_guard<std::mutex> lock(mtx);
  return queue.size() >= static_cast<size_t>(maxSize);
}

bool Channel::isEmpty() const {
  std::lock_guard<std::mutex> lock(mtx);
  return queue.empty();
}

double Channel::getAverageDepth() const {
  std::lock_guard<std::mutex> lock(mtx);
  if (depthHistory.empty()) return 0.0;
  double sum = std::accumulate(depthHistory.begin(), depthHistory.end(), 0);
  return sum / depthHistory.size();
}