#ifndef TCP_CONNECTION_H
#define TCP_CONNECTION_H

#include <string>
#include <event2/event.h>
#include <event2/bufferevent.h>
#include "httpcontext.h"

class NetworkInterface;

class TcpConnection {
public:
  TcpConnection(struct event_base* base, evutil_socket_t fd, NetworkInterface* owner, struct sockaddr* addr);
  ~TcpConnection();

  TcpConnection(const TcpConnection&) = delete;
  TcpConnection& operator=(const TcpConnection&) = delete;
  
  bufferevent* get_bufferev() const { return bev_; }
  NetworkInterface* get_owner() const { return owner_; }
  const std::string& get_client_ip() const { return client_ip_; }
  int get_client_port() const { return client_port_; }
  
  
  HttpContext* get_context()  { return &context_; }
private:
  void parse_address(struct sockaddr* addr);//供构造函数调用

  struct bufferevent* bev_;
  NetworkInterface* owner_;
  std::string client_ip_;
  int client_port_;
  HttpContext context_;
};

#endif