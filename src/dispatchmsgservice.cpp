#include "dispatchmsgservice.h"
#include "ieventhandler.h"
#include "ievent.h"
#include "threadpool.h"
#include "mobilecodereqev.h"
#include "loginreq.h"
#include "unlockreq.h"
#include "networkinterface.h"
#include "bike.pb.h"
#include "spdlog/spdlog.h"

#include <iostream>

DispatchMsgService::DispatchMsgService(ThreadPool& pool) : 
    thread_pool_(pool) 
{
  spdlog::debug("DispatchMsgService is created.");
}

void DispatchMsgService::subscribe(u32 eid, iEventHandler* handler) {
  if (!handler) {
    return ;
  }
  std::lock_guard<std::mutex> lock(subscribers_mutex_);
  if (subscribers_.find(eid) != subscribers_.end()) {
    spdlog::error("Warning: Event ID {} is already subscribed. Overwriting.", eid);
  }
  subscribers_[eid] = handler;
  spdlog::debug("Event ID {} has been subscribed.", eid);
}

void DispatchMsgService::unsubscribe(u32 eid) {
  std::lock_guard<std::mutex> lock(subscribers_mutex_);
  auto it = subscribers_.find(eid);
  if (it != subscribers_.end()) {
    subscribers_.erase(it);
    spdlog::debug("Event ID {} has been unsubscribed.", eid);
  }
}

void DispatchMsgService::enqueue(std::unique_ptr<iEvent> ev) {
  if (!ev) {
    return ;
  }
  thread_pool_.enqueue([this, ev = std::move(ev)]() mutable {
    process(std::move(ev));
  });
  //这是处理 unique_ptr 移动的完美方式
  //ev = std::move(ev)是把ev移交给lambda参数
  //mutable保证lambda内可以进行再次移交
  //再次移交是将其移交给process的参数
}

void DispatchMsgService::process(std::unique_ptr<iEvent> ev) {
  if (!ev) return;

  iEventHandler* handler = nullptr;
  // [优化]去掉了lock
  // std::lock_guard<std::mutex> lock(subscribers_mutex_);
  
  auto it = subscribers_.find(ev->getEid());
  if (it != subscribers_.end()) {
    handler = it->second;
  }

  if (handler) {
    handler->handle(std::move(ev));   // 不再使用 release()！
  } else {
    spdlog::error("[DispatchMsgService] No handler for Event ID {}", ev->getEid());
  }
}

std::unique_ptr<iEvent> DispatchMsgService::create_event(u32 type_id, const char* data, u32 len) {
  std::unique_ptr<iEvent> event = nullptr;
  switch (type_id) {
    case EEVENTID_GET_MOBILE_CODE_REQ: {
      tutorial::mobile_request req_proto;
      if (req_proto.ParseFromArray(data, len)) {
        event = std::make_unique<MobileCodeReqEv>(req_proto.mobile());
      } else {
        spdlog::error("[EventFactory] Failed to parse mobile_request");
      }
      break;
    }
    case EEVENTID_LOGIN_REQ: {
      tutorial::login_request req_proto;
      if (req_proto.ParseFromArray(data, len)) {
        event = std::make_unique<LoginReq>(req_proto.mobile(), req_proto.icode());
      } else {
        spdlog::error("[EventFactory] Failed to parse login_request");
      }
      break;
    }
    case EEVENTID_UNLOCK_REQ: {
      tutorial::unlock_request req_proto;
      if (req_proto.ParseFromArray(data, len)) {
        event = std::make_unique<UnlockReq>(req_proto.mobile(), req_proto.dev_no(), req_proto.token());
      } else {
        spdlog::error("[EventFactory] Failed to parse unlock_request");
      }
      break;
    }
    default:
      spdlog::error("[EventFactory] Unknown event type_id: {}", type_id);
      break;
  }
  return event;
}
void DispatchMsgService::post_response(std::unique_ptr<iEvent> ev) {
  if (!network_interface_) {
    spdlog::error("[DispatchMsgService] network_interface_ is null, dropping response");
    return;
  }
  
  network_interface_->send_response(std::move(ev));
}