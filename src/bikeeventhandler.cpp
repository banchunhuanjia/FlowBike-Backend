#include "bikeeventhandler.h"
#include "unlockreq.h"
#include "unlockrsp.h"
#include "dispatchmsgservice.h"
#include "bikedao.h"
#include "userdao.h"
#include "connectionpool.h"
#include "spdlog/spdlog.h"
#include "redistool.h"

#include <string>

BikeEventHandler::BikeEventHandler(DispatchMsgService* dms) : dms_(dms){
  bike_dao_ = std::make_unique<BikeDao>();
  user_dao_ = std::make_unique<UserDao>();

  if (!dms_) {
    spdlog::error("Error: DispatchMsgService pointer is null in BikeEventHandler constructor!");
    return ;
  }

  spdlog::debug("[BikeEventHandler] Self-registering for events...");
  dms_->subscribe(EEVENTID_UNLOCK_REQ, this);
  dispatcher_[EEVENTID_UNLOCK_REQ] = [this](iEvent* ev) -> iEvent* {
    auto* unlock_ev = dynamic_cast<UnlockReq*>(ev);
    if (unlock_ev) {
      return handle_unlock_req(unlock_ev);
    } else {
      spdlog::error("Event cast failed for UnlockReq");
      return nullptr;
    }
  };
}

BikeEventHandler::~BikeEventHandler() {
  if (!dms_) return ;
  spdlog::debug("[BikeEventHandler] Self-unsubscribing from events...");
  dms_->unsubscribe(EEVENTID_UNLOCK_REQ);
}

void BikeEventHandler::handle(std::unique_ptr<iEvent> ev) {
  if (ev == nullptr) {
    spdlog::debug("input ev is NULL");
    return ;
  }
  u32 eid = ev->getEid();
  auto it = dispatcher_.find(eid);
  if (it != dispatcher_.end()) {
    //不创建raw_req，这样内存泄漏，没人释放
    iEvent* raw_rsp = it->second(ev.get());

    if (raw_rsp) {
      std::unique_ptr<iEvent> rsp(raw_rsp);
      rsp->set_fd(ev->get_fd());
      dms_->post_response(std::move(rsp));
    }
  } else {
    spdlog::error("Unkonwn Event ID: {}", eid);
  }
}

UnlockRsp* BikeEventHandler::handle_unlock_req(UnlockReq* ev) {
  std::string mobile_ = ev->getMobile();
  int dev_no_ = ev->getDevNo();
  std::string req_token = ev->getToken();

  spdlog::debug("Handling Unlock Req. Mobile: {}, Token: {}", mobile_, req_token);

  std::string key = "uToken:" + req_token;
  std::string redis_mobile = RedisTool::getThreadLocal()->getString(key);

  if (redis_mobile.empty()) {
    spdlog::warn("Token invalid or expired. Key: {}", key);
    return new UnlockRsp(ERRC_INVALID_DATA);
  }
  if (redis_mobile != mobile_) {
    spdlog::critical("Security Alert! Token mobile ({}) != Request mobile ({})", redis_mobile, mobile_);
    return new UnlockRsp(ERRC_INVALID_DATA);
  }

  spdlog::debug("Token Authentication Passed.");

  auto conn = ConnectionPool::getConnectionPool()->getConnection();
  if (!conn) {
    spdlog::error("Failed to get DB connection");
    return new UnlockRsp(ERRO_PROCESS_FAILED);
  }

  //开启事务，从现在开始，每一步操作不commit就不会生效，所以不会出现扣了费但开锁失败这种事件间断的问题

  if (!conn->transaction()) {
    spdlog::error("Failed to start transaction");
    return new UnlockRsp(ERRO_PROCESS_FAILED);
  }

  User user_;
  if (!user_dao_->queryByMobile(mobile_, user_, conn.get())) {
    spdlog::warn("User not exists");
    //这里加入回滚
    conn->rollback();
    return new UnlockRsp(ERRC_INVALID_DATA);
  }
  if (user_.balance <= 0) {
    spdlog::warn("Insufficient balance. Mobile: {}, Balance: {}", mobile_, user_.balance);
    conn->rollback();
    return new UnlockRsp(ERRO_PROCESS_FAILED);
  }

  Bike bike_;
  if (!bike_dao_->queryByDevNo(dev_no_, bike_, conn.get())) {
    spdlog::error("Invalid QR code");
    conn->rollback();
    return new UnlockRsp(ERRC_INVALID_DATA);
  }
  if (bike_.status == 2) {
    spdlog::warn("Bike {} is damaged/under repair", dev_no_);
    conn->rollback();
    return new UnlockRsp(ERRO_BIKE_IS_DAMAGED);
  }
  // 为了省一次update，我们检查一下有没有被骑
  if (bike_.status == 1) {
    spdlog::warn("Bike {} is already taken (Query check).", dev_no_);
    conn->rollback();
    return new UnlockRsp(ERRO_BIKE_IS_TAKEN);
  }
  if (!bike_dao_->updateStatus(dev_no_, 0, 1, conn.get())) {
    spdlog::error("Too slow, the bike has been taken");
    conn->rollback();
    return new UnlockRsp(ERRO_BIKE_IS_TAKEN);
  }


  //spdlog::critical("🛑 TEST: Simulating Server Crash BEFORE Commit! 🛑");
    
    // 我们故意回滚，或者直接 return (让 shared_ptr 析构，连接归还)
    // 注意：如果我们直接 return，连接没 Commit 就还回池子了。
    // 在标准的连接池实现中，这其实是不安全的（脏连接）。
    // 但为了测试“数据库没变”，我们显式调用 rollback 模拟“事务失败”。
    
  //conn->rollback(); 
  //return new UnlockRsp(ERRO_PROCESS_FAILED);

  // 以上都是各种意外情况，这里是最终
  if (conn->commit()) {
    spdlog::debug("Unlocked successfully");
    return new UnlockRsp(ERRC_SUCCESS);
  } else {
    spdlog::error("Transaction Commit Failed!");
    conn->rollback(); // 提交失败也要回滚
    return new UnlockRsp(ERRO_PROCESS_FAILED);
  }
}