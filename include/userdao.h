#ifndef USER_DAO_H
#define USER_DAO_H

#include <string>


class MysqlConn;

// 用于承载从数据库中读出来的数据
struct User {
  int id;
  std::string mobile;
  std::string nickname;
  int verify_code;
  double balance;
  double deposit;
};

class UserDao {
public:
  UserDao(){};
  ~UserDao(){};

  bool insert(const std::string& mobile);

  bool queryByMobile(const std::string& mobile, User& user, MysqlConn* conn);

  bool updateVerifyCode(const std::string& mobile, int code);

  int getVerifyCode(const std::string& mobile);

  double getBalance(const std::string& mobile, MysqlConn* conn);

private:
  //无需存ConnectionPool成员
};

#endif