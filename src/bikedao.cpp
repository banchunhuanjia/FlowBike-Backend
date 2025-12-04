#include "bikedao.h"
#include "mysqlconn.h"
#include "connectionpool.h"
#include "spdlog/spdlog.h"
#include <string>

BikeDao::BikeDao() {};
BikeDao::~BikeDao() {};

bool BikeDao::queryByDevNo(int dev_no, Bike& bike, MysqlConn* conn) {
  std::shared_ptr<MysqlConn> pool_conn = nullptr;
  MysqlConn* op_conn = conn;
  if (op_conn == nullptr) {
    pool_conn = ConnectionPool::getConnectionPool()->getConnection();
    op_conn = pool_conn.get();
  }
  if (op_conn == nullptr) return false;

  std::string sql = "SELECT id, dev_no, status, longitude, latitude FROM bike WHERE dev_no = " + std::to_string(dev_no);

  if (!op_conn->query(sql)) {
    spdlog::error("Query bike failed. DevNo: {}", dev_no);
    return false;
  }

  if (op_conn->next()) {
    bike.id = std::stoi(op_conn->value(0));
    bike.dev_no = std::stoi(op_conn->value(1));
    bike.status = std::stoi(op_conn->value(2));
    bike.longitude = std::stod(op_conn->value(3));
    bike.latitude = std::stod(op_conn->value(4));
    return true;
  }
    
  return false;
}

bool BikeDao::updateStatus(int dev_no, int old_status, int new_status, MysqlConn* conn) {
  std::shared_ptr<MysqlConn> pool_conn = nullptr;
  MysqlConn* op_conn = conn;
  if (op_conn == nullptr) {
    pool_conn = ConnectionPool::getConnectionPool()->getConnection();
    op_conn = pool_conn.get();
  }
  if (op_conn == nullptr) return false;

  //我们修改了先查
  std::string sql = "UPDATE bike SET status = " + std::to_string(new_status) + 
                    " WHERE dev_no = " + std::to_string(dev_no) + 
                    " AND status = " + std::to_string(old_status);

  if (!op_conn->update(sql)) {
    spdlog::error("Update bike status failed. DevNo: {}", dev_no);
    return false;
  }

  //对于修改，我们应该有一个判断就是拿到受影响行数，即使受影响行数=0,也算CAS成功但是我们的业务没成功
  //所以做到这里的时候我们新引入了mysqlconn的获取受影响函数的接口
  return op_conn->getAffectedRows() > 0;
}