#ifndef ERROR_MANAGER_H
#define ERROR_MANAGER_H

#include <string>
#include <unordered_map>
#include <optional>
#include "eventtype.h"
#include "types.h"

class ErrorManager {
public:
  static ErrorManager& getInstance();
  //单例
  std::optional<std::string> getDesc(i32 code);
private:
  //防止外部创建实例就要私有化构造
  ErrorManager();
  ~ErrorManager() = default;
  ErrorManager(const ErrorManager&) = delete;
  ErrorManager& operator=(const ErrorManager&) = delete;

  std::unordered_map<i32, std::string> error_map_; 
};

#endif