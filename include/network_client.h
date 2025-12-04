#ifndef NETWORK_CLIENT_H
#define NETWORK_CLIENT_H

#include <string>
#include "types.h"
#include "packetheader.h"

class NetworkClient {
public:
  NetworkClient(const std::string& ip, int port);
  ~NetworkClient();

  bool start(); 

  bool sendRequest(u32 code, const std::string& body);

  bool getResponse(int& out_msg_code, std::string& out_data);
  //  用引用参数，把状态码带出来
  //  使用token我们需要把他带出来，但是有的rsp没有token所以我们没有的带出desc
  //  使用非const引用才能将它带出来

  void Close(); // 这个是大写的

private: 
  std::string ip_;
  int port_;
  int sock_;
  bool is_connected_;
};

#endif