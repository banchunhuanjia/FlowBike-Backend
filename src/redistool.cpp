#include "redistool.h"
#include "spdlog/spdlog.h"
#include "global_config.h"
RedisTool::RedisTool() : context_(nullptr), port_(0){}

RedisTool::~RedisTool() {
  if (context_) {
    redisFree(context_);
  }
}

bool RedisTool::connect(const std::string& ip, int port) {
  if (context_) {
    redisFree(context_);
  }
  ip_ = ip;
  port_ = port;

  context_ = redisConnect(ip.c_str(), port);
  if (context_ == nullptr || context_->err) {
    if (context_) {
      spdlog::error("Redis Connection Error: {}", context_->err);
      redisFree(context_);
      context_ = nullptr;
    } else {
      spdlog::error("Redis Connection Error: Can't allocate context");
    }
    return false;
  }
  return true;
}

bool RedisTool::setString(const std::string& key, const std::string& value, int timeout) {
  if (!context_) return false;

  redisReply* reply = nullptr;

  if (timeout <= 0) {
    reply = (redisReply*)redisCommand(context_, "SET %s %s", key.c_str(), value.c_str());
  } else {
    reply = (redisReply*)redisCommand(context_, "SET %s %s EX %d", key.c_str(), value.c_str(), timeout);
  }

  bool success = false;

  if (reply) {
    if (reply->type == REDIS_REPLY_STATUS && reply->str != nullptr && std::string(reply->str) == "OK") {
      success = true;
    }
    freeReplyObject(reply);
  }
  return success;
}

std::string RedisTool::getString(const std::string& key) {
  if (!context_) return "";

  redisReply* reply = (redisReply*)redisCommand(context_, "GET %s", key.c_str());

  std::string result = "";
  if (reply) {
    if (reply->type == REDIS_REPLY_STRING) {
      result = reply->str;
    }
    freeReplyObject(reply);
  }
  return result;
}

bool RedisTool::delKey(const std::string& key) {
  if (!context_) return false;

  redisReply* reply = (redisReply*)redisCommand(context_, "DEL %s", key.c_str());

  bool success = false;
  if (reply) {
    if (reply->type == REDIS_REPLY_INTEGER && reply->integer > 0) {
      success = true;
    }
    freeReplyObject(reply);
  }
  return success;
}

// 获取当前线程专属的Redis连接
RedisTool* RedisTool::getThreadLocal() {
  static thread_local RedisTool t_redis;
  // static保证当前线程仅初始化一次
  // thread_local保证每个线程都有自己独立的t_redis实例
  
  // if (!t_redis.isConnected()) {
  //   std::string ip = "127.0.0.1";
  //   int port = 6379;
  //   try {
  //     ip = GlobalConfig::getInstance().getString("redis", "ip", "127.0.0.1");
  //     port = GlobalConfig::getInstance().getInt("redis", "port", 6379);
  //   } catch (...) {
  //     spdlog::warn("⚠️ Config read failed/crashed. Fallback to default Redis 127.0.0.1:6379");
  //     ip = "127.0.0.1";
  //     port = 6379;
  //   }
  //   t_redis.connect(ip, port);
  // }
  if (!t_redis.isConnected()) {

    std::string ip = GlobalConfig::getInstance().getString("redis", "ip", "127.0.0.1");
    int port = GlobalConfig::getInstance().getInt("redis", "port", 6379);

    t_redis.connect(ip, port);

    spdlog::debug("--- RedisTool: Hardcoded Connect to {}:{} ---", ip, port);
  }
  return &t_redis;
}