#include "tcpconnection.h"
#include "networkinterface.h"
#include "spdlog/spdlog.h"

#include <iostream>
#include <arpa/inet.h>
#include <cstring>
#include <cerrno>

TcpConnection::TcpConnection(struct event_base* base, evutil_socket_t fd,
                            NetworkInterface* owner, struct sockaddr* addr)
  : bev_(nullptr),
    owner_(owner),
    client_port_(0){
  bev_ = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE);
  if (!bev_) {
    spdlog::error("[TcpConnection] Error: Failed to create bufferevent for fd: {}", fd);
    throw std::runtime_error("Failed to create bufferevent");
  }

  //设置回调函数，把NetworkInterface的静态回调函数注册到这个bufferevent上
  //关键在传this当上下文指针，这样静态回调知道是那个Tcp对象触发的事件
  bufferevent_setcb(bev_, NetworkInterface::static_read_cb, nullptr, NetworkInterface::static_event_cb, this);
  bufferevent_enable(bev_, EV_READ | EV_WRITE);
  
  parse_address(addr);

  spdlog::debug("[TcpConnection] New connection created for {} : {} (fd: {})", client_ip_, client_port_, fd);
}

TcpConnection::~TcpConnection() {
  // if (bev_) {
  //   bufferevent_free(bev_);
  //   bev_ = nullptr;
  // } 与networkinterface 的handle_event冲突
  spdlog::debug("[TcpConnection] Connection for {} : {} destroyed.", client_ip_, client_port_);
}

void TcpConnection::parse_address(struct sockaddr* addr) {
  if (addr == nullptr) return;

  if (addr->sa_family == AF_INET) {
    struct sockaddr_in* sin = reinterpret_cast<struct sockaddr_in*>(addr);

    char ip_buf[INET_ADDRSTRLEN];
    if (inet_ntop(AF_INET, &sin->sin_addr, ip_buf, sizeof(ip_buf))) {
      client_ip_ = ip_buf;
    } else {
      client_ip_ = "invalid_ip";
      spdlog::error("[TcpConnection] Error: inet_ntop failed: {}", strerror(errno));
    }

    client_port_ = ntohs(sin->sin_port);
  }
}