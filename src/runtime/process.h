#pragma once

#include <functional>
#include <memory>
#include <vector>

class Process {
public:
  using ProcessFn = std::function<void()>;
  
  Process(const std::string& name, ProcessFn fn);
  
  void execute();
  void setName(const std::string& n) { name = n; }
  std::string getName() const { return name; }
  
private:
  std::string name;
  ProcessFn processFunction;
};

using ProcessPtr = std::shared_ptr<Process>;
