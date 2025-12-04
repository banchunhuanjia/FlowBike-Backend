#include "mysqlconn.h"
#include "spdlog/spdlog.h"

MysqlConn::MysqlConn() {
  conn_ = mysql_init(nullptr);
  if (conn_ == nullptr) {
    spdlog::error("MysqlConn init failed");
  }
  if (conn_) { // 设置字符集
    mysql_set_character_set(conn_, "utf8");
  }
}

MysqlConn::~MysqlConn() {
  freeResult();

  if (conn_ != nullptr) {
    mysql_close(conn_);
    conn_ = nullptr;
  }
}

bool MysqlConn::connect(const std::string& user, const std::string& passwd, 
                        const std::string& dbName, const std::string& ip, 
                        unsigned short port) {
  MYSQL* p = mysql_real_connect(conn_, ip.c_str(), user.c_str(), passwd.c_str(), dbName.c_str(), port, nullptr, 0);

  if (p==nullptr) {
    spdlog::error("Mysql connect failed: {}", mysql_error(conn_));
    return false;
  }
  return true;
}

bool MysqlConn::update(const std::string& sql) {
  // mysql_query 0表示成功
  if (mysql_query(conn_, sql.c_str())) {
    spdlog::error("Update failed: {} | SQL: {}", mysql_error(conn_), sql);
    return false;
  }
  return true;
}

bool MysqlConn::query(const std::string& sql) {
  // 执行前清空上次结果
  freeResult();

  if (mysql_query(conn_, sql.c_str())) {
    spdlog::error("Query failed: {} | SQL: {}", mysql_error(conn_), sql);
    return false;
  }

  // 获取结果集
  result_ = mysql_store_result(conn_);
  if (result_ == nullptr) {
    if (mysql_field_count(conn_) == 0) {
      //非查询语句，执行错了函数但是也应该返回成功，不然会rollback
      return true;
    }
    spdlog::error("Store result failed: {}", mysql_error(conn_));
    return false;
  }
  return true;
}

bool MysqlConn::next() {
  if (result_ != nullptr) {
    row_ = mysql_fetch_row(result_);
    if (row_ != nullptr) {
      return true;
    }
  }
  return false;
}

std::string MysqlConn::value(int index) {
  int col_count = mysql_num_fields(result_);
  if (index >= col_count || index <0) {
    return "";
  }
  char* val = row_[index];
  if (val == nullptr) {
    return "";
  }
  unsigned long length = mysql_fetch_lengths(result_)[index];
  return std::string(val, length);
}


bool MysqlConn::transaction() {
  // 开启事务：关闭自动提交
  return mysql_autocommit(conn_, false) == 0;
}

bool MysqlConn::commit() {
  return mysql_commit(conn_) == 0;
}

bool MysqlConn::rollback() {
  return mysql_rollback(conn_) == 0;
}

bool MysqlConn::ping() {
  return mysql_ping(conn_) == 0;// =0是成功
}

void MysqlConn::refreshAliveTime() {
  alive_time_ = std::chrono::steady_clock::now();
}

long long MysqlConn::getAliveTime() {
  std::chrono::nanoseconds res = std::chrono::steady_clock::now() - alive_time_;
  std::chrono::milliseconds mill = std::chrono::duration_cast<std::chrono::milliseconds>(res);
  return mill.count();
}

long long MysqlConn::getAffectedRows() {
  return mysql_affected_rows(conn_);
}


void MysqlConn::freeResult() {
  if (result_) {
    mysql_free_result(result_);
    result_ = nullptr;
  }
}