#ifndef REDIS_TOOL_H
#define REDIS_TOOL_H

#include <string>
#include <hiredis/hiredis.h>

class RedisTool {
public:
  RedisTool();
  ~RedisTool();

  bool connect(const std::string& ip, int port);

  // SET key value EX timeout
  bool setString(const std::string& key, const std::string& value, int timeout = -1);

  std::string getString(const std::string& key);

  bool delKey(const std::string& key);

  bool isConnected() const {
    return context_ != nullptr;
  }

  static RedisTool* getThreadLocal();
private:
  redisContext* context_;
  std::string ip_;
  int port_;
};

#endif