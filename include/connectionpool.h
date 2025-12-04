// 对于连接仓库（使用shared_ptr），在用完连接后不销毁，回到仓库中，需要利用shared_ptr的自定义删除器
#ifndef CONNECTIONPOOL_H
#define CONNECTIONPOOL_H

#include <memory>
#include <string>
#include <atomic>
#include <queue>
#include <mutex>
#include <condition_variable>

#include "mysqlconn.h"

class ConnectionPool {
public:
  static ConnectionPool* getConnectionPool();

  std::shared_ptr<MysqlConn> getConnection();

  void init(const std::string& user, const std::string& pwd,
            const std::string& dbName, const std::string& ip,
            unsigned short port, int minSize, int maxSize, int maxIdleTime);
  // init没有写在构造函数中，因为这是静态单例，他在main执行或者第一次调用时就初始化了
  // 但是这时候可能还没到config
private:
  ConnectionPool();
  ~ConnectionPool();

  ConnectionPool(const ConnectionPool&) = delete;
  ConnectionPool& operator=(const ConnectionPool&) = delete;

  void produceConnection();

  // 扫描线程函数，用于定期销毁空闲太久的连接，用alive_time_
  void scannerConnectionTask();

  std::string ip_;
  std::string user_;
  std::string pwd_;
  std::string dbName_;
  unsigned short port_;

  int minSize_;
  int maxSize_;
  std::atomic_int currentSize_;// 已创建的连接，包括空闲和正被使用

  std::queue<MysqlConn*> connectionQ_;// 空闲连接队列
  std::mutex mutex_;
  std::condition_variable cond_;

  long long maxIdleTime_;
};

#endif