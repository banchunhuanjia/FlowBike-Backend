#ifndef BIKE_DAO_H
#define BIKE_DAO_H

class MysqlConn;

struct Bike{
  int id;
  int dev_no;
  int status;
  double longitude;
  double latitude;
};

class BikeDao {
public:
  BikeDao();
  ~BikeDao();

  //是先查后改(改依赖查询结果)会引发竞态，只查不会，这个函数用来确定单车状态
  bool queryByDevNo(int dev_no, Bike& bike, MysqlConn* conn);

  //使用乐观锁，与此同时我们加入new，old这样借车和换车的重复部分就不会出现了
  bool updateStatus(int dev_no, int old_status, int new_status, MysqlConn* conn);
};

#endif