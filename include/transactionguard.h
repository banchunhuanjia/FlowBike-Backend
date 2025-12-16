#ifndef TRANSACTION_GUARD_H
#define TRANSACTION_GUARD_H

#include "mysqlconn.h"

class TransactionGuard {
public:
  explicit TransactionGuard(MysqlConn* conn) : conn_(conn), is_started_(false), is_committed_(false) {
    if (conn_) {
      is_started_ = conn_->transaction();
    }
  }

 //核心:析构函数，如果事件开启但是没提交还到析构了那必是出错了，所以回滚
  ~TransactionGuard() {
    if (conn_ && is_started_ && !is_committed_) {
      conn_->rollback();
    }
  }

  bool isStarted() const {
    return is_started_;
  }

  bool commit() {
    if (conn_ && is_started_) {
      if (conn_->commit()) {
        is_committed_ = true;
        return true;
      }
    }
    return false;
  }

  TransactionGuard(const TransactionGuard&) = delete;
  TransactionGuard& operator=(const TransactionGuard&) = delete;
private:
  MysqlConn* conn_;
  bool is_started_;
  bool is_committed_;
};

#endif