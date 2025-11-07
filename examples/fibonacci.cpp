#include <iostream>
#include <queue>
#include <map>
#include <memory>
#include <optional>
#include <thread>
#include <mutex>
#include <chrono>
#include <vector>
#include <numeric>

// ============ Channel Implementation ============
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
  explicit Channel(int maxSize = 4) : maxSize(maxSize), maxDepth(0), totalMessages(0) {}
  
  void send(const ChannelValue& value) {
    std::lock_guard<std::mutex> lock(mtx);
    if (queue.size() < static_cast<size_t>(maxSize)) {
      queue.push(value);
      totalMessages++;
      int currentDepth = queue.size();
      depthHistory.push_back(currentDepth);
      if (currentDepth > maxDepth) maxDepth = currentDepth;
    }
  }
  
  std::optional<ChannelValue> tryRecv() {
    std::lock_guard<std::mutex> lock(mtx);
    if (queue.empty()) return std::nullopt;
    auto value = queue.front();
    queue.pop();
    return value;
  }
  
  bool isEmpty() const {
    std::lock_guard<std::mutex> lock(mtx);
    return queue.empty();
  }
  
  int getSize() const { return queue.size(); }
  int getMaxDepth() const { return maxDepth; }
  int getTotalMessages() const { return totalMessages; }
  double getAverageDepth() const {
    if (depthHistory.empty()) return 0.0;
    double sum = std::accumulate(depthHistory.begin(), depthHistory.end(), 0);
    return sum / depthHistory.size();
  }
  
private:
  std::queue<ChannelValue> queue;
  int maxSize;
  mutable std::mutex mtx;
  int maxDepth = 0;
  int totalMessages = 0;
  std::vector<int> depthHistory;
};

// ============ Statistics ============
struct Statistics {
  std::string clockName;
  int frequency;
  long long ticksExecuted;
  double averageSlack;
  std::map<std::string, int> channelMaxDepth;
  std::map<std::string, double> channelAvgDepth;
  std::map<std::string, int> channelMessages;
};

std::map<std::string, std::shared_ptr<Channel>> gChannels;
std::vector<Statistics> gStatistics;

// Helper function for println
template<typename T>
void println(const T& value) {
  std::cout << value << std::endl;
}

int fibonacci(int n) {
  if ((n <= 1)) {
    return n;
  }
  return (fibonacci((n - 1)) + fibonacci((n - 2)));
}

int main() {
  int result = fibonacci(10);
  println(result);
}

// User-defined main function will be called
