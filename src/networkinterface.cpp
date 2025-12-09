#include "networkinterface.h"
#include "dispatchmsgservice.h"
#include "tcpconnection.h"
#include "httpcontext.h"
#include "ievent.h"
#include "bike.pb.h"
#include "spdlog/spdlog.h"
#include "eventtype.h"


#include <iostream>
#include <stdexcept>
#include <cstring>
#include <arpa/inet.h>
#include <vector>
#include <event2/event.h>
#include <signal.h>


NetworkInterface::NetworkInterface(DispatchMsgService* dms) :dms_(dms), base_(nullptr), listener_(nullptr) {
  GOOGLE_PROTOBUF_VERIFY_VERSION;//handle_read会用到，这是全局初始化，所以放在构造函数
  base_ = event_base_new();
  if (!base_) {
    throw std::runtime_error("[NetworkInterface] Failed to create event_base");
  }
  init_router();
}
NetworkInterface::~NetworkInterface() {
  if (ev_sigint_) {
    event_free(ev_sigint_);
    ev_sigint_ = nullptr;
  }
  if (ev_sigterm_) {
    event_free(ev_sigterm_);
    ev_sigterm_ = nullptr;
  }
  if (listener_) {
    evconnlistener_free(listener_);
    listener_ = nullptr;
  }
  if (base_) {
    event_base_free(base_);
    base_ = nullptr;
  }
  spdlog::debug("NetworkInterface] Destoryed");
}

void NetworkInterface::init_router() {
  url_router_["/api/mobile"] = EEVENTID_GET_MOBILE_CODE_REQ;
  url_router_["/api/login"] = EEVENTID_LOGIN_REQ;
  url_router_["/api/unlock"] = EEVENTID_UNLOCK_REQ;

  spdlog::debug("[NetworkInterface] Router initialized. Mapped /api/mobile, /api/login, /api/unlock");
}

bool NetworkInterface::start(int port) {
  if (!base_) return false;
  struct sockaddr_in sin;
  memset(&sin, 0, sizeof(sin));
  sin.sin_family = AF_INET;
  sin.sin_addr.s_addr = htonl(0);
  sin.sin_port = htons(port);

  listener_ = evconnlistener_new_bind(
    base_,
    static_accept_cb,
    this,
    LEV_OPT_REUSEABLE |LEV_OPT_CLOSE_ON_FREE,
    -1,
    (struct sockaddr*)&sin, sizeof(sin)
  );
  if (!listener_) {
    spdlog::error("[NetworkInterface] Error: Failed to create listener on port {}", port);
    return false;
  }

  //使用lievent的信号处理
  ev_sigint_ = evsignal_new(base_, SIGINT, [](evutil_socket_t, short, void* ctx) {
    static_cast<NetworkInterface*>(ctx)->handle_siganl(SIGINT);
  }, this);
  if (ev_sigint_) evsignal_add(ev_sigint_, nullptr);
  
  ev_sigterm_ = evsignal_new(base_, SIGTERM, [](evutil_socket_t, short, void* ctx) {
    static_cast<NetworkInterface*>(ctx)->handle_siganl(SIGTERM);
  }, this);
  if (ev_sigterm_) evsignal_add(ev_sigterm_, nullptr);

  spdlog::debug("[NetworkInterface] Server started, listening on port {} ...", port);

  timeval tv {0, 5000};
  event* flush_event = event_new(base_, -1, EV_PERSIST, flush_timer_cb, this);
  event_add(flush_event, &tv);

  event_base_dispatch(base_);
  
  spdlog::debug("[NetworkInterface] Event loop finished");
  return true;
}

void NetworkInterface::stop() {
  spdlog::debug("[NetworkInterface] Received stop signal. Requesting event loop to exit...");
  
  if (listener_) {
    evconnlistener_disable(listener_);
    evconnlistener_free(listener_);
    listener_ = nullptr;
  }

  connections_.clear();

  if (base_) {
    event_base_loopexit(base_, nullptr); 
  }
}

void NetworkInterface::handle_siganl(int sig) {
  spdlog::debug("[NetworkInterface] Signal {} caught in event loop. Initiating shutdown...");

  stop();
}


//void send_response(evutil_socket_t fd, std::unique_ptr<iEvent> ev);
void NetworkInterface::static_accept_cb(struct evconnlistener* listener, evutil_socket_t fd,
                              struct sockaddr* addr, int socklen, void* ctx)
{
  NetworkInterface* self = static_cast<NetworkInterface*>(ctx);
  self->accept_new_connection(listener, fd, addr, socklen);
}

//为何这两个回调是ctx是TcpConnection的this指针
void NetworkInterface::static_read_cb(struct bufferevent* bev, void* ctx) {
  TcpConnection* conn = static_cast<TcpConnection*>(ctx);
  conn->get_owner()->handle_read(bev, conn); 
}

void NetworkInterface::static_event_cb(struct bufferevent* bev, short events, void* ctx) {
  TcpConnection* conn = static_cast<TcpConnection*>(ctx);
  conn->get_owner()->handle_event(bev, events, conn);
}

void NetworkInterface::flush_timer_cb(evutil_socket_t, short, void* arg) {
  static_cast<NetworkInterface*>(arg)->flush_send_queue();
}


void NetworkInterface::accept_new_connection(struct evconnlistener* listener, evutil_socket_t fd,
                            struct sockaddr* addr, int socklen)
{
  try {
    connections_[fd] = std::make_unique<TcpConnection>(base_, fd, this, addr);
  } catch (const std::runtime_error& e) {
    spdlog::error("[networkInterface] Error creating TcpConnection: {}", e.what());
    close(fd);
  }
}

void NetworkInterface::handle_read(struct bufferevent* bev, TcpConnection* conn) {
  struct evbuffer* input = bufferevent_get_input(bev);
  HttpContext* context = conn->get_context();

  while (true) {
    size_t len = evbuffer_get_length(input);
    if (len == 0) break;
    char* data = (char*)evbuffer_pullup(input, -1);

    HttpCode code = context->parse(data, len);
    if (code == NO_REQUEST) {
      break;
    } else if (code == GET_REQUEST) {
      HttpRequest& req = context->getRequest();

      //应该放在reset前
      std::string path = req.getPath();
      std::string body = req.getBody();
      
      // 处理Buffer,充值Context
      size_t processed_len = context->getParseLen();
      evbuffer_drain(input, processed_len); // 真正移除数据
      context->reset();// 重置索引

      auto it = url_router_.find(path);
      if (it != url_router_.end()) {
        u32 type_id = it->second;

        std::unique_ptr<iEvent> event = dms_->create_event(type_id, body.data(), body.size());
        if (event) {
          event->set_fd(bufferevent_getfd(bev));
          dms_->enqueue(std::move(event));
        }
      } else {
        spdlog::warn("[NetworkInterface] 404 Not Found: {}", path);
        
        std::string error_body = "Error 404: The requested URL " + path + " was not found.";
        std::string http_resp;
        http_resp += "HTTP/1.1 404 Not Found\r\n";
        http_resp += "Content-Type: text/plain\r\n";
        http_resp += "Content-Length: " + std::to_string(error_body.size()) + "\r\n";
        http_resp += "\r\n";
        http_resp += error_body;

        ResponsePacket p;
        p.fd = bufferevent_getfd(bev);
        p.response_data = std::move(http_resp);

        {
          std::lock_guard<std::mutex> lock(resp_mtx);
          resp_queue.push(std::move(p));
        }
      }
    } else {
      evbuffer_drain(input, len);
      context->reset();
      break;
    }
  }
  
}

void NetworkInterface::handle_event(struct bufferevent* bev, short events, TcpConnection* conn) {
  if (events & (BEV_EVENT_EOF | BEV_EVENT_ERROR)) {
    if (events & BEV_EVENT_EOF) {
      spdlog::debug("[NetworkInterface] Connection closed by perr");
    } else {
      spdlog::error("[NetworkInterface] Connection error: {}", evutil_socket_error_to_string(EVUTIL_SOCKET_ERROR()));
    }
  }

  evutil_socket_t fd = bufferevent_getfd(bev);
  spdlog::debug("[NetworkInterface] Connection closing fd = {}", fd);

  {//清理resp_queue 中属于该bev的未发送数据
    std::lock_guard<std::mutex> lock(resp_mtx);
    std::queue<ResponsePacket> new_q;

    while (!resp_queue.empty()) {
      auto &pkt = resp_queue.front();
      if (pkt.fd != fd) {
        new_q.push(std::move(pkt));//只保留非当前连接的数据
      }
      resp_queue.pop();
    }
    resp_queue = std::move(new_q);
  }

  spdlog::debug("[NetworkInterface] FREE bufferevent fd = {}", fd);
  bufferevent_free(bev);//先free
  connections_.erase(fd);
}

void NetworkInterface::flush_send_queue() {
  std::lock_guard<std::mutex> lock(resp_mtx);
  while (!resp_queue.empty()) {
    auto p = std::move(resp_queue.front());
    resp_queue.pop();

    auto it = connections_.find(p.fd);
    if (it != connections_.end()) {
      bufferevent* bev = it->second->get_bufferev();
      if (bev) {
        bufferevent_write(bev, p.response_data.data(), p.response_data.size());
      }
    }
  }
}

void NetworkInterface::send_response(std::unique_ptr<iEvent> ev) {  
  if (!ev) return;
  evutil_socket_t fd = ev->get_fd();

  ResponsePacket p;
  p.fd = fd;
  
  std::string binary_body = ev->serialize();
  std::string http_resp;
  http_resp += "HTTP/1.1 200 OK\r\n";
  http_resp += "Content-Type: application/x-protobuf\r\n";
  http_resp += "Content-Length: " + std::to_string(binary_body.size()) + "\r\n";
  http_resp += "Connection: Keep-Alive\r\n";
  http_resp += "\r\n";
  http_resp += binary_body;

  p.response_data = std::move(http_resp);

  std::lock_guard<std::mutex> lock(resp_mtx);
  resp_queue.push(std::move(p));
}