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

void incrementer(std::shared_ptr<Channel> out) {
  static int counter = 0;
  (counter = (counter + 1));
  println(counter);
  gChannels["out"]->send(ChannelValue(counter));
}

void accumulator(std::shared_ptr<Channel> in, std::shared_ptr<Channel> out) {
  static int sum = 0;
  (sum = (sum + 10));
  println(sum);
  gChannels["out"]->send(ChannelValue(sum));
}

void logger(std::shared_ptr<Channel> in) {
  static int logged = 0;
  (logged = (logged + 1));
  println(logged);
}

int main() {
  // Initialize channels
  gChannels["out"] = std::make_shared<Channel>(4);
  gChannels["in"] = std::make_shared<Channel>(4);
  gChannels["out"] = std::make_shared<Channel>(4);
  gChannels["in"] = std::make_shared<Channel>(4);

  // Run simulation
  int numTicks = 10;
  // Clock: tick at 50 Hz
  double periodMs = 1000.0 / 50;
  long long ticksExecuted = 0;
  std::vector<double> slacks;

  for (int tick = 0; tick < numTicks; tick++) {
    auto tickStart = std::chrono::high_resolution_clock::now();
    incrementer(gChannels["out"]);
    accumulator(gChannels["in"], gChannels["out"]);
    logger(gChannels["in"]);
    ticksExecuted++;
    auto tickEnd = std::chrono::high_resolution_clock::now();
    auto elapsed = std::chrono::duration<double, std::milli>(tickEnd - tickStart).count();
    double slack = periodMs - elapsed;
    slacks.push_back(slack);
    // Only sleep if frequency is specified (not max-speed)
    if (50 < 999000 && slack > 0) {
      std::this_thread::sleep_for(std::chrono::milliseconds((long long)slack));
    }
  }

  // Print statistics
  std::cout << "\n=== Statistics ===\n";
  std::cout << "Clock: tick\n";
  std::cout << "Frequency: 50 Hz\n";
  std::cout << "Period: " << periodMs << " ms\n";
  std::cout << "Ticks executed: " << ticksExecuted << "\n";
  double avgSlack = 0;
  double minSlack = slacks[0];
  double maxSlack = slacks[0];
  for (double s : slacks) {
    avgSlack += s;
    if (s < minSlack) minSlack = s;
    if (s > maxSlack) maxSlack = s;
  }
  avgSlack /= slacks.size();
  std::cout << "Average slack: " << avgSlack << " ms\n";
  std::cout << "Min slack: " << minSlack << " ms\n";
  std::cout << "Max slack: " << maxSlack << " ms\n";

  std::cout << "\n=== Channel Statistics ===\n";
  for (auto& [name, channel] : gChannels) {
    if (channel->getTotalMessages() > 0) {
      std::cout << "Channel " << name << ":\n";
      std::cout << "  Messages: " << channel->getTotalMessages() << "\n";
      std::cout << "  Max depth: " << channel->getMaxDepth() << "\n";
      std::cout << "  Avg depth: " << channel->getAverageDepth() << "\n";
    }
  }
  return 0;
}
