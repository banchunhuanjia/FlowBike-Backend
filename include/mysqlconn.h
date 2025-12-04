#ifndef MYSQLCONN_H
#define MYSQLCONN_H

#include <string>
#include <chrono>
#include <mysql/mysql.h>

class MysqlConn {
public:
  MysqlConn();
  ~MysqlConn();

  bool connect(const std::string& user, const std::string& passwd,
              const std::string& dbName, const std::string& ip, unsigned short port);

  bool update(const std::string& sql);
  bool query(const std::string& sql);

  bool next(); // 遍历每一行查询结果

  std::string value(int index); // 获取结果集中的字段值

  // 事务模块，是原子操作
  bool transaction();
  bool commit();
  bool rollback();

  // 检查断连，用在连接池的检查
  bool ping();

  void refreshAliveTime();

  long long getAliveTime();

  long long getAffectedRows();

private:
  void freeResult();

  MYSQL* conn_ = nullptr;
  MYSQL_RES* result_ = nullptr;
  MYSQL_ROW  row_ = nullptr;
  //MYSQL_ROW是个指针类，但是row只复制值就行，不用把他设置成MYSQL_ROW*
  
  std::chrono::steady_clock::time_point alive_time_;
};

#endif