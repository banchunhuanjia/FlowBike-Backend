#include "userdao.h"
#include "mysqlconn.h"
#include "connectionpool.h"
#include "spdlog/spdlog.h"

bool UserDao::insert(const std::string& mobile) {
  auto conn = ConnectionPool::getConnectionPool()->getConnection();
  if (!conn) return false;

  std::string sql = "INSERT INTO user (mobile, nickname) VALUES ('" + mobile + "', '" + mobile + "')";

  if (!conn->update(sql)) {
    spdlog::error("Insert user failed. Mobile: {}", mobile);
    return false;
  }
  return true;
}

bool UserDao::queryByMobile(const std::string& mobile, User& user, MysqlConn* conn) {
  std::shared_ptr<MysqlConn> pool_conn = nullptr;
  MysqlConn* op_conn = conn;
  if (op_conn == nullptr) {
    pool_conn = ConnectionPool::getConnectionPool()->getConnection();
    op_conn = pool_conn.get();
  }
  if (op_conn == nullptr) return false;

  std::string sql = "SELECT id, mobile, nickname, verify_code, balance, deposit FROM user WHERE mobile = '" + mobile  + "'";

  if (!op_conn->query(sql)) {
    spdlog::error("Query user failed. Mobile: {}", mobile);
    return false;
  }

  if (op_conn->next()) {
    user.id = std::stoi(op_conn->value(0));
    user.mobile = op_conn->value(1);
    user.nickname = op_conn->value(2);
    
    std::string code_str = op_conn->value(3);
    user.verify_code = code_str.empty() ? 0 : std::stoi(code_str);
    
    user.balance = std::stod(op_conn->value(4));
    user.deposit = std::stod(op_conn->value(5));
    return true;
  }
  return false;
}

bool UserDao::updateVerifyCode(const std::string& mobile, int code) {
  auto conn = ConnectionPool::getConnectionPool()->getConnection();
  if (!conn) return false;

  // 使用如果有就update如果没有就insert的sql语句
  std::string sql = "INSERT INTO user (mobile, nickname, verify_code) "
                    "VALUES ('" + mobile + "', '" + mobile + "', " + std::to_string(code) + ") "
                    "ON DUPLICATE KEY UPDATE verify_code = " + std::to_string(code);
  // 有to_string就不用加'' 
  spdlog::debug("Executing Upsert SQL: {}", sql);

  if (!conn->update(sql)) {
    spdlog::error("Failed to save verify code for {}", mobile);
    return false;
  }
  return true;
}

int UserDao::getVerifyCode(const std::string& mobile) {
  auto conn = ConnectionPool::getConnectionPool()->getConnection();
  if (!conn) return -1;

  std::string sql = "SELECT verify_code FROM user WHERE mobile = '" + mobile + "'";
  if (!conn->query(sql)) return -1;

  if (conn->next()) {
    std::string val = conn->value(0);
    if (val.empty()) return 0;
    return std::stoi(val);
  }

  return 0;
}

double UserDao::getBalance(const std::string& mobile, MysqlConn* conn) {
  std::shared_ptr<MysqlConn> pool_conn = nullptr;
  MysqlConn* op_conn = conn;
  if (op_conn == nullptr) {
    pool_conn = ConnectionPool::getConnectionPool()->getConnection();
    op_conn = pool_conn.get();
  }
  if (op_conn == nullptr) return -1.0;

  std::string sql = "SELECT balance FROM user WHERE mobile = '" + mobile + "'";
  if (!op_conn->query(sql)) return -1.0;

  if (op_conn->next()) {
    std::string val = op_conn->value(0);
    if (val.empty()) return 0;
    return std::stod(val);
  }
  
  return -1.0;
}