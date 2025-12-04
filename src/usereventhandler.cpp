#include "usereventhandler.h"
#include "mobilecodereqev.h"
#include "mobilecoderspev.h"
#include "loginreq.h"
#include "loginrsp.h"
#include "dispatchmsgservice.h"
#include "spdlog/spdlog.h"

#include "global_config.h"
#include "redistool.h"

#include "uuid_generator.h"

#include <random>
#include <string>



UserEventHandler::UserEventHandler(DispatchMsgService* dms) : dms_(dms){
  user_dao_ = std::make_unique<UserDao>();
  
  if (!dms_) {
    spdlog::error("Error: DispatchMsgService pointer is null in UserEventHandler constructor!");
    return ;
  }
  spdlog::debug("[UserEventHandler] Self-registering for events...");
  dms_->subscribe(EEVENTID_GET_MOBILE_CODE_REQ, this);
  dispatcher_[EEVENTID_GET_MOBILE_CODE_REQ] = [this](iEvent* ev) -> iEvent* {
    // 防御一下吧
    auto* mobilecode_ev = dynamic_cast<MobileCodeReqEv*>(ev);
    if (mobilecode_ev) {
      return handle_mobile_code_req(mobilecode_ev);
    } else {
      spdlog::error("Event cast failed for MobileCodeReq");
      return nullptr;
    }
  };
  
  dms_->subscribe(EEVENTID_LOGIN_REQ, this);
  dispatcher_[EEVENTID_LOGIN_REQ] = [this](iEvent* ev) -> iEvent* {
    auto* login_ev = dynamic_cast<LoginReq*>(ev);
    if (login_ev) {
      return handle_login_req(login_ev);
    } else {
      spdlog::error("Event cast failed for LoginReq");
      return nullptr;
    }
  };
} 

UserEventHandler::~UserEventHandler() {
  if (!dms_) {
    return ;
  }
  spdlog::debug("[UserEventHandler] Self-unsubscribing from events...");
  dms_->unsubscribe(EEVENTID_GET_MOBILE_CODE_REQ);
  dms_->unsubscribe(EEVENTID_LOGIN_REQ);
}

void UserEventHandler::handle(std::unique_ptr<iEvent> ev) {
  //判断ev的类型，然后调用我们的私有方法
  if (ev == nullptr) {
    spdlog::debug("input ev is NULL");
    return ;
  }
  u32 eid = ev->getEid();
  auto it = dispatcher_.find(eid);
  if (it != dispatcher_.end()) {
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

MobileCodeRspEv* UserEventHandler::handle_mobile_code_req(MobileCodeReqEv* ev) {
  std::string mobile_ = ev->get_mobile();
  i32 icode = gen_icode();
  spdlog::debug("Generated code {} for mobile {}", icode, mobile_);
  // if (user_dao_->updateVerifyCode(mobile_, icode)) {
  //   spdlog::debug("Verify code saved to DBsuccessfully (Upsert)");
  //   return new MobileCodeRspEv(ERRC_SUCCESS, icode);
  // } else {
  //   spdlog::error("Failed to update verify code in DB");
  //   return new MobileCodeRspEv(ERRO_PROCESS_FAILED, 0); 
  // }

  std::string key = "uCode:" + mobile_;
  std::string value = std::to_string(icode);
  
  auto redis = RedisTool::getThreadLocal();
  if (redis->setString(key, value, 300)) {
    spdlog::debug("Verify code saved to Redis. Key: {}", key);
    return new MobileCodeRspEv(ERRC_SUCCESS, icode);
  } else {
    spdlog::error("Failed to save code to Redis. Mobile: {}", mobile_);
    return new MobileCodeRspEv(ERRO_PROCESS_FAILED, 0);
  }
} 

LoginRsp* UserEventHandler::handle_login_req(LoginReq* ev) {
  std::string mobile_ = ev->get_mobile();
  i32 icode = ev->get_icode();

  std::string code_key = "uCode:" + mobile_;
  auto redis = RedisTool::getThreadLocal();
  std::string verify_code = "";
  if (redis) {
    verify_code = redis->getString(code_key);
  }
  // i32 verifycode = user_dao_->getVerifyCode(mobile_);
 
  spdlog::debug("Trying to verify, mobile phone {}", mobile_);
 
  // 修改输入0也能登录的bug
  // 防止stoi崩溃
  bool is_verified = false;
  try {
    if (!verify_code.empty() && icode == std::stoi(verify_code)) {
      is_verified = true;
    }
  } catch (...) {
    spdlog::warn("Invalid code format in Redis for mobile: {}", mobile_);
    is_verified = false;
  }

  if (is_verified) {
    spdlog::debug("Verify successfully, Generating Token ...");

    std::string token = UuidGenerator::generate();
    std::string key = "uToken:" + token;

    if (redis) {
      redis->setString(key, mobile_, 86400);
      redis->delKey(code_key);
      // 添加验证成功后删除验证码的逻辑
    }
    
    LoginRsp* rsp = new LoginRsp(ERRC_SUCCESS);
    rsp->setToken(token);
    return rsp;
  } else {
    spdlog::debug("Verify failed");
    return new LoginRsp(ERRC_INVALID_DATA);
  }
}



i32 UserEventHandler::gen_icode() {
  static std::random_device rd;
  static std::mt19937 gen(rd());
  std::uniform_int_distribution<> distrib(100000, 999999);
  return distrib(gen);
}