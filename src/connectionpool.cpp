#include "connectionpool.h"
#include "spdlog/spdlog.h"

#include <chrono>
#include <thread>

ConnectionPool* ConnectionPool::getConnectionPool() {
  static ConnectionPool pool;
  return &pool;
}

ConnectionPool::ConnectionPool(): currentSize_(0), minSize_(0), maxSize_(0){}

void ConnectionPool::init(const std::string& user, const std::string& pwd, 
                          const std::string& dbName, const std::string& ip, 
                          unsigned short port, int minSize, int maxSize, int maxIdleTime) {
  user_ = user;
  pwd_ =pwd;
  dbName_ = dbName;
  ip_ = ip;
  port_ = port;
  minSize_ = minSize;
  maxSize_ = maxSize;
  maxIdleTime_ = maxIdleTime;

  for (int i = 0; i < minSize_; i++) {
    produceConnection();
  }

  //启动后台线程去定期检查销毁
  std::thread scanner([this](){
    this->scannerConnectionTask();
  });
  scanner.detach();
}

void ConnectionPool::produceConnection() {
  MysqlConn* conn = new MysqlConn();
  if (conn->connect(user_, pwd_, dbName_, ip_, port_)) {
    conn->refreshAliveTime();
    connectionQ_.push(conn);
    currentSize_++;
  } else {
    spdlog::error("Failed to create new connection");
    delete conn;
  }
}

std::shared_ptr<MysqlConn> ConnectionPool::getConnection() {
  std::unique_lock<std::mutex> lock(mutex_);
  //需要调用cond_.wait_for(lock, ...),cond_的等待操作需要先解锁，等待，再加锁，lock_guard不支持中途开锁
  while (connectionQ_.empty()) {
    if (currentSize_ < maxSize_) {
      produceConnection();
    } else {
      if (cond_.wait_for(lock, std::chrono::milliseconds(1000)) == std::cv_status::timeout) {
        if (connectionQ_.empty()) {
          spdlog::warn("ConnectionPool busy, timeout!");
          return nullptr;
        }
      }
    }
  }

  MysqlConn* conn = connectionQ_.front();
  connectionQ_.pop();

  if (!conn->ping()) {
    delete conn;
    conn = new MysqlConn();
    conn->connect(user_, pwd_, dbName_, ip_, port_);
  }

  // std::shared_ptr<T> p(指针对象, 删除器函数)
  std::shared_ptr<MysqlConn> sp(conn, [this](MysqlConn* p){
    std::lock_guard<std::mutex> lock(mutex_);
    p->refreshAliveTime();
    connectionQ_.push(p);
    cond_.notify_one();
  });

  return sp;
}


void ConnectionPool::scannerConnectionTask() {
  while (true) {
    std::this_thread::sleep_for(std::chrono::seconds(10));

    std::unique_lock<std::mutex> lock(mutex_);

    while (currentSize_ > minSize_) {
      if (connectionQ_.empty()) break;
      
      MysqlConn* conn = connectionQ_.front();

      if (conn->getAliveTime() >= maxIdleTime_) {
        connectionQ_.pop();
        delete conn;

        currentSize_--;
        spdlog::debug("Scanner: Destoryed an idle connection. Current size {}", currentSize_);
      } else break;
    }
  }
}

ConnectionPool::~ConnectionPool() {
  std::lock_guard<std::mutex> lock(mutex_);
  while (!connectionQ_.empty()) {
    MysqlConn* conn = connectionQ_.front();
    connectionQ_.pop();
    delete conn;
  }
}