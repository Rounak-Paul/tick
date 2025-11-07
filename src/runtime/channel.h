#pragma once

#include <queue>
#include <memory>
#include <variant>
#include <optional>
#include <mutex>
#include <string>
#include <vector>

class ChannelValue {
public:
  using Value = std::variant<int, double, bool, std::string>;
  
  ChannelValue() = default;
  explicit ChannelValue(int v) : value(v) {}
  explicit ChannelValue(double v) : value(v) {}
  explicit ChannelValue(bool v) : value(v) {}
  explicit ChannelValue(const std::string& v) : value(v) {}
  
  bool isInt() const { return std::holds_alternative<int>(value); }
  bool isDouble() const { return std::holds_alternative<double>(value); }
  bool isBool() const { return std::holds_alternative<bool>(value); }
  bool isString() const { return std::holds_alternative<std::string>(value); }
  
  int getInt() const { return std::get<int>(value); }
  double getDouble() const { return std::get<double>(value); }
  bool getBool() const { return std::get<bool>(value); }
  std::string getString() const { return std::get<std::string>(value); }
  
private:
  Value value;
};

class Channel {
public:
  explicit Channel(int maxSize = 4);
  
  void send(const ChannelValue& value);
  std::optional<ChannelValue> tryRecv();
  bool isFull() const;
  bool isEmpty() const;
  
  int getSize() const { return queue.size(); }
  int getCapacity() const { return maxSize; }
  int getMaxDepth() const { return maxDepth; }
  int getTotalMessages() const { return totalMessages; }
  double getAverageDepth() const;
  
private:
  std::queue<ChannelValue> queue;
  int maxSize;
  mutable std::mutex mtx;
  
  // Statistics
  int maxDepth = 0;
  int totalMessages = 0;
  std::vector<int> depthHistory;
};
