#ifndef NETWORK_INTERFACE_H
#define NETWORK_INTERFACE_H

class DispatchMsgService;
class TcpConnection;
class iEvent;

#include <event2/listener.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <unordered_map>
#include <unordered_map>
#include <memory>
#include <string>
#include <mutex>
#include <queue>

#include "packetheader.h"

struct ResponsePacket {
  //bufferevent* bev;
  //使用socket fd ，而不是使用裸的bufferevent*
  evutil_socket_t fd;
  std::string body;
  PacketHeader header;
};



class NetworkInterface {
public:
  NetworkInterface(DispatchMsgService* dms);
  ~NetworkInterface();

  NetworkInterface(const NetworkInterface&) = delete;
  NetworkInterface& operator=(const NetworkInterface&) = delete; 

  bool start(int port);
  void stop();
  //从业务逻辑层发送请求
  void send_response(std::unique_ptr<iEvent> ev);

  //这里可以优化成友元
  static void static_accept_cb(struct evconnlistener* listener, evutil_socket_t fd,
                              struct sockaddr* addr, int socklen, void* ctx);
  static void static_read_cb(struct bufferevent* bev, void* ctx);
  static void static_event_cb(struct bufferevent* bev, short events, void* ctx);
  
  static void flush_timer_cb(evutil_socket_t, short, void *arg);
private:
  void accept_new_connection(struct evconnlistener* listener, evutil_socket_t fd,
                            struct sockaddr* addr, int socklen);
  void handle_read(struct bufferevent* bev, TcpConnection* conn);
  void handle_event(struct bufferevent* bev, short events, TcpConnection* conn);

  void handle_siganl(int sig);
  void flush_send_queue();

private:
  DispatchMsgService* dms_;
  struct event_base* base_;
  struct evconnlistener* listener_;

  struct event* ev_sigint_;
  struct event* ev_sigterm_;

  //管理TcpConnection的生命周期
  std::unordered_map<evutil_socket_t, std::unique_ptr<TcpConnection>> connections_;

  //将发送操作搬回网络线程执行
  std::mutex resp_mtx;
  std::queue<ResponsePacket> resp_queue;
};

#endif