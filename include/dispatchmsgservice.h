#ifndef DISPATCH_MSG_SERVICE_H
#define DISPATCH_MSG_SERVICE_H

#include "types.h"
#include <memory>
#include <mutex>
#include <unordered_map>

class ThreadPool;
class iEventHandler;
class iEvent;
class NetworkInterface;

class DispatchMsgService {
public:
  explicit DispatchMsgService(ThreadPool& pool);
  ~DispatchMsgService() = default;
  
  DispatchMsgService(const DispatchMsgService&) = delete;
  DispatchMsgService& operator=(const DispatchMsgService&) = delete;
  
  void subscribe(u32 eid, iEventHandler* handler);
  void unsubscribe(u32 eid);

  void enqueue(std::unique_ptr<iEvent> ev);

  //新增创建事件的工厂方法
  std::unique_ptr<iEvent> create_event(u32 type_id, const char* data, u32 len);

  void set_networkinterface(NetworkInterface* ni) {
    network_interface_ = ni;
  };//是为了不会出现初始化顺序问题，所以不能在构造函数里面set
  void post_response(std::unique_ptr<iEvent> ev);
private:
  //真正的事件处理函数，在线程池中执行
  void process(std::unique_ptr<iEvent> ev);

  ThreadPool& thread_pool_;//使用引用，因为dms不拥有线程池

  std::unordered_map<u32, iEventHandler*> subscribers_;
  std::mutex subscribers_mutex_;

  NetworkInterface* network_interface_ = nullptr;
};

#endif